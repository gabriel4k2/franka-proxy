// LeFranX-derived streaming backend. Modified from upstream architecture:
// explicit sessions, Asio framing, nonblocking callback handoff, lab settings.
// See ../UPSTREAM.md and ../LICENSE.LeFranX (Apache-2.0).
#include "control.hpp"
#include "network.hpp"

#include <atomic>
#include <csignal>
#include <iostream>
#include <mutex>
#include <thread>
#include <franka/robot.h>

namespace {
volatile std::sig_atomic_t interrupted = 0;
void interrupt(int) { interrupted = 1; }

using namespace streaming;

class RobotSession {
public:
    RobotSession(const std::string& ip, bool realtime)
        : robot_(ip, realtime ? franka::RealtimeConfig::kEnforce : franka::RealtimeConfig::kIgnore) {
        state_ = robot_.readOnce();
        sampled_ = Clock::now();
    }
    ~RobotSession() { stop(); }

    void request_stop() { stop_requested_ = true; }

    Json handle(const Json& request) {
        const auto op = request.at("op").get<std::string>();
        if (op == "start") start();
        else if (op == "stop") stop();
        else if (op == "target") {
            if (!running_ || !ready_ || stop_requested_)
                throw std::runtime_error("no accepting stream; call start explicitly");
            const auto& values = request.at("q");
            if (!values.is_array() || values.size() != 7)
                throw std::invalid_argument("q must contain exactly seven numbers");
            Joints q;
            for (std::size_t i = 0; i < 7; ++i) {
                if (!values[i].is_number()) throw std::invalid_argument("q must contain numbers");
                q[i] = values[i].get<double>();
            }
            // Validation and timestamping happen outside the real-time callback.
            std::lock_guard lock(mutex_);
            if (Clock::now() - sampled_ > std::chrono::milliseconds(100))
                throw std::runtime_error("robot state is stale");
            check_target(q, state_.q);
            target_ = Target{q, Clock::now(), ++sequence_};
            return {{"sequence", sequence_}};
        } else if (op != "state") {
            throw std::invalid_argument("unknown operation");
        }
        return snapshot();
    }

    void stop() {
        request_stop();
        if (worker_.joinable()) worker_.join();
    }

private:
    void start() {
        if (running_) throw std::runtime_error("stream already running or stopping");
        if (worker_.joinable()) worker_.join();
        const auto state = robot_.readOnce();
        check_target(state.q, state.q);
        if (!near_zero(state.dq, 0.01) || !near_zero(state.dq_d, 0.001) ||
            !near_zero(state.ddq_d, 0.01))
            throw std::runtime_error("start requires the robot to be at rest");

        // Existing proxy's normal-motion settings, NOT LeFranX's 250 Nm limits.
        // Unlike the existing helper, failures propagate and prevent control.
        robot_.setJointImpedance({{3000, 3000, 3000, 2500, 2500, 2000, 2000}});
        robot_.setCollisionBehavior(
            {{20, 20, 18, 18, 16, 14, 12}}, {{40, 40, 38, 38, 36, 34, 32}},
            {{20, 20, 18, 18, 16, 14, 12}}, {{40, 40, 38, 38, 36, 34, 32}},
            {{20, 20, 20, 25, 25, 25}}, {{40, 40, 40, 45, 45, 45}},
            {{20, 20, 20, 25, 25, 25}}, {{40, 40, 40, 45, 45, 45}});
        {
            std::lock_guard lock(mutex_);
            state_ = state;
            sampled_ = Clock::now();
            target_ = {};
            applied_sequence_ = 0;
            status_ = "holding";
            reason_ = "";
            fault_.clear();
        }
        ready_ = false;
        stop_requested_ = false;
        running_ = true;
        try { worker_ = std::thread([this] { run(); }); }
        catch (...) { running_ = false; throw; }

        const auto deadline = Clock::now() + std::chrono::seconds(3);
        while (!ready_ && running_ && !interrupted && Clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (ready_ && running_ && !interrupted) return;
        stop();
        std::lock_guard lock(mutex_);
        throw std::runtime_error(fault_.empty() ? "stream startup interrupted or timed out" : fault_);
    }

    void run() {
        try {
            Control control;
            Target current;
            bool first = true;
            robot_.control([&](const franka::RobotState& state, franka::Duration) {
                const auto now = Clock::now();
                if (first) {
                    // Recheck at actual control entry, not just at the earlier readOnce.
                    check_target(state.q, state.q);
                    if (!near_zero(state.dq, 0.01) || !near_zero(state.dq_d, 0.001) ||
                        !near_zero(state.ddq_d, 0.01))
                        throw std::runtime_error("robot moved before control entry");
                    first = false;
                }
                {
                    std::unique_lock lock(mutex_, std::try_to_lock);
                    if (lock.owns_lock()) current = target_;
                }
                const auto velocity = control.step(state.q, state.dq, state.dq_d,
                                                   current, now, stop_requested_);
                {
                    std::unique_lock lock(mutex_, std::try_to_lock);
                    if (lock.owns_lock()) {
                        state_ = state;
                        sampled_ = now;
                        status_ = control.status;
                        reason_ = control.reason;
                        applied_sequence_ = current.sequence;
                        ready_ = true;
                    }
                }
                const franka::JointVelocities command(velocity);
                return control.finished ? franka::MotionFinished(command) : command;
            }, franka::ControllerMode::kJointImpedance, true, 100.);
        } catch (const std::exception& exc) {
            // Throwing out of the callback aborts control through libfranka.
            // Never pretend a discontinuous zero command was a smooth stop.
            std::lock_guard lock(mutex_);
            fault_ = exc.what();
            std::cerr << "stream failed: " << fault_ << '\n';
        }
        {
            std::lock_guard lock(mutex_);
            status_ = fault_.empty() ? "inactive" : "failed";
        }
        running_ = false;
    }

    Json snapshot() {
        if (!running_) {
            if (worker_.joinable()) worker_.join();
            // No background state reader competing for ownership of robot_.
            const auto state = robot_.readOnce();
            std::lock_guard lock(mutex_);
            state_ = state;
            sampled_ = Clock::now();
        }
        franka::RobotState state;
        std::string status, message;
        std::uint64_t sequence;
        Clock::time_point sampled;
        {
            std::lock_guard lock(mutex_);
            state = state_;
            status = status_;
            message = fault_.empty() ? reason_ : fault_;
            sequence = applied_sequence_;
            sampled = sampled_;
        }
        return {{"q", state.q}, {"dq", state.dq}, {"status", status}, {"message", message},
                {"sequence", sequence}, {"robot_time", state.time.toSec()},
                {"state_age", std::chrono::duration<double>(Clock::now() - sampled).count()},
                {"control_command_success_rate", state.control_command_success_rate}};
    }

    franka::Robot robot_;
    std::thread worker_;
    std::atomic_bool running_{false}, ready_{false}, stop_requested_{false};
    std::mutex mutex_;
    franka::RobotState state_{};
    Clock::time_point sampled_{};
    Target target_;
    std::uint64_t sequence_ = 0, applied_sequence_ = 0;
    const char* status_ = "inactive";
    const char* reason_ = "";
    std::string fault_;
};
} // namespace

int main(int argc, char** argv) {
    if (argc < 2 || std::string(argv[1]) == "--help") {
        std::cout << "Usage: franka_stream_server ROBOT_IP [BIND_ADDRESS] [PORT] [--allow-non-realtime]\n"
                     "Defaults: 127.0.0.1:5000, realtime enforced. Panda arm only.\n"
                     "Stop the existing proxy first. No motion until a client sends start and target.\n";
        return argc < 2 ? 1 : 0;
    }
    try {
        if (argc > 5 || (argc == 5 && std::string(argv[4]) != "--allow-non-realtime"))
            throw std::invalid_argument("invalid arguments; use --help");
        const auto address = asio::ip::make_address(argc >= 3 ? argv[2] : "127.0.0.1");
        const auto port_text = argc >= 4 ? std::string(argv[3]) : "5000";
        std::size_t used = 0;
        const auto port = std::stoul(port_text, &used);
        if (used != port_text.size() || port == 0 || port > 65535)
            throw std::invalid_argument("port must be in [1, 65535]");
        std::signal(SIGINT, interrupt);
        std::signal(SIGTERM, interrupt);
        asio::io_context io;
        RobotSession robot(argv[1], argc != 5);
        Network network(io, {address, static_cast<unsigned short>(port)},
                        [&](const Json& request) { return robot.handle(request); },
                        [&] { robot.request_stop(); });
        std::cout << "Streaming server on " << address << ':' << port << '\n';
        while (!interrupted) {
            io.run_for(std::chrono::milliseconds(20));
            io.restart();
        }
        network.shutdown(); // cancels socket reads before waiting for braking
        robot.stop();
    } catch (const std::exception& exc) {
        std::cerr << exc.what() << '\n';
        return 1;
    }
}

#include "control.hpp"
#include "network.hpp"

#include <atomic>
#include <iostream>
#include <limits>
#include <thread>

using namespace streaming;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template<class F> void rejects(F call) {
    bool rejected = false;
    try { call(); } catch (const std::exception&) { rejected = true; }
    require(rejected, "expected rejection");
}

void test_control() {
    Joints q{0., -0.785, 0., -2., 0., 1.571, 0.785};
    Joints velocity{}, acceleration{};
    auto now = Clock::now();
    Target target;
    Control control;
    const auto tick = [&](bool stop = false) {
        now += std::chrono::milliseconds(1);
        const auto next = control.step(q, velocity, velocity, target, now, stop);
        for (std::size_t i = 0; i < 7; ++i) {
            const double a = (next[i] - velocity[i]) / 0.001;
            require(std::isfinite(next[i]) && std::abs(next[i]) <= Control::max_velocity[i] + 1e-6,
                    "velocity bound");
            require(std::abs(a) <= Control::max_acceleration[i] + 1e-6, "acceleration bound");
            require(std::abs(a - acceleration[i]) / 0.001 <= Control::max_jerk[i] + 1e-4,
                    "jerk bound");
            acceleration[i] = a;
            q[i] += next[i] * 0.001; // synthetic perfect velocity tracking, not a robot model
        }
        velocity = next;
    };
    for (int i = 0; i < 20; ++i) tick();
    require(near_zero(velocity, 0.), "no target must hold");
    auto goal = q;
    goal[3] += 0.15;
    check_target(goal, q);
    target = {goal, now, 1};
    for (int i = 0; i < 150; ++i) tick();
    require(velocity[3] > 0.01, "target should initiate motion");
    goal[3] = q[3] - 0.1;
    target = {goal, now, 2}; // replace a target during motion
    for (int i = 0; i < 400; ++i) tick();
    require(velocity[3] < 0., "latest target should replace previous target");
    for (int i = 0; i < 1200; ++i) tick();
    require(near_zero(velocity, 1e-6) && std::string(control.reason) == "target timeout",
            "watchdog must brake and hold");

    goal = q;
    goal[3] += 0.15;
    target = {goal, now, 3};
    for (int i = 0; i < 150; ++i) tick();
    require(velocity[3] > 0.01, "fresh target should resume motion");
    tick(true);
    // Stop is latched even if the flag clears and fresh targets keep arriving.
    for (int i = 0; i < 2000 && !control.finished; ++i) {
        target.received = now;
        ++target.sequence;
        tick();
    }
    require(control.finished && near_zero(velocity, 1e-6), "stop must finish at rest");

    goal = q;
    goal[0] += 0.21;
    rejects([&] { check_target(goal, q); });
    goal = q;
    goal[3] = 0.;
    rejects([&] { check_target(goal, q); });
    goal = q;
    goal[0] = std::numeric_limits<double>::quiet_NaN();
    rejects([&] { check_target(goal, q); });

    Control stuck;
    Joints moving{};
    moving[0] = 0.1;
    stuck.step(q, moving, moving, {}, now, true);
    rejects([&] { stuck.step(q, moving, moving, {}, now + std::chrono::seconds(6), true); });
    Control unsafe;
    goal = q;
    goal[3] = 0.; // bypass the API deliberately; trajectory guard must still reject
    rejects([&] { unsafe.step(q, {}, {}, Target{goal, now, 1}, now, false); });
}

void test_network() {
    asio::io_context io;
    std::atomic_int disconnected{0};
    Network network(io, {asio::ip::address_v4::loopback(), 0},
                    [](const Json& request) { return Json{{"echo", request.at("n")}}; },
                    [&] { ++disconnected; });
    const auto port = network.port();
    // RAII keeps an assertion failure from leaving a joinable thread behind.
    struct Runner {
        asio::io_context& io;
        Network& network;
        std::thread thread;
        Runner(asio::io_context& io, Network& network)
            : io(io), network(network), thread([&io] { io.run(); }) {}
        ~Runner() {
            if (thread.joinable()) {
                asio::post(io, [this] { network.shutdown(); });
                thread.join();
            }
        }
    } runner(io, network);
    asio::io_context client_io;
    Tcp::socket socket(client_io);
    socket.connect({asio::ip::address_v4::loopback(), port});
    asio::streambuf buffer;
    const auto response = [&] {
        asio::read_until(socket, buffer, '\n');
        std::string line;
        std::istream input(&buffer);
        std::getline(input, line);
        return Json::parse(line);
    };
    asio::write(socket, asio::buffer(std::string("{\"n\":")));
    asio::write(socket, asio::buffer(std::string("1}\n{\"n\":2}\n")));
    require(response().at("echo") == 1 && response().at("echo") == 2,
            "fragmented and coalesced frames");
    asio::write(socket, asio::buffer(std::string("not json\n{\"n\":3}\n")));
    require(response().at("ok") == false && response().at("echo") == 3,
            "bad request must not desynchronize framing");
    Tcp::socket second(client_io);
    second.connect({asio::ip::address_v4::loopback(), port});
    char byte;
    std::error_code error;
    second.read_some(asio::buffer(&byte, 1), error);
    require(bool(error), "second client must not take control");
    // A full unterminated frame is rejected and invokes the disconnect stop hook.
    asio::write(socket, asio::buffer(std::string(4096, 'x')));
    socket.read_some(asio::buffer(&byte, 1), error);
    require(bool(error), "oversized frame must disconnect");
    while (!disconnected) std::this_thread::yield();
    require(disconnected == 1, "disconnect must request stop exactly once");
    socket.close();
    socket.connect({asio::ip::address_v4::loopback(), port});
    asio::write(socket, asio::buffer(std::string("{\"n\":4}\n")));
    require(response().at("echo") == 4, "new client can connect after owner leaves");
    // Shutdown with an idle connected client must cancel the pending read.
    asio::post(io, [&] { network.shutdown(); });
    socket.read_some(asio::buffer(&byte, 1), error);
    require(bool(error), "shutdown must unblock an idle connection");
    runner.thread.join();
    require(disconnected == 2, "shutdown must also request stop");
}

int main() {
    try {
        test_control();
        test_network();
        std::cout << "Control and network checks passed; no robot connection opened.\n";
    } catch (const std::exception& exc) {
        std::cerr << exc.what() << '\n';
        return 1;
    }
}

#pragma once
// Adapted from LeFranX (Apache-2.0), a39906e6629f39490950fe8bd20f4f992ed74fd7.
// Modified: explicit braking, validation and no I/O or locks in the control step.
// See ../UPSTREAM.md and ../LICENSE.LeFranX.

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <ruckig/ruckig.hpp>

namespace streaming {
using Joints = std::array<double, 7>;
using Clock = std::chrono::steady_clock;

// Panda limits from this repository's franka_proxy_util.cpp, not FR3 limits.
inline constexpr Joints lower{-2.8973, -1.7628, -2.8973, -3.0718, -2.8973, -0.0175, -2.8973};
inline constexpr Joints upper{2.8973, 1.7628, 2.8973, -0.069, 2.8973, 3.7525, 2.8973};
inline constexpr double margin = 0.01;
inline constexpr double max_target_distance = 0.2;
inline constexpr double target_timeout = 0.5;

struct Target {
    Joints q{};
    Clock::time_point received{};
    std::uint64_t sequence = 0;
};

inline void check_target(const Joints& target, const Joints& measured) {
    for (std::size_t i = 0; i < 7; ++i) {
        if (!std::isfinite(target[i]) || !std::isfinite(measured[i]))
            throw std::invalid_argument("nonfinite joint position");
        if (target[i] < lower[i] + margin || target[i] > upper[i] - margin)
            throw std::invalid_argument("target outside Panda joint limits plus margin");
        if (std::abs(target[i] - measured[i]) > max_target_distance)
            throw std::invalid_argument("target more than 0.2 rad from measured position");
    }
}

inline bool near_zero(const Joints& values, double tolerance) {
    for (double value : values)
        if (!std::isfinite(value) || std::abs(value) > tolerance)
            return false;
    return true;
}

class Control {
public:
    // LeFranX's original limits. Deliberately not the proposed patches' tuning.
    inline static constexpr Joints max_velocity{1.5, 1.5, 1.8, 1.8, 2., 2., 2.};
    inline static constexpr Joints max_acceleration{4., 4., 4., 4., 6., 6., 6.};
    inline static constexpr Joints max_jerk{8., 8., 8., 8., 12., 12., 12.};

    Control() {
        input_.max_velocity = max_velocity;
        input_.max_acceleration = max_acceleration;
        input_.max_jerk = max_jerk;
        input_.target_velocity.fill(0.);
        input_.target_acceleration.fill(0.);
        output_.new_velocity.fill(0.);
        output_.new_acceleration.fill(0.);
    }

    // Like LeFranX: measured position, previous planned derivatives, 1 ms horizon.
    // dq and dq_d are only used to confirm rest before ending a session.
    Joints step(const Joints& q, const Joints& dq, const Joints& dq_d,
                const Target& target, Clock::time_point now, bool stop) {
        if (stop && !stopping_) {
            stopping_ = true;
            stop_started_ = now;
        }
        const bool stale = target.sequence == 0 ||
            std::chrono::duration<double>(now - target.received).count() > target_timeout;
        const bool brake = stopping_ || stale;
        input_.current_position = q;
        input_.current_velocity = output_.new_velocity;
        input_.current_acceleration = output_.new_acceleration;
        input_.target_position = target.sequence ? target.q : q;
        input_.control_interface = brake ? ruckig::ControlInterface::Velocity
                                        : ruckig::ControlInterface::Position;
        input_.synchronization = brake ? ruckig::Synchronization::None
                                      : ruckig::Synchronization::Time;
        for (std::size_t i = 0; i < 7; ++i)
            if (!std::isfinite(q[i]) || q[i] < lower[i] || q[i] > upper[i])
                throw std::runtime_error("measured position outside Panda limits");

        const auto result = otg_.update(input_, output_);
        if (result != ruckig::Result::Working && result != ruckig::Result::Finished)
            throw std::runtime_error("Ruckig failed: " + std::to_string(static_cast<int>(result)));

        // Ruckig 0.15.3's extrema include an unset final position in velocity
        // mode, so this whole-trajectory guard applies to tracking only.
        // Braking has a next-step guard below, not a stopping-distance guarantee.
        if (!brake && output_.new_calculation) {
            const auto extrema = output_.trajectory.get_position_extrema();
            for (std::size_t i = 0; i < 7; ++i)
                if (extrema[i].min < lower[i] || extrema[i].max > upper[i])
                    throw std::runtime_error("planned trajectory leaves Panda joint limits: joint " +
                        std::to_string(i) + " range [" + std::to_string(extrema[i].min) +
                        ", " + std::to_string(extrema[i].max) + "]");
        }
        for (std::size_t i = 0; i < 7; ++i)
            if (!std::isfinite(output_.new_position[i]) ||
                output_.new_position[i] < lower[i] || output_.new_position[i] > upper[i])
                throw std::runtime_error("next planned position leaves Panda joint limits");
        const bool command_at_rest = near_zero(output_.new_velocity, 1e-6) &&
                                     near_zero(output_.new_acceleration, 1e-5);
        status = stopping_ ? "stopping" : brake ? (command_at_rest ? "holding" : "braking")
                                                    : (command_at_rest ? "holding" : "tracking");
        reason = stale && target.sequence ? "target timeout" : "";
        if (stopping_) {
            rest_ticks_ = command_at_rest && near_zero(dq, 0.01) && near_zero(dq_d, 0.001)
                              ? rest_ticks_ + 1 : 0;
            finished = rest_ticks_ >= 10;
            if (!finished && now - stop_started_ > std::chrono::seconds(5))
                throw std::runtime_error("stop did not reach rest within 5 seconds");
        }
        return output_.new_velocity;
    }

    const char* status = "holding";
    const char* reason = "";
    bool finished = false;

private:
    ruckig::Ruckig<7> otg_{0.001};
    ruckig::InputParameter<7> input_;
    ruckig::OutputParameter<7> output_;
    bool stopping_ = false;
    int rest_ticks_ = 0;
    Clock::time_point stop_started_{};
};
} // namespace streaming

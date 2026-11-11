#include "sequence_tracking_statistics.hpp"
#include <franka_proxy_share/motion_timing.hpp>

#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace
{
void require(bool condition, const char* message)
{
	if (!condition)
		throw std::runtime_error(message);
}

bool near(double actual, double expected)
{
	return std::abs(actual - expected) < 1e-12;
}

void test_tracking_statistics()
{
	franka_proxy::detail::sequence_tracking_statistics stats;
	require(stats.callbacks == 0 && stats.stall_episodes == 0, "empty statistics");

	const auto sample = [&](double period_s, double error_m, bool stalled) {
		stats.record_period(period_s);
		stats.record_tracking(error_m, stalled, period_s);
	};
	// Initial zero period, then nominal and delayed callbacks; two stall episodes.
	sample(0.000, 0.001, false);
	sample(0.001, 0.004, false);
	sample(0.001, 0.006, true);
	sample(0.003, 0.009, true);
	sample(0.002, 0.002, false);
	sample(0.001, 0.007, true);

	require(stats.callbacks == 6, "callback count");
	require(near(stats.elapsed_s, 0.008), "physical time uses actual callback periods");
	require(near(stats.max_period_s, 0.003), "maximum callback period");
	require(stats.stalled_callbacks == 3 && stats.stall_episodes == 2, "stall counts");
	require(near(stats.stall_s, 0.005), "stall duration is not callback count times 1ms");
	require(near(stats.longest_stall_s, 0.004), "longest contiguous stall");
	require(near(stats.peak_error_m, 0.009), "error is recorded before the 5mm clamp");
	require(near(stats.last_error_m, 0.007), "last error");

	sample(0.002, 0.012, true);
	require(stats.stall_episodes == 2, "ongoing stall remains one episode");
	require(near(stats.longest_stall_s, 0.004), "shorter ongoing stall does not replace longest");
	sample(0.003, 0.013, true);
	require(near(stats.longest_stall_s, 0.006), "unfinished stall contributes to longest");

	franka_proxy::detail::sequence_tracking_statistics force_control;
	force_control.record_tracking(0.020, false, 0.001);
	require(near(force_control.peak_error_m, 0.020) && force_control.stalled_callbacks == 0,
		"error alone does not imply a stall when position selection is disabled");
}

void exercise_phase_logs()
{
	using namespace franka_proxy::detail;
	const auto batch = next_motion_timing_id();
	require(next_motion_timing_id() > batch, "unique process-local IDs");
	{
		motion_phase_log phase("test", batch, "returned", 133);
		phase.finish();
		phase.finish(); // Explicit completion and destruction must not duplicate output.
	}
	try
	{
		motion_phase_log phase("test", batch, "throws", 133);
		throw std::runtime_error("synthetic failure");
	}
	catch (const std::runtime_error&)
	{
	}
}
}

int main()
{
	try
	{
		test_tracking_statistics();
		exercise_phase_logs();
		std::puts("Offline sequence timing checks passed; no robot connection opened.");
		return 0;
	}
	catch (const std::exception& exc)
	{
		std::fprintf(stderr, "FAIL: %s\n", exc.what());
		return 1;
	}
}

#ifndef INCLUDED__FRANKA_PROXY__SEQUENCE_TRACKING_STATISTICS_HPP
#define INCLUDED__FRANKA_PROXY__SEQUENCE_TRACKING_STATISTICS_HPP

#include <algorithm>
#include <cstddef>

namespace franka_proxy::detail
{
// Fixed-size counters only: no I/O, allocation, or clock queries in the callback.
struct sequence_tracking_statistics
{
	void record_period(double period_s) noexcept
	{
		++callbacks;
		elapsed_s += period_s;
		max_period_s = std::max(max_period_s, period_s);
	}

	void record_tracking(double error_m, bool stalled, double period_s) noexcept
	{
		last_error_m = error_m;
		peak_error_m = std::max(peak_error_m, error_m);
		if (stalled)
		{
			++stalled_callbacks;
			if (!was_stalled)
				++stall_episodes;
			stall_s += period_s;
			current_stall_s += period_s;
			longest_stall_s = std::max(longest_stall_s, current_stall_s);
		}
		else
			current_stall_s = 0.0;
		was_stalled = stalled;
	}

	std::size_t callbacks = 0;
	std::size_t stalled_callbacks = 0;
	std::size_t stall_episodes = 0;
	double elapsed_s = 0.0;
	double max_period_s = 0.0;
	double stall_s = 0.0;
	double longest_stall_s = 0.0;
	double last_error_m = 0.0;
	double peak_error_m = 0.0;

private:
	bool was_stalled = false;
	double current_stall_s = 0.0;
};
}
#endif

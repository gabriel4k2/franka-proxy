#ifndef INCLUDED__FRANKA_PROXY_SHARE__MOTION_TIMING_HPP
#define INCLUDED__FRANKA_PROXY_SHARE__MOTION_TIMING_HPP

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <exception>

namespace franka_proxy::detail
{
inline unsigned long long next_motion_timing_id() noexcept
{
	static std::atomic<unsigned long long> next{0};
	return ++next;
}

// Only use outside the real-time callback. IDs are local to each process.
class motion_phase_log
{
public:
	motion_phase_log(const char* side, unsigned long long batch, const char* phase,
	                 std::size_t samples) noexcept
		: side_(side), batch_(batch), phase_(phase), exceptions_(std::uncaught_exceptions())
	{
		std::fprintf(stderr,
			"[move_sequence] side=%s batch=%llu phase=%s event=start samples=%zu\n",
			side_, batch_, phase_, samples);
		std::fflush(stderr);
		started_ = std::chrono::steady_clock::now();
	}

	~motion_phase_log() { finish(); }
	motion_phase_log(const motion_phase_log&) = delete;
	motion_phase_log& operator=(const motion_phase_log&) = delete;

	void finish() noexcept
	{
		if (finished_)
			return;
		const double elapsed_ms = std::chrono::duration<double, std::milli>(
			std::chrono::steady_clock::now() - started_).count();
		std::fprintf(stderr,
			"[move_sequence] side=%s batch=%llu phase=%s event=end status=%s wall_ms=%.3f\n",
			side_, batch_, phase_,
			std::uncaught_exceptions() > exceptions_ ? "exception" : "returned", elapsed_ms);
		std::fflush(stderr);
		finished_ = true;
	}

private:
	const char* side_;
	unsigned long long batch_;
	const char* phase_;
	int exceptions_;
	std::chrono::steady_clock::time_point started_;
	bool finished_ = false;
};
}
#endif

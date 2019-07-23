/**
 *************************************************************************
 *
 * @file timer.hpp
 *
 * Header for timers.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__TIMER_HPP)
#define INCLUDED__VIRAL_CORE__TIMER_HPP


#include "build_config.hpp"


#if defined(VIRAL__USE_CPP11_TIMER)
	#include <chrono>
#endif


#include "util.hpp"
#include "string.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup timer_module Timer Module
 *
 * Holds various timer utilities.
 *
 ************************************************************************/


/**
 *************************************************************************
 *
 * @class ms_counter
 * @ingroup timer_module
 *
 * Provides realtime counting on Microsoft Windows, using either
 * a high-performance counter or a millisecond clock.
 *
 ************************************************************************/
#if defined(VIRAL__USE_MS_TIMER)


class ms_counter :
	private non_copyable
{
	public:

		ms_counter();

		int64 query_count() const;

		int64 frequency() const;
		float seconds_per_tick() const;
		bool has_high_performance_counter() const;


	private:

		int64 frequency_;
		float seconds_per_tick_;
		bool has_high_performance_counter_;
};


#endif




/**
 *************************************************************************
 *
 * @class free_timer
 * @ingroup timer_module
 *
 * Counts seconds_passed() since free_timer() or the last restart().
 *
 * Usage notes:
 * - Do not accumulate seconds_passed() over multiple small intervals
 *		to avoid excessive rounding errors. Use a step_timer instead.
 * - Each seconds_passed() call possibly queries an OS-internal timer.
 *		This may be expensive, thus seconds_passed() is not suitable
 *		for very frequent usage.
 * - @attent All timer operations may potentially throw an exception.
 *		If restart() throws, then the timer remains unchanged.
 *
 ************************************************************************/
class free_timer :
	private non_copyable
{

	public:

		free_timer();

		void restart();
		float seconds_passed() const;


	private:

		#if defined(VIRAL__USE_MS_TIMER)
			ms_counter counter_;
			int64 base_count_;
		#endif

		#if defined(VIRAL__USE_CPP11_TIMER)
			float seconds_per_tick_;
			std::chrono::steady_clock::time_point base_time_;
		#endif

		#if !defined(VIRAL__USE_MS_TIMER) && \
			!defined(VIRAL__USE_CPP11_TIMER)
				#error "Not implemented for current timer back-end."
		#endif
};




/**
 *************************************************************************
 *
 * @class step_timer
 * @ingroup timer_module
 *
 * A fixed, accumulation-based timestep mechanism for realtime
 * synchronized simulations.
 *
 * Usage overview:
 * - The client application calls step_timer::update() in a loop.
 *		This accumulates the time that has passed since the last
 *		update in a timer-internal counter.
 * - The application iteratively extracts completely accumulated
 *		timesteps from the counter with step_timer::next_timestep().
 *		For each timestep, the application performs per-frame
 *		logics until it has processed all pending steps.
 * - When waiting for new timesteps, clients should reduce CPU load
 *		at the cost of (slightly) inaccurate timesteps by calling
 *		step_timer::try_sleep() before each update().
 * - After prolonged delays, such as loading operations, clients
 *		should reset the timer with step_timer::restart().
 *
 * Missed timestep behaviour:
 * - In general, all computers that meet system requirements should
 *		execute fixed timestep logics (excluding rendering) in realtime.
 * - A step_timer may fall behind realtime due to inaccurate try_sleep(),
 *		same-thread rendering, slow hardware, or heavy load situations.
 *		In such cases, the step_timer accumulates multiple timesteps,
 *		which the client application retroactively catches up with.
 *		This may lead to missed input or an unresponsive application,
 *		even though the fixed timestep rate might be held on average.
 * - Time accumulation caps at a constant @bref{maximum_seconds} to avoid
 *		a "spiral of death" if an application cannot maintain timesteps
 *		for prolonged time periods. In this case, both the step_timer
 *		and the client application slow down when compared to realtime.
 *
 * Timestep interpolation:
 * - Ideally, logics timestep rates exceed rendering frame rates
 *		(e.g. 100hz logics for 60hz rendering). This setup usually
 *		suffices for rendering without stutter or time aliasing,
 *		in particular if used together with a render_task.
 * - For even smoother rendering, applications can interpolate current
 *		and previous logic states with the remaining timestep fraction
 *		step_timer::accum_factor() at the cost of a one-timestep lag.
 * - Interpolation in theory allows for rendering rates higher than
 *		logics rates. However, this prohibits the use of try_sleep()
 *		and is not recommended.
 * - @attent Fixed timesteps improve stability of application logics
 *		such as physics or collision detection. Do not perform partial
 *		timesteps instead of interpolation.
 * 
 ************************************************************************/
class step_timer :
	private non_copyable
{

	public:

		/**
		 * Create timestep-based application timer.
		 *
		 * @bref{timestep_seconds} defines the realtime interval,
		 * greater than zero, for each fixed logics step.
		 *
		 * @bref{maximum_seconds} indicates the maximum number
		 * of seconds to accumulate, and must be greater than
		 * @bref{timestep_seconds}.
		 */
		step_timer
			(float timestep_seconds, float maximum_seconds);


		/**
		 * Restart timing with an acumulated time of zero,
		 * such as after some prolonged loading operation.
		 */
		void restart();


		/**
		 * Reduce CPU load by trying to sleep until an
		 * explicit update() will report a next_timestep().
		 * Sleeping is inexact, and a subsequent update()
		 * may accumulate no or multiple next_timestep()s.
		 */
		void try_sleep() const;


		/**
		 * Accumulate all time that has passed since
		 * the preceding call to update().
		 */
		void update();


		/**
		 * Extract a completely finished timestep from the
		 * accumulator and return @true. A return of @false
		 * indicates that no complete timestep was available.
		 */
		bool next_timestep();


		/**
		 * Test if at least a single next_timestep()
		 * has accumulated in the timer.
		 */
		bool has_next_timestep() const;


		/**
		 * Retrieve total accumulated time, in seconds.
		 */
		float accum_seconds() const;


		/**
		 * Retrieve the fractional count of complete and partial
		 * timesteps that currently are accumulated, mostly for
		 * use in interpolation.
		 */
		float accum_factor() const;


		/** Construction-set properties, see step_timer(). */
		//@{
		const float timestep_seconds;
		const float maximum_seconds;
		//@}


	private:

		/**
		 * Minimum time for which to try_sleep().
		 * Lower values reduce CPU load at cost
		 * of timestep accuracy.
		 */
		static const float try_sleep_seconds_epsilon_;

	
		#if defined(VIRAL__USE_MS_TIMER)
			ms_counter counter_;
			int64 last_count_;

			int64 counts_per_step_;
			int64 maximum_accum_count_;
			int64 accum_count_;
		#endif

		#if defined(VIRAL__USE_CPP11_TIMER)
			float frequency_;
			std::chrono::steady_clock::time_point last_time_;
			
			intmax_t counts_per_step_;
			intmax_t maximum_accum_count_;
			intmax_t accum_count_;			
		#endif
		
		#if !defined(VIRAL__USE_MS_TIMER) && \
			!defined(VIRAL__USE_CPP11_TIMER)
				#error "Not implemented for current timer back-end."
		#endif
};




} /* namespace viral_core */

#endif /* !defined(INCLUDED__VIRAL_CORE__TIMER_HPP) */

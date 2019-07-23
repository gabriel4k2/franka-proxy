/**
 *************************************************************************
 *
 * @file timer.cpp
 *
 * Timer implementation.
 *
 ************************************************************************/


#include "timer.hpp"


#if defined(VIRAL__USE_MS_TIMER)
	#include <windows.h>
#endif


#include "log.hpp"
#include "exception.hpp"
#include "thread_util.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// ms_counter
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__USE_MS_TIMER)


ms_counter::ms_counter()
	:
	frequency_(0),
	seconds_per_tick_(0),
	has_high_performance_counter_(false)
{
	has_high_performance_counter_ =
		(QueryPerformanceFrequency
			(reinterpret_cast<LARGE_INTEGER*>(&frequency_)) != 0);

	if (!has_high_performance_counter_)
		frequency_ = 1000;

	seconds_per_tick_ =
		1.f / static_cast<float>(frequency_);
}


int64 ms_counter::query_count() const
{
	int64 result;

	if (has_high_performance_counter_)
		QueryPerformanceCounter
			(reinterpret_cast<LARGE_INTEGER*>(&result));
	else
		result = timeGetTime();

	return result;
}


int64 ms_counter::frequency() const
	{ return frequency_; }
float ms_counter::seconds_per_tick() const
	{ return seconds_per_tick_; }
bool ms_counter::has_high_performance_counter() const
	{ return has_high_performance_counter_; }


#endif /* defined(VIRAL__USE_MS_TIMER) */




//////////////////////////////////////////////////////////////////////////
//
//	free_timer, Microsoft Windows implementation
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__USE_MS_TIMER)

free_timer::free_timer()
	: base_count_(counter_.query_count()) { }

void free_timer::restart()
	{ base_count_ = counter_.query_count(); }

float free_timer::seconds_passed() const
{
	return
		static_cast<float>(counter_.query_count() - base_count_) *
		counter_.seconds_per_tick();
}

#endif /* defined(VIRAL__USE_MS_TIMER) */




//////////////////////////////////////////////////////////////////////////
//
// free_timer, C++11 std implementation
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__USE_CPP11_TIMER)

free_timer::free_timer()
	:
	seconds_per_tick_
		(static_cast<float>(std::chrono::steady_clock::period::num) /
		 static_cast<float>(std::chrono::steady_clock::period::den)),
	base_time_(std::chrono::steady_clock::now())
{ }

void free_timer::restart()
	{ base_time_ = std::chrono::steady_clock::now(); }

float free_timer::seconds_passed() const
{
	return
		static_cast<float>
			((std::chrono::steady_clock::now() - base_time_).count()) *
			seconds_per_tick_;
}

#endif /* defined(VIRAL__USE_CPP11_TIMER) */




//////////////////////////////////////////////////////////////////////////
//
// step_timer, Microsoft Windows implementation
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__USE_MS_TIMER)


step_timer::step_timer
	(float timestep_seconds, float maximum_seconds)
	:
	timestep_seconds(timestep_seconds), maximum_seconds(maximum_seconds),
	last_count_(counter_.query_count()),
	counts_per_step_(0), maximum_accum_count_(0), accum_count_(0)
{
	if (timestep_seconds <= 0 || maximum_seconds < timestep_seconds)
	{
		LOG_ERROR
			("Invalid timestep interval provided for timer.");
		throw invalid_parameter();
	}

	counts_per_step_ =
		static_cast<int64>(timestep_seconds * counter_.frequency());
	counts_per_step_ = max(counts_per_step_, int64(1));
	
	maximum_accum_count_ =
		static_cast<int64>(maximum_seconds * counter_.frequency());
	maximum_accum_count_ = max(maximum_accum_count_, int64(1));
}


void step_timer::restart()
{
	last_count_ = counter_.query_count();
	accum_count_ = 0;
}


void step_timer::try_sleep() const
{
	if (has_next_timestep()) return;

	int64 now_accum_count =
		accum_count_ + (counter_.query_count() - last_count_);

	float now_accum_seconds =
		static_cast<float>(now_accum_count) /
			 static_cast<float>(counter_.frequency());

	float pending_seconds =
		timestep_seconds - now_accum_seconds;
	float sleep_seconds =
		pending_seconds - try_sleep_seconds_epsilon_;

	if (sleep_seconds > 0)
		thread_util::sleep_seconds(sleep_seconds);
}


void step_timer::update()
{
	int64 current_count = counter_.query_count();

	accum_count_ += (current_count - last_count_);
	accum_count_ = min(accum_count_, maximum_accum_count_);

	last_count_ = current_count;
}


bool step_timer::next_timestep()
{
	if (accum_count_ < counts_per_step_)
		return false;

	accum_count_ -= counts_per_step_;
	return true;
}


bool step_timer::has_next_timestep() const
	{ return accum_count_ >= counts_per_step_; }

float step_timer::accum_seconds() const
	{ return static_cast<float>(accum_count_) /
			 static_cast<float>(counter_.frequency()); }

float step_timer::accum_factor() const
	{ return static_cast<float>(accum_count_) /
			 static_cast<float>(counts_per_step_); }


// Thread sleeping precision on MS Windows is no better than 1ms.
// Below 1ms, we play nice if other programs want time slices,
// and we perform busy waiting if the next timestep is impending.
const float step_timer::try_sleep_seconds_epsilon_ = 0.0005f;


#endif /* defined(VIRAL__USE_MS_TIMER) */




//////////////////////////////////////////////////////////////////////////
//
// step_timer, C++ 11 std implementation
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__USE_CPP11_TIMER)


step_timer::step_timer
	(float timestep_seconds, float maximum_seconds)
	:
	timestep_seconds(timestep_seconds),
	maximum_seconds(maximum_seconds),

	frequency_
		(1.f / (static_cast<float>(std::chrono::steady_clock::period::num) /
				static_cast<float>(std::chrono::steady_clock::period::den))),

	last_time_(std::chrono::steady_clock::now()),

	counts_per_step_(0),
	maximum_accum_count_(0),
	accum_count_(0)
{
	if (timestep_seconds <= 0 || maximum_seconds < timestep_seconds)
	{
		LOG_ERROR
			("Invalid timestep interval provided for timer.");
		throw invalid_parameter();
	}

	counts_per_step_ =
		static_cast<intmax_t>(timestep_seconds * frequency_);
	counts_per_step_ = max(counts_per_step_, intmax_t(1));
	
	maximum_accum_count_ =
		static_cast<intmax_t>(maximum_seconds * frequency_);
	maximum_accum_count_ = max(maximum_accum_count_, intmax_t(1));
}


void step_timer::restart()
{
	last_time_ = std::chrono::steady_clock::now();
	accum_count_ = 0;
}


void step_timer::try_sleep() const
{
	if (has_next_timestep()) return;

	std::chrono::steady_clock::time_point now
		(std::chrono::steady_clock::now());

	intmax_t now_accum_count =
		accum_count_ + (now - last_time_).count();

	float now_accum_seconds =
		static_cast<float>(now_accum_count) /
			 static_cast<float>(frequency_);

	float pending_seconds =
		timestep_seconds - now_accum_seconds;
	float sleep_seconds =
		pending_seconds - try_sleep_seconds_epsilon_;

	if (sleep_seconds > 0)
		thread_util::sleep_seconds(sleep_seconds);
}


void step_timer::update()
{
	std::chrono::steady_clock::time_point now
		(std::chrono::steady_clock::now());

	accum_count_ += (now - last_time_).count();
	accum_count_ = min(accum_count_, maximum_accum_count_);

	last_time_ = now;
}


bool step_timer::next_timestep()
{
	if (accum_count_ < counts_per_step_)
		return false;

	accum_count_ -= counts_per_step_;
	return true;
}


bool step_timer::has_next_timestep() const
	{ return accum_count_ >= counts_per_step_; }

float step_timer::accum_seconds() const
	{ return static_cast<float>(accum_count_) /
			 static_cast<float>(frequency_); }

float step_timer::accum_factor() const
	{ return static_cast<float>(accum_count_) /
			 static_cast<float>(counts_per_step_); }


// Just an educated guess on the general precision
// of std library thread sleeping.
const float step_timer::try_sleep_seconds_epsilon_ = 0.001f;


#endif /* defined(VIRAL__USE_CPP11_TIMER) */




} /* namespace viral_core */

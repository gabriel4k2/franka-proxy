/**
 *************************************************************************
 *
 * @file random.cpp
 *
 * Random generator implementation.
 *
 ************************************************************************/


#include "random.hpp"


#include <time.h>


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//	lcg_generator
//
//////////////////////////////////////////////////////////////////////////


lcg_generator::lcg_generator()
	: seed_(default_seed), current_value_(default_seed)
		{ next_number(); }

lcg_generator::lcg_generator(uint64 seed)
	: seed_(seed), current_value_(seed)
		{ next_number(); }


uint64 lcg_generator::seed() const
	{ return seed_; }


void lcg_generator::restart_seed_with_time()
	{ restart(static_cast<uint64>(time(NULL))); }


void lcg_generator::restart(uint64 seed)
{
	seed_ = seed;
	current_value_ = seed;
	next_number();
}


void lcg_generator::next_number()
	{ current_value_ = (current_value_ * factor_ + shift_) % maximum_; }


bool lcg_generator::random_bool()
	{ return (random_uint64() % 2 != 0); }


int64 lcg_generator::random_int64()
	{ return static_cast<int64>(random_uint64()); }

int64 lcg_generator::random_int64
	(int64 min, int64 max)
{
	return min + static_cast<int64>
			(random_uint64() % static_cast<uint64>(max + 1 - min));
}


uint64 lcg_generator::random_uint64()
	{ next_number(); return current_value_; }

uint64 lcg_generator::random_uint64
	(uint64 min, uint64 max)
{
	return random_uint64() % (max + 1 - min) + min;
}


float lcg_generator::random_float()
{
	next_number(); 

	// Divide in double range to avoid extensive issues
	// with precision on rather large uint64 values.
	return
		static_cast<float>
			(static_cast<double>(current_value_) /
			 static_cast<double>(maximum_));
}

float lcg_generator::random_float(float min, float max)
	{ return random_float() * (max - min) + min; }




} /* namespace viral_core */

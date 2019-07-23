/**
 *************************************************************************
 *
 * @file random.hpp
 *
 * Header for the random generator collection.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__RANDOM_HPP)
#define INCLUDED__VIRAL_CORE__RANDOM_HPP


#include "build_config.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class lcg_generator
 *
 * A basic linear congruential pseudorandom number generator
 * for simple, but cross-platform random number needs.
 *
 * Notes:
 * - The creation function for deriving new random numbers is
 * @code
 *		new_value = (previous_value * factor_ + shift_) % maximum_
 * @endcode
 *		with certain "good" values for @bref{factor_} and @bref{shift_}.
 *		The inital value is @bref{default_seed}, unless the user
 *		provides a custom seed on construction.
 * - The period has been tested to be > 10000 for several seed values,
 *		but may exhibit the usual LCG lattice structure close up.
 * - Unlike implementation specific rand() / srand(), lcg_generator
 * 		provides consistent behaviour over all platforms.
 *
 ************************************************************************/
class lcg_generator
{

	public:

		lcg_generator();
		lcg_generator(uint64 seed);


		uint64 seed() const;


		/** Restart with a new random number based on @bref{seed}. */
		void restart(uint64 seed);


		/** Restart with pseudo-random seed from current system time. */
		void restart_seed_with_time();


		/** Create new random number, but don't use it. */
		void next_number();


		/**
		 * Create a new random number from the pseudo random sequence,
		 * cast to the given type and restricted to an optional range.
		 *
		 * The default range for integers is [0, @bref{maximum_} - 1].
		 * The default range for floats is [0, 1].
		 *
		 * Empty ranges (i.e. @bref{min} > @bref{max})
		 * may return arbitrary numbers outside the range.
		 */
		//@{
		bool random_bool();

		int64 random_int64();
		int64 random_int64(int64 min, int64 max);

		uint64 random_uint64();
		uint64 random_uint64(uint64 min, uint64 max);

		float random_float();
		float random_float(float min, float max);
		//@}


		/** The seed used to initialize a default generator. */
		static const uint64 default_seed = 91648253;


	private:

		static const uint64 factor_ = 214013;
		static const uint64 shift_ = 2531011;


		/**
		 * Largest prime in uint64 range
		 * (i.e. < 18446744073709551615).
		 */
		static const uint64 maximum_ = 18446744073709551557; 


		uint64 seed_;
		uint64 current_value_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__RANDOM_HPP) */

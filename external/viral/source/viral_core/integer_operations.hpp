/**
 *************************************************************************
 *
 * @file integer_operations.hpp
 *
 * Safe integer operations.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__INTEGER_OPERATIONS_HPP)
#define INCLUDED__VIRAL_CORE__INTEGER_OPERATIONS_HPP


#include "build_config.hpp"


#include "exception.hpp"
#include "integer_traits.hpp"
#include "integer_checks.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class integer_operations
 * @ingroup utility_module
 *
 * Static utilities that perform checked and thus safe integer operations.
 *
 * Usage notes:
 * - See integer_checks for general notes on safe and unsafe operations.
 * - Integer operations do not log to avoid cyclic dependencies.
 *
 ************************************************************************/
class integer_operations :
	private non_constructable
{

	public:

		/**
		 * Cast an @bref{in}put value to an integer type @bref{T}.
		 * Throws if the operation is not integer_checks::operation_safe.
		 */
		//@{
		template <typename T>
			static T cast_or_throw(int8 in)
				{ return cast_or_throw_helper<T, int8>(in); }
		template <typename T>
			static T cast_or_throw(uint8 in)
				{ return cast_or_throw_helper<T, uint8>(in); }

		template <typename T>
			static T cast_or_throw(int16 in)
				{ return cast_or_throw_helper<T, int16>(in); }
		template <typename T>
			static T cast_or_throw(uint16 in)
				{ return cast_or_throw_helper<T, uint16>(in); }

		template <typename T>
			static T cast_or_throw(int32 in)
				{ return cast_or_throw_helper<T, int32>(in); }
		template <typename T>
			static T cast_or_throw(uint32 in)
				{ return cast_or_throw_helper<T, uint32>(in); }

		template <typename T>
			static T cast_or_throw(int64 in)
				{ return cast_or_throw_helper<T, int64>(in); }
		template <typename T>
			static T cast_or_throw(uint64 in)
				{ return cast_or_throw_helper<T, uint64>(in); }

		template <typename T>
			static T cast_or_throw(float in)
				{ return cast_or_throw_helper<T, float>(in); }
		template <typename T>
			static T cast_or_throw(double in)
				{ return cast_or_throw_helper<T, double>(in); }
		//@}


		/**
		 * Cast an @bref{in}put value to an integer type @bref{T}.
		 * Terminates if the operation is not integer_checks::operation_safe.
		 */
		//@{
		template <typename T>
			static T cast_or_terminate(int8 in) NOTHROW
				{ return cast_or_terminate_helper<T, int8>(in); }
		template <typename T>
			static T cast_or_terminate(uint8 in) NOTHROW
				{ return cast_or_terminate_helper<T, uint8>(in); }

		template <typename T>
			static T cast_or_terminate(int16 in) NOTHROW
				{ return cast_or_terminate_helper<T, int16>(in); }
		template <typename T>
			static T cast_or_terminate(uint16 in) NOTHROW
				{ return cast_or_terminate_helper<T, uint16>(in); }

		template <typename T>
			static T cast_or_terminate(int32 in) NOTHROW
				{ return cast_or_terminate_helper<T, int32>(in); }
		template <typename T>
			static T cast_or_terminate(uint32 in) NOTHROW
				{ return cast_or_terminate_helper<T, uint32>(in); }

		template <typename T>
			static T cast_or_terminate(int64 in) NOTHROW
				{ return cast_or_terminate_helper<T, int64>(in); }
		template <typename T>
			static T cast_or_terminate(uint64 in) NOTHROW
				{ return cast_or_terminate_helper<T, uint64>(in); }

		template <typename T>
			static T cast_or_terminate(float in) NOTHROW
				{ return cast_or_terminate_helper<T, float>(in); }
		template <typename T>
			static T cast_or_terminate(double in) NOTHROW
				{ return cast_or_terminate_helper<T, double>(in); }
		//@}


		/**
		 * Cast an @bref{in}put value to an integer type @bref{T}.
		 * Clamps unsafe operations to safe maximum or minimum.
		 * Note (signless) NaN floats clamp to a zero integer.
		 */
		//@{
		template <typename T>
			static T cast_or_clamp(int8 in) NOTHROW
				{ return cast_or_clamp_helper<T, int8>(in); }
		template <typename T>
			static T cast_or_clamp(uint8 in) NOTHROW
				{ return cast_or_clamp_helper<T, uint8>(in); }

		template <typename T>
			static T cast_or_clamp(int16 in) NOTHROW
				{ return cast_or_clamp_helper<T, int16>(in); }
		template <typename T>
			static T cast_or_clamp(uint16 in) NOTHROW
				{ return cast_or_clamp_helper<T, uint16>(in); }

		template <typename T>
			static T cast_or_clamp(int32 in) NOTHROW
				{ return cast_or_clamp_helper<T, int32>(in); }
		template <typename T>
			static T cast_or_clamp(uint32 in) NOTHROW
				{ return cast_or_clamp_helper<T, uint32>(in); }

		template <typename T>
			static T cast_or_clamp(int64 in) NOTHROW
				{ return cast_or_clamp_helper<T, int64>(in); }
		template <typename T>
			static T cast_or_clamp(uint64 in) NOTHROW
				{ return cast_or_clamp_helper<T, uint64>(in); }

		template <typename T>
			static T cast_or_clamp(float in) NOTHROW
				{ return cast_or_clamp_helper<T, float>(in); }
		template <typename T>
			static T cast_or_clamp(double in) NOTHROW
				{ return cast_or_clamp_helper<T, double>(in); }
		//@}


		/**
		 * Add two integers @bref{a} and @bref{b}.
		 * Throw, terminate, or clamp on unsafe operations.
		 */
		//@{
		template <typename A, typename B>
			static auto add_or_throw(A a, B b)
				-> decltype(a + b)
		{
			if (!integer_checks::is_add_safe(a, b))
				throw integer_overflow();

			return a + b;
		}

		template <typename A, typename B>
			static auto add_or_terminate(A a, B b) NOTHROW
				-> decltype(a + b)
		{
			if (!integer_checks::is_add_safe(a, b))
				std::terminate();

			return a + b;
		}

		template <typename A, typename B>
			static auto add_or_clamp(A a, B b) NOTHROW
				-> decltype(a + b)
		{
			integer_checks::operation_safety_type safety =
				integer_checks::classify_add(a, b);

			if (safety == integer_checks::operation_unsafe_exceeds_max)
				return integer_traits<decltype(a + b)>::max;

			if (safety == integer_checks::operation_unsafe_exceeds_min)
				return integer_traits<decltype(a + b)>::min;

			return a + b;
		}
		//@}


		/**
		 * Multiply two integers @bref{a} and @bref{b}.
		 * Throw, terminate, or clamp on unsafe operations.
		 */
		//@{
		template <typename A, typename B>
			static auto mult_or_throw(A a, B b)
				-> decltype(a * b)
		{
			if (!integer_checks::is_mult_safe(a, b))
				throw integer_overflow();

			return a * b;
		}

		template <typename A, typename B>
			static auto mult_or_terminate(A a, B b) NOTHROW
				-> decltype(a * b)
		{
			if (!integer_checks::is_mult_safe(a, b))
				std::terminate();

			return a * b;
		}

		template <typename A, typename B>
			static auto mult_or_clamp(A a, B b) NOTHROW
				-> decltype(a * b)
		{
			integer_checks::operation_safety_type safety =
				integer_checks::classify_mult(a, b);

			if (safety == integer_checks::operation_unsafe_exceeds_max)
				return integer_traits<decltype(a * b)>::max;

			if (safety == integer_checks::operation_unsafe_exceeds_min)
				return integer_traits<decltype(a * b)>::min;

			return a * b;
		}
		//@}


	private:

		template <typename T, typename U>
			 static T cast_or_throw_helper(U in)
		{
			if (!integer_checks::is_cast_safe<T>(in))
				throw integer_overflow();

			return static_cast<T>(in);
		}

		template <typename T, typename U>
			 static T cast_or_terminate_helper(U in) NOTHROW
		{
			if (!integer_checks::is_cast_safe<T>(in))
				std::terminate();

			return static_cast<T>(in);
		}

		template <typename T, typename U>
			 static T cast_or_clamp_helper(U in) NOTHROW
		{
			integer_checks::operation_safety_type safety =
				integer_checks::classify_cast<T>(in);

			if (safety == integer_checks::operation_unsafe_exceeds_max)
				return integer_traits<T>::max;

			if (safety == integer_checks::operation_unsafe_exceeds_min)
				return integer_traits<T>::min;

			if (safety == integer_checks::operation_unsafe_float_is_nan)
				return 0;

			return static_cast<T>(in);
		}
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__INTEGER_OPERATIONS_HPP) */

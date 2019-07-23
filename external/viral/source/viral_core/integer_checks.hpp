/**
 *************************************************************************
 *
 * @file integer_checks.hpp
 *
 * Safety checks for integer operations.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__INTEGER_CHECKS_HPP)
#define INCLUDED__VIRAL_CORE__INTEGER_CHECKS_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "exception.hpp"
#include "static_if.hpp"
#include "integer_traits.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class integer_checks_support
 * @ingroup utility_module
 *
 * Tests if the integer_checks utility supports a type @bref{T}.
 *
 * Implementation notes:
 * - Most integer_checks make assumptions on supported integers,
 *		including type promotions and maximum available widths.
 * - To avoid errors with new types (e.g. int128, once supported),
 *		integer_checks_support checks for implemented integers
 *		and causes compiler errors on unsupported types.
 * - If there is an error in regards to integer_checks_support,
 *		first extend all integer_checks to work with the new type,
 *		then add the new type to below list of supported types.
 *
 ************************************************************************/
//@{
template <typename T> struct integer_checks_support;

template <> struct integer_checks_support<int8> { typedef int yes; };
template <> struct integer_checks_support<uint8> { typedef int yes; };

template <> struct integer_checks_support<int16> { typedef int yes; };
template <> struct integer_checks_support<uint16> { typedef int yes; };

template <> struct integer_checks_support<int32> { typedef int yes; };
template <> struct integer_checks_support<uint32> { typedef int yes; };

template <> struct integer_checks_support<int64> { typedef int yes; };
template <> struct integer_checks_support<uint64> { typedef int yes; };
//@}




/**
 *************************************************************************
 *
 * @class integer_checks
 * @ingroup utility_module
 *
 * Static utilities that detect unsafe integer operations
 * based on actual integer values.
 * 
 * Unsafe integer-to-integer operations:
 * - Integer-to-integer conversions and arithmetics classify as unsafe
 *		if the target type does not accomodate the correct result.
 * - Usual overflow classifies as unsafe (e.g. exceeding integer_traits::min
 *		or integer_traits::max of the result type with integer arithmetics).
 *		By standard, overflow shows undefined behaviour (signed result type)
 *		or uses an unexpected modulo (unsigned result type).
 * - @attent Integer arithmetics may auto-promote to larger result types.
 *		If this result foregoes overflow, the operation classifies as safe.
 *		However, any assignment to the original (smaller-than-result) type
 *		causes a compiler warning and should be checked as well.
 * - Conversion between signed and unsigned reinterprets the result sign
 *		(e.g. conversion of negative signed value to unsigned target type).
 *		While bitwise reinterpretation has defined and lossless behaviour,
 *		the result is incorrect and has caveats (e.g. as a loop condition).
 *		Such operations classify as an unsafe overflow instead.
 *
 * Unsafe float-to-integer operations:
 * - Checks for float-to-integer conversions allow approximate values,
 *		but classify a conversion unsafe if it exceeds the target range.
 *		By standard, conversions have undefined results in this case.
 * - @attent Float-to-integer conversions may falsely classify as unsafe
 *		for floats that are very close to absolutely large range limits
 *		(e.g. maximum of uint32 or minimum and maximum of int32).
 * 
 ************************************************************************/
class integer_checks :
	private non_constructable
{

	public:

		enum operation_safety_type
		{
			operation_safe,
			operation_unsafe_exceeds_max,
			operation_unsafe_exceeds_min,
			operation_unsafe_float_is_nan
		};


		/**
		 * Find the operation_safety_type of a static_cast
		 * from an integer or floating point @bref{in}put	
		 * to an integer type @bref{T}.
		 */
		//@{
		template <typename T>
			static operation_safety_type classify_cast(int8 in) NOTHROW
				{ return classify_cast_helper_integer<T, int8>(in); }
		template <typename T>
			static operation_safety_type classify_cast(uint8 in) NOTHROW
				{ return classify_cast_helper_integer<T, uint8>(in); }

		template <typename T>
			static operation_safety_type classify_cast(int16 in) NOTHROW
				{ return classify_cast_helper_integer<T, int16>(in); }
		template <typename T>
			static operation_safety_type classify_cast(uint16 in) NOTHROW
				{ return classify_cast_helper_integer<T, uint16>(in); }

		template <typename T>
			static operation_safety_type classify_cast(int32 in) NOTHROW
				{ return classify_cast_helper_integer<T, int32>(in); }
		template <typename T>
			static operation_safety_type classify_cast(uint32 in) NOTHROW
				{ return classify_cast_helper_integer<T, uint32>(in); }

		template <typename T>
			static operation_safety_type classify_cast(int64 in) NOTHROW
				{ return classify_cast_helper_integer<T, int64>(in); }
		template <typename T>
			static operation_safety_type classify_cast(uint64 in) NOTHROW
				{ return classify_cast_helper_integer<T, uint64>(in); }

		template <typename T>
			static operation_safety_type classify_cast(float in) NOTHROW
				{ return classify_cast_helper_float<T, float>(in); }
		template <typename T>
			static operation_safety_type classify_cast(double in) NOTHROW
				{ return classify_cast_helper_float<T, double>(in); }
		//@}


		/**
		 * Check whether classify_cast() from an @bref{in}put
		 * to an integer type @bref{T} is @bref{operation_safe}.
		 */
		//@{
		template <typename T>
			static bool is_cast_safe(int8 in) NOTHROW
				{ return classify_cast<T>(in) == operation_safe; }
		template <typename T>
			static bool is_cast_safe(uint8 in) NOTHROW
				{ return classify_cast<T>(in) == operation_safe; }

		template <typename T>
			static bool is_cast_safe(int16 in) NOTHROW
				{ return classify_cast<T>(in) == operation_safe; }
		template <typename T>
			static bool is_cast_safe(uint16 in) NOTHROW
				{ return classify_cast<T>(in) == operation_safe; }

		template <typename T>
			static bool is_cast_safe(int32 in) NOTHROW
				{ return classify_cast<T>(in) == operation_safe; }
		template <typename T>
			static bool is_cast_safe(uint32 in) NOTHROW
				{ return classify_cast<T>(in) == operation_safe; }

		template <typename T>
			static bool is_cast_safe(int64 in) NOTHROW
				{ return classify_cast<T>(in) == operation_safe; }
		template <typename T>
			static bool is_cast_safe(uint64 in) NOTHROW
				{ return classify_cast<T>(in) == operation_safe; }

		template <typename T>
			static bool is_cast_safe(float in) NOTHROW
				{ return classify_cast<T>(in) == operation_safe; }
		template <typename T>
			static bool is_cast_safe(double in) NOTHROW
				{ return classify_cast<T>(in) == operation_safe; }
		//@}


		/**
		 * Find operation_safety_type for adding @bref{a} and @bref{b}
		 * of two (possibly different) integer types @bref{A} and @bref{B}.
		 */
		template <typename A, typename B>
			static operation_safety_type
				classify_add(A a, B b) NOTHROW
		{
			typedef typename integer_checks_support<A>::yes A_supported;		
			typedef typename integer_checks_support<B>::yes B_supported;		

	
			typedef decltype(a + b) result_type;


			// Auto promotion to int:
			// - Auto width promotion to signed integer always stores valid sum.
			// - Conversion back to original types produces a compiler warning.
			//		Clients should explicitly downcast (e.g. by integer_operations)
			//		if the desired type does not match the temporary auto promotion.
			// - Note that int need not always be int32 in this case.
			if (integer_traits<A>::bits < integer_traits<int>::bits &&
				integer_traits<B>::bits < integer_traits<int>::bits)
					return operation_safe;


			// Check sum through type without overflow:
			// - We widen ints below int64 and without auto width promotion
			//		(usually int32, uint32) to int64 in order to validate
			//		a correct result against the range of the result type.
			// - Note that traits range is auto-promoted to int64 here.
			if (integer_traits<A>::bits < integer_traits<int64>::bits &&
				integer_traits<B>::bits < integer_traits<int64>::bits)
			{
				int64 a_int64 = static_cast<int64>(a);
				int64 b_int64 = static_cast<int64>(b);

				if (a_int64 + b_int64 < integer_traits<result_type>::min)
					return operation_unsafe_exceeds_min;

				if (a_int64 + b_int64 > integer_traits<result_type>::max)
					return operation_unsafe_exceeds_max;

				return operation_safe;
			}


			// At least one of the types must be int64 or uint64.
			// Larger integer types (i.e. int128) are unsupported
			// and cause errors through overflow_check_support.


			// Check for overflow in signed int64:
			// - The compiler performs auto type promotion to int64
			//		if neither of the types is uint64.
			// - The int64 values always correctly represent both inputs,
			//		but there may be overflow due to addition.
			// - Overflow on signed addition causes undefined behaviour,
			//		thus map to (possibly negated) unsigned addition,
			//		then check this addition for overflow.
			// - Signed values with different signs never cause overflow,
			//		so no need to actually check these.
			if (!static_equal<A, uint64>::result &&
				!static_equal<B, uint64>::result)
			{
				if ((a > 0 && b < 0) ||
					(a < 0 && b > 0))
						return operation_safe;

				if (a > 0 && b > 0)
				{
					uint64 a_uint64 = static_cast<uint64>(a);	
					uint64 b_uint64 = static_cast<uint64>(b);

					if (a_uint64 + b_uint64 > integer_traits<int64>::max)
						return operation_unsafe_exceeds_max;
	
					return operation_safe;
				}


				// Here: a <= 0 && b <= 0
				// Further note that |int64_min| + |int64_min| > uint64_max,
				// thus the below test is rearranged to avoid this overflow.
				uint64 a_abs_uint64 = abs_unsigned(a);
				uint64 b_abs_uint64 = abs_unsigned(b);

				uint64 a_abs_unit64_max =
					abs_unsigned(integer_traits<int64>::min) - b_abs_uint64;

				if (a_abs_uint64 > a_abs_unit64_max)
					return operation_unsafe_exceeds_min;

				return operation_safe;
			}


			// Check for overflow where at least one type is uint64:
			// - The compiler performs auto width promotion to uint64.
			// - Adding a signed, auto-promoted integer still works
			//		due to 2s complement and bit reinterpretation,
			//		as long as the result remains positive.
			// - If both values are positive, exactly convert to uint64.
			//		Then check for overflow by guaranteed unsigned modulo.
			uint64 a_abs_uint64 = abs_unsigned(a);
			uint64 b_abs_uint64 = abs_unsigned(b);

			if (a < 0) // implies b >= 0
			{
				if (a_abs_uint64 > b_abs_uint64)
					return operation_unsafe_exceeds_min;
				
				return operation_safe;
			}

			if (b < 0) // implies a >= 0
			{
				if (b_abs_uint64 > a_abs_uint64)
					return operation_unsafe_exceeds_min;

				return operation_safe;
			}


			// Here: a >= 0 && b >= 0
			// Unsigned overflow has defined modulo behaviour,
			// so check if modulo occurs to detect overflow.
			if (a_abs_uint64 + b_abs_uint64 < a_abs_uint64)
				return operation_unsafe_exceeds_max;

			return operation_safe;
		}


		/** Check whether classify_add() is @bref{operation_safe}. */
		template <typename A, typename B>
			static bool is_add_safe(A a, B b) NOTHROW
		{
			return classify_add(a, b) == operation_safe;
		}


		/**
		 * Find operation_safety_type for multiplying @bref{a} and @bref{b}
		 * of two (possibly different) integer types @bref{A} and @bref{B}.
		 */
		template <typename A, typename B>
			static operation_safety_type
				classify_mult(A a, B b) NOTHROW
		{
			typedef typename integer_checks_support<A>::yes A_supported;		
			typedef typename integer_checks_support<B>::yes B_supported;		


			typedef decltype(a * b) result_type;


			// Multiplying by zero results in zero,
			// which is valid in each result_type.
			// Early out to simplify later cases.
			if (a == 0 || b == 0)
				return operation_safe;


			// Fetch correct absolutes of inputs in unsigned type
			// that corresponds to the (possibly signed) result type.
			// Note result and unsigned result are at least as wide
			// as both input types, so there is no overflow.
			typename integer_traits<result_type>::unsigned_type
				a_abs_unsigned = abs_unsigned(a);
			typename integer_traits<result_type>::unsigned_type
				b_abs_unsigned = abs_unsigned(b);


			// Check if a negative result exceeds the negative limit.
			// To avoid overflow, use
			//			(abs_a <= truncate(abs_min / abs_b)) iff
			//			(abs_a <= abs_min / abs_b) iff
			//			(abs_a * abs_b <= abs_min) iff
			//			(a * b >= min),
			// where either a or b is negative.
			// Work in unsigned range to handle one signed parameter
			// without (wrong) auto promotion of signed to unsigned.
			if ((a < 0 && b > 0) ||
				(a > 0 && b < 0))
			{
				typename integer_traits<result_type>::unsigned_type a_abs_unsigned_max =
					abs_unsigned(integer_traits<result_type>::min) / b_abs_unsigned; 

				if (a_abs_unsigned > a_abs_unsigned_max)
					return operation_unsafe_exceeds_min;

				return operation_safe;
			}


			// Check if a positive result exceeds the positive limit.
			// To avoid overflow, use
			//			(a <= truncate(max / b)) iff
			//			(a <= max / b) iff
			//			(a * b <= max),
			// where neither a nor b is negative,
			// or where both a and b are negative.
			typename integer_traits<result_type>::unsigned_type a_abs_unsigned_max =
				integer_traits<result_type>::max / b_abs_unsigned;

			if (a_abs_unsigned > a_abs_unsigned_max)
				return operation_unsafe_exceeds_max;

			return operation_safe;
		}


		/** Check whether classify_mult() is @bref{operation_safe}. */
		template <typename A, typename B>
			static bool is_mult_safe(A a, B b) NOTHROW
		{
			return classify_mult(a, b) == operation_safe;
		}


	private:

		template <typename T, typename U>
			static operation_safety_type
				classify_cast_helper_integer(U in) NOTHROW
		{
			typedef typename integer_checks_support<T>::yes T_supported;		
			typedef typename integer_checks_support<U>::yes U_supported;		


			// Conversion from signed to unsigned:
			// - Negative in values can never be represented.
			// - Otherwise, convert in to positive in_unsigned without loss.
			// - Compare positive in_unsigned against unsigned target range.
			//		Auto promotion casts at least to larger of both ranges.
			if (integer_traits<U>::is_signed &&
				integer_traits<T>::is_unsigned)
			{
				if (in < 0)
					return operation_unsafe_exceeds_min;

				typename integer_traits<U>::unsigned_type in_unsigned
					(static_cast<typename integer_traits<U>::unsigned_type>(in));

				if (in_unsigned > integer_traits<T>::max)
					return operation_unsafe_exceeds_max;

				return operation_safe;
			}

			
			// Conversion from unsigned to signed:
			// - Maximum target range is positive, so convert to unsigned.
			// - Compare unsigned in value against unsigned maximum range.
			//		Auto promotion casts at least to larger of both ranges.
			// - The cast to in_unsigned is not required, but suppresses
			//		a false-positive warning on a signed U that does not
			//		actually enter the if clause.
			if (integer_traits<U>::is_unsigned &&
				integer_traits<T>::is_signed)
			{
				typename integer_traits<U>::unsigned_type in_unsigned
					(static_cast<typename integer_traits<U>::unsigned_type>(in));

				typename integer_traits<T>::unsigned_type max_unsigned
					(static_cast<typename integer_traits<T>::unsigned_type>
						(integer_traits<T>::max));

				if (in_unsigned > max_unsigned)
					return operation_unsafe_exceeds_max;

				return operation_safe;
			}


			// Conversion from signed to signed:
			// - Compare signed in value against signed range limits.
			//		Auto promotion casts at least to larger of both ranges.
			// - The static_casts to signed are not required, but suppress
			//		false-positive warnings on unsigned T or U that do not
			//		actually enter the if clause.
			if (integer_traits<U>::is_signed &&
				integer_traits<T>::is_signed)
			{
				typename integer_traits<U>::signed_type in_signed
					(static_cast<typename integer_traits<U>::signed_type>(in));

				typename integer_traits<T>::signed_type min_signed
					(static_cast<typename integer_traits<T>::signed_type>
						(integer_traits<T>::min));
				typename integer_traits<T>::signed_type max_signed
					(static_cast<typename integer_traits<T>::signed_type>
						(integer_traits<T>::max));

				if (in_signed > max_signed)
					return operation_unsafe_exceeds_max;

				if (in_signed < min_signed)
					return operation_unsafe_exceeds_min;

				return operation_safe;
			}


			// Conversion from unsigned to unsigned:
			// Compare unsigned in value against unsigned range limits.
			// Auto promotion casts at least to larger of both ranges.
			if (in > integer_traits<T>::max)
				return operation_unsafe_exceeds_max;

			return operation_safe;
		}


		template <typename T, typename U>
			static operation_safety_type
				classify_cast_helper_float(U in) NOTHROW
		{
			typedef typename integer_checks_support<T>::yes T_supported;		


			// Handle NaN,
			// which is not equal to itself.
			if (in != in)
				return operation_unsafe_float_is_nan;


			if (integer_traits<T>::is_signed)
			{
				// Implementation notes:
				// - static_cast<U>(integer_max) = integer_max +- epsilon,
				//		we get either of the neighboring representable values.
				//		There is no overflow even with int64 to float or double.
				// - For in <= integer_max - epsilon < integer_max + epsilon,
				//		it holds that static_cast<integer>(in) <= integer_max,
				//		independent of rounding mode.
				// - Since we do not know whether we get +epsilon or -epsilon,
				//		we only accept if in < static_cast<float>(integer_max),
				//		which causes false-negatives for the last valid float.
				// - Infinity specials compare just as usual floats.
				// - A similar rationale applies to integer_min.

				if (in <= static_cast<U>(integer_traits<T>::min))
					return operation_unsafe_exceeds_min;

				if (in >= static_cast<U>(integer_traits<T>::max))
					return operation_unsafe_exceeds_max;

				return operation_safe;
			}
			

			// Special case for unsigned integers:
			// We avoid false negatives for zero floats.
			if (in < 0)
				return operation_unsafe_exceeds_min;

			if (in >= static_cast<U>(integer_traits<T>::max))
				return operation_unsafe_exceeds_max;

			return operation_safe;
		}
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__INTEGER_CHECKS_HPP) */

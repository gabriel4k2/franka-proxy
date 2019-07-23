/**
 *************************************************************************
 *
 * @file util.hpp
 *
 * General utility header.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__UTIL_HPP)
#define INCLUDED__VIRAL_CORE__UTIL_HPP


#include "build_config.hpp"


#include <typeinfo>


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup utility_module Utility Module
 *
 * Provides assorted helper classes and functions, such as swap templates
 * or non-copyable and bool evaluation base classes.
 *
 ************************************************************************/


/**
 * Make a unique name (e.g. for a local variable)
 * from both @bref{basename} and @bref{extension}.
 * Notably, MAKE_UNIQUE_VARIABLE_NAME() accepts
 * and correctly resolves the __LINE__ macro.
 *
 * @ingroup utility_module
 */
//@{
#define MAKE_UNIQUE_VARIABLE_NAME_HELPER(basename, extension) \
	basename##_##extension
#define MAKE_UNIQUE_VARIABLE_NAME(basename, extension) \
	MAKE_UNIQUE_VARIABLE_NAME_HELPER(basename, extension)
//@}




/**
 * Mark a local variable as unused on purpose.
 * This suppresses a warning on unused locals.
 * @ingroup utility_module
 */
template <typename T>
	void unused_on_purpose(const T& t) NOTHROW { t; }




/**
 * Variant of sizeof() that returns an int64 size
 * and does not compile on (improbable) overflow.
 * @ingroup utility_module
 */
//@{
template <typename T, bool valid>
	struct size_of_helper;

template <typename T>
	struct size_of_helper<T, true>
		{ typedef T result; };

template <typename T>
	int64 size_of() NOTHROW
{
	typedef size_of_helper<T, (sizeof(T) < 2147483647)> helper;
	return sizeof(helper::result);
}
//@}




/**
 * Swap two intrinsic variables or objects.
 * @ingroup utility_module
 *
 * @attent For compatibility, objects need to provide
 *		their own non-throwing swap_with(T& other) methods.
 */
//@{
template <typename T>
	void swap(T& a, T& b) NOTHROW
		{ a.swap_with(b); }
template <typename T>
	void swap(T*& a, T*& b) NOTHROW
		{ T* temp = a; a = b; b = temp; }

void swap(char& a, char& b) NOTHROW;
void swap(int8& a, int8& b) NOTHROW;
void swap(uint8& a, uint8& b) NOTHROW;
void swap(int16& a, int16& b) NOTHROW;
void swap(uint16& a, uint16& b) NOTHROW;
void swap(int32& a, int32& b) NOTHROW;
void swap(uint32& a, uint32& b) NOTHROW;
void swap(int64& a, int64& b) NOTHROW;
void swap(uint64& a, uint64& b) NOTHROW;
void swap(float& a, float& b) NOTHROW;
void swap(double& a, double& b) NOTHROW;
//@}




/**
 * Explicit assignment based swap.
 * @ingroup utility_module
 *
 * @attent
 * Unlike swap(), this call does not raise a compiler error
 * for unsupported classes. Furthermore, users must provide
 * non-throwing copy constructors and assignment operators
 * to ensure NOTHROW behaviour.
 */
template <typename T>
	void swap_assign(T& a, T& b) NOTHROW
		{ T temp(a); a = b; b = temp; }




/**
 * Determine the smaller or larger of two variables.
 * NOTHROW for basic types, or types with NOTHROW
 * copy constructors and compare operators.
 * @ingroup utility_module
 */
//@{
template <typename T>
	T max(T a, T b) { return a > b ? a : b; }
template <typename T>
	T min(T a, T b) { return a > b ? b : a; }

inline int64 max(int32 a, int64 b) NOTHROW
	{ return max<int64>(a, b); } // Auto-promote to larger type.
inline int64 max(int64 a, int32 b) NOTHROW
	{ return max<int64>(a, b); } // Auto-promote to larger type.

inline int64 min(int32 a, int64 b) NOTHROW
	{ return min<int64>(a, b); } // Auto-promote to larger type.
inline int64 min(int64 a, int32 b) NOTHROW
	{ return min<int64>(a, b); } // Auto-promote to larger type.
//@}




/**
 * Correct absolutes of signed and unsigned integers.
 *
 * Implementation notes:
 * - Because @bref{-value} == @bref{value} for minimum integers
 *		without auto promotion to int, we need to separately handle
 *		that case on conversion.
 * - To make no assumptions about int width, we do the above check
 *		even for small types, where it is not detrimental.
 * - Unsigned variants enable signedness-agnostic template code.
 */
//@{
inline uint8 abs_unsigned(int8 value) NOTHROW
{
	return value == (-127 - 1) ? // Avoid minus on unsigned.
				static_cast<uint8>(128) :
		   value > 0 ?
				static_cast<uint8>(value) :
				static_cast<uint8>(-value);
}

inline uint8 abs_unsigned(uint8 value) NOTHROW
	{ return value; }


inline uint16 abs_unsigned(int16 value) NOTHROW
{
	return value == (-32767 - 1) ? // Avoid minus on unsigned.
				static_cast<uint16>(32768) :
		   value > 0 ?
				static_cast<uint16>(value) :
				static_cast<uint16>(-value);
}

inline uint16 abs_unsigned(uint16 value) NOTHROW
	{ return value; }


inline uint32 abs_unsigned(int32 value) NOTHROW
{
	return value == (-2147483647 - 1) ?  // Avoid minus on unsigned.
				static_cast<uint32>(2147483648) :
		   value > 0 ?
				static_cast<uint32>(value) :
				static_cast<uint32>(-value);
}

inline uint32 abs_unsigned(uint32 value) NOTHROW
	{ return value; }


inline uint64 abs_unsigned(int64 value) NOTHROW
{
	return value == (-9223372036854775807 - 1) ?  // Avoid minus on unsigned.
				static_cast<uint64>(9223372036854775808) :
		   value > 0 ?
				static_cast<uint64>(value) :
				static_cast<uint64>(-value);
}

inline uint64 abs_unsigned(uint64 value) NOTHROW
	{ return value; }
//@}




/**
 * Test two floating point values for approximate equality,
 * either with default or explicit epsilons.
 *
 * Usage notes:
 * - Values are nearly_equal() if their absolute difference
 *		is smaller than either a constant or a linear epsilon.
 * - The value-independent @bref{epsilon_constant} cancels
 *		numerical noise around the origin.
 * - The @bref{epsilon_linear} scales with the larger absolute
 *		of the incoming values, and accounts for numerical noise
 *		growing alongside absolute values.
 * - Epsilons are only a patch for imprecise floating points,
 *		and do not necessarily guarantee numerical robustness,
 *		in particular if error propagates worse than linearly.
 *
 * Default epsilons are @bref{nearly_equal_epsilon_constant}
 * and @bref{nearly_equal_epsilon_linear}.
 *
 * @ingroup utility_module
 */
//@{
const float nearly_equal_epsilon_constant = 1e-10f;
const float nearly_equal_epsilon_linear = 1e-5f;

bool nearly_equal(float value, float target) NOTHROW;
bool nearly_equal(float value, float target,
				  float epsilon_constant,
				  float epsilon_linear) NOTHROW;
//@}




/**
 *************************************************************************
 *
 * Map between an enum @bref{constant} and its string representation
 * for some string or enum input @bref{variable}.
 *
 * @ingroup utility_module
 *
 ************************************************************************/
//@{
#define RETURN_STRING_ON_ENUM_MATCH(variable, constant) \
	if (variable == constant) return #constant;
#define RETURN_ENUM_ON_STRING_MATCH(variable, constant) \
	if (variable == #constant) return constant;
//@}




/**
 *************************************************************************
 *
 * @class non_copyable
 * @ingroup utility_module
 *
 * Privately derive from this class to disallow autogeneration
 * of copy constructor and assignment operator.
 *
 ************************************************************************/
class non_copyable
{

	protected:

		non_copyable() { }
		~non_copyable() { }

	private:

		non_copyable(const non_copyable& other) = delete;
		const non_copyable& operator =(const non_copyable& other) = delete;
};




/**
 *************************************************************************
 *
 * @class non_constructable
 * @ingroup utility_module
 *
 * Privately derive from this class to disallow construction
 * of a given object.
 *
 ************************************************************************/
class non_constructable
{

	protected:

		~non_constructable() { }

	private:

		non_constructable() = delete;
		non_constructable(const non_constructable& other) = delete;
};




/**
 *************************************************************************
 *
 * @class bool_convert
 * @ingroup utility_module
 *
 * Robust conversion to boolean, following along the lines
 * of the C++ safe bool idiom.
 *
 * Usage notes:
 * - Privately derive some type @bref{T} from bool_convert<T>.
 * - Create a conversion operator to @bref{bool_conversion_type}.
 * - Return either @bref{&T::bool_conversion_is_true} or @NULL.
 *
 * Implementation notes:
 * - In contrast to operator bool(), bool_convert does not allow
 *		nonsensical operations (e.g. assignment to integer).
 * - In contrast to explicit operator bool(), bool_convert supports
 *		implicit conversion to bool in a return statement.
 *
 ************************************************************************/
template <typename T>
	class bool_convert
{
	protected:

		typedef void (bool_convert<T>::*bool_conversion_type)() const;
		void bool_conversion_is_true() const { }
};




/**
 *************************************************************************
 *
 * @class type_info_ref
 * @ingroup utility_module
 *
 * Encapsulates a reference to std::type_info 
 * for use in sets, maps, and other containers.
 *
 ************************************************************************/
class type_info_ref
{
	public:
	
		type_info_ref(const std::type_info& ref) NOTHROW;
		type_info_ref(const type_info_ref& other) NOTHROW;

		const std::type_info& get() const NOTHROW;

		type_info_ref& operator =(const type_info_ref& other) NOTHROW;
		
		bool operator ==(const type_info_ref& other) const NOTHROW;
		bool operator !=(const type_info_ref& other) const NOTHROW;

		bool operator <(const type_info_ref& other) const NOTHROW;
		bool operator >(const type_info_ref& other) const NOTHROW;
		bool operator <=(const type_info_ref& other) const NOTHROW;
		bool operator >=(const type_info_ref& other) const NOTHROW;


	private:

		const std::type_info* ptr_; // never @NULL
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__UTIL_HPP) */


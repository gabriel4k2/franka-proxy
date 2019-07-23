/**
 *************************************************************************
 *
 * @file integer_traits.hpp
 *
 * Traits for fixed-width integers, including minimum and maximum values.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__INTEGER_TRAITS_HPP)
#define INCLUDED__VIRAL_CORE__INTEGER_TRAITS_HPP


#include "build_config.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class integer_traits
 * @ingroup utility_module
 *
 * Specializations of this template store traits for integers.
 *
 ************************************************************************/
template <typename T> struct integer_traits;


/**
 * Individual integer traits.
 * @ingroup utility_module
 */
//@{
template <> struct integer_traits<int8> :
	private non_constructable
{
	typedef int8 type;

	typedef int8 signed_type;
	typedef uint8 unsigned_type;

	static const bool is_signed = true;
	static const bool is_unsigned = false;

	static const int64 bits = 8; 
	static const int64 bytes = 1;

	static const int8 min = -127 - 1; // Avoid minus on unsigned.
	static const int8 max = 127;
};

template <> struct integer_traits<uint8> :
	private non_constructable
{
	typedef uint8 type;

	typedef int8 signed_type;
	typedef uint8 unsigned_type;

	static const bool is_signed = false;
	static const bool is_unsigned = true;

	static const int64 bits = 8; 
	static const int64 bytes = 1;

	static const uint8 min = 0;
	static const uint8 max = 255;
};


template <> struct integer_traits<int16> :
	private non_constructable
{
	typedef int16 type;

	typedef int16 signed_type;
	typedef uint16 unsigned_type;

	static const bool is_signed = true;
	static const bool is_unsigned = false;

	static const int64 bits = 16; 
	static const int64 bytes = 2;

	static const int16 min = -32767 - 1; // Avoid minus on unsigned.
	static const int16 max = 32767;
};

template <> struct integer_traits<uint16> :
	private non_constructable
{
	typedef uint16 type;

	typedef int16 signed_type;
	typedef uint16 unsigned_type;

	static const bool is_signed = false;
	static const bool is_unsigned = true;

	static const int64 bits = 16; 
	static const int64 bytes = 2;

	static const uint16 min = 0;
	static const uint16 max = 65535;
};


template <> struct integer_traits<int32> :
	private non_constructable
{
	typedef int32 type;

	typedef int32 signed_type;
	typedef uint32 unsigned_type;

	static const bool is_signed = true;
	static const bool is_unsigned = false;

	static const int64 bits = 32; 
	static const int64 bytes = 4;

	static const int32 min = -2147483647 - 1; // Avoid minus on unsigned.
	static const int32 max = 2147483647;
};

template <> struct integer_traits<uint32> :
	private non_constructable
{
	typedef uint32 type;

	typedef int32 signed_type;
	typedef uint32 unsigned_type;

	static const bool is_signed = false;
	static const bool is_unsigned = true;

	static const int64 bits = 32; 
	static const int64 bytes = 4;

	static const uint32 min = 0;
	static const uint32 max = 4294967295;
};


template <> struct integer_traits<int64> :
	private non_constructable
{
	typedef int64 type;

	typedef int64 signed_type;
	typedef uint64 unsigned_type;

	static const bool is_signed = true;
	static const bool is_unsigned = false;

	static const int64 bits = 64; 
	static const int64 bytes = 8;

	static const int64 min = -9223372036854775807 - 1; // Avoid minus on unsigned.
	static const int64 max = 9223372036854775807;
};

template <> struct integer_traits<uint64> :
	private non_constructable
{
	typedef uint64 type;

	typedef int64 signed_type;
	typedef uint64 unsigned_type;

	static const bool is_signed = false;
	static const bool is_unsigned = true;

	static const int64 bits = 64; 
	static const int64 bytes = 8;

	static const uint64 min = 0;
	static const uint64 max = 18446744073709551615;
};
//@}




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__INTEGER_TRAITS_HPP) */

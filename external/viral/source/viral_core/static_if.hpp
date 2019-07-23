/**
 *************************************************************************
 *
 * @file static_if.hpp
 *
 * Compile-time branching for templates.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__STATIC_IF_HPP)
#define INCLUDED__VIRAL_CORE__STATIC_IF_HPP


#include "build_config.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class static_if
 * @ingroup utility_module
 *
 * If @bref{condition} is @true, return @bref{A}, otherwise @bref{B}.
 *
 ************************************************************************/
//@{
template <bool condition, typename A, typename B>
	class static_if;

template <typename A, typename B>
	class static_if<true, A, B> :
		private non_constructable
			{ public: typedef A result; };

template <typename A, typename B>
	class static_if<false, A, B> :
		private non_constructable
			{ public: typedef B result; };
//@}




/**
 *************************************************************************
 *
 * @class static_equal
 * @ingroup utility_module
 *
 * Set @bref{result} to whether @bref{A} and @bref{B} are equal.
 *
 ************************************************************************/
//@{
template <typename A, typename B>
	class static_equal
		{ public: static const bool result = false; };

template <typename T>
	class static_equal<T, T>
		{ public: static const bool result = true; };
//@}




/**
 *************************************************************************
 *
 * @class static_if_const
 * @ingroup utility_module
 *
 * If type @bref{T} is const, return type @bref{A}, otherwise @bref{B}.
 *
 ************************************************************************/
//@{
template <typename T, typename A, typename B>
	class static_if_const :
		private non_constructable
			{ public: typedef B result; };

template <typename T, typename A, typename B>
	class static_if_const<const T, A, B> :
		private non_constructable
			{ public: typedef A result; };
//@}




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__STATIC_IF_HPP) */


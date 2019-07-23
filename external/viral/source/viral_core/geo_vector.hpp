/**
 *************************************************************************
 *
 * @file geo_vector.hpp
 *
 * Vectors in 3D space.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GEO_VECTOR_HPP)
#define INCLUDED__VIRAL_CORE__GEO_VECTOR_HPP


#include "build_config.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup geo_module Geometry Module
 *
 * Contains geometry helper classes for 2D and 3D operations,
 * like vectors, rotations and matrices.
 *
 ************************************************************************/


/**
 *************************************************************************
 *
 * @class vector
 * @ingroup geo_module
 *
 * Basic functionality for storing and manipulating 3D vectors.
 *
 ************************************************************************/
class vector
{

	public:

		vector() NOTHROW;
		vector(float x, float y, float z) NOTHROW;


		const float& operator [](int64 index) const NOTHROW;
		float& operator [](int64 index) NOTHROW;


		/** Usual vector arithmetics. */
		//@{
		vector operator -() const NOTHROW; 

		vector operator +(const vector &v2) const NOTHROW;
		vector operator -(const vector &v2) const NOTHROW;
		vector operator *(const vector &v2) const NOTHROW;
		vector operator /(const vector &v2) const NOTHROW;

		vector& operator +=(const vector &v2) NOTHROW; 
		vector& operator -=(const vector &v2) NOTHROW;
		vector& operator *=(const vector &v2) NOTHROW;
		vector& operator /=(const vector &v2) NOTHROW;

		vector operator *(float val) const NOTHROW;
		vector operator /(float val) const NOTHROW;
		vector& operator *=(float val) NOTHROW;
		vector& operator /=(float val) NOTHROW;

		bool operator ==(const vector &v2) const NOTHROW;
		bool operator !=(const vector &v2) const NOTHROW;
		//@}


		void swap_with(vector& other) NOTHROW;


		float length() const NOTHROW;


		/**
		 * Normalize vector.
		 * Returns zero for near-zero vectors with length()
		 * smaller than @bref{normalize_epsilon}.
		 */
		//@{
		vector normalized() const NOTHROW;
		void normalize() NOTHROW;
		//@}


		/**
		 * Normalize maximum absolute vector component to +1 or -1.
		 * Returns zero for vectors with a maximum absolute component
		 * smaller than @bref{normalize_epsilon}.
		 * Faster but less accurate than normalize().
		 */
		//@{
		vector normalized_absmax() const NOTHROW;
		void normalize_absmax() NOTHROW;
		//@}


		float dot(const vector &v2) const NOTHROW;
		vector cross(const vector &v2) const NOTHROW;


		/** Per-component mathematical functions. */
		//@{
		float min() const NOTHROW;
		float max() const NOTHROW;
		float absmin() const NOTHROW;
		float absmax() const NOTHROW;

		vector min(const vector& v2) const NOTHROW;
		vector max(const vector& v2) const NOTHROW;
		vector abs() const NOTHROW;
		vector floor() const NOTHROW;
		vector ceil() const NOTHROW;
		//@}


		/**
		 * Calculate sum, square sum, and product
		 * over all components.
		 */
		//@{
		float sum() const NOTHROW;
		float square_sum() const NOTHROW;
		float product() const NOTHROW;
		//@}


		/**
		 * Test if two vectors are nearly equal
		 * under default or explicit epsilons.
		 * See viral_core::nearly_equal().
		 */
		//@{
		bool nearly_equal
			(const vector& v2) const NOTHROW;

		bool nearly_equal
			(const vector& v2,
			 float epsilon_constant,
			 float epsilon_linear) const NOTHROW;
		//@}


		static const float nearly_equal_epsilon_constant;
		static const float nearly_equal_epsilon_linear;
		static const float normalize_epsilon;


		union
		{
			float c[3];
			struct { float x, y, z; };
		};
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GEO_VECTOR_HPP) */

/**
 *************************************************************************
 *
 * @file geo_vector.cpp
 *
 * Vectors in 3D space, implementation.
 *
 ************************************************************************/


#include "geo_vector.hpp"


#include <math.h>


#include "util.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// vector
//
//////////////////////////////////////////////////////////////////////////


vector::vector() NOTHROW
	: x(0), y(0), z(0) { }

vector::vector(float x, float y, float z) NOTHROW
	: x(x), y(y), z(z) { }


const float& vector::operator [](int64 index) const NOTHROW
	{ return c[index]; }
float& vector::operator [](int64 index) NOTHROW
	{ return c[index]; }


vector vector::operator -() const NOTHROW
	{ return vector(-x, -y, -z); }

vector vector::operator +(const vector &v2) const NOTHROW
	{ return vector(x + v2.x, y + v2.y, z + v2.z); }
vector vector::operator -(const vector &v2) const NOTHROW
	{ return vector(x - v2.x, y - v2.y, z - v2.z); }
vector vector::operator *(const vector &v2) const NOTHROW
	{ return vector(x * v2.x, y * v2.y, z * v2.z); }
vector vector::operator /(const vector &v2) const NOTHROW
	{ return vector(x / v2.x, y / v2.y, z / v2.z); }

vector& vector::operator +=(const vector &v2) NOTHROW
	{ x += v2.x; y += v2.y; z += v2.z; return *this; }
vector& vector::operator -=(const vector &v2) NOTHROW
	{ x -= v2.x; y -= v2.y; z -= v2.z; return *this; }
vector& vector::operator *=(const vector &v2) NOTHROW
	{ x *= v2.x; y *= v2.y; z *= v2.z; return *this; }
vector& vector::operator /=(const vector &v2) NOTHROW
	{ x /= v2.x; y /= v2.y; z /= v2.z; return *this; }

vector vector::operator *(float val) const NOTHROW
	{ return vector(x * val, y * val, z * val); }

vector vector::operator /(float val) const NOTHROW
{
	float invval = 1 / val;
	return vector(x * invval, y * invval, z * invval);
}

vector& vector::operator *=(float val) NOTHROW
	{ x *= val; y *= val; z *= val; return *this; }

vector& vector::operator /=(float val) NOTHROW
{
	float invval = 1 / val;
	x *= invval; y *= invval; z *= invval;
	return *this;
}

bool vector::operator ==(const vector &v2) const NOTHROW
	{ return x == v2.x && y == v2.y && z == v2.z; }
bool vector::operator !=(const vector &v2) const NOTHROW
	{ return x != v2.x || y != v2.y || z != v2.z; }


void vector::swap_with(vector& other) NOTHROW
{
	swap(x, other.x);
	swap(y, other.y);
	swap(z, other.z);
}


float vector::length() const NOTHROW
	{ return sqrtf(x * x + y * y + z * z); }


vector vector::normalized() const NOTHROW
{
	float len = length();
	float invlen = (len < normalize_epsilon) ? 0 : (1 / len);
	return vector(x * invlen, y * invlen, z * invlen);
}

void vector::normalize() NOTHROW
{
	float len = length();
	float invlen = (len < normalize_epsilon) ? 0 : (1 / len);
	x *= invlen; y *= invlen; z *= invlen;
}


vector vector::normalized_absmax() const NOTHROW
{
	float am = absmax();
	float invam = (am < normalize_epsilon) ? 0 : (1 / am);
	return vector(x * invam, y * invam, z * invam);
}

void vector::normalize_absmax() NOTHROW
{
	float am = absmax();
	float invam = (am < normalize_epsilon) ? 0 : (1 / am);
	x *= invam; y *= invam; z *= invam;
}


float vector::dot(const vector &v2) const NOTHROW
	{ return x * v2.x + y * v2.y + z * v2.z; }

vector vector::cross(const vector &v2) const NOTHROW
{
	return
		vector(y * v2.z - z * v2.y,
			   z * v2.x - x * v2.z,
			   x * v2.y - y * v2.x);
}


float vector::min() const NOTHROW
	{ return viral_core::min(x, viral_core::min(y, z)); }
float vector::max() const NOTHROW
	{ return viral_core::max(x, viral_core::max(y, z)); }
float vector::absmin() const NOTHROW
	{ return viral_core::min(fabsf(x), viral_core::min(fabsf(y), fabsf(z))); }
float vector::absmax() const NOTHROW
	{ return viral_core::max(fabsf(x), viral_core::max(fabsf(y), fabsf(z))); }


vector vector::min(const vector& v2) const NOTHROW
{
	return vector(viral_core::min(x, v2.x),
				  viral_core::min(y, v2.y),
				  viral_core::min(z, v2.z));
}

vector vector::max(const vector& v2) const NOTHROW
{
	return vector(viral_core::max(x, v2.x),
				  viral_core::max(y, v2.y),
				  viral_core::max(z, v2.z));
}

vector vector::abs() const NOTHROW
	{ return vector(fabsf(x), fabsf(y), fabsf(z)); }

vector vector::floor() const NOTHROW
	{ return vector(::floorf(x), ::floorf(y), ::floorf(z)); }
vector vector::ceil() const NOTHROW
	{ return vector(::ceilf(x), ::ceilf(y), ::ceilf(z)); }


float vector::sum() const NOTHROW
	{ return x + y + z; }
float vector::square_sum() const NOTHROW
	{ return x * x + y * y + z * z; }
float vector::product() const NOTHROW
	{ return x * y * z; }


bool vector::nearly_equal(const vector &v2) const NOTHROW
{
	return nearly_equal(v2, nearly_equal_epsilon_constant,
							nearly_equal_epsilon_linear);
}

bool vector::nearly_equal
	(const vector &v2,
	 float epsilon_constant,
	 float epsilon_linear) const NOTHROW
{
	return viral_core::nearly_equal
				(x, v2.x, epsilon_constant, epsilon_linear) &&
		   viral_core::nearly_equal
				(y, v2.y, epsilon_constant, epsilon_linear) &&
		   viral_core::nearly_equal
				(z, v2.z, epsilon_constant, epsilon_linear);
}


const float vector::nearly_equal_epsilon_constant =
	viral_core::nearly_equal_epsilon_constant;
const float vector::nearly_equal_epsilon_linear =
	viral_core::nearly_equal_epsilon_linear;


/**
 * A 1e-10f epsilon gives enough leeway for vectors
 * with coordinates in the 1e10f range.
 */
const float vector::normalize_epsilon = 1e-10f;




} /* namespace viral_core */

/**
 *************************************************************************
 *
 * @file geo_transform.cpp
 *
 * Affine transformations, implementation.
 *
 ************************************************************************/


#include "geo_transform.hpp"


#include <math.h>


#include "geo_vector.hpp"
#include "geo_util.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// rotation
//
//////////////////////////////////////////////////////////////////////////


rotation::rotation() NOTHROW
	: w(1), x(0), y(0), z(0) { }


rotation::rotation
	(const vector& axis, float angle) NOTHROW
{
	// Notes:
	// - The incoming axis (potentially non-unit)
	//		must be normalized. Then, the resulting
	//		quaternion automatically has unit length.
	// - Consider that w = cosf(angle * 0.5) is near 1
	//		whenever sinf(angle * 0.5) is near 0.
	//		Hence quaternion length is 1 and normalization
	//		always is stable.

	if (axis.nearly_equal(vector(0, 0, 0)))
		{ w = 1; x = y = z = 0; return; }

	w = cosf(angle * geo_constants::degrees_to_radians * 0.5f);
	float sinangle =
		sinf(angle * geo_constants::degrees_to_radians * 0.5f);

	vector naxis(axis.normalized());
	x = naxis.x * sinangle;
	y = naxis.y * sinangle;
	z = naxis.z * sinangle;
}


rotation::rotation
	(const vector& ax, const vector& ay, const vector& az) NOTHROW
{
	// Robust conversion of rotation matrix
	// (ax, ay, az) to quaternion.
	float trace = ax.x + ay.y + az.z;

	if (trace > 0)
	{ 
		w = 0.5f * sqrtf(1 + trace);		
		float inv_w = 0.25f / w;

		x = (ay.z - az.y) * inv_w;
		y = (az.x - ax.z) * inv_w; 
		z = (ax.y - ay.x) * inv_w;
	}
	else if (ax.x > ay.y && ax.x > az.z)
	{ 
		x = 0.5f * sqrtf(1 + ax.x - ay.y - az.z);
		float inv_x = 0.25f / x;

		w = (ay.z - az.y) * inv_x;
		y = (ax.y + ay.x) * inv_x; 
		z = (ax.z + az.x) * inv_x; 
	}
	else if (ay.y > az.z)
	{ 
		y = 0.5f * sqrtf(1 + ay.y - ax.x - az.z);
		float inv_y = 0.25f / y;

		w = (az.x - ax.z) * inv_y;
		x = (ay.x + ax.y) * inv_y; 
		z = (ay.z + az.y) * inv_y; 
	}
	else
	{ 
		z = 0.5f * sqrtf(1 + az.z - ax.x - ay.y);
		float inv_z = 0.25f / z;

		w = (ax.y - ay.x) * inv_z;
		x = (az.x + ax.z) * inv_z;
		y = (az.y + ay.z) * inv_z;
	}


	// Just in case matrix was not orthonormal.
	normalize();
}


rotation rotation::quaternion
	(float w, float x, float y, float z) NOTHROW
		{ return rotation(w, x, y, z); }


vector rotation::axis() const NOTHROW
{
	float a = angle();
	float sina =
		sinf(a * geo_constants::degrees_to_radians * 0.5f);

	if (nearly_equal(sina, 0))
		return vector(1, 0, 0);

	float sina_inv = 1 / sina;
	return vector(x * sina_inv, y * sina_inv, z * sina_inv);
}

float rotation::angle() const NOTHROW
{
	// Below, acosf(w) * 2 gives an angle in [0°, 360°].
	// This works because w = cosf(angle * 2) ensures
	// unique cosines over the full 360 degrees range.
	return acosf(w) * 2 * geo_constants::radians_to_degrees;
}


const float& rotation::operator [](int64 index) const NOTHROW
	{ return c[index]; }
float& rotation::operator [](int64 index) NOTHROW
	{ return c[index]; }


rotation rotation::operator -() const NOTHROW
	{ return rotation(w, -x, -y, -z); }


rotation rotation::operator *(const rotation& q2) const NOTHROW
{
	return
		rotation(w * q2.w - x * q2.x - y * q2.y - z * q2.z,
				 w * q2.x + x * q2.w + y * q2.z - z * q2.y,
				 w * q2.y - x * q2.z + y * q2.w + z * q2.x,
				 w * q2.z + x * q2.y - y * q2.x + z * q2.w);
}

rotation& rotation::operator *=(const rotation& q2) NOTHROW
	{ return (*this) = (*this) * q2; }

vector rotation::operator *(const vector& vect) const NOTHROW
{
	// Build a 3x3 matrix from the rotation (via some temporaries),
	// rotate the vector by the matrix, and return result.
	float
		xx = 2 * x * x,    yy = 2 * y * y,    zz = 2 * z * z,
		wx = 2 * w * x,    wy = 2 * w * y,    wz = 2 * w * z,
		xy = 2 * x * y,    yz = 2 * y * z,    zx = 2 * z * x;

	float
		c0 = 1 - yy - zz,	c1 = xy - wz,		c2 = zx + wy,
		c4 = xy + wz,		c5 = 1 - zz - xx,	c6 = yz - wx,
		c8 = zx - wy,		c9 = yz + wx,		c22 = 1 - xx - yy;

	return
		vector(c0 * vect.x + c1 * vect.y + c2 * vect.z,
			   c4 * vect.x + c5 * vect.y + c6 * vect.z,
			   c8 * vect.x + c9 * vect.y + c22 * vect.z);
}

bool rotation::operator ==(const rotation& q2) const NOTHROW
	{ return w == q2.w && x == q2.x && y == q2.y && z == q2.z; }

bool rotation::operator !=(const rotation& q2) const NOTHROW
	{ return w != q2.w || x != q2.x || y != q2.y || z != q2.z; }


void rotation::swap_with(rotation& other) NOTHROW
{
	swap(w, other.w);
	swap(x, other.x);
	swap(y, other.y);
	swap(z, other.z);
}


float rotation::magnitude() const NOTHROW
	{ return sqrtf(w * w + x * x + y * y + z * z); }

rotation rotation::normalized() const NOTHROW
{
	float invlen = 1 / sqrtf(w * w + x * x + y * y + z * z);
	return rotation(w * invlen, x * invlen, y * invlen, z * invlen);
}

void rotation::normalize() NOTHROW
{
	float invlen = 1 / sqrtf(w * w + x * x + y * y + z * z);
	w *= invlen; x *= invlen; y *= invlen; z *= invlen;
}


rotation::rotation(float w, float x, float y, float z) NOTHROW
	: w(w), x(x), y(y), z(z)
{
	/**
	 * Private on purpose. Creation of a non-unit rotation
	 * requires a more explicit call to quaternion().
	 */
}




//////////////////////////////////////////////////////////////////////////
//
// matrix
//
//////////////////////////////////////////////////////////////////////////


matrix::matrix() NOTHROW
	:
	c00(1), c01(0), c02(0), c03(0),
	c10(0), c11(1), c12(0), c13(0),
	c20(0), c21(0), c22(1), c23(0),
	c30(0), c31(0), c32(0), c33(1)
{ }


matrix::matrix
	(float c00, float c01, float c02, float c03,
	 float c10, float c11, float c12, float c13,
	 float c20, float c21, float c22, float c23,
	 float c30, float c31, float c32, float c33) NOTHROW
	:
	c00(c00), c01(c01), c02(c02), c03(c03),
	c10(c10), c11(c11), c12(c12), c13(c13),
	c20(c20), c21(c21), c22(c22), c23(c23),
	c30(c30), c31(c31), c32(c32), c33(c33)
{ }


const float& matrix::operator [](int64 index) const NOTHROW
	{ return c[index]; }
float& matrix::operator [](int64 index) NOTHROW
	{ return c[index]; }


vector matrix::x_axis() const NOTHROW { return vector(c00, c10, c20); }
vector matrix::y_axis() const NOTHROW { return vector(c01, c11, c21); }
vector matrix::z_axis() const NOTHROW { return vector(c02, c12, c22); }
vector matrix::origin() const NOTHROW { return vector(c03, c13, c23); }


bool matrix::operator ==(const matrix& other) const NOTHROW
{
	return
		c00 == other.c00 && c01 == other.c01 && c02 == other.c02 && c03 == other.c03 &&
		c10 == other.c10 && c11 == other.c11 && c12 == other.c12 && c13 == other.c13 &&
		c20 == other.c20 && c21 == other.c21 && c22 == other.c22 && c23 == other.c23 &&
		c30 == other.c30 && c31 == other.c31 && c32 == other.c32 && c33 == other.c33;
}

bool matrix::operator !=(const matrix& other) const NOTHROW
{
	return
		c00 != other.c00 || c01 != other.c01 || c02 != other.c02 || c03 != other.c03 ||
		c10 != other.c10 || c11 != other.c11 || c12 != other.c12 || c13 != other.c13 ||
		c20 != other.c20 || c21 != other.c21 || c22 != other.c22 || c23 != other.c23 ||
		c30 != other.c30 || c31 != other.c31 || c32 != other.c32 || c33 != other.c33;
}


bool matrix::nearly_equal(const matrix& m2) const NOTHROW
	{ return nearly_equal(m2, nearly_equal_epsilon_constant,
							  nearly_equal_epsilon_linear); }


bool matrix::nearly_equal
	(const matrix& m2,
	 float epsilon_constant,
	 float epsilon_linear) const NOTHROW
{
	return viral_core::nearly_equal(c00, m2.c00, epsilon_constant, epsilon_linear) &&
		   viral_core::nearly_equal(c01, m2.c01, epsilon_constant, epsilon_linear) &&
		   viral_core::nearly_equal(c02, m2.c02, epsilon_constant, epsilon_linear) &&
		   viral_core::nearly_equal(c03, m2.c03, epsilon_constant, epsilon_linear) &&

		   viral_core::nearly_equal(c10, m2.c10, epsilon_constant, epsilon_linear) &&
		   viral_core::nearly_equal(c11, m2.c11, epsilon_constant, epsilon_linear) &&
		   viral_core::nearly_equal(c12, m2.c12, epsilon_constant, epsilon_linear) &&
		   viral_core::nearly_equal(c13, m2.c13, epsilon_constant, epsilon_linear) &&

		   viral_core::nearly_equal(c20, m2.c20, epsilon_constant, epsilon_linear) &&
		   viral_core::nearly_equal(c21, m2.c21, epsilon_constant, epsilon_linear) &&
		   viral_core::nearly_equal(c22, m2.c22, epsilon_constant, epsilon_linear) &&
		   viral_core::nearly_equal(c23, m2.c23, epsilon_constant, epsilon_linear) &&

		   viral_core::nearly_equal(c30, m2.c30, epsilon_constant, epsilon_linear) &&
		   viral_core::nearly_equal(c31, m2.c31, epsilon_constant, epsilon_linear) &&
		   viral_core::nearly_equal(c32, m2.c32, epsilon_constant, epsilon_linear) &&
		   viral_core::nearly_equal(c33, m2.c33, epsilon_constant, epsilon_linear);
}


void matrix::swap_with(matrix& other) NOTHROW
{
	swap(c00, other.c00); swap(c01, other.c01); swap(c02, other.c02); swap(c03, other.c03);
	swap(c10, other.c10); swap(c11, other.c11); swap(c12, other.c12); swap(c13, other.c13);
	swap(c20, other.c20); swap(c21, other.c21); swap(c22, other.c22); swap(c23, other.c23);
	swap(c30, other.c30); swap(c31, other.c31); swap(c32, other.c32); swap(c33, other.c33);
}


matrix matrix::transposed() const NOTHROW
{
	return matrix
		(c00, c10, c20, c30,
		 c01, c11, c21, c31,
		 c02, c12, c22, c32, 
		 c03, c13, c23, c33);
}


matrix matrix::zero() NOTHROW
	{ return matrix(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0); }


matrix matrix::translate(const vector& translate) NOTHROW
{
	return matrix
		(1, 0, 0, translate.x,
		 0, 1, 0, translate.y,
		 0, 0, 1, translate.z,
		 0, 0, 0, 1);
}


matrix matrix::rotate(const rotation &rotate) NOTHROW
{
	float
		xx = 2 * rotate.x * rotate.x,
		yy = 2 * rotate.y * rotate.y,
		zz = 2 * rotate.z * rotate.z,

		wx = 2 * rotate.w * rotate.x,
		wy = 2 * rotate.w * rotate.y,
		wz = 2 * rotate.w * rotate.z,

		xy = 2 * rotate.x * rotate.y,
		yz = 2 * rotate.y * rotate.z,
		zx = 2 * rotate.z * rotate.x;

	float
		r00 = 1 - yy - zz,	r01 = xy - wz,		r02 = zx + wy,
		r10 = xy + wz,		r11 = 1 - zz - xx,	r12 = yz - wx,
		r20 = zx - wy,		r21 = yz + wx,		r22 = 1 - xx - yy;

	return matrix
		(r00, r01, r02, 0,
		 r10, r11, r12, 0,
		 r20, r21, r22, 0,
		 0, 0, 0, 1);
}


matrix matrix::scale(const vector& factor) NOTHROW
{
	return matrix
		(factor.x, 0, 0, 0,
		 0, factor.y, 0, 0,
		 0, 0, factor.z, 0,
		 0, 0, 0, 1);		
}


matrix matrix::scale(float factor) NOTHROW
{
	return matrix
		(factor, 0, 0, 0,
		 0, factor, 0, 0,
		 0, 0, factor, 0,
		 0, 0, 0, 1);
}


matrix matrix::invert_rigid() const NOTHROW
{
	// Idea: Mp = p' => Rp + t = p'.
	// Invert, then exploit R^-1 = R^t
	// for orthonormal matrices.

	return matrix
		(c00, c10, c20, -(c00 * c03 + c10 * c13 + c20 * c23),
		 c01, c11, c21, -(c01 * c03 + c11 * c13 + c21 * c23),
		 c02, c12, c22, -(c02 * c03 + c12 * c13 + c22 * c23),
		 0, 0, 0, 1);
}


matrix matrix::invert_affine() const NOTHROW
{
	// Idea: Mp = p' => Xp + t = p',
	// where X is an arbitrary 3x3 matrix.
	// Invert X, then continue as above.

	float det_0022 =
		c00 * c11 * c22 - c00 * c12 * c21 +
		c01 * c12 * c20 - c01 * c10 * c22 +
		c02 * c10 * c21 - c02 * c11 * c20;
	
	if (fabs(det_0022) < invert_epsilon)
		return zero();

	float
		invdet = 1.f / det_0022,

		i00 = c11 * c22 - c21 * c12,
		i01 = c02 * c21 - c22 * c01,
		i02 = c01 * c12 - c11 * c02,

		i10 = c12 * c20 - c22 * c10,
		i11 = c00 * c22 - c20 * c02,
		i12 = c02 * c10 - c12 * c00,

		i20 = c10 * c21 - c20 * c11,
		i21 = c01 * c20 - c21 * c00,
		i22 = c00 * c11 - c10 * c01;

	return matrix
		(invdet * i00, invdet * i01, invdet * i02,
			-invdet * (i00 * c03 + i01 * c13 + i02 * c23),
		 invdet * i10, invdet * i11, invdet * i12,
			-invdet * (i10 * c03 + i11 * c13 + i12 * c23),
		 invdet * i20, invdet * i21, invdet * i22,
			-invdet * (i20 * c03 + i21 * c13 + i22 * c23),
		 0, 0, 0, 1);
}


matrix matrix::ortho
	(float sizex, float sizey, float sizez) NOTHROW
{
	float
		scalex = 2 / sizex,
		scaley = 2 / sizey,
		scalez = 2 / sizez;

	return
		scale(vector(scalex, scaley, scalez));
}


matrix matrix::inverse_ortho
	(float sizex, float sizey, float sizez) NOTHROW
{
	float
		invscalex = sizex / 2,
		invscaley = sizey / 2,
		invscalez = sizez / 2;

	return
		scale(vector(invscalex, invscaley, invscalez));
}


matrix matrix::perspective
	(float sizex, float sizey, float nearz, float farz) NOTHROW
{
	float
		scalex = 2 * nearz / sizex,
		scaley = 2 * nearz / sizey,

		scalez = (farz + nearz) / (farz - nearz), 
		movez = - (2 * farz * nearz) / (farz - nearz),
		depth = 1;

	return matrix
		(scalex, 0, 0, 0,
		 0, scaley, 0, 0,
		 0, 0, scalez, movez,
		 0, 0, depth, 0);
}


matrix matrix::inverse_perspective
	(float sizex, float sizey, float nearz, float farz) NOTHROW
{
	float
		invscalex = sizex / (2 * nearz),
		invscaley = sizey / (2 * nearz),

		invw = (farz + nearz) / (2 * farz * nearz), 
		invmovez = 1,
		invdepth = -(farz - nearz) / (2 * farz * nearz);

	return matrix
		(invscalex, 0, 0, 0,
		 0, invscaley, 0, 0,
		 0, 0, 0, invmovez,
		 0, 0, invdepth, invw);
}


matrix matrix::operator *(float factor) const NOTHROW
{
	return matrix
		(c00 * factor, c01 * factor, c02 * factor, c03 * factor,
		 c10 * factor, c11 * factor, c12 * factor, c13 * factor, 
		 c20 * factor, c21 * factor, c22 * factor, c23 * factor, 
		 c30 * factor, c31 * factor, c32 * factor, c33 * factor);
}


matrix& matrix::operator *=(float factor) NOTHROW
{
	*this = *this * factor;
	return *this;
}


matrix matrix::operator *(const matrix &m2) const NOTHROW
{
	return matrix
		(c00 * m2.c00 + c01 * m2.c10 + c02 * m2.c20 + c03 * m2.c30,
		 c00 * m2.c01 + c01 * m2.c11 + c02 * m2.c21 + c03 * m2.c31, 
		 c00 * m2.c02 + c01 * m2.c12 + c02 * m2.c22 + c03 * m2.c32, 
		 c00 * m2.c03 + c01 * m2.c13 + c02 * m2.c23 + c03 * m2.c33,

		 c10 * m2.c00 + c11 * m2.c10 + c12 * m2.c20 + c13 * m2.c30,
		 c10 * m2.c01 + c11 * m2.c11 + c12 * m2.c21 + c13 * m2.c31,
		 c10 * m2.c02 + c11 * m2.c12 + c12 * m2.c22 + c13 * m2.c32,
		 c10 * m2.c03 + c11 * m2.c13 + c12 * m2.c23 + c13 * m2.c33,

		 c20 * m2.c00 + c21 * m2.c10 + c22 * m2.c20 + c23 * m2.c30,
		 c20 * m2.c01 + c21 * m2.c11 + c22 * m2.c21 + c23 * m2.c31,
		 c20 * m2.c02 + c21 * m2.c12 + c22 * m2.c22 + c23 * m2.c32,
		 c20 * m2.c03 + c21 * m2.c13 + c22 * m2.c23 + c23 * m2.c33,

		 c30 * m2.c00 + c31 * m2.c10 + c32 * m2.c20 + c33 * m2.c30,
		 c30 * m2.c01 + c31 * m2.c11 + c32 * m2.c21 + c33 * m2.c31,
		 c30 * m2.c02 + c31 * m2.c12 + c32 * m2.c22 + c33 * m2.c32,
		 c30 * m2.c03 + c31 * m2.c13 + c32 * m2.c23 + c33 * m2.c33);
}


matrix& matrix::operator *=(const matrix& m2) NOTHROW
{
	*this = *this * m2;
	return *this;
}


vector matrix::operator *(const vector& vect) const NOTHROW
{
	return vector
		(c00 * vect.x + c01 * vect.y + c02 * vect.z + c03,
		 c10 * vect.x + c11 * vect.y + c12 * vect.z + c13,
		 c20 * vect.x + c21 * vect.y + c22 * vect.z + c23);
}


vector matrix::project(const vector& vect) const NOTHROW
{
	// Inverse w is required for rehomogenizing.
	// This works for projections with both perspective()
	// and inverse_perspective() matrices.
	float invw = 
		1 / (c30 * vect.x + c31 * vect.y + c32 * vect.z + c33);

	return vector
		((c00 * vect.x + c01 * vect.y + c02 * vect.z + c03) * invw,
		 (c10 * vect.x + c11 * vect.y + c12 * vect.z + c13) * invw,
		 (c20 * vect.x + c21 * vect.y + c22 * vect.z + c23) * invw);
}


vector matrix::transform_direction(const vector &direction) const NOTHROW
{
	return vector
		(c00 * direction.x + c01 * direction.y + c02 * direction.z,
		 c10 * direction.x + c11 * direction.y + c12 * direction.z,
		 c20 * direction.x + c21 * direction.y + c22 * direction.z);
}


const float matrix::nearly_equal_epsilon_constant =
	viral_core::nearly_equal_epsilon_constant;
const float matrix::nearly_equal_epsilon_linear =
	viral_core::nearly_equal_epsilon_linear;


/**
 * A 1e-10f epsilon gives enough leeway for matrices
 * with coordinates in the 1e10f range.
 */
const float matrix::invert_epsilon = 1e-10f;




//////////////////////////////////////////////////////////////////////////
//
// si_transform
//
//////////////////////////////////////////////////////////////////////////


si_transform::si_transform() NOTHROW
	:
	scale(1.f)
{ }


si_transform::si_transform
	(const vector& position,
	 const rotation& orientation,
	 float scale) NOTHROW
	:
	position(position),
	orientation(orientation),
	scale(scale)
{ }


si_transform si_transform::invert() const NOTHROW
{
	/**
	 * Implementation rationale:
	 *
	 * Solve
	 *		O * s * v + p = v'
	 * for v.
	 *
	 * => O * s * v = v' - p
	 * => v = (1/s) * (O^-1) * (v' - p)
	 * => v = (O^-1) * (1/s) * v' - (O^-1) * (1/s) * p
	 */

	if (fabs(scale) < invert_scale_epsilon)
		return si_transform();

	float inv_scale = 1.f / scale;
	rotation inv_orientation(-orientation);

	return si_transform
			((inv_orientation * (-position)) * inv_scale,
			 inv_orientation, inv_scale);
}


si_transform si_transform::operator *
	(const si_transform& other) const NOTHROW
{
	/**
	 * Implementation rationale:
	 *
	 * Simplify
	 *		v' = O * s * v + p,
	 *		v'' = O' * s' * v' + p'.
	 *
	 * => v'' = O' * s' * (O * s * v + p) + p'
	 * => v'' = O' * s' * O * s * v + O' * s' * p + p'
	 * => v'' = O' * O * s' * s * v + O' * s' * p + p'
	 */

	return si_transform
			(orientation * (other.position * scale) + position,
			 orientation * other.orientation,
			 scale * other.scale);
}


si_transform& si_transform::operator *=
	(const si_transform& other) NOTHROW
		{ *this = *this * other; return *this; }


vector si_transform::operator *(const vector& vect) const NOTHROW
	{ return orientation * (vect * scale) + position; }

vector si_transform::transform_direction
	(const vector &direction) const NOTHROW
		{ return (orientation * direction) * (scale < 0 ? -1.f : 1.f); }


matrix si_transform::build_local_to_world() const NOTHROW
{
	return
		matrix::translate(position) *
		matrix::rotate(orientation) *
		matrix::scale(scale);	
}


matrix si_transform::build_world_to_local() const NOTHROW
{
	return
		matrix::scale(1.f / scale) *
		matrix::rotate(-orientation) *
		matrix::translate(-position);
}


matrix si_transform::build_normals_to_world() const NOTHROW
{
	return
		matrix::rotate(orientation) *
		matrix::scale((scale < 0) ? -1.f : 1.f);
}


matrix si_transform::build_normals_to_local() const NOTHROW
{
	return
		matrix::scale((scale < 0) ? -1.f : 1.f) *
		matrix::rotate(-orientation);
}


si_transform si_transform::from_matrix(const matrix& m)
{
	vector x_axis(m.x_axis());
	vector y_axis(m.y_axis());
	vector z_axis(m.z_axis());

	float scale = (1.f / 3.f) *
		(x_axis.length() + y_axis.length() + z_axis.length()) *
		((x_axis.cross(y_axis).dot(z_axis) < 0) ? -1.f : 1.f);

	float inv_scale =
		(fabsf(scale) > from_matrix_scale_epsilon) ?
			(1.f / scale) : 0.f;

	rotation orientation
		(x_axis * inv_scale,
		 y_axis * inv_scale,
		 z_axis * inv_scale);

	return si_transform(m.origin(), orientation, scale);
}


bool si_transform::representable(const matrix& m)
{
	float x_len = m.x_axis().length();
	float y_len = m.y_axis().length();
	float z_len = m.z_axis().length();

	if (!nearly_equal(x_len, y_len) ||
		!nearly_equal(y_len, z_len) ||
		!nearly_equal(z_len, x_len))
			return false;

	vector x_unit(m.x_axis().normalized());
	vector y_unit(m.y_axis().normalized());
	vector z_unit(m.z_axis().normalized());

	if (!nearly_equal(x_unit.dot(y_unit), 0.f, representable_epsilon, 0.f) ||
		!nearly_equal(y_unit.dot(z_unit), 0.f, representable_epsilon, 0.f) ||
		!nearly_equal(z_unit.dot(x_unit), 0.f, representable_epsilon, 0.f))
			return false;

	if (!nearly_equal(m.c30, 0.f) || !nearly_equal(m.c31, 0.f) ||
		!nearly_equal(m.c32, 0.f) || !nearly_equal(m.c33, 1.f))
			return false;

	return true;
}


const float si_transform::invert_scale_epsilon = 1e-10f;
const float si_transform::from_matrix_scale_epsilon = 1e-6f;
const float si_transform::representable_epsilon = 1e-5f;




} /* namespace viral_core */

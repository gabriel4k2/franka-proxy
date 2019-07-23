/**
 *************************************************************************
 *
 * @file geo_transform.hpp
 *
 * Affine transformations.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GEO_TRANSFORM_HPP)
#define INCLUDED__VIRAL_CORE__GEO_TRANSFORM_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"
#include "geo_vector.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class rotation
 * @ingroup geo_module
 *
 * 3D rotation class based on unit quaternions.
 *
 * Quaternion notes:
 * - @bref{w} is the real component.
 * - @bref{x}, @bref{y} and @bref{z} are the imaginary components.
 * - Non-unit quaternions may also be stored, but exhibit undefined
 *		(though uncritical) mathematical behaviour.
 *
 * Rotation conventions:
 * - A positive rotation of 90 degrees around any coordinate axis
 *		transfers another coordinate axis to its consecutive axis
 *		(i.e. x to y around z, y to z around x, z to x around y)
 *		in the shortest way possible.
 * - When looking against the direction of some rotation axis,
 *		default camera projections show positive rotation angles
 *		as counter-clock rotations.
 *
 ************************************************************************/
class rotation
{

	public:

		/** Create identity rotation. */
		rotation() NOTHROW;


		/**
		 * Create rotation of @bref{angle} degrees around
		 * a given, non-unit @bref{axis}.
		 */
		rotation(const vector& axis, float angle) NOTHROW;


		/**
		 * Create a rotation from local coordinates
		 * to given orthonormal basis @bref{(x,y,z)}
		 * in the world coordinate frame.
		 */
		rotation
			(const vector&x, const vector& y, const vector& z) NOTHROW;


		/**
		 * Directly create rotation from arbitrary
		 * (possibly non-unit) quaternion.
		 * @attent Do not pass in angle-axis values!
		 */
		static rotation quaternion
			(float w, float x, float y, float z) NOTHROW;


		/**
		 * Calculate normalized rotation axis and
		 * rotation angle in [0, 360] degrees.
		 * Returns an arbitrary, normalized axis
		 * for near-zero rotations.
		 *
		 * @attent Due to ambiguities, this call
		 * does not always return the angle and axis
		 * provided on construction, but instead
		 * may return (-axis, 360 - angle).
		 */
		//@{
		vector axis() const NOTHROW;
		float angle() const NOTHROW;
		//@}


		const float& operator [](int64 index) const NOTHROW;
		float& operator [](int64 index) NOTHROW;


		/** Invert rotation. */
		rotation operator -() const NOTHROW;

		/** Concat rotations. */
		//@{
		rotation operator *(const rotation& q2) const NOTHROW;
		rotation& operator *=(const rotation& q2) NOTHROW;
		//@}

		/** Rotate a vector. */
		vector operator *(const vector& vect) const NOTHROW;

		/** Epsilon-less comparison. */
		//@{
		bool operator ==(const rotation& q2) const NOTHROW;
		bool operator !=(const rotation& q2) const NOTHROW;
		//@}


		void swap_with(rotation& other) NOTHROW;


		/** Retrieve length of encapsulated quaternion, usually 1. */
		float magnitude() const NOTHROW;


		/** Quaternion normalization, undefined for near-null quats. */
		//@{
		rotation normalized() const NOTHROW;
		void normalize() NOTHROW;
		//@}


		union
		{
			float c[4];
			struct { float w, x, y, z; };
		};


	private:

		rotation(float w, float x, float y, float z) NOTHROW;
};




/**
 *************************************************************************
 *
 * @class matrix
 * @ingroup geo_module
 *
 * 4x4 geometric matrix in row-first layout.
 *
 ************************************************************************/
class matrix
{

	public:

		/** Construct 4x4 unit matrix. */
		matrix() NOTHROW;

		matrix
			(float c00, float c01, float c02, float c03,
			 float c10, float c11, float c12, float c13,
			 float c20, float c21, float c22, float c23,
			 float c30, float c31, float c32, float c33) NOTHROW;


		const float& operator [](int64 index) const NOTHROW;
		float& operator [](int64 index) NOTHROW;


		/**
		 * Retrieve an axis or the origin translation
		 * of the represented coordinate system.
		 */
		//@{
		vector x_axis() const NOTHROW;
		vector y_axis() const NOTHROW;
		vector z_axis() const NOTHROW;
		vector origin() const NOTHROW;
		//@}


		bool operator ==(const matrix& other) const NOTHROW;
		bool operator !=(const matrix& other) const NOTHROW;


		/**
		 * Test if two matrices are nearly equal
		 * under default or explicit epsilons.
		 * See viral_core::nearly_equal().
		 */
		//@{
		bool nearly_equal
			(const matrix& m2) const NOTHROW;

		bool nearly_equal
			(const matrix& m2,
			 float epsilon_constant,
			 float epsilon_linear) const NOTHROW;
		//@}


		void swap_with(matrix& other) NOTHROW;


		matrix transposed() const NOTHROW;


		/** Create a matrix with all-zero entries. */
		static matrix zero() NOTHROW;


		/** Create transformation matrix. */
		//@{
		static matrix translate(const vector &translate) NOTHROW;
		static matrix rotate(const rotation &rotate) NOTHROW;
		static matrix scale(const vector &factor) NOTHROW;
		static matrix scale(float factor) NOTHROW;
		//@}


		/**
		 * Fast inverse of rigid transformation,
		 * made only of translate()s and rotate()s.
		 */
		matrix invert_rigid() const NOTHROW;


		/**
		 * Inverse of general affine transformation,
		 * including scale() and shear components.
		 * (Near-)Singular input gives a zero() matrix.
		 */
		matrix invert_affine() const NOTHROW;


		/**
		 * Create orthographic or inverse orthographic projection matrix.
		 *
		 * An orthographic projection matrix uses an origin-centered
		 * viewing cube. It is defined by distances @bref{sizex} between
		 * left and right bounding planes, @bref{sizey} between top and
		 * bottom bounding planes, and @bref{sizez} between near and far
		 * bounding planes.
		 *
		 * After transformation, coordinates from within the viewing cube
		 * end up in the normalized cube [-1,1]x[-1,1]x[-1,1].
		 *
		 * Orthographic projection maintains orientation and handedness,
		 * e.g. incoming -y maps to -y within the normalized cube.
		 * See @ref camera for further camera axis conventions.
		 */
		//@{
		static matrix ortho
			(float sizex, float sizey, float sizez) NOTHROW;	
		static matrix inverse_ortho
			(float sizex, float sizey, float sizez) NOTHROW;
		//@}

		
		/**
		 * Create perspective or inverse perspective projection matrix.
		 *
		 * A perspective projection matrix uses an origin-rooted
		 * viewing frustum that looks along the positive z axis.
		 * It is defined by distances @bref{nearz} and @bref{farz}
		 * to the near and far depth planes, @bref{sizex} between
		 * left and right bounding planes, and @bref{sizey} between
		 * top and bottom bounding planes. The latter two are measured
		 * on the near depth plane.
		 *
		 * After transformation and homogenization, coordinates from
		 * within the viewing frustum end up in the normalized cube
		 * [-1,1]x[-1,1]x[-1,1].
		 *
		 * Perspective projection maintains orientation and handedness,
		 * e.g. incoming -y maps to -y within the normalized cube.
		 * See @ref camera for further camera axis conventions.
		 *
		 * @attent Perspective and inverse perspective matrices
		 * describe non-homogenous output vectors that must explicitly
		 * be homogenized. Normal matrix-vector multiplication
		 * does not do this, but project() must be used instead.
		 */
		//@{
		static matrix perspective
			(float sizex, float sizey, float nearz, float farz) NOTHROW;
		static matrix inverse_perspective
			(float sizex, float sizey, float nearz, float farz) NOTHROW;
		//@}


		/** Scale matrix with given @bref{factor}. */
		//@{
		matrix operator *(float factor) const NOTHROW;
		matrix& operator *=(float factor) NOTHROW;
		//@}


		/** Right multiply or multiply-assign with another matrix. */
		//@{
		matrix operator *(const matrix &m2) const NOTHROW;
		matrix& operator *=(const matrix &m2) NOTHROW; 
		//@}


		/**
		 * Affine transformation of a vector by this matrix,
		 * disregards any perspective stretching.
		 */
		vector operator *(const vector &vect) const NOTHROW;


		/**
		 * Project a vector by using this matrix, including any
		 * perspective-induced stretching and re-homogenization.
		 * In particular, projection is required for perspective()
		 * and inverse_perspective() matrices.
		 */
		vector project(const vector &vect) const NOTHROW;


		/**
		 * Transform a @bref{direction} vector by the rotation
		 * and scale within the upper 3x3 part of the matrix.
		 * If the matrix is orthonormal, only rotate the vector.
		 */
		vector transform_direction(const vector &direction) const NOTHROW;


		static const float nearly_equal_epsilon_constant;
		static const float nearly_equal_epsilon_linear;
		static const float invert_epsilon;


		union
		{
			float c[16];

			struct
			{
				float c00, c01, c02, c03,
					  c10, c11, c12, c13,
					  c20, c21, c22, c23,
					  c30, c31, c32, c33;
			};
		};
};




/**
 *************************************************************************
 *
 * @class si_transform
 * @ingroup geo_module
 *
 * A similarity transformation given through @bref{position},
 * @bref{orientation}, and uniform @bref{scale}.
 *
 ************************************************************************/
class si_transform
{

	public:

		/** Create default identity transform. */
		si_transform() NOTHROW;


		si_transform
			(const vector& position,
			 const rotation& orientation,
			 float scale) NOTHROW;


		/**
		 * Invert transformation.
		 * Returns identity for near-zero @bref{scale}.
		 */
		si_transform invert() const NOTHROW;


		/** Concat transformations. */
		//@{
		si_transform operator *(const si_transform& other) const NOTHROW;
		si_transform& operator *=(const si_transform& other) NOTHROW;
		//@}


		/** Transform a vector. */
		vector operator *(const vector& vect) const NOTHROW;


		/**
		 * Transform a directional vector while maintaining its length,
		 * disregarding any translation and enforcing a +1 or -1 scale.
		 */
		vector transform_direction(const vector &direction) const NOTHROW;


		/**
		 * Build matrices that transform among
		 * local and world coordinates.
		 *
		 * Variants for normals do not apply
		 * @bref{position} or @bref{scale}.
		 */
		//@{
		matrix build_local_to_world() const NOTHROW;
		matrix build_world_to_local() const NOTHROW;

		matrix build_normals_to_world() const NOTHROW;
		matrix build_normals_to_local() const NOTHROW;
		//@}


		/**
		 * Split transformation matrix into @bref{position},
		 * @bref{orientation}, and @bref{scale} components.
		 * Results are (somewhat) approximate for matrices
		 * that are not representable() as a si_transform.
		 */
		static si_transform from_matrix(const matrix& m);


		/**
		 * Test if some matrix maps to a si_transform.
		 * Such matrices may only express translation,
		 * rotation, and uniform scale, and must not
		 * have shear or non-uniform scale.
		 *
		 * Testing uses a small bias for false-positives
		 * in order to account for numerical precision.
		 */
		static bool representable(const matrix& m);


		vector position;
		rotation orientation;
		float scale;


		static const float invert_scale_epsilon;
		static const float from_matrix_scale_epsilon;
		static const float representable_epsilon;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GEO_TRANSFORM_HPP) */
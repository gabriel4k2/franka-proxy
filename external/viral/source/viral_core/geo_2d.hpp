/**
 *************************************************************************
 *
 * @file geo_2d.hpp
 *
 * Header for geometry framework, 2D primitives.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GEO_2D_HPP)
#define INCLUDED__VIRAL_CORE__GEO_2D_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class vector2i
 * @ingroup geo_module
 *
 * A 2D integer vector.
 *
 ************************************************************************/
class vector2i
{
	public:

		vector2i() NOTHROW;
		vector2i(int64 x, int64 y) NOTHROW;

		const int64& operator [](int64 index) const NOTHROW;
		int64& operator [](int64 index) NOTHROW;


		/** Usual vector arithmetics. */
		//@{
		vector2i operator -() const NOTHROW;

		vector2i operator +(const vector2i &v2) const NOTHROW;
		vector2i operator -(const vector2i &v2) const NOTHROW;
		vector2i operator *(const vector2i &v2) const NOTHROW;
		vector2i operator /(const vector2i &v2) const NOTHROW;

		vector2i& operator +=(const vector2i &v2) NOTHROW;
		vector2i& operator -=(const vector2i &v2) NOTHROW;
		vector2i& operator *=(const vector2i &v2) NOTHROW;
		vector2i& operator /=(const vector2i &v2) NOTHROW;

		vector2i operator *(int64 val) const NOTHROW;
		vector2i operator /(int64 val) const NOTHROW;
		vector2i& operator *=(int64 val) NOTHROW;
		vector2i& operator /=(int64 val) NOTHROW;

		bool operator ==(const vector2i &v2) const NOTHROW;
		bool operator !=(const vector2i &v2) const NOTHROW;
		//@}


		void swap_with(vector2i& other) NOTHROW;


		/** Per-component mathematical functions. */
		//@{
		int64 min() const NOTHROW;
		int64 max() const NOTHROW;
		vector2i min(const vector2i& v2) const NOTHROW;
		vector2i max(const vector2i& v2) const NOTHROW;
		vector2i abs() const NOTHROW;
		//@}


		/**
		 * Convert to a float vector as by static_cast.
		 * Approximate results on not representable vectors.
		 */
		vector2f to_vector2f() const NOTHROW;


		union
		{
			int64 c[2];
			struct { int64 x, y; };
		};
};




/**
 *************************************************************************
 * 
 * @class rect2i
 * @ingroup geo_module
 *
 * A 2D rectangle with integer coordinates.
 *
 * Notes:
 * - A rectangle contains all points @bref{pos} + @bref{a} * @bref{size},
 *		with @bref{a} in [0,1]x[0,1]. In particular, a rect2i holds points
 *		even for negative sizes, and at least consists of a single point.
 *
 ************************************************************************/
class rect2i
{

	public:

		/** Create default rectangle with pos (0, 0) and size (0, 0). */
		rect2i() NOTHROW;

		rect2i(int64 pos_x, int64 pos_y, int64 size_x, int64 size_y) NOTHROW;
		rect2i(const vector2i& pos, const vector2i& size) NOTHROW;

		void set(int64 pos_x, int64 pos_y, int64 size_x, int64 size_y) NOTHROW;
		void set(const vector2i& new_pos, const vector2i& new_size) NOTHROW;


		/** Calculate @bref{pos} + @bref{size} on the fly. */
		//@{
		vector2i end() const NOTHROW;
		int64 end_x() const NOTHROW;
		int64 end_y() const NOTHROW;
		//@}


		bool operator ==(const rect2i& other) const NOTHROW;
		bool operator !=(const rect2i& other) const NOTHROW;


		void swap_with(rect2i& other) NOTHROW;


		/** Test if a point is inside the rectangle. */
		//@{
		bool contains(int64 x, int64 y) const NOTHROW;
		bool contains(const vector2i& point) const NOTHROW;
		//@}


		/**
		 * Calculate rectangle that contains the same points
		 * as the original one, but with negated size values.
		 */
		rect2i flipped_rect() const NOTHROW;


		/**
		 * Grow to enclose a given point or another rectangle.
		 * The rectangle keeps the signs of its size variables,
		 * or grows to positive size if size is zero.
		 */
		//@{
		void grow_to(int64 x, int64 y) NOTHROW;
		void grow_to(const rect2i& other) NOTHROW;
		//@}


		/**
		 * Convert to a float rectangle as by static_cast.
		 * Approximate results on not representable rectangles.
		 */
		rect2f to_rect2f() const NOTHROW;


		vector2i pos;
		vector2i size;
};




/**
 *************************************************************************
 *
 * @class vector2f
 * @ingroup geo_module
 *
 * A 2D floating point vector.
 *
 ************************************************************************/
class vector2f
{
	public:

		vector2f() NOTHROW;
		vector2f(float x, float y) NOTHROW;

		const float& operator [](int64 index) const NOTHROW;
		float& operator [](int64 index) NOTHROW;


		/** Usual vector arithmetics. */
		//@{
		vector2f operator -() const NOTHROW;

		vector2f operator +(const vector2f &v2) const NOTHROW;
		vector2f operator -(const vector2f &v2) const NOTHROW;
		vector2f operator *(const vector2f &v2) const NOTHROW;
		vector2f operator /(const vector2f &v2) const NOTHROW;

		vector2f& operator +=(const vector2f &v2) NOTHROW;
		vector2f& operator -=(const vector2f &v2) NOTHROW;
		vector2f& operator *=(const vector2f &v2) NOTHROW;
		vector2f& operator /=(const vector2f &v2) NOTHROW;

		vector2f operator *(float val) const NOTHROW;
		vector2f operator /(float val) const NOTHROW;
		vector2f& operator *=(float val) NOTHROW;
		vector2f& operator /=(float val) NOTHROW;

		bool operator ==(const vector2f &v2) const NOTHROW;
		bool operator !=(const vector2f &v2) const NOTHROW;
		//@}


		void swap_with(vector2f& other) NOTHROW;


		float dot(const vector2f &v2) const NOTHROW;
		float length() const NOTHROW;


		/** Normalize vector, undefined for near-null vectors. */
		//@{
		vector2f normalized() const NOTHROW;
		void normalize() NOTHROW;
		//@}


		/** Per-component mathematical functions. */
		//@{
		float min() const NOTHROW;
		float max() const NOTHROW;
		vector2f min(const vector2f& v2) const NOTHROW;
		vector2f max(const vector2f& v2) const NOTHROW;
		vector2f abs() const NOTHROW;
		vector2f floor() const NOTHROW;
		vector2f ceil() const NOTHROW;
		//@}


		/**
		 * Convert to an integer vector.
		 * Clamp as by integer_operations::cast_or_clamp().
		 */
		vector2i to_vector2i() const NOTHROW;


		union
		{
			float c[2];
			struct { float x, y; };
		};
};




/**
 *************************************************************************
 * 
 * @class rect2f
 * @ingroup geo_module
 *
 * A 2D rectangle with floating point coordinates.
 *
 * Notes:
 * - A rectangle contains all points @bref{pos} + @bref{a} * @bref{size},
 *		with @bref{a} in [0,1]x[0,1]. In particular, a rect2f holds points
 *		even for negative sizes, and at least consists of a single point.
 *
 ************************************************************************/
class rect2f
{

	public:

		/** Create default rectangle with pos (0, 0) and size (0, 0). */
		rect2f() NOTHROW;

		rect2f(float pos_x, float pos_y, float size_x, float size_y) NOTHROW;
		rect2f(const vector2f& pos, const vector2f& size) NOTHROW;

		void set(float pos_x, float pos_y, float size_x, float size_y) NOTHROW;
		void set(const vector2f& new_pos, const vector2f& new_size) NOTHROW;


		/** Calculate @bref{pos} + @bref{size} on the fly. */
		//@{
		vector2f end() const NOTHROW;
		float end_x() const NOTHROW;
		float end_y() const NOTHROW;
		//@}


		bool operator ==(const rect2f& other) const NOTHROW;
		bool operator !=(const rect2f& other) const NOTHROW;


		void swap_with(rect2f& other) NOTHROW;


		/**
		 * Test if a point is inside the rectangle.
		 * Does not include any implicit epsilon.
		 */
		//@{
		bool contains(float x, float y) const NOTHROW;
		bool contains(const vector2f& point) const NOTHROW;
		//@}


		/**
		 * Build rectangle with both position and extents
		 * on the next lower integral border or rounded
		 * down to the next integer.
		 */
		//@{
		rect2f floor_rect() const NOTHROW;
		rect2f round_rect() const NOTHROW;
		//@}


		/**
		 * Calculate rectangle that contains the same points
		 * as the original one, but with negated size values.
		 */
		rect2f flipped_rect() const NOTHROW;


		/**
		 * Create a rectangle that contains the same points,
		 * but only has positive sizes.
		 */
		rect2f with_positive_size() const NOTHROW;


		/**
		 * Test if two rectangles share at least one common point,
		 * including any edge or corner vertices.
		 */
		bool intersecting(const rect2f& other) const NOTHROW;


		/**
		 * Determine if this rectangle intersects with an@bref{other} one.
		 * On any intersection, store the shared rectangle as @bref{result}.
		 * Signs on the result size match those of the calling rectangle.
		 * If there is no intersection, @bref{result} remains unchanged.
		 */
		bool try_intersect
			(const rect2f& other, rect2f &result) const NOTHROW;


		/**
		 * Grow to enclose a given point or another rectangle.
		 * The rectangle keeps the signs of its size variables.
		 */
		//@{
		void grow_to(float x, float y) NOTHROW;
		void grow_to(const rect2f& other) NOTHROW;
		//@}


		/**
		 * Performs a coordinate-wise linear mapping of the calling rectangle
		 * into an image space, where the mapping function is defined via the
		 * preimage and image of another rectangle.
		 *
		 * In detail, the call finds a scaling center point @bref{p} and
		 * a scaling factor @bref{f}, so that
		 *		(@bref{preimage} - @bref{p}) * @bref{f} + @bref{p} = @bref{image}.
		 * Then it calculates the same mapping on the calling rectangle.
		 * Thus the returned rectangle is
		 *		(@bref{this} - @bref{p}) * @bref{f} + @bref{p}.
		 *
		 * If the preimage rectangle extends by 0 in any direction,
		 * that direction and position are set to 0 in the result.
		 *
		 * A typical use case is to recalculate texture coordinates
		 * of a geometry rectangle after it has been clipped:
		 * @code
		 *   clippedtexcoord =
		 *		clippedrect.interpolate_mapping(origrect, origtexcoord);
		 * @endcode
		 */
		rect2f apply_linear_map
			(const rect2f& preimage, const rect2f& image) const NOTHROW;


		/**
		 * Convert to an integer rectangle.
		 * Clamp as by integer_operations::cast_or_clamp().
		 */
		rect2i to_rect2i() const NOTHROW;


		vector2f pos;
		vector2f size;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GEO_2D_HPP) */

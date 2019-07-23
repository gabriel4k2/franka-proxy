/**
 *************************************************************************
 *
 * @file geo_primitive.hpp
 *
 * Geometric primitives.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GEO_PRIMITIVE_HPP)
#define INCLUDED__VIRAL_CORE__GEO_PRIMITIVE_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"


#include "geo_vector.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class line
 * @ingroup geo_module
 *
 * A line that extends into infinity.
 *
 * Usage notes:
 * - A line contains all points @bref{origin} + @bref{a} * @bref{direction}
 *		for @bref{a} in ]-infinity, infinity[.
 * - Usually, line @bref{direction} should be normalized to a unit vector.
 *		Otherwise, lines may exhibit unexpected but well-defined behaviour.
 *		Note that lines do not automatically normalize incoming directions.
 * - Degenerated lines with a near-zero @bref{direction} vector are allowed
 *		and contain but a single point @bref{origin}.
 *
 ************************************************************************/
class line
{
	public:

		line() NOTHROW;
		line(const vector& origin, const vector& direction) NOTHROW;

		bool operator ==(const line& other) const NOTHROW;
		bool operator !=(const line& other) const NOTHROW;

		void swap_with(line& other) NOTHROW;


		vector origin;
		vector direction;
};




/**
 *************************************************************************
 *
 * @class segment
 * @ingroup geo_module
 *
 * A line segment, defined by start and end point.
 *
 * Usage notes:
 * - A line segment contains all points @bref{a} * @bref{start}
 *		to (1 - @bref{a}) * @bref{end} with @bref{a} in [0, 1].
 *		Notably, degenerate segments with @bref{start} = @bref{end}
 *		are explicitly allowed and consist only of a single point.
 *
 ************************************************************************/
class segment
{

	public:

		segment() NOTHROW;
		segment(const vector& start, const vector& end) NOTHROW;

		bool operator ==(const segment& other) const NOTHROW;
		bool operator !=(const segment& other) const NOTHROW;

		void swap_with(segment& other) NOTHROW;

		float length() const NOTHROW;


		/** Reset segment, as by constructor call. */
		void set(const vector& new_start, const vector& new_end) NOTHROW;


		/**
		 * Check if segment contains given @bref{point},
		 * either with default or explicit epsilons.
		 * On degenerate lines, only see if @bref{point}
		 * is nearly_equal() to @bref{start}.
		 */
		//@{
		bool contains(const vector& point) const NOTHROW;
		bool contains(const vector& point,
					  float segment_degenerate_epsilon,
					  float contains_epsilon_constant,
					  float contains_epsilon_linear) const NOTHROW;
		//@}


		/**
		 * Find point on segment closest to input @bref{point}.
		 * Always returns @bref{start} for degenerate segments.
		 */
		//@{	
		vector closest_on_segment(const vector& point) const NOTHROW;
		vector closest_on_segment(const vector& point,
								  float segment_degenerate_epsilon) const NOTHROW;
		//@}


		static const float default_segment_degenerate_epsilon;
		static const float default_contains_epsilon_constant;
		static const float default_contains_epsilon_linear;


		vector start;
		vector end;
};




/**
 *************************************************************************
 *
 * @class plane
 * @ingroup geo_module
 *
 * A plane defined by a normal and a distance along the normal between
 * the origin and the plane.
 *
 * Notes:
 * - Plane input data is not checked for consistency or wellformedness
 *		(e.g. null normal or collinear points). Notably, planes do not
 *		automatically normalize any incoming explicit plane normals.
 * - Invalid planes may show unexpected but well defined behaviour
 *		and may be desireable in certain cases.
 *
 ************************************************************************/
class plane
{

	public:

		/** Default plane with (0, 0, 0) normal and 0 distance. */
		plane() NOTHROW;

		/** Plane from normal vector and distance. */
		plane(const vector& normal, float distance) NOTHROW;

		/** Plane from normal coordinates and distance. */
		plane(float nx, float ny, float nz, float distance) NOTHROW;

		/** Plane through points, front-side points are counter-clockwise. */
		plane(const vector& a, const vector& b, const vector& c) NOTHROW;

		/** Plane through triangle, front-side points are counter-clockwise. */
		plane(const triangle& t) NOTHROW;

		/** Plane with given orientation through point. */
		plane(const vector& normal, const vector& point) NOTHROW;


		bool operator ==(const plane& other) const NOTHROW;
		bool operator !=(const plane& other) const NOTHROW;


		void swap_with(plane& other) NOTHROW;


		/** Reset plane, as by the appropriate constructor calls. */
		//@{
		void set(const vector& new_normal, float new_distance) NOTHROW;
		void set(float nx, float ny, float nz, float new_distance) NOTHROW;
		void set(const vector& a, const vector& b, const vector& c) NOTHROW;
		void set(const triangle& t) NOTHROW;
		void set(const vector& new_normal, const vector& point) NOTHROW;
		//@}


		/** Invert plane normal while maintaining on-plane points. */
		//@{
		void flip() NOTHROW;
		plane flipped() const NOTHROW;
		//@}


		/**
		 * Check if plane contains given @bref{point}
		 * with either default or explicit epsilons.
		 * See viral_core::nearly_equal().
		 */
		//@{
		bool contains(const vector& point) const NOTHROW;
		bool contains(const vector& point,
					  float contains_epsilon_constant,
					  float contains_epsilon_linear) const NOTHROW;
		//@}


		/**
		 * Retrieve positive (point in front of normal side) or negative
		 * (point behind normal side) distance of @bref{point} to plane.
		 */
		float signed_distance_to(const vector& point) const NOTHROW;		


		/** Find point on plane closest to input @bref{point}. */
		vector closest_on_plane(const vector& point) const NOTHROW;


		vector normal;
		float distance;


		static const float default_contains_epsilon_constant;
		static const float default_contains_epsilon_linear;
};




/**
 *************************************************************************
 *
 * @class triangle
 * @ingroup geo_module
 *
 * A triangle with three explicit vertices.
 *
 * Usage notes:
 * - Triangle front-faces are defined by counter-clockwise winding
 *		when one looks against the normal direction. This matches
 *		general @ref rotation conventions for positive angles.
 * - For efficiency reasons, a triangle explicitly stores its @bref{p}lane.
 *		While constructors automatically generate a consistent @ref plane,
 *		clients can still setup triangles with plane-vertex inconsistencies.
 *
 * Degenerate triangles:
 * - Creating degenerate() triangles (i.e. points, segments) is possible.
 *		Notably, constructors do not throw on linearly dependent edges.
 * - Degenerate or inconsistent triangles may behave unexpectedly,
 *		but behaviour still remains well-defined (e.g. no crashing).
 *
 ************************************************************************/
class triangle
{
	public:

		triangle() NOTHROW;
		triangle(const triangle& other) NOTHROW;
		triangle(const vector& a, const vector& b, const vector& c) NOTHROW;


		bool operator ==(const triangle& other) const NOTHROW;
		bool operator !=(const triangle& other) const NOTHROW;


		void swap_with(triangle& other) NOTHROW;


		/**
		 * Determine if the triangle is (almost) degenerate,
		 * optionally with a custom @bref{degenerate_epsilon}.
		 */
		//@{
		bool degenerate() const NOTHROW;
		bool degenerate(float degenerate_epsilon) const NOTHROW;
		//@}


		/**
		 * Convert in-between cartesian and (normalized) barycentric coordinates.
		 * Returns approximate results for points outside the triangle plane,
		 * for denormalized barycentric coordinates, and for degenerate() triangles.
		 */
		//@{
		vector to_barycentric(const vector& cartesian) const NOTHROW;
		vector from_barycentric(const vector& barycentric) const NOTHROW;
		//@}


		/**
		 * Determine if a given @bref{point} is on a triangle.
		 * Optionally, omit triangle plane distance checks,
		 * and deliver arbitrary results for any @bref{point}s
		 * not very close to the triangle @bref{p}lane.
		 *
		 * The @bref{contains_unchecked_epsilon} controls bias
		 * towards false-positives (greater than zero, the default)
		 * or false-negatives (less than zero). Even with biasing,
		 * results may be sketchy for degenerate() triangles.
		 */
		//@{
		bool contains(const vector& point) const NOTHROW;		
		bool contains(const vector& point,
					  float plane_epsilon_constant,
					  float plane_epsilon_linear,
					  float contains_unchecked_epsilon) const NOTHROW;	

		bool contains_unchecked(const vector& point) const NOTHROW;
		bool contains_unchecked(const vector& point,
								float contains_unchecked_epsilon) const NOTHROW;
		//@}


		/**
		 * Find minimum-radius bounding sphere or
		 * minimum-size bounding box around triangle.
		 * Boundings are tight (epsilon-less) fits,
		 * except for degenerate() triangles.
		 */
		//@{
		sphere build_bounding_sphere() const NOTHROW;
		box build_bounding_box() const NOTHROW;
		//@}


		/**
		 * Apply an arbitrary @bref{t}ransformation to this triangle.
		 *
		 * Usage notes:
		 * - Transform matrices are somewhat faster than si_transform%s.
		 */
		//@{
		triangle transform(const si_transform& t) const NOTHROW;
		triangle transform(const matrix& t) const NOTHROW;
		//@}


		static const float default_degenerate_epsilon;
		static const float barycentric_epsilon;
		static const float default_contains_unchecked_epsilon;		


		vector a, b, c;
		plane p;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GEO_PRIMITIVE_HPP) */
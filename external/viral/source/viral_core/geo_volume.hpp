/**
 *************************************************************************
 *
 * @file geo_volume.hpp
 *
 * Volumetric primitives.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GEO_VOLUME_HPP)
#define INCLUDED__VIRAL_CORE__GEO_VOLUME_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"


#include "geo_vector.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class sphere
 * @ingroup geo_module
 *
 * A sphere in 3D space, defined by @bref{position} and @bref{radius}.
 *
 * Notes:
 * - A sphere contains all points with a distance to @bref{position}
 *		equal to or less than @bref{radius}. Notably, this implies
 *		that a sphere with negative radius is empty, and a sphere
 *		with radius 0 contains a single point.
 *
 ************************************************************************/
class sphere
{
	public:

		/** Default sphere at position (0, 0, 0) and with radius 0. */
		sphere() NOTHROW;

		sphere(const vector& position, float radius) NOTHROW;


		bool operator ==(const sphere& other) const NOTHROW;
		bool operator !=(const sphere& other) const NOTHROW;


		void swap_with(sphere& other) NOTHROW;


		/** Determine volume of sphere, always zero or greater. */
		float volume() const NOTHROW;

		/** Test if the sphere contains no point (i.e. radius < 0). */
		bool empty() const NOTHROW;


		/**
		 * Test if two spheres share at least one point,
		 * including any point on the sphere surface.
		 * Gives approximate results, in particular
		 * for very small or very close spheres.
		 * Returns @false for empty() spheres.
		 */
		bool intersecting(const sphere& other) const NOTHROW;


		/**
		 * Determine if this sphere intersects with an@bref{other} sphere.
		 * In case of any intersection, store the @bref{intersection_center},
		 * the center of the lense shapes that result from sphere intersection.
		 * For small or very close spheres, results may be rough approximations.
		 * If there is no intersection (e.g. either input sphere is empty()),
		 * return @false and do not change @bref{intersection_center}.
		 */
		bool try_intersection_center
			(const sphere& other, vector &intersection_center) const NOTHROW;


		/** Determine volume of intersection with another sphere. */
		float intersection_volume(const sphere& other) const NOTHROW;


		/**
		 * Grow the sphere radius to enclose a given @bref{point}
		 * or sphere while maintaining the sphere @bref{position}.
		 */
		//@{
		void grow_radius_to(const vector &point) NOTHROW;
		void grow_radius_to(const sphere &s) NOTHROW;
		//@}


		/**
		 * Enclose an additional @bref{point} or sphere.
		 * Unlike grow_radius_to(), this call ensures minimal
		 * @bref{radius} increase by moving @bref{position}.
		 */
		//@{
		void enclose(const vector& point) NOTHROW;
		void enclose(const sphere& s) NOTHROW;
		//@}
		 

		/**
		 * Test whether a given point is inside the sphere.
		 * Only enlarges (or shrinks, on negative values) the sphere
		 * by @bref{epsilon} if explicitly specified.
		 */
		//@{
		bool contains(float x, float y, float z) const NOTHROW;
		bool contains(const vector& point) const NOTHROW;
		bool contains(float x, float y, float z, float epsilon) const NOTHROW;
		bool contains(const vector& point, float epsilon) const NOTHROW;
		//@}


		/**
		 * Find a tight-fit bounding box.
		 * Empty spheres return an empty, but still tight-bounding, box.
		 */
		box build_bounding_box() const NOTHROW;


		/**
		 * Apply a similarity @bref{t}ransformation to this bounding sphere.
		 *
		 * Usage notes:
		 * - Similarity transforms of non-empty() spheres are exact.
		 * - An empty() sphere still returns a non-empty() volume covering.
		 * - Transform matrices are supported, but are less robust and slower.
		 * - @attent Matrices which do not map to a similarity transformation
		 *		will cause faulty (non-bounding) results.
		 */
		//@{
		sphere transform_and_bound(const si_transform& t) const NOTHROW;
		sphere transform_and_bound(const matrix& t) const NOTHROW;
		//@}


		static const float intersect_epsilon;
		static const float intersection_volume_distance_epsilon;
		static const float intersection_volume_radius_epsilon;
		static const float enclose_epsilon;
		

		vector position;
		float radius;
};




/**
 *************************************************************************
 *
 * @class box
 * @ingroup geo_module
 *
 * A 3D, axis aligned bounding box, defined via
 * @bref{max}imum and @bref{min}imum corner coordinates.
 *
 * Notes:
 * - A box contains all points >= min and <= max, hence it is empty()
 *		if max < min. This differs from 2D rectangles (rect2f, rect2i),
 *		which contain points even for negative sizes.
 *
 ************************************************************************/
class box
{

	public:

		/** Create box with coordinates (0, 0, 0) and (0, 0, 0). */
		box() NOTHROW;

		box(const vector& min, const vector& max) NOTHROW;


		bool operator ==(const box& other) const NOTHROW;
		bool operator !=(const box& other) const NOTHROW;


		void swap_with(box& other) NOTHROW;


		/**
		 * Return half-way point between @bref{min} and @bref{max}.
		 * No special handling for empty() boxes.
		 */
		vector mid() const NOTHROW;


		vector size() const NOTHROW;
		float size_x() const NOTHROW;
		float size_y() const NOTHROW;
		float size_z() const NOTHROW;


		/** Determine volume of box, always zero or greater. */
		float volume() const NOTHROW;

		/** Test if the box contains no point (i.e. min > max). */
		bool empty() const NOTHROW;


		/**
		 * Test if two boxes share at least one point,
		 * including any edge or corner vertices.
		 * Returns @false for empty() boxes.
		 */
		bool intersecting(const box& other) const NOTHROW;


		/**
		 * Determine if this box intersects with an@bref{other} box.
		 * In case of any intersection, store the result in @bref{target}.
		 * If there is no intersection (e.g. either input box is empty()),
		 * return @false and do not change @bref{target}.
		 */
		bool try_intersect
			(const box& other, box &target) const NOTHROW;


		/** Determine volume of intersection with another box. */
		float intersection_volume(const box& other) const NOTHROW;


		/** Grow the box to enclose a given @bref{point} or @bref{b}ox. */
		//@{
		void grow_to(const vector &point) NOTHROW;
		void grow_to(const box &b) NOTHROW;
		//@}


		/**
		 * Test whether a given point is inside the box.
		 * Only enlarges (or shrinks, on negative values) the box
		 * by @bref{epsilon} if explicitly specified.
		 */
		//@{
		bool contains(float x, float y, float z) const NOTHROW;
		bool contains(const vector& point) const NOTHROW;
		bool contains(float x, float y, float z, float epsilon) const NOTHROW;
		bool contains(const vector& point, float epsilon) const NOTHROW;
		//@}


		/**
		 * Find a tight-fit bounding sphere.
		 * Empty boxes return an empty, but still tight-bounding, sphere.
		 */
		sphere build_bounding_sphere() const NOTHROW;


		/**
		 * Build an axis-aligned bounding box over the temporary oriented box
		 * that results from a similarity @bref{t}ransformation of this box.
		 *
		 * Usage notes:
		 * - An empty() box still returns a non-empty() volume covering.
		 * - The transformation matrix variant reduces calculation overhead.
		 * - @attent Matrices which do not map to a similarity transformation
		 *		will cause faulty (non-bounding) results.
		 */
		//@{
		box transform_and_bound(const si_transform& t) const NOTHROW;
		box transform_and_bound(const matrix& t) const NOTHROW;
		//@}
		

		vector min;
		vector max;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GEO_VOLUME_HPP) */

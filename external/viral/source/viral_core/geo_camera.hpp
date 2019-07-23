/**
 *************************************************************************
 *
 * @file geo_camera.hpp
 *
 * Camera-specific geometry.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GEO_CAMERA_HPP)
#define INCLUDED__VIRAL_CORE__GEO_CAMERA_HPP


#include "build_config.hpp"


#include "geo_vector.hpp"
#include "geo_transform.hpp"
#include "geo_primitive.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class frustum
 * @ingroup geo_module
 *
 * Viewing frustum made from eight vertices
 * and six associated, inward-pointing planes.
 *
 * General usage notes:
 * - Frustum @bref{vertices} must form a valid frustum pyramid.
 *		Malformed cases (e.g. degenerate or non-planar sides)
 *		exhibit unexpected but uncritical behaviour.
 * - All containment tests add a small epsilon to object sizes,
 *		which helps with object culling tests.
 *
 * Handedness considerations:
 * - Frustum @bref{vertex_type}s refer to a normalized cube [-1,1]^3
 *		(e.g. such as used by matrix::perspective() and @ref camera)
 *		while frustum @bref{vertices} are in world space coordinates.
 * - Handedness may be inconsistent between frustum @bref{vertex_type}s
 *		and @bref{vertices}, for example due to camera::post_scale.
 *		A frustum detects and handles inconsistent handedness to ensure
 *		inward-pointing planes in all (non-malformed) cases.
 *
 ************************************************************************/
class frustum
{

	public:

		enum vertex_type
		{
			near_xneg_yneg, near_xpos_yneg, near_xpos_ypos, near_xneg_ypos,
			far_xneg_yneg, far_xpos_yneg, far_xpos_ypos, far_xneg_ypos,
			vertex_count
		};

		enum plane_type
		{
			x_neg, x_pos, y_neg, y_pos, z_near, z_far,
			plane_count
		};


		/** Create initially empty, malformed frustum. */
		frustum() NOTHROW;


		/**
		 * Create a frustum from indicated points on near (far) plane,
		 * on positive (negative) x (y) axis, and build plane equations.
		 */
		frustum(const vector &n_xn_yn, const vector &n_xp_yn,
				const vector &n_xp_yp, const vector &n_xn_yp,
				const vector &f_xn_yn, const vector &f_xp_yn,
				const vector &f_xp_yp, const vector &f_xn_yp) NOTHROW;


		bool operator ==(const frustum& other) const NOTHROW;
		bool operator !=(const frustum& other) const NOTHROW;


		void swap_with(frustum& other) NOTHROW;


		/** Reset all frustum vertices and rebuild all planes. */
		void set
			(const vector &n_xn_yp, const vector &n_xp_yp,
			 const vector &n_xp_yn, const vector &n_xn_yn,
			 const vector &f_xn_yp, const vector &f_xp_yp,
			 const vector &f_xp_yn, const vector &f_xn_yn) NOTHROW;


		/** Rebuild all plane equations from vertices. */
		void rebuild_planes() NOTHROW;


		/**
		 * Find bounding box around (optionally transformed) frustum.
		 *
		 * @attent Matrices that are not a similarity transformation
		 * will cause faulty (non-bounding) results.
		 */
		//@{
		box build_bounding_box() const NOTHROW;
		box build_bounding_box(const matrix& transform) const NOTHROW;
		//@}


		/**
		 * Efficiently test whether the frustum definitely
		 * does or does not contain a @bref{s}phere.
		 *
		 * @attent A return of @false signifies that
		 * the sphere may both be inside or outside.
		 */
		//@{
		bool is_definitely_inside(const sphere& s) const NOTHROW;
		bool is_definitely_outside(const sphere& s) const NOTHROW;
		//@}


		/**
		 * Build a new frustum through linear interpolation
		 * of near and far planes over the frustum range,
		 * where a zero factor gives the current near plane
		 * and a one factor gives the current far plane.
		 */
		frustum near_far_interpolate
			(float near_factor, float far_factor) const NOTHROW;


		vector vertices[vertex_count];
		plane planes[plane_count];


		static const float cull_epsilon_constant;
		static const float cull_epsilon_linear;
};




/**
 *************************************************************************
 *
 * @class camera
 * @ingroup geo_module
 *
 * Holds the world coordinate position, the orientation and
 * the projection mode of a virtual camera. 
 *
 * Camera conventions:
 * - A default camera looks into the direction of the z axis, and uses y
 *		as the down axis and x as the right axis. This setup is consistent
 *		with image and texture coordinates (see image_raw and @ref mesh),
 *		and implies right-handed coordinates after perspective transforms.
 * - By convention, a normalized cube (i.e. [-1, 1]^3) maps to screen space
 *		so that x points to the right, y to the bottom, and z into the screen,
 *		with (-1, -1, -1) at the top-left viewport corner on the near plane,
 *		and (1, 1, 1) at the bottom-right viewport corner on the far plane.
 *		See matrix::perspective() and matrix::ortho() for details.
 * - Backend APIs may require additional transformations to realize
 *		above camera conventions. Such transforms should be invisible
 *		to client applications.
 * - While discouraged, clients can specify post-projection transforms
 *		(e.g. @bref{post_scale}) to realize non-conventional mappings,
 *		for example to shift the optical center or to toggle handedness.
 *		Note that the latter also changes rotational directions.
 *
 ************************************************************************/
class camera
{

	public:

		enum projection_mode
			{ orthographic, perspective };


		camera() NOTHROW
			:
			projection(perspective),
			field_of_view_x(45.f), aspect_y_to_x(0.75f),
			near_plane(1.f), far_plane(8192.f),
			post_scale(1.f, 1.f, 1.f)
		{ }


		camera(const vector &position, const rotation &orientation,
			   projection_mode projection,
			   float field_of_view_x, float aspect_y_to_x,
			   float near_plane, float far_plane) NOTHROW
			: 
			position(position), orientation(orientation),
			projection(projection),
			field_of_view_x(field_of_view_x), aspect_y_to_x(aspect_y_to_x),
			near_plane(near_plane), far_plane(far_plane),
			post_scale(1.f, 1.f, 1.f)
		{ }


		camera(const vector &position, const rotation &orientation,
			   projection_mode projection,
			   float field_of_view_x, float aspect_y_to_x,
			   float near_plane, float far_plane,
			   const vector& post_translate,
			   const vector& post_scale) NOTHROW
			: 
			position(position), orientation(orientation),
			projection(projection),
			field_of_view_x(field_of_view_x), aspect_y_to_x(aspect_y_to_x),
			near_plane(near_plane), far_plane(far_plane),
			post_translate(post_translate),
			post_scale(post_scale)
		{ }


		void swap_with(camera& other) NOTHROW;


		matrix build_world_to_camera() const NOTHROW;
		matrix build_camera_to_world() const NOTHROW;
		matrix build_projection() const NOTHROW;
		matrix build_inverse_projection() const NOTHROW;


		frustum build_frustum() const NOTHROW;


		vector position;
		rotation orientation;


		/**
		 * For in-depth details on supported projection modes,
		 * see matrix::perspective() and matrix::ortho(),
		 * as well as @ref camera documentation.
		 */
		projection_mode projection;


		/**
		 * Horizontal extents of the field of view as distance
		 * in between left and right boundings, either in degrees
		 * (perspective) or in units (orthographic).
		 */
		float field_of_view_x;


		/**
		 * Aspect ratio of vertical to horizontal field of view.
		 */
		float aspect_y_to_x;


		/**
		 * Distance from origin to near and far depth plane.
		 * In orthographic mode, only far_plane is considered.
		 */
		//@{
		float near_plane;
		float far_plane;
		//@}


		/**
		 * Utility transforms applied after matrix::perspective()
		 * or matrix::ortho() to reach a normalized cube [-1,1]^3.
		 */
		//@{
		vector post_translate;
		vector post_scale;
		//@}
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GEO_CAMERA_HPP) */
/**
 *************************************************************************
 *
 * @file geo_camera.cpp
 *
 * Camera-specific geometry, implementation.
 *
 ************************************************************************/


#include "geo_camera.hpp"


#include <math.h>


#include "util.hpp"
#include "geo_util.hpp"
#include "geo_volume.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// frustum
//
//////////////////////////////////////////////////////////////////////////


frustum::frustum() NOTHROW { }


frustum::frustum
	(const vector &n_xn_yn, const vector &n_xp_yn,
	 const vector &n_xp_yp, const vector &n_xn_yp,
	 const vector &f_xn_yn, const vector &f_xp_yn,
	 const vector &f_xp_yp, const vector &f_xn_yp) NOTHROW
{
	vertices[near_xneg_yneg] = n_xn_yn;
	vertices[near_xpos_yneg] = n_xp_yn;
	vertices[near_xpos_ypos] = n_xp_yp;
	vertices[near_xneg_ypos] = n_xn_yp;

	vertices[far_xneg_yneg] = f_xn_yn;
	vertices[far_xpos_yneg] = f_xp_yn;
	vertices[far_xpos_ypos] = f_xp_yp;
	vertices[far_xneg_ypos] = f_xn_yp;

	rebuild_planes();
}


bool frustum::operator ==(const frustum& other) const NOTHROW
{
	for (int64 i = 0; i < vertex_count; ++i)
		if (vertices[i] != other.vertices[i]) return false;
	for (int64 i = 0; i < plane_count; ++i)
		if (planes[i] != other.planes[i]) return false;
	
	return true;
}

bool frustum::operator !=(const frustum& other) const NOTHROW
	{ return !(operator ==(other)); }


void frustum::swap_with(frustum& other) NOTHROW
{
	for (int64 i = 0; i < vertex_count; ++i)
		swap(vertices[i], other.vertices[i]);

	for (int64 i = 0; i < plane_count; ++i)
		swap(planes[i], other.planes[i]);
}


void frustum::set
	(const vector &n_xn_yn, const vector &n_xp_yn,
	 const vector &n_xp_yp, const vector &n_xn_yp,
	 const vector &f_xn_yn, const vector &f_xp_yn,
	 const vector &f_xp_yp, const vector &f_xn_yp) NOTHROW
{
	vertices[near_xneg_yneg] = n_xn_yn;
	vertices[near_xpos_yneg] = n_xp_yn;
	vertices[near_xpos_ypos] = n_xp_yp;
	vertices[near_xneg_ypos] = n_xn_yp;

	vertices[far_xneg_yneg] = f_xn_yn;
	vertices[far_xpos_yneg] = f_xp_yn;
	vertices[far_xpos_ypos] = f_xp_yp;
	vertices[far_xneg_ypos] = f_xn_yp;

	rebuild_planes();
}


void frustum::rebuild_planes() NOTHROW
{
	/**
	 * Implementation notes:
	 * - We detect inconsistent handedness between vertex types
	 *		and vertices with the cross product of frustum edges.
	 * - If handedness is inconsistent, we traverse vertex_types
	 *		in clockwise instead of counter-clockwise winding
	 *		to get counter-clockwise vertices and inward normals.
	 */

	vector frustum_x(vertices[near_xpos_yneg] - vertices[near_xneg_yneg]);
	vector frustum_y(vertices[near_xneg_ypos] - vertices[near_xneg_yneg]);
	vector frustum_z(vertices[far_xneg_yneg] - vertices[near_xneg_yneg]);

	bool handedness_match =
		frustum_x.cross(frustum_y).dot(frustum_z) >= 0;

	if (handedness_match)
	{
		planes[x_neg].set
			(vertices[near_xneg_yneg], vertices[near_xneg_ypos], vertices[far_xneg_yneg]);
		planes[x_pos].set
			(vertices[near_xpos_yneg], vertices[far_xpos_yneg], vertices[near_xpos_ypos]);

		planes[y_neg].set
			(vertices[near_xneg_yneg], vertices[far_xneg_yneg], vertices[near_xpos_yneg]);
		planes[y_pos].set
			(vertices[near_xneg_ypos], vertices[near_xpos_ypos], vertices[far_xneg_ypos]);

		planes[z_near].set
			(vertices[near_xneg_yneg], vertices[near_xpos_yneg], vertices[near_xneg_ypos]);
		planes[z_far].set
			(vertices[far_xneg_yneg], vertices[far_xneg_ypos], vertices[far_xpos_yneg]);
	}
	else
	{
		planes[x_neg].set
			(vertices[near_xneg_yneg], vertices[far_xneg_yneg], vertices[near_xneg_ypos]);
		planes[x_pos].set
			(vertices[near_xpos_yneg], vertices[near_xpos_ypos], vertices[far_xpos_yneg]);

		planes[y_neg].set
			(vertices[near_xneg_yneg], vertices[near_xpos_yneg], vertices[far_xneg_yneg]);
		planes[y_pos].set
			(vertices[near_xneg_ypos], vertices[far_xneg_ypos], vertices[near_xpos_ypos]);

		planes[z_near].set
			(vertices[near_xneg_yneg], vertices[near_xneg_ypos], vertices[near_xpos_yneg]);
		planes[z_far].set
			(vertices[far_xneg_yneg], vertices[far_xpos_yneg], vertices[far_xneg_ypos]);
	}
}


box frustum::build_bounding_box() const NOTHROW
{
	box result(vertices[0], vertices[0]);

	for (int64 i = 1; i < vertex_count; ++i)
	{
		result.min = result.min.min(vertices[i]);
		result.max = result.max.max(vertices[i]);
	}

	return result;
}


box frustum::build_bounding_box(const matrix& transform) const NOTHROW
{
	vector transformed[frustum::vertex_count] =
		{
			transform * vertices[frustum::near_xneg_yneg],
			transform * vertices[frustum::near_xpos_yneg],
			transform * vertices[frustum::near_xpos_ypos],
			transform * vertices[frustum::near_xneg_ypos],

			transform * vertices[frustum::far_xneg_yneg],
			transform * vertices[frustum::far_xpos_yneg],
			transform * vertices[frustum::far_xpos_ypos],
			transform * vertices[frustum::far_xneg_ypos]
		};

	box result(transformed[0], transformed[0]);

	for (int64 i = 1; i < vertex_count; ++i)
	{
		result.min = result.min.min(transformed[i]);
		result.max = result.max.max(transformed[i]);
	}

	return result;
}


bool frustum::is_definitely_inside(const sphere& s) const NOTHROW
{
	float biased_radius =
		s.radius + s.radius * cull_epsilon_linear + cull_epsilon_constant;

	for (int64 i = 0; i < plane_count; ++i)
		if (planes[i].signed_distance_to(s.position) < biased_radius)
			return false;

	return true;
}


bool frustum::is_definitely_outside(const sphere& s) const NOTHROW
{
	float biased_radius =
		s.radius + s.radius * cull_epsilon_linear + cull_epsilon_constant;

	for (int64 i = 0; i < plane_count; ++i)
		if (planes[i].signed_distance_to(s.position) < -biased_radius)
			return true;

	return false;
}


frustum frustum::near_far_interpolate
	(float near_factor, float far_factor) const NOTHROW
{
	vector delta[4] =
		{ vertices[far_xneg_yneg] - vertices[near_xneg_yneg],
		  vertices[far_xpos_yneg] - vertices[near_xpos_yneg],
		  vertices[far_xpos_ypos] - vertices[near_xpos_ypos],
		  vertices[far_xneg_ypos] - vertices[near_xneg_ypos] };

	return frustum
			(vertices[near_xneg_yneg] + delta[0] * near_factor,
			 vertices[near_xpos_yneg] + delta[1] * near_factor,
			 vertices[near_xpos_ypos] + delta[2] * near_factor,
			 vertices[near_xneg_ypos] + delta[3] * near_factor,

			 vertices[near_xneg_yneg] + delta[0] * far_factor,
			 vertices[near_xpos_yneg] + delta[1] * far_factor,
			 vertices[near_xpos_ypos] + delta[2] * far_factor,
			 vertices[near_xneg_ypos] + delta[3] * far_factor);
}


/** Culling is efficient even for rather coarse epsilons. */
//@{
const float frustum::cull_epsilon_constant = 0.0001f;
const float frustum::cull_epsilon_linear = 0.00001f;
//@}




//////////////////////////////////////////////////////////////////////////
//
//	camera
//
//////////////////////////////////////////////////////////////////////////


void camera::swap_with(camera& other) NOTHROW
{
	swap(position, other.position);
	swap(orientation, other.orientation);
	swap_assign(projection, other.projection);
	swap(field_of_view_x, other.field_of_view_x);
	swap(aspect_y_to_x, other.aspect_y_to_x);
	swap(near_plane, other.near_plane);
	swap(far_plane, other.far_plane);
	swap(post_translate, other.post_translate);
	swap(post_scale, other.post_scale);
}


matrix camera::build_world_to_camera() const NOTHROW
{
	return
		matrix::rotate(-orientation) * matrix::translate(-position);
}

matrix camera::build_camera_to_world() const NOTHROW
{
	return
		matrix::translate(position) * matrix::rotate(orientation);
}

matrix camera::build_projection() const NOTHROW
{
	if (projection == orthographic)
	{
		return
			matrix::translate(post_translate) *
			matrix::scale(post_scale) *
			matrix::ortho
				(field_of_view_x, field_of_view_x * aspect_y_to_x, far_plane);
	}
	else if (projection == perspective)
	{
		float sizex =
			2 * near_plane * tanf(field_of_view_x * geo_constants::degrees_to_radians / 2);

		return 
			matrix::translate(post_translate) *
			matrix::scale(post_scale) *
			matrix::perspective
				(sizex, sizex * aspect_y_to_x, near_plane, far_plane);
	}

	return matrix();
}

matrix camera::build_inverse_projection() const NOTHROW
{
	if (projection == orthographic)
	{
		return
			matrix::inverse_ortho
				(field_of_view_x, field_of_view_x * aspect_y_to_x, far_plane) *
			matrix::scale
				(vector(1.f / post_scale.x, 1.f / post_scale.y, 1.f / post_scale.z)) *
			matrix::translate
				(-post_translate);
	}
	else if (projection == perspective)
	{
		float sizex =
			2 * near_plane * tanf(field_of_view_x * geo_constants::degrees_to_radians / 2);

		return
			matrix::inverse_perspective
				(sizex, sizex * aspect_y_to_x, near_plane, far_plane) *
			matrix::scale
				(vector(1.f / post_scale.x, 1.f / post_scale.y, 1.f / post_scale.z)) *
			matrix::translate
				(-post_translate);
	}

	return matrix();
}


frustum camera::build_frustum() const NOTHROW
{
	// Retrieve frustum vertices by inverse
	// projection of normalized cube vertices.

	matrix invmat =
		build_camera_to_world() * build_inverse_projection();

	return frustum(invmat.project(vector(-1, -1, -1)),
				   invmat.project(vector(1, -1, -1)),
				   invmat.project(vector(1, 1, -1)),
				   invmat.project(vector(-1, 1, -1)),

				   invmat.project(vector(-1, -1, 1)),
				   invmat.project(vector(1, -1, 1)),
				   invmat.project(vector(1, 1, 1)),
				   invmat.project(vector(-1, 1, 1)));
}




} /* namespace viral_core */

/**
 *************************************************************************
 *
 * @file mesh_tesselate.cpp
 *
 * Mesh tesselation utilities, implementation.
 *
 ************************************************************************/


#include "mesh_tesselate.hpp"


#include "array.hpp"
#include "geo_primitive.hpp"
#include "mesh.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// mesh_tesselate
//
//////////////////////////////////////////////////////////////////////////


dynamic_buffer<sphere>
	mesh_tesselate::tesselate_triangles_by_spheres
		(const mesh& msh, float sphere_radius)
{
	// Fetch existing streams.
	const mesh_stream_vector& vertices =
		msh.vertices();
	const mesh_stream_triangle& triangles =
		msh.triangles();


	// Preallocate result storage
	// with an estimate of results.
	dynamic_buffer<sphere> result;
	result.reserve(triangles.size());


	// Iteratively subdivide each triangle
	// up to given bounding sphere radius.
	dynamic_buffer<triangle> pending;
	pending.reserve(triangles.size());

	for (int64 i = 0; i < triangles.size(); ++i)
	{
		pending.clear();
		pending.push_back
			(triangle(vertices[triangles[i].i],
					  vertices[triangles[i].j],
					  vertices[triangles[i].k]));
		
		while (!pending.empty())
		{
			triangle tri(pending[pending.size() - 1]);
			pending.pop_back();

			sphere bounding_sphere
				(tri.build_bounding_sphere());


			// Discard too small, possibly degenerate triangles.
			if (bounding_sphere.radius <= triangle_degenerate_sphere_radius_)
				continue;


			// Accept bounding spheres below given radius.
			if (bounding_sphere.radius <= sphere_radius)	
			{
				result.push_back(bounding_sphere);
				continue;				
			}

			
			// Triangle larger than allowed radius,
			// so subdivide along major edge.
			split_major_edge_and_add_to_pending(tri, pending);
		}
	}


	// Success.
	return std::move(result);
}


dynamic_buffer<box>
	mesh_tesselate::tesselate_triangles_by_boxes
		(const mesh& msh, float box_size)
{
	// Fetch existing streams.
	const mesh_stream_vector& vertices =
		msh.vertices();
	const mesh_stream_triangle& triangles =
		msh.triangles();


	// Preallocate result storage
	// with an estimate of results.
	dynamic_buffer<box> result;
	result.reserve(triangles.size());


	// Iteratively subdivide each triangle
	// up to given bounding box size.
	dynamic_buffer<triangle> pending;
	pending.reserve(triangles.size());

	for (int64 i = 0; i < triangles.size(); ++i)
	{
		pending.clear();
		pending.push_back
			(triangle(vertices[triangles[i].i],
					  vertices[triangles[i].j],
					  vertices[triangles[i].k]));
		
		while (!pending.empty())
		{
			triangle tri(pending[pending.size() - 1]);
			pending.pop_back();

			box bounding_box
				(tri.build_bounding_box());
			float bounding_box_size_max =
				bounding_box.size().max();


			// Discard too small, possibly degenerate triangles.
			if (bounding_box_size_max <= triangle_degenerate_box_size_)
				continue;


			// Accept bounding boxes below given size.
			if (bounding_box_size_max <= box_size)	
			{
				result.push_back(bounding_box);
				continue;				
			}

			
			// Triangle larger than allowed size,
			// so subdivide along major edge.
			split_major_edge_and_add_to_pending(tri, pending);
		}
	}


	// Success.
	return std::move(result);
}


void mesh_tesselate::split_major_edge_and_add_to_pending
	(const triangle& tri, dynamic_buffer<triangle>& pending)
{
	/**
	 * Determine major edge, add mid point,
	 * build two counter-clockwise triangles,
	 * finally add these to @bref{pending}.
	 */

	float lengths_squared[3] =
		{ (tri.b - tri.a).square_sum(),
			(tri.c - tri.b).square_sum(),
			(tri.a - tri.c).square_sum() };

	if (lengths_squared[0] > lengths_squared[1] &&
		lengths_squared[0] > lengths_squared[2])
	{
		vector mid_ba((tri.b + tri.a) * 0.5f);
		pending.push_back
			(triangle(tri.c, tri.a, mid_ba));
		pending.push_back
			(triangle(tri.c, mid_ba, tri.b));
	}
	else if (lengths_squared[1] > lengths_squared[2] &&
			 lengths_squared[1] > lengths_squared[0])
	{
		vector mid_cb((tri.c + tri.b) * 0.5f);
		pending.push_back
			(triangle(tri.a, tri.b, mid_cb));
		pending.push_back
			(triangle(tri.a, mid_cb, tri.c));
	}
	else
	{
		vector mid_ac((tri.a + tri.c) * 0.5f);
		pending.push_back
			(triangle(tri.b, tri.c, mid_ac));
		pending.push_back
			(triangle(tri.b, mid_ac, tri.a));
	}		
}


const float mesh_tesselate::triangle_degenerate_sphere_radius_ = 1e-5f;
const float mesh_tesselate::triangle_degenerate_box_size_ = 1e-5f;




} /* namespace viral_core */

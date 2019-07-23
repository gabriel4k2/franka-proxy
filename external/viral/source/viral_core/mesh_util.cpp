/**
 *************************************************************************
 *
 * @file mesh_util.cpp
 *
 * Implementation for mesh utility operations.
 *
 ************************************************************************/


#include "mesh_util.hpp"


#include <math.h>


#include "log.hpp"
#include "color.hpp"
#include "geo_transform.hpp"
#include "mesh.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// mesh_util
//
//////////////////////////////////////////////////////////////////////////


void mesh_util::apply_transform
	(mesh& m, const si_transform& transform)
{
	matrix local_to_world
		(transform.build_local_to_world());
	matrix normals_to_world
		(transform.build_normals_to_world());

	mesh_stream_vector& vertices = m.vertices();
	int64 vertex_count = m.vertex_count();	
	for (int64 i = 0; i < vertex_count; ++i)
		vertices[i] = local_to_world * vertices[i];

	if (m.has_normals())
	{
		mesh_stream_vector& normals = m.normals();
		for (int64 i = 0; i < vertex_count; ++i)
			normals[i] = normals_to_world * normals[i];
	}

	if (m.has_tangents())
	{
		mesh_stream_vector& tangents = m.tangents();
		for (int64 i = 0; i < vertex_count; ++i)
			tangents[i] = normals_to_world * tangents[i];
	}

	if (m.has_bitangents())
	{
		mesh_stream_vector& bitangents = m.bitangents();
		for (int64 i = 0; i < vertex_count; ++i)
			bitangents[i] = normals_to_world * bitangents[i];
	}

	if (m.has_planes())
	{
		const mesh_stream_triangle& triangles = m.triangles();
		int64 triangle_count = m.triangle_count();

		// Rebuilding plane equations is easy to do
		// and more robust than applying transforms.
		mesh_stream_plane& planes = m.planes();
		for (int64 i = 0; i < triangle_count; ++i)
			planes[i] = plane(vertices[triangles[i].i],
							  vertices[triangles[i].j],
							  vertices[triangles[i].k]);
	}
}


void mesh_util::apply_transform
	(mesh& m, const matrix& mat)
		{ apply_transform(m, si_transform::from_matrix(mat)); }


void mesh_util::center_mesh
	(mesh& m, bool center_x, bool center_y, bool center_z)
{
	if (!center_x && !center_y && !center_z)
		return;

	mesh_stream_vector& vertices = m.vertices();
	int64 vertex_count = m.vertex_count();

	vector average;
	for (int64 i = 0; i < vertex_count; ++i)
		average += vertices[i];

	average /= static_cast<float>(vertex_count);

	vector offset(-average);
	if (!center_x) offset[0] = 0;
	if (!center_y) offset[1] = 0;
	if (!center_z) offset[2] = 0;

	apply_transform
		(m, si_transform(offset, rotation(), 1.f));
}


void mesh_util::build_vertex_normals(mesh& m)
{
	const mesh_stream_vector& vertices = m.vertices();
	const int64 vertex_count = m.vertex_count();

	const mesh_stream_triangle& triangles = m.triangles();
	const int64 triangle_count = m.triangle_count(); 

	mesh_stream_vector& normals = m.allocate_normals();
	memory::fill_with_zero_typed
		(normals.data(), normals.size());

	for (int64 t = 0; t < triangle_count; ++t)
	{
		vector va(vertices[triangles[t].i]);
		vector vb(vertices[triangles[t].j]);
		vector vc(vertices[triangles[t].k]);

		vector tri_normal((vb - va).cross(vc - va));

		// Use non-unit normal.
		// Its length corresponds to triangle area,
		// and weighs normal contribution accordingly.
		normals[triangles[t].i] += tri_normal;
		normals[triangles[t].j] += tri_normal;
		normals[triangles[t].k] += tri_normal;
	}

	for (int64 v = 0; v < vertex_count; ++v)
		normals[v].normalize();
}


void mesh_util::build_vertex_colors(mesh& m, const color& c)
{
	mesh_stream_color& colors = m.allocate_colors();
	const int64 vertex_count = m.vertex_count();

	for (int64 i = 0; i < vertex_count; ++i)
		colors[i] = c;
}


void mesh_util::flip_texcoord_origin_y(mesh &m)
{
	if (!m.has_texcoords()) return;

	mesh_stream_vector& texcoords = m.texcoords();
	int64 size = texcoords.size();

	for (int64 i = 0; i < size; ++i)
		texcoords[i].y = 1.f - texcoords[i].y;
}


void mesh_util::build_tangential_basis(mesh& m)
{
	/**
	 * Calculation of the tangential basis:
	 * - The tangential basis describes the model-space directions
	 *		that correspond to the u and v axes of texture space.
	 *		Normal maps define normals relative to this basis.
	 * - The transformation from texture to model space is linear:
	 *		m_model = A * t_texture, where m_model is in R^3,
	 *		t_texture is in R^2, and A is a 3x2 matrix.
	 * - From the edges of each triangle, we know two model-space
	 *		directions m_1, m_2, and associated texture coordinate
	 *		directions t_1, t_2.
	 * - This gives the linear system (m_1, m_2) = A * (t_1, t_2).
	 *		Solving for A leads to A = (m_1, m_2) * (t_1, t_2)^-1.
	 *		Hence, calculating A involves algebraic inversion
	 *		of the 2x2 texture coordinate matrix.
	 * - Then, A * (1, 0) and A * (0, 1) give the desired
	 *		tangent and bitangent of the respective triangle.
	 * - Finally, we smooth the tangential basis over multiple faces
	 *		by averaging all per-triangle tangential bases on each
	 *		contained vertex.
	 * - Algebraic inversion fails for degenerate texture coordinates.
	 *		This case results in arbitrary tangents and bitangents.
	 *		Usually, artists must manually fix such cases.
	 */


	// Fetch texture coordinates.
	// We cannot use m.texcoords(), since missing texcoords are no error.
	mesh::const_stream_iterator find_texcoords =
		m.find_vertex_stream(mesh_stream_registry::texcoords_stream_name);

	if (!find_texcoords) return;

	if (typeid(*find_texcoords->value) != typeid(mesh_stream_vector))
	{
		LOG_ERROR("Texture coordinate stream has invalid type.");
		throw invalid_state();
	}

	const mesh_stream_vector& texcoords =
		*(static_cast<const mesh_stream_vector*>(find_texcoords->value));


	// Build tangential basis.
	const mesh_stream_triangle& triangles = m.triangles();
	int64 tcount = m.triangle_count();

	const mesh_stream_vector& vertices = m.vertices();
	int64 vcount = m.vertex_count();

	mesh_stream_vector& tangents = m.allocate_tangents();
	mesh_stream_vector& bitangents = m.allocate_bitangents();

	for (int64 i = 0; i < vcount; ++i)
	{
		tangents[i] = vector();
		bitangents[i] = vector();
	}

	for (int64 i = 0; i < tcount; ++i)
	{
		// Difference vectors.
		vector m_1(vertices[triangles[i].j] - vertices[triangles[i].i]);
		vector m_2(vertices[triangles[i].k] - vertices[triangles[i].i]);

		vector t_1(texcoords[triangles[i].j] - texcoords[triangles[i].i]);
		vector t_2(texcoords[triangles[i].k] - texcoords[triangles[i].i]);


		// Texture cordinate matrix:
		// t_1.x    t_2.x 
		// t_1.y    t_2.y
		// can be inverted algebraically.
		float determinant =
			1 / (t_1.x * t_2.y - t_1.y * t_2.x);

		if (fabsf(determinant) < 0.0001)
			continue; // Degenerate mapping, system is singular.

		float t_inv[4] =
			{  t_2.y * determinant, -t_2.x * determinant,
			  -t_1.y * determinant,  t_1.x * determinant };


		// Build matrix A = (m_1, m_2) * t_inv.
		// Then calculate and apply tangents and bitangents
		// with trivial A * (1, 0) and A * (0, 1).
		float A[6] =
			{ m_1.x * t_inv[0] + m_2.x * t_inv[2],   m_1.x * t_inv[1] + m_2.x * t_inv[3],
			  m_1.y * t_inv[0] + m_2.y * t_inv[2],   m_1.y * t_inv[1] + m_2.y * t_inv[3],
			  m_1.z * t_inv[0] + m_2.z * t_inv[2],   m_1.z * t_inv[1] + m_2.z * t_inv[3] };

		vector tangent(A[0], A[2], A[4]);
		vector bitangent(A[1], A[3], A[5]);

		for (int64 j = 0; j < 3; ++j)
		{
			tangents[triangles[i].C[j]] += tangent;
			bitangents[triangles[i].C[j]] += bitangent;
		}
	}

	for (int64 i = 0; i < vcount; ++i)
	{
		tangents[i].normalize();
		bitangents[i].normalize();
	}
}


void mesh_util::build_plane_equations(mesh& m)
{
	mesh_stream_vector& vertices = m.vertices();
	mesh_stream_triangle& triangles = m.triangles();
	int64 tcount = m.triangle_count();

	auto_pointer<mesh_stream_plane> planes(new mesh_stream_plane());
	planes->resize(tcount);

	for (int64 i = 0; i < tcount; ++i)
		(*planes)[i] =
			plane(vertices[triangles[i].i],
				  vertices[triangles[i].j],
				  vertices[triangles[i].k]);

	m.insert_triangle_stream
		(mesh_stream_registry::plane_stream_name,
		 std::move(planes));
}




} /* namespace viral_core */

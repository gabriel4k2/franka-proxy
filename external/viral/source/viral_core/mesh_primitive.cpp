/**
 *************************************************************************
 *
 * @file mesh_primitive.cpp
 *
 * Meshes from primitive shapes, implementation.
 *
 ************************************************************************/


#include "mesh_primitive.hpp"


#include <math.h>


#include "log.hpp"
#include "exception.hpp"
#include "color.hpp"
#include "geo_2d.hpp"
#include "geo_util.hpp"
#include "geo_transform.hpp"
#include "mesh.hpp"
#include "mesh_util.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// mesh_primitive
//
//////////////////////////////////////////////////////////////////////////


mesh mesh_primitive::build_quad
	(const vector& origin, const rotation& orientation,
	 const vector2f& size, int64 build_flags)
{
	/** Quad maps to XY plane and faces into positive Z direction. */

	mesh msh;

	mesh_stream_vector& vertices = msh.allocate_vertices(4);
	vertices.at(0) = vector(0, 0, 0);
	vertices.at(1) = vector(size.x, 0, 0);
	vertices.at(2) = vector(size.x, size.y, 0);
	vertices.at(3) = vector(0, size.y, 0);

	if ((build_flags & build_triangles) != 0)
	{
		mesh_stream_triangle& triangles = msh.allocate_triangles(2);
		triangles.at(0) = mesh_triangle(0, 1, 3);
		triangles.at(1) = mesh_triangle(1, 2, 3);
	}

	if ((build_flags & build_lines) != 0)
	{
		mesh_stream_line& lines = msh.allocate_lines(4);
		lines.at(0) = mesh_line(0, 1);
		lines.at(1) = mesh_line(1, 2);
		lines.at(2) = mesh_line(2, 3);
		lines.at(3) = mesh_line(3, 0);
	}

	if ((build_flags & build_normals) != 0)
	{
		mesh_stream_vector& normals = msh.allocate_normals();
		normals.at(0) = normals.at(1) =
			normals.at(2) = normals.at(3) = vector(0, 0, 1);
	}

	if ((build_flags & build_texcoords) != 0)
	{
		mesh_stream_vector& texcoords = msh.allocate_texcoords();
		texcoords.at(0) = vector(1, 0, 0);
		texcoords.at(1) = vector(0, 0, 0);
		texcoords.at(2) = vector(0, 1, 0);
		texcoords.at(3) = vector(1, 1, 0);
	}

	build_generic_postprocess
		(msh, origin, orientation, build_flags);

	return std::move(msh);
}


mesh mesh_primitive::build_circle
	(const vector& origin, const rotation& orientation,
	 float radius, int64 segment_count, int64 build_flags)
{
	/** Circle maps to XY plane and faces into positive Z direction. */

	if (segment_count < 3)
	{
		LOG_ERROR("Cannot create circle with less than 3 segments.");
		throw invalid_parameter();
	}

	if (segment_count > max_segment_count)
	{
		LOG_ERROR("Segment count exceeds implementation maximum.");
		throw invalid_parameter();
	}


	mesh msh;


	// Build circle vertices.
	// The last vertex is the circle center.
	mesh_stream_vector& vertices =
		msh.allocate_vertices(segment_count + 1);

	const int64 center = segment_count;
	vertices.at(center) = vector(1.f, 1.f, 0) * radius;

	for (int64 i = 0; i < segment_count; ++i)
	{
		float angle =
			(static_cast<float>(i) / segment_count) * geo_constants::double_pi;

		vertices.at(i) =
			vector(cosf(angle), sinf(angle), 0) * radius +
			vertices.at(center);
	}


	// Build circle connectivity:
	// Triangles, lines.
	if ((build_flags & build_triangles) != 0)
	{
		mesh_stream_triangle& triangles = msh.allocate_triangles(segment_count);
		for (int64 i = 0; i < segment_count; ++i)
			triangles.at(i) = mesh_triangle(center, i, (i + 1) % segment_count);
	}

	if ((build_flags & build_lines) != 0)
	{
		mesh_stream_line& lines = msh.allocate_lines(segment_count);
		for (int64 i = 0; i < segment_count; ++i)
			lines.at(i) = mesh_line((i + 1) % segment_count, i);
	}


	// Build circle vertex streams:
	// Normals, texcoords.
	if ((build_flags & build_normals) != 0)
	{
		mesh_stream_vector& normals = msh.allocate_normals();
		for (int64 i = 0; i < segment_count + 1; ++i)
			normals.at(i) = vector(0, 0, 1);
	}

	if ((build_flags & build_texcoords) != 0)
	{
		mesh_stream_vector& texcoords = msh.allocate_texcoords();
		for (int64 i = 0; i < segment_count; ++i)
		{
			texcoords.at(i) = vertices.at(i) / (2 * radius);
			texcoords.at(i).x = 1 - texcoords.at(i).x;
		}
		texcoords.at(segment_count) = vector(0.5f, 0.5f, 0);
	}


	// Finish and return circle.
	build_generic_postprocess
		(msh, origin, orientation, build_flags);

	return std::move(msh);
}


mesh mesh_primitive::build_box
	(const vector& origin, const rotation& orientation,
	 const vector& size, int64 build_flags)
{
	mesh msh;


	// Build box vertices.
	// Idea: Build values for XY face only,
	// then swap coordinates for correct plane,
	// and finally flip for opposite sides.
	mesh_stream_vector& vertices =
		msh.allocate_vertices(24);

	for (int64 s = 0; s < 6; ++s)
	for (int64 v = 0; v < 4; ++v)
	{
		float flip = (s >= 3) ? 1.f : 0.f;
		float values[3] = { (v == 1 || v == 2) ? (1.f - flip) : flip,
							(v == 2 || v == 3) ? 1.f : 0.f, flip };

		vertices.at(s * 4 + v) =
			vector(values[(2 * s) % 3],
				   values[(2 * s + 1) % 3],
				   values[(2 * s + 2) % 3]) * size;
	}


	// Build box connectivity:
	// Triangles, lines.
	if ((build_flags & build_triangles) != 0)
	{
		mesh_stream_triangle& triangles = msh.allocate_triangles(12);
		for (int64 i = 0; i < 6; ++i)
		{
			triangles.at(i * 2) =
				mesh_triangle(i * 4, i * 4 + 3, i * 4 + 1);
			triangles.at(i * 2 + 1) =
				mesh_triangle(i * 4 + 1, i * 4 + 3, i * 4 + 2);
		}
	}

	if ((build_flags & build_lines) != 0)
	{
		mesh_stream_line& lines = msh.allocate_lines(24);
		for (int64 i = 0; i < 6; ++i)
		{
			lines.at(i * 4) = mesh_line(i * 4, i * 4 + 1);
			lines.at(i * 4 + 1) = mesh_line(i * 4 + 1, i * 4 + 2);
			lines.at(i * 4 + 2) = mesh_line(i * 4 + 2, i * 4 + 3);
			lines.at(i * 4 + 3) = mesh_line(i * 4 + 3, i * 4);
		}
	}


	// Build box vertex streams:
	// Normals, texcoords.
	if ((build_flags & build_normals) != 0)
	{
		mesh_stream_vector& normals = msh.allocate_normals();
		for (int64 s = 0; s < 6; ++s)
		for (int64 v = 0; v < 4; ++v)
			normals.at(s * 4 + v) =
				vector(s == 1 ? -1.f : (s == 4 ? 1.f : 0.f),
					   s == 2 ? -1.f : (s == 5 ? 1.f : 0.f),
					   s == 0 ? -1.f : (s == 3 ? 1.f : 0.f));
	}	

	if ((build_flags & build_texcoords) != 0)
	{
		/**
		 * Box texture coordinates do not use a specific up direction.
		 * Instead, faces at the origin respectively map UV to XY, YZ, ZX.
		 * On opposing faces, U flips around to avoid mirroring.
		 */

		mesh_stream_vector& texcoords = msh.allocate_texcoords();
		for (int64 i = 0; i < 6; ++i)
		{
			texcoords.at(i * 4) = vector(0, 0, 0);
			texcoords.at(i * 4 + 1) = vector(1, 0, 0);
			texcoords.at(i * 4 + 2) = vector(1, 1, 0);
			texcoords.at(i * 4 + 3) = vector(0, 1, 0);
		}
	}


	// Finish and return box.
	build_generic_postprocess
		(msh, origin, orientation, build_flags);

	return std::move(msh);
}


mesh mesh_primitive::build_cylinder
	(const vector& origin, const rotation& orientation,
	 float height, float radius, int64 segment_count,
	 int64 build_flags)
{
	if (segment_count < 3)
	{
		LOG_ERROR("Cannot create cylinder with less than 3 segments.");
		throw invalid_parameter();
	}

	if (segment_count > max_segment_count)
	{
		LOG_ERROR("Segment count exceeds implementation maximum.");
		throw invalid_parameter();
	}


	mesh msh;


	// Build cylinder vertices.
	// We must split up the lateral surface and
	// detach the cylinder caps because we need
	// vertex duplicates for normals and texcoords.
	// The last two vertices are the circle centers.
	const int64 point_count = segment_count + 1;
	mesh_stream_vector& vertices =
		msh.allocate_vertices(4 * point_count + 2);

	const int64 center_low = 4 * point_count;
	vertices.at(center_low) = vector(1.f, 1.f, 0) * radius;

	const int64 center_high = 4 * point_count + 1;
	vertices.at(center_high) = vertices.at(center_low) + vector(0, 0, height);

	for (int64 i = 0; i < point_count; ++i)
	{
		float angle =
			(static_cast<float>(i) / segment_count) * geo_constants::double_pi;

		vertices.at(i) = vertices.at(i + 2 * point_count) =
			vector(cosf(angle), sinf(angle), 0) * radius + vertices.at(center_low);
		vertices.at(i + point_count) = vertices.at(i + 3 * point_count) =
			vector(cosf(angle), sinf(angle), 0) * radius + vertices.at(center_high);
	}


	// Build cylinder connectivity:
	// Triangles, lines.
	if ((build_flags & build_triangles) != 0)
	{
		mesh_stream_triangle& triangles = msh.allocate_triangles(4 * segment_count);
		for (int64 i = 0; i < segment_count; ++i)
		{
			const int64 low_first = i;
			const int64 low_next = i + 1;
			const int64 high_first = low_first + point_count;
			const int64 high_next = low_next + point_count;

			triangles.at(i) = mesh_triangle(center_low, low_next, low_first);
			triangles.at(i + segment_count) = mesh_triangle(center_high, high_first, high_next);

			const int64 lateral_a = low_first + 2 * point_count;
			const int64 lateral_b = low_next + 2 * point_count;
			const int64 lateral_c = low_next + 3 * point_count;
			const int64 lateral_d = low_first + 3 * point_count;

			triangles.at(i + 2 * segment_count) = mesh_triangle(lateral_a, lateral_b, lateral_c);
			triangles.at(i + 3 * segment_count) = mesh_triangle(lateral_a, lateral_c, lateral_d);
		}
	}

	if ((build_flags & build_lines) != 0)
	{
		mesh_stream_line& lines = msh.allocate_lines(3 * segment_count);
		for (int64 i = 0; i < segment_count; ++i)
		{
			const int64 low_first = i;
			const int64 low_next = i + 1;
			const int64 high_first = low_first + point_count;
			const int64 high_next = low_next + point_count;

			lines.at(i) = mesh_line(low_first, low_next);
			lines.at(i + segment_count) = mesh_line(high_first, high_next);
			lines.at(i + 2 * segment_count) = mesh_line(low_first, high_first);
		}
	}


	// Build cylinder vertex streams:
	// Normals, texcoords.
	if ((build_flags & build_normals) != 0)
	{
		mesh_stream_vector& normals = msh.allocate_normals();
		for (int64 i = 0; i < point_count; ++i)
		{
			normals.at(i) = vector(0, 0, -1);
			normals.at(i + point_count) = vector(0, 0, 1);

			normals.at(i + 2 * point_count) =
				normals.at(i + 3 * point_count) =
					vector(vertices.at(i + 2 * point_count).x,
						   vertices.at(i + 2 * point_count).y, 0).normalized();
		}	

		normals.at(center_low) = vector(0, 0, -1);
		normals.at(center_high) = vector(0, 0, 1);
	}

	if ((build_flags & build_texcoords) != 0)
	{
		mesh_stream_vector& texcoords = msh.allocate_texcoords();
		for (int64 i = 0; i < point_count; ++i)
		{
			texcoords.at(i) = vertices.at(i) / (2 * radius);
			texcoords.at(i + point_count) = texcoords.at(i);
			texcoords.at(i + point_count).x = 1 - texcoords.at(i).x;

			float angle_normalized = (static_cast<float>(i) / segment_count);
			texcoords.at(i + 2 * point_count) = vector(1.f - angle_normalized, 0, 0);
			texcoords.at(i + 3 * point_count) = vector(1.f - angle_normalized, 1, 0);
		}

		texcoords.at(4 * point_count) = vector(0.5f, 0.5f, 0);
		texcoords.at(4 * point_count + 1) = vector(0.5f, 0.5f, 0);
	}


	// Finish and return cylinder.
	build_generic_postprocess
		(msh, origin, orientation, build_flags);

	return std::move(msh);
}


mesh mesh_primitive::build_cone
	(const vector& origin, const rotation& orientation,
	 float height, float radius, int64 segment_count,
	 int64 build_flags)
{
	if (segment_count < 3)
	{
		LOG_ERROR("Cannot create cone with less than 3 segments.");
		throw invalid_parameter();
	}

	if (segment_count > max_segment_count)
	{
		LOG_ERROR("Segment count exceeds implementation maximum.");
		throw invalid_parameter();
	}


	mesh msh;


	// Build cone vertices.
	// We must separate the cone top from the cone bottom
	// as we require vertex duplicates for normals and texcoords.
	// The last vertices are the circle center and the cone top.
	mesh_stream_vector& vertices =
		msh.allocate_vertices(2 * segment_count + 2);

	const int64 center = 2 * segment_count;
	const int64 top = 2 * segment_count + 1;
	vertices.at(center) = vector(1.f, 1.f, 0) * radius;
	vertices.at(top) = vertices.at(center) + vector(0, 0, height);

	for (int64 i = 0; i < segment_count; ++i)
	{
		float angle =
			(static_cast<float>(i) / segment_count) * geo_constants::double_pi;

		vertices.at(i) = vertices.at(i + segment_count) =
			vector(cosf(angle), sinf(angle), 0) * radius +
			vertices.at(center);
	}


	// Build cone connectivity:
	// Triangles, lines.
	if ((build_flags & build_triangles) != 0)
	{
		mesh_stream_triangle& triangles = msh.allocate_triangles(2 * segment_count);
		for (int64 i = 0; i < segment_count; ++i)
		{
			triangles.at(i) =
				mesh_triangle(center, (i + 1) % segment_count, i);

			triangles.at(i + segment_count) =
				mesh_triangle(top, i + segment_count,
								   (i + 1) % segment_count + segment_count);
		}
	}

	if ((build_flags & build_lines) != 0)
	{
		mesh_stream_line& lines = msh.allocate_lines(2 * segment_count);
		for (int64 i = 0; i < segment_count; ++i)
		{
			lines.at(i) = mesh_line((i + 1) % segment_count, i);
			lines.at(i + segment_count) = mesh_line(i + segment_count, top);
		}
	}


	// Build cone vertex streams:
	// Normals, texcoords.
	if ((build_flags & build_normals) != 0)
	{
		// Explicit normal building is too laborious,
		// and face-to-vertex normal estimation works here.
		mesh_util::build_vertex_normals(msh);
	}

	if ((build_flags & build_texcoords) != 0)
	{
		mesh_stream_vector& texcoords = msh.allocate_texcoords();
		for (int64 i = 0; i < segment_count; ++i)
		{
			texcoords.at(i) = vertices.at(i) / (2 * radius);
			texcoords.at(i + segment_count) = texcoords.at(i);
			texcoords.at(i + segment_count).x = 1 - texcoords.at(i).x;
		}

		texcoords.at(center) = vector(0.5f, 0.5f, 0);
		texcoords.at(top) = vector(0.5f, 0.5f, 0);
	}


	// Finish and return cone.
	build_generic_postprocess
		(msh, origin, orientation, build_flags);

	return std::move(msh);
}


mesh mesh_primitive::build_sphere
	(const vector& origin, const rotation& orientation,
	 float radius, int64 segment_count, int64 build_flags)
{
	return build_ellipsoid
		(origin, orientation,
		 vector(radius, radius, radius),
		 segment_count, build_flags);
}


mesh mesh_primitive::build_ellipsoid
	(const vector& origin, const rotation& orientation,
	 const vector& radius, int64 segment_count, int64 build_flags)
{
	if (segment_count < 3)
	{
		LOG_ERROR("Cannot create ellipsoid with less than 3 segments.");
		throw invalid_parameter();
	}

	if (segment_count > max_segment_count)
	{
		LOG_ERROR("Segment count exceeds implementation maximum.");
		throw invalid_parameter();
	}


	mesh msh;


	// Build ellipsoid vertices.
	// We must duplicate slice endpoints for proper texturing.
	// The last vertices are the sphere bottom and top points.
	const int64 ring_count = segment_count;
	const int64 slice_count = segment_count;
	const int64 point_count = slice_count + 1;

	mesh_stream_vector& vertices =
		msh.allocate_vertices(ring_count * point_count + 2);

	const int64 bottom = ring_count * point_count;
	const int64 top = ring_count * point_count + 1;

	for (int64 r = 0; r < ring_count; ++r)
	{
		float ring_angle =
			(static_cast<float>(r + 1) / (ring_count + 1)) * geo_constants::pi;

		float ring_radius = sinf(ring_angle);
		float ring_offset = -cosf(ring_angle);

		for (int64 s = 0; s < point_count; ++s)
		{
			float slice_angle =
				(static_cast<float>(s) / slice_count) * geo_constants::double_pi;

			vertices.at(r * point_count + s) =
				vector(cosf(slice_angle) * ring_radius + 1.f,
					   sinf(slice_angle) * ring_radius + 1.f,
					   ring_offset + 1.f) * radius;
		}
	}

	vertices.at(bottom) = vector(radius.x, radius.y, 0);
	vertices.at(top) = vector(radius.x, radius.y, 2 * radius.z);


	// Build ellipsoid connectivity:
	// Triangles, lines.
	if ((build_flags & build_triangles) != 0)
	{
		mesh_stream_triangle& triangles =
			msh.allocate_triangles((ring_count - 1) * slice_count * 2 +
								  2 * slice_count);

		int64 target_index = 0;

		for (int64 r = 0; r < ring_count - 1; ++r)
		for (int64 s = 0; s < slice_count; ++s)
		{
			int64 va = r * point_count + s;
			int64 vb = r * point_count + s + 1;
			int64 vc = (r + 1) * point_count + s + 1;
			int64 vd = (r + 1) * point_count + s;

			triangles.at(target_index) = mesh_triangle(va, vb, vc);
			triangles.at(target_index + 1) = mesh_triangle(va, vc, vd);
			target_index += 2;
		}

		for (int64 s = 0; s < slice_count; ++s)
		{
			int64 snext = s + 1;

			triangles.at(target_index) =
				mesh_triangle(s, bottom, snext);
			triangles.at(target_index + slice_count) =
				mesh_triangle((ring_count - 1) * point_count + s,
							  (ring_count - 1) * point_count + snext, top);

			++target_index;
		}
	}

	if ((build_flags & build_lines) != 0)
	{
		mesh_stream_line& lines =
			msh.allocate_lines(ring_count * slice_count +
							  (ring_count - 1) * slice_count +
							  2 * slice_count);

		int64 target_index = 0;

		for (int64 r = 0; r < ring_count; ++r)
		for (int64 s = 0; s < slice_count; ++s)
		{
			lines.at(target_index) =
				mesh_line(r * point_count + s,
						  r * point_count + s + 1);
			++target_index;
		}
		
		for (int64 s = 0; s < slice_count; ++s)
		for (int64 r = 0; r < ring_count - 1; ++r)
		{
			lines.at(target_index) =
				mesh_line(r * point_count + s,
						  (r + 1) * point_count + s);
			++target_index;
		}

		for (int64 s = 0; s < slice_count; ++s)
		{
			lines.at(target_index) = mesh_line(bottom, s);
			lines.at(target_index + slice_count) =
				mesh_line(top, (ring_count - 1) * point_count + s);

			++target_index;
		}
	}


	// Build ellipsoid vertex streams:
	// Normals, texcoords.
	if ((build_flags & build_normals) != 0)
	{
		mesh_stream_vector& normals =
			msh.allocate_normals();

		vector inv_radius_sqr
			(vector(1, 1, 1) /  (radius * radius));

		for (int64 r = 0; r < ring_count; ++r)
		for (int64 s = 0; s < point_count; ++s)
			normals.at(r * point_count + s) =
				((vertices.at(r * point_count + s) - radius) *
					inv_radius_sqr).normalized();

		normals.at(bottom) = vector(0, 0, -1);
		normals.at(top) = vector(0, 0, 1);
	}

	if ((build_flags & build_texcoords) != 0)
	{
		mesh_stream_vector& texcoords =
			msh.allocate_texcoords();

		for (int64 r = 0; r < ring_count; ++r)
		{
			float ring_angle_ratio =
				(static_cast<float>(r + 1) / (ring_count + 1));

			for (int64 s = 0; s < point_count; ++s)
			{
				float slice_angle_ratio =
					(static_cast<float>(s) / slice_count);

				texcoords.at(r * point_count + s) =
					vector(1.f - slice_angle_ratio, ring_angle_ratio, 0);
			}
		}

		texcoords.at(bottom) = vector(0.5f, 0, 0);
		texcoords.at(top) = vector(0.5f, 1, 0);
	}


	// Finish and return ellipsoid.
	build_generic_postprocess
		(msh, origin, orientation, build_flags);

	return std::move(msh);
}


void mesh_primitive::build_generic_postprocess
	(mesh& msh, const vector& origin, const rotation& orientation,
	 int64 build_flags)
{
	/**
	 * Apply primitive-independent post-processing
	 * to incoming @bref{m}esh.
	 */

	// Validate here for fault-free mesh_util run.
	msh.validate();

	mesh_util::center_mesh
		(msh, (build_flags & center_x) != 0,
		      (build_flags & center_y) != 0,
		      (build_flags & center_z) != 0);

	mesh_util::apply_transform
		(msh, si_transform(origin, orientation, 1.f));

	// Primitives use unique normals for correct sharp edges.

	if ((build_flags & build_colors) != 0)
		mesh_util::build_vertex_colors(msh, color(1, 1, 1, 1));

	// Primitives use unique texture coordinates.

	if ((build_flags & build_tangents) != 0)
	{
		if ((build_flags & build_normals) == 0 ||
			(build_flags & build_texcoords) == 0)
		{
			LOG_INFO("Cannot build tangents without normals or texcoords.");
			throw invalid_parameter();
		}

		// Primitive transformation above has updated normals.
		// Tangent basis will be relative to the new space.
		mesh_util::build_tangential_basis(msh);
	}

	msh.rebuild_boundings();
}




} /* namespace viral_core */

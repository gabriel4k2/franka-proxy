/**
 *************************************************************************
 *
 * @file gui_render.cpp
 *
 * GUI rendering utilities, implementation.
 *
 ************************************************************************/


#include "gui_render.hpp"


#include <viral_core/log.hpp>
#include <viral_core/mesh.hpp>
#include <viral_core/model.hpp>
#include <viral_core/render_resource.hpp>
#include <viral_core/render_scene.hpp>
#include <viral_core/render_command.hpp>


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_render_layer
//
//////////////////////////////////////////////////////////////////////////


auto_pointer<mesh> gui_render_layer::release_target_mesh() NOTHROW
	{ return std::move(target_mesh_); }


gui_render_layer::gui_render_layer
	(const mesh& initial_mesh,
	 const shared_instance<render_material_id>& material)
	:
	material(material),
	target_mesh_(new mesh())
{
	exact_bounding_rects_.reserve(initial_rects_reserve_);

	// Our target mesh only accumulates texcoords.
	// We cannot simply clone the incoming mesh,
	// as it might have surplus vertex streams
	// which would need resizing on later append_mesh().
	target_mesh_->allocate_texcoords();
		 
	append_mesh(initial_mesh);
}


gui_render_layer::classify_mesh_result
	gui_render_layer::classify_mesh
		(const mesh& source_mesh,
		 const shared_instance<render_material_id>& source_material) const
{
	/**
	 * Determine if we can merge a @bref{source_mesh} with this layer,
	 * or if we can at least push the mesh past our layer without
	 * disrupting correct depth order.
	 */

	if (source_material == material) return on_layer;

	float epsilon = gui_render_util::render_epsilon;

	rect2f epsilon_rect
		(source_mesh.bounding_box().min.x + epsilon,
		 source_mesh.bounding_box().min.y + epsilon,
		 source_mesh.bounding_box().size_x() - 2 * epsilon,
		 source_mesh.bounding_box().size_y() - 2 * epsilon);

	if (!bounding_rect_.intersecting(epsilon_rect))
		return no_conflict;

	for (int64 i = 0; i < exact_bounding_rects_.size(); ++i)
	{
		if (exact_bounding_rects_[i].intersecting(epsilon_rect))
			return conflict;
	}

	return no_conflict;
}


void gui_render_layer::append_mesh(const mesh& source_mesh)
{
	/**
	 * Merge a @bref{source_mesh} with this layer,
	 * so that it renders atop all existing triangles.
	 * The mesh is already known to use our @bref{material_}.
	 */


	// Append per-vertex data from source mesh streams
	// to target mesh streams. We only copy texcoords
	// and vertices, as the GUI is usually unlit.
	const int64 old_vertex_count =
		target_mesh_->vertex_count();
	const int64 new_vertex_count =
		target_mesh_->vertex_count() + source_mesh.vertex_count();

	mesh_stream_vector& target_texcoords
		(target_mesh_->vertex_stream<mesh_stream_vector>
			(mesh_stream_registry::texcoords_stream_name));
	const mesh_stream_vector& source_texcoords
		(source_mesh.vertex_stream<mesh_stream_vector>
			(mesh_stream_registry::texcoords_stream_name));

	target_mesh_->allocate_vertices(new_vertex_count);
	SCOPE_GUARD
		(guard_vertices,
			(*target_mesh_, &mesh::allocate_vertices, old_vertex_count));

	target_texcoords.resize(new_vertex_count);
	SCOPE_GUARD
		(guard_texcoords,
			(target_texcoords, &mesh_stream_vector::resize, old_vertex_count));

	memory::copy_block_typed
		(target_mesh_->vertices().data() + old_vertex_count,
		 source_mesh.vertices().data(), source_mesh.vertex_count());
	memory::copy_block_typed
		(target_texcoords.data() + old_vertex_count,
		 source_texcoords.data(), source_mesh.vertex_count());


	// Copy triangles from source mesh to target mesh.
	// We need to adjust triangle indices to match above vertex offsets.
	const int64 old_triangle_count =
		target_mesh_->triangle_count();
	const int64 new_triangle_count =
		target_mesh_->triangle_count() + source_mesh.triangle_count();

	target_mesh_->allocate_triangles(new_triangle_count);
	SCOPE_GUARD
		(guard_triangles,
			(*target_mesh_, &mesh::allocate_triangles, old_triangle_count));

	for (int64 i = old_triangle_count; i < new_triangle_count; ++i)
	{
		target_mesh_->triangles()[i] =
			mesh_triangle
				(source_mesh.triangles()[i - old_triangle_count].i + old_vertex_count,
				 source_mesh.triangles()[i - old_triangle_count].j + old_vertex_count,
				 source_mesh.triangles()[i - old_triangle_count].k + old_vertex_count);
	}


	// Update bounding rects so that we may block
	// future meshes on classify_mesh().
	rect2f new_bounding_rect
		(source_mesh.bounding_box().min.x,
		 source_mesh.bounding_box().min.y,
		 source_mesh.bounding_box().size_x(),
		 source_mesh.bounding_box().size_y());

	exact_bounding_rects_.push_back(new_bounding_rect);
	SCOPE_GUARD
		(guard_bounding_rects,
			(exact_bounding_rects_, &dynamic_buffer<rect2f>::pop_back));

	bounding_rect_.grow_to(new_bounding_rect);


	// Success.
	guard_vertices.dismiss();
	guard_texcoords.dismiss();
	guard_triangles.dismiss();
	guard_bounding_rects.dismiss();
}


const int64 gui_render_layer::initial_rects_reserve_ = 128;




//////////////////////////////////////////////////////////////////////////
//
// gui_render_packer
//
//////////////////////////////////////////////////////////////////////////


gui_render_packer::gui_render_packer() { }


void gui_render_packer::pack_mesh
	(const mesh& source_mesh,
	 const shared_instance<render_material_id>& material)
{
	// Ignore empty meshes (e.g. due to clipping),
	// as these anyways do not contribute to rendering.
	if (source_mesh.vertex_count() == 0 ||
		source_mesh.bounding_box().size().max() < gui_render_util::render_epsilon)
			return;


	// See if we can push the mesh onto any existing layer.
	// This only works if the mesh does not intersect
	// the bounding box of a higher-level incompatible layer.
	for (render_layer_list::iterator it
			(render_layers_.last()); it; --it)
	{
		gui_render_layer::classify_mesh_result
			classify = (*it)->classify_mesh(source_mesh, material);

		if (classify == gui_render_layer::on_layer)
		{
			(*it)->append_mesh(source_mesh);
			return;
		}
		else if (classify == gui_render_layer::conflict)
			break;
	}


	// We had a depth order conflict, or we do not
	// yet have a suitable layer for given material.
	// Thus, create a topmost new layer for our mesh.
	auto_pointer<gui_render_layer> new_layer
		(new gui_render_layer(source_mesh, material));
	render_layers_.push_back
		(std::move(new_layer));
}


gui_render_packer::const_render_layer_iterator
	gui_render_packer::first_render_layer() const NOTHROW
		{ return render_layers_.first(); }




//////////////////////////////////////////////////////////////////////////
//
// gui_render_util
//
//////////////////////////////////////////////////////////////////////////


mesh gui_render_util::create_mesh_from_rect
	(const rect2f& extents, const rect2f& texcoords,
	 const rect2f& clip_rect, bool enable_clip,
	 bool pixel_integral)
{
	return create_mesh_from_rects
				(&extents, &texcoords, 1,
				 clip_rect, enable_clip, pixel_integral);
}


mesh gui_render_util::create_mesh_from_rects
	(const rect2f* extents, const rect2f* texcoords, int64 count,
	 const rect2f& clip_rect, bool enable_clip,
	 bool pixel_integral)
{
	if (count < 0)
	{
		LOG_ERROR("Invalid, negative rectangle count.");
		throw invalid_parameter();
	}

	if (count > 0 && (!extents || !texcoords))
	{
		LOG_ERROR("Cannot create mesh from NULL rectangles.");
		throw null_pointer();
	}


	// Preallocate a next-best guess of storage
	// required for rectangles without clipping.
	mesh msh;

	mesh_stream_vector& vertices = msh.allocate_vertices(count * 4);
	mesh_stream_triangle& triangles = msh.allocate_triangles(count * 2);
	mesh_stream_vector& texcoords_stream = msh.allocate_texcoords();


	// Iterate over all rectangles,
	// clip, and insert into streams.
	int64 dest_strm = 0;
	int64 dest_tri = 0;
	int64 numinserted = 0;

	for (int64 i = 0; i < count; ++i)
	{

		// Don't insert too small rectangles.
		if (extents[i].size.min() < render_epsilon)
			continue;


		// If clipping is enabled, clip extents and build
		// resulting tex coords. Abort if clipping yields
		// an invalid sliver rectangle.
		rect2f extents_clipped;
		rect2f texcoords_clipped;

		if (enable_clip)
		{
			if (!extents[i].try_intersect(clip_rect, extents_clipped) ||
				 extents_clipped.size.min() < render_epsilon)
					continue;

			texcoords_clipped =
				((extents[i] == extents_clipped) ? texcoords[i] :
					extents_clipped.apply_linear_map(extents[i], texcoords[i]));
		}
		else
		{
			extents_clipped = extents[i];
			texcoords_clipped = texcoords[i];
		}


		// Add vertices to mesh, then round coordinates
		// if the client wants pixel integral rendering.
		vertices[dest_strm] =
			vector(extents_clipped.pos.x, extents_clipped.pos.y, 0);
		vertices[dest_strm + 1] =
			vector(extents_clipped.end_x(), extents_clipped.pos.y, 0);
		vertices[dest_strm + 2] =
			vector(extents_clipped.end_x(), extents_clipped.end_y(), 0);
		vertices[dest_strm + 3] =
			vector(extents_clipped.pos.x, extents_clipped.end_y(), 0);

		if (pixel_integral)
		{
			vertices[dest_strm] = vertices[dest_strm].floor();
			vertices[dest_strm + 1] = vertices[dest_strm + 1].floor();
			vertices[dest_strm + 2] = vertices[dest_strm + 2].floor();
			vertices[dest_strm + 3] = vertices[dest_strm + 3].floor();
		}


		// Provide texcoords and triangles.
		// Note texture coordinates must not be adjusted
		// for pixel perfect display, as integral sizes
		// remain integral after rounding.
		texcoords_stream[dest_strm] =
			vector(texcoords_clipped.pos.x, texcoords_clipped.pos.y, 0);
		texcoords_stream[dest_strm + 1] =
			vector(texcoords_clipped.end_x(), texcoords_clipped.pos.y, 0);
		texcoords_stream[dest_strm + 2] =
			vector(texcoords_clipped.end_x(), texcoords_clipped.end_y(), 0);
		texcoords_stream[dest_strm + 3] =
			vector(texcoords_clipped.pos.x, texcoords_clipped.end_y(), 0);

		triangles[dest_tri] =
			mesh_triangle(dest_strm, dest_strm + 3, dest_strm + 1);
		triangles[dest_tri + 1] =
			mesh_triangle(dest_strm + 1, dest_strm + 3, dest_strm + 2);

		dest_strm += 4;
		dest_tri += 2;
		++numinserted;
	}


	// Shrink back to actually required storage space,
	// then put together final mesh and return it.
	vertices.resize(4 * numinserted);
	triangles.resize(2 * numinserted);
	texcoords_stream.resize(4 * numinserted);

	msh.validate();
	msh.rebuild_boundings();

	return std::move(msh);
}


model gui_render_util::create_model_from_rect
	(const string& group_name,
	 const rect2f& extents, const rect2f& texcoords,
	 const rect2f& clip_rect, bool enable_clip,
	 bool pixel_integral)
{
	return create_model_from_rects
		(group_name, &extents, &texcoords, 1,
		 clip_rect, enable_clip, pixel_integral);
}


model gui_render_util::create_model_from_rects
	(const string& group_name,
	 const rect2f* extents, const rect2f* texcoords, int64 count,
	 const rect2f& clip_rect, bool enable_clip,
	 bool pixel_integral)
{
	mesh msh
		(create_mesh_from_rects
			(extents, texcoords, count,
			 clip_rect, enable_clip, pixel_integral));

	model mdl;
	mdl.insert_group(group_name, std::move(msh));
	mdl.validate();
	mdl.rebuild_boundings();

	return std::move(mdl);
}


const float gui_render_util::render_epsilon = 0.0001f;




} /* namespace viral_gui */

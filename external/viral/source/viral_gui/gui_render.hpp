/**
 *************************************************************************
 *
 * @file gui_render.hpp
 *
 * GUI rendering utilities.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_RENDER_HPP)
#define INCLUDED__VIRAL_GUI__GUI_RENDER_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/mesh_forward.hpp>
#include <viral_core/render_forward.hpp>


#include <viral_core/util.hpp>
#include <viral_core/auto_pointer.hpp>
#include <viral_core/shared_pointer.hpp>
#include <viral_core/buffer.hpp>
#include <viral_core/owning_list.hpp>
#include <viral_core/string.hpp>
#include <viral_core/geo_2d.hpp>


namespace viral_gui
{


using namespace viral_core;


/**
 *************************************************************************
 *
 * @class gui_render_layer
 * @ingroup gui_module
 *
 * Merges depth- and material-consistent GUI meshes into a single mesh.
 *
 ************************************************************************/
class gui_render_layer :
	private non_copyable
{
	friend class gui_render_packer;

	public:

		/** Fetch final mesh for inserting into a render_model_data. */
		auto_pointer<mesh> release_target_mesh() NOTHROW;

		/** The material collected on this layer. */
		const shared_instance<render_material_id> material;


	private:

		enum classify_mesh_result
			{ conflict, no_conflict, on_layer };

		gui_render_layer
			(const mesh& initial_mesh,
			 const shared_instance<render_material_id>& material);

		classify_mesh_result classify_mesh
			(const mesh& source_mesh,
			 const shared_instance<render_material_id>& source_material) const;

		void append_mesh(const mesh& source_mesh);


		static const int64 initial_rects_reserve_;

		auto_pointer<mesh> target_mesh_;
		rect2f bounding_rect_;
		dynamic_buffer<rect2f> exact_bounding_rects_;
};




/**
 *************************************************************************
 *
 * @class gui_render_packer
 * @ingroup gui_module
 *
 * Reduces draw calls by merging many meshes from multiple gui_element%s
 * into fewer meshes based on materials and depth order.
 *
 * Usage notes:
 * - See render_puppet_data, render_model_data, and render_stack_data
 *		for considerations on depth order within rendering.
 *
 ************************************************************************/
class gui_render_packer :
	private non_copyable
{

	public:

		gui_render_packer();


		/**
		 * Pack a @bref{source_mesh} with given @bref{material}
		 * so that it renders above all previously packed meshes.
		 *
		 * Usually, packing merges meshes with the same material
		 * to reduce draw calls. If this could violate depth order,
		 * the packer instead creates a new, separate mesh.
		 *
		 * @attent The mesh::bounding_box() of @bref{source_mesh}
		 * must be valid to ensure correct testing for depth order.
		 */
		void pack_mesh
			(const mesh& source_mesh,
			 const shared_instance<render_material_id>& material);


		/** Access to contained render_layers. */
		//@{
		typedef owning_list<gui_render_layer> render_layer_list;	
		typedef render_layer_list::const_iterator const_render_layer_iterator;
		const_render_layer_iterator first_render_layer() const NOTHROW;
		//@}


	private:
		
		render_layer_list render_layers_;
};




/**
 *************************************************************************
 *
 * @class gui_render_util
 * @ingroup gui_module
 *
 * Static utilities for GUI rendering.
 *
 ************************************************************************/
class gui_render_util :
	private non_constructable
{

	public:

		/**
		 * Create mesh with vertices and triangles for a rectangle
		 * of given @bref{extents}. Additionally, the mesh holds
		 * a mesh_stream for given @bref{texcoords}.
		 *
		 * On optional @bref{enable_clip}, the @bref{clip_rect}
		 * clips @bref{extents}. In this case, @bref{texcoords}
		 * are adjusted appropriately.
		 * 
		 * The @bref{pixel_integral} flag rounds (clipped) extents
		 * to the nearest integer pixel for pixel-exact display.
		 */
		static mesh create_mesh_from_rect
			(const rect2f& extents, const rect2f& texcoords,
			 const rect2f& clip_rect, bool enable_clip,
			 bool pixel_integral);


		/**
		 * Create a single mesh from @bref{count} rectangles,
		 * similar to create_mesh_from_rect().
		 */
		static mesh create_mesh_from_rects
			(const rect2f* extents, const rect2f* texcoords, int64 count,
			 const rect2f& clip_rect, bool enable_clip,
			 bool pixel_integral);


		/**
		 * First create_mesh_from_rect(), then insert resulting
		 * mesh into a model under given model @bref{group_name}.
		 */
		static model create_model_from_rect
			(const string& group_name,
			 const rect2f& extents, const rect2f& texcoords,
			 const rect2f& clip_rect, bool enable_clip,
			 bool pixel_integral);


		/**
		 * Create a model from @bref{count} rectangles,
		 * similar to create_model_from_rect().
		 */
		static model create_model_from_rects
			(const string& group_name,
			 const rect2f* extents, const rect2f* texcoords, int64 count,
			 const rect2f& clip_rect, bool enable_clip,
			 bool pixel_integral);
		

		/**
		 * Geometry smaller than this epsilon is not rendered
		 * to avoid visual artifacts (e.g. after clipping).
		 */
		static const float render_epsilon;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_RENDER_HPP) */
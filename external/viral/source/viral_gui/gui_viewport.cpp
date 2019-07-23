/**
 *************************************************************************
 *
 * @file gui_viewport.cpp
 *
 * 3D viewport in GUI, implementation.
 *
 ************************************************************************/


#include "gui_viewport.hpp"


#include <viral_core/log.hpp>
#include <viral_core/render_resource.hpp>
#include <viral_core/render_command.hpp>


#include "gui.hpp"
#include "gui_render.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_viewport
//
//////////////////////////////////////////////////////////////////////////


gui_viewport::gui_viewport
	(const string& name, gui_context& gui,
	 const vector2f& preferred_size,
	 const render_texture_canvas_data& texture_template,
	 const render_material_data& material_template)
	:
	gui_element(name, gui),
	preferred_size_(preferred_size),

	texture_template_
		(clone_dynamic(texture_template)), // Clone any derived type.

	material_template_
		(clone_dynamic(material_template)) // Clone any derived type.
{
	if (!texture_template_ || !material_template_)
	{
		LOG_ERROR("Failed to clone() incoming texture or material template.");
		throw assert_failed(); // In this case, clone() probably is broken.
	}
}


vector2f gui_viewport::preferred_size() const
	{ return preferred_size_; }


void gui_viewport::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	/** Reimplement in derived classes. */
}


void gui_viewport::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;

	if (!canvas_) return;

	if (!material_)
	{
		shared_instance<render_texture_id> texture
			(new render_texture_id(name + "_texture"));
		auto_pointer<render_texture_canvas_data> texture_data
			(clone_dynamic(*texture_template_));
		texture_data->source_canvas = canvas_;
		queue.commit
			(std::move(texture_data), texture);

		material_.reset
			(new render_material_id(name + "_material"));
		auto_pointer<render_material_data> material_data
			(clone_dynamic(*material_template_));
		material_data->texture = texture;
		queue.commit
			(std::move(material_data), material_);
	}

	mesh msh
		(gui_render_util::create_mesh_from_rect
			(arrange_area(), rect2f(0, 0, 1, 1),
			 clip_area(), true, gui.pixel_integral_rendering));

	packer.pack_mesh
		(msh, material_.instance_or_throw());
}


void gui_viewport::set_canvas
	(const shared_pointer<render_canvas_id>& canvas) NOTHROW
{
	canvas_ = canvas;
	material_.reset(); // Enforce material rebuild for new canvas.
}

const shared_pointer<render_canvas_id>&
	gui_viewport::canvas() const NOTHROW
		{ return canvas_; }


void gui_viewport::realize_layout() { }




} /* namespace viral_gui */

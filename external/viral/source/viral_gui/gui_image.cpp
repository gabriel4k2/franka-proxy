/**	
 *************************************************************************
 *
 * @file gui_image.cpp
 *
 * Non-interactive image elements, implementation.
 *
 ************************************************************************/


#include "gui_image.hpp"


#include <viral_core/log.hpp>
#include <viral_core/render_resource.hpp>
#include <viral_core/render_command.hpp>


#include "gui.hpp"
#include "gui_render.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_image
//
//////////////////////////////////////////////////////////////////////////


gui_image::gui_image
	(const string& name, gui_context& gui,
	 const shared_instance<gui_image_style>& image_style,
	 const shared_instance<render_material_id>& material,
	 const vector2i& image_size)
	:
	gui_element(name, gui),
	image_style(image_style),
	internal_material_id_(new render_material_id(name + "_material")),
	internal_texture_id_(new render_texture_id(name + "_texture"))
{
	apply_material(material, image_size);
}	


gui_image::gui_image
	(const string& name, gui_context& gui,
	 const shared_instance<gui_image_style>& image_style,
	 const image_rgba& source_image,
	 const render_material_data& material_data)
	:
	gui_element(name, gui),
	image_style(image_style),
	internal_material_id_(new render_material_id(name + "_material")),
	internal_texture_id_(new render_texture_id(name + "_texture"))
{
	apply_source_image(source_image, material_data);
}


vector2f gui_image::preferred_size() const
{
	if (image_style().pixel_scale == 0)
		return image_style().base_size;

	return vector2f
			(image_style().base_size.x * image_style().pixel_scale * image_size_.x,
			 image_style().base_size.y * image_style().pixel_scale * image_size_.y);
}


void gui_image::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{ /** No action. */ }


void gui_image::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;
	commit_pending_image(queue);
	pack_image(packer);
}


void gui_image::apply_material
	(const shared_instance<render_material_id>& material,
	 const vector2i& image_size)
{
	// Directly update image_size() and material() as specified.
	// We reset any pending image and any pending material data
	// as a safety precaution to avoid accidential data overwrites.

	if (image_size.min() < 0)
	{
		LOG_ERROR("Image size must not be negative.");
		throw invalid_parameter();
	}

	// Strong exception safety: NOTHROW state update.
	material_ = material;
	image_size_ = image_size;

	pending_image_.reset();
	pending_material_data_.reset();

	want_layout();
}


void gui_image::apply_source_image
	(const image_rgba& source_image,
	 const render_material_data& material_data)
{
	// Instantly modify client-visible image state,
	// update image_size() and material() accordingly,
	// but commit material data on next commit_render().
	// Note we always use the internal IDs for this.

	auto_pointer<image_rgba> new_image
		(clone(source_image));
	auto_pointer<render_material_data> new_material_data
		(clone_dynamic(material_data));

	// Strong exception safety: NOTHROW state update.
	image_size_ = source_image.size();
	material_ = internal_material_id_;

	pending_image_ = std::move(new_image);
	pending_material_data_ = std::move(new_material_data);

	want_layout();
}


void gui_image::apply_and_commit_source_image
	(const image_rgba& source_image,
	 const render_material_data& material_data,
	 render_command_queue& queue)
{
	// See apply_source_image(), but directly call
	// commit_pending_image() unless changes in size
	// or material ID require a layout rebuild.

	auto_pointer<image_rgba> new_image
		(clone(source_image));
	auto_pointer<render_material_data> new_material_data
		(clone_dynamic(material_data));

	bool contents_changed =
		image_size_ != source_image.size() ||
		material_ != internal_material_id_;

	// Strong exception safety: NOTHROW state update.
	// Final commit may throw, which results in a retry
	// on the next want_layout() pass.
	image_size_ = source_image.size();
	material_ = internal_material_id_;

	pending_image_ = std::move(new_image);
	pending_material_data_ = std::move(new_material_data);

	if (contents_changed) want_layout();
		else commit_pending_image(queue);
}


const vector2i& gui_image::image_size() const NOTHROW
	{ return image_size_; }
const shared_pointer<render_material_id>& gui_image::material() const NOTHROW
	{ return material_; }


rect2f gui_image::build_unclipped_image_rect() const
{
	// Return empty rect for very small images
	// to avoid numeric instability issues.
	vector2f pref_size(preferred_size());
	rect2f area(arrange_area());
	if (pref_size.min() < gui_render_util::render_epsilon)
		return rect2f(area.pos + area.size * 0.5f, vector2f());


	// Build scaling factor for resulting image
	// after naive or aspect correct scaling.
	float scale_x = area.size.x / pref_size.x;
	float scale_y = area.size.y / pref_size.y;

	if (image_style().aspect == aspect_fit_x)
		scale_y = scale_x;
	else if (image_style().aspect == aspect_fit_y)
		scale_x = scale_y;
	else if (image_style().aspect == aspect_fit_both)
		scale_x = scale_y = min(scale_x, scale_y);
	else if (image_style().aspect == aspect_fill)
		scale_x = scale_y = max(scale_x, scale_y);
	else if (image_style().aspect != no_aspect)
	{
		LOG_ERROR("Invalid aspect mode specified.");
		throw assert_failed();
	}


	// Avoid upscaling or downscaling
	// if prohibited by client.
	if (!image_style().upscale)
	{
		scale_x = min(scale_x, 1.f);
		scale_y = min(scale_y, 1.f);
	}

	if (!image_style().downscale)
	{
		scale_x = max(scale_x, 1.f);
		scale_y = max(scale_y, 1.f);
	}


	// Apply scaling to preferred-size image.
	// Center result within arrange area.
	rect2f result(vector2f(), pref_size);
	result.size.x *= scale_x;
	result.size.y *= scale_y;
	result.pos =
		area.pos + (area.size - result.size) * 0.5f;

	return result;
}


void gui_image::realize_layout() { }


void gui_image::commit_pending_image
	(render_command_queue& queue)
{
	/**
	 * Apply pending image from apply_source_image()
	 * to internal texture and material IDs.
	 */

	
	// Clear individually pending image or material.
	// Should not actually happen, as both are set
	// at the same time.
	if (!pending_image_ || !pending_material_data_)
	{
		pending_image_.reset();
		pending_material_data_.reset();
		return;
	}


	// Commit texture and material to internal IDs.
	//
	// Implementation notes:
	// - We reuse both the @bref{internal_texture_id_}
	//		and the @bref{internal_material_id_} to avoid
	//		spamming the renderer on frequent image updates.
	// - We must pass the material data by auto_pointer
	//		to keep any specific client material class.
	render_texture_2d_data texture_data;
	texture_data.image_2d = clone(*pending_image_);
	texture_data.compression =
		image_style().allow_compression ?
			render_texture_data::compression_lossy :
			render_texture_data::compression_none;
	queue.commit
		(std::move(texture_data), internal_texture_id_);

	auto_pointer<render_material_data> material_data
		(clone_dynamic(*pending_material_data_));
	material_data->texture = internal_texture_id_;
	queue.commit
		(std::move(material_data), internal_material_id_);


	// NOTHROW from here.
	pending_image_.reset();
	pending_material_data_.reset();
}


void gui_image::pack_image(gui_render_packer& packer)
{
	if (!material_) return;
	
	mesh msh
		(gui_render_util::create_mesh_from_rect
			(build_unclipped_image_rect(), rect2f(0, 0, 1, 1),
			 clip_area(), true, gui.pixel_integral_rendering));

	packer.pack_mesh
		(msh, material_.instance_or_throw());
}




//////////////////////////////////////////////////////////////////////////
//
// gui_image_style
//
//////////////////////////////////////////////////////////////////////////


gui_image_style::gui_image_style
	(const vector2f& base_size,
	 float pixel_scale,
	 bool downscale, bool upscale,
	 gui_image::aspect_mode aspect,
	 bool allow_compression)
	:
	base_size(base_size),
	pixel_scale(pixel_scale),
	downscale(downscale),
	upscale(upscale),
	aspect(aspect),
	allow_compression(allow_compression)
{
	if (base_size.min() <= 0)
	{
		LOG_ERROR("Base image size must be greater than zero.")
		throw invalid_parameter();
	}

	if (pixel_scale < 0)
	{
		LOG_ERROR("Pixel scale must not be negative.");
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */

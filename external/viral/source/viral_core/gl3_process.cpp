/**
 *************************************************************************
 *
 * @file gl3_process.cpp
 *
 * Implementation of OpenGL rendering process.
 *
 ************************************************************************/


#include "gl3_process.hpp"


#include "log.hpp"
#include "integer_operations.hpp"
#include "window.hpp"
#include "gl3_scene.hpp"
#include "gl3_render_target.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// gl3_layer
//
//////////////////////////////////////////////////////////////////////////


const render_layer_data& gl3_layer::layer_data() const
	{ return layer_data_; }


void gl3_layer::execute
	(gl3_render_target& canvas_target)
{
	update_background(canvas_target);
	execute_kernel(canvas_target);
}


gl3_layer::gl3_layer
	(const covert_object_manager& object_manager,
	 const gl3_api& gl3)
	:
	object_manager_(object_manager), gl3_(gl3),
	kernel_(object_manager_, gl3_)
{ }


void gl3_layer::set_data(auto_pointer<covert_data> data)
{
	if (!data)
	{
		LOG_ERROR("Cannot apply NULL data.");
		throw null_pointer();
	}

	auto_pointer<render_layer_data> layer_data
		(data.try_cast_exact<render_layer_data>());

	if (!layer_data)
	{
		LOG_ERROR("Incoming data must be a render_layer_data.");
		throw type_mismatch();
	}

	layer_data_ = *layer_data;
}


auto_pointer<covert_data> gl3_layer::data()
	{ return layer_data_.clone(); }


void gl3_layer::update_background
	(gl3_render_target& canvas_target)
{
	render_layer_data::background_mode background =
		layer_data_.background;

	// We require a viewport to correctly restrict
	// clear and blit operations.
	canvas_target.set_viewport(layer_data_.viewport);
	canvas_target.clear_depth();

	if (background == render_layer_data::background_clear)
	{
		canvas_target.clear_colors(layer_data_.background_color);
	}

	else if (background == render_layer_data::background_copy)
	{
		if (!layer_data_.background_canvas)
		{
			LOG_ERROR("Cannot copy background from NULL canvas.");
			throw null_pointer();
		}

		shared_pointer<render_canvas_id> background_canvas_lock
			(layer_data_.background_canvas.lock());

		if (!background_canvas_lock)
		{
			LOG_ERROR("Background canvas was already deleted.\n"
					  "Please ensure strong ownership while rendering.");
			throw invalid_state();
		}

		const gl3_canvas &background_canvas =
			object_manager_.object_for_id<gl3_canvas>(background_canvas_lock);

		background_canvas.canvas_target().copy_to_render_target
			(canvas_target, rect2i(vector2i(), canvas_target.size()), false);
	}

	else if (background != render_layer_data::background_overlay)
	{
		LOG_ERROR("Unsupported background mode on gl3_layer.");
		throw unsupported_feature();
	}
}


void gl3_layer::execute_kernel
	(gl3_render_target& canvas_target)
{
	gl3_scene &scene =
		object_manager_.object_for_id<gl3_scene>(layer_data_.scene);

	kernel_.execute
		(layer_data_.cam, scene,
		 canvas_target, layer_data_.viewport);
}




//////////////////////////////////////////////////////////////////////////
//
// gl3_canvas
//
//////////////////////////////////////////////////////////////////////////


const render_canvas_data& gl3_canvas::canvas_data() const
	{ return canvas_data_; }


const gl3_render_target& gl3_canvas::canvas_target() const
{
	if (!canvas_target_)
	{
		LOG_ERROR("No canvas target available.\n"
				  "Please execute() canvas first,\n"
				  "i.e. by rendering to it.");
		throw assert_failed();
	}

	return *canvas_target_;
}


void gl3_canvas::execute
	(const vector2i& unreliable_window_size)
{
	SCOPE_GUARD
		(executing_guard, (*this, &gl3_canvas::reset_executing));

	executing_ = true;

	update_target();
	execute_layers();
	prepare_target_texture();

	try_copy_to_window(unreliable_window_size);
	try_fetch_result_rgba();
	try_fetch_result_depth();
}


bool gl3_canvas::executing() const NOTHROW
	{ return executing_; }


gl3_canvas::gl3_canvas
	(const covert_object_manager& object_manager,
	 const gl3_api& gl3)
	:
	object_manager_(object_manager), gl3_(gl3),
	executing_(false)
{ }


void gl3_canvas::set_data(auto_pointer<covert_data> data)
{
	if (!data)
	{
		LOG_ERROR("Cannot apply NULL data.");
		throw null_pointer();
	}

	auto_pointer<render_canvas_data> canvas_data
		(data.try_cast_exact<render_canvas_data>());

	if (!canvas_data)
	{
		LOG_ERROR("Incoming data must be a render_canvas_data.");
		throw type_mismatch();
	}

	canvas_data_ = *canvas_data;
}


auto_pointer<covert_data> gl3_canvas::data()
	{ return canvas_data_.clone(); }


void gl3_canvas::update_target()
{
	// Ensure target is not @NULL and correctly sized
	// for all consecutive operations.
	gl3_render_target::update
		(gl3_, canvas_target_,
		 canvas_data_.pixel_size,
		 canvas_data_.wrap,
		 canvas_data_.filter,
		 canvas_data_.color_space);
}


void gl3_canvas::execute_layers()
{
	for (render_canvas_data::layer_list::const_iterator it
			(canvas_data_.layers.first()); it; ++it)
	{
		if (!it)
		{
			LOG_ERROR("List of layers contains a NULL layer.");
			throw null_pointer();
		}

		gl3_layer &layer =
			object_manager_.object_for_id<gl3_layer>(*it);

		layer.execute(*canvas_target_);
	}
}


void gl3_canvas::prepare_target_texture()
	{ canvas_target_->prepare_texture(); }


void gl3_canvas::try_copy_to_window
	(const vector2i& unreliable_window_size)
{
	if (!canvas_data_.copy_to_window)
		return;

	canvas_target_->copy_to_window
		(canvas_data_.target_window_pixels,
		 unreliable_window_size, false);
}


void gl3_canvas::try_fetch_result_rgba()
{
	if (!canvas_data_.query_result_rgba)
	{
		canvas_data_.result_rgba.free();
		return;
	}


	// Fetch and possibly resize result storage.
	image_rgba& result = canvas_data_.result_rgba;
	if (result.size() != canvas_target_->size())
	{
		image_rgba resize_result(canvas_target_->size());
		swap(result, resize_result);
	}

	GLsizei result_size_x =
		integer_operations::cast_or_throw<GLsizei>(result.size().x);
	GLsizei result_size_y =
		integer_operations::cast_or_throw<GLsizei>(result.size().y);


	// Read pixels to result storage.
	canvas_target_->bind_color_read();
	glReadPixels
		(0, 0, result_size_x, result_size_y,
		 GL_RGBA, GL_UNSIGNED_BYTE, result.data());
}


void gl3_canvas::try_fetch_result_depth()
{
	if (!canvas_data_.query_result_depth)
	{
		canvas_data_.result_depth.free();
		return;
	}


	// Fetch and possibly resize result storage.
	image_depth& result = canvas_data_.result_depth;
	if (result.size() != canvas_target_->size())
	{
		image_depth resize_result(canvas_target_->size());
		swap(result, resize_result);
	}

	GLsizei result_size_x =
		integer_operations::cast_or_throw<GLsizei>(result.size().x);
	GLsizei result_size_y =
		integer_operations::cast_or_throw<GLsizei>(result.size().y);


	// Read pixels to result storage.
	canvas_target_->bind_color_read();
	glReadPixels
		(0, 0, result_size_x, result_size_y,
		 GL_DEPTH_COMPONENT, GL_FLOAT, result.data());
}


void gl3_canvas::reset_executing() NOTHROW
	{ executing_ = false; }




//////////////////////////////////////////////////////////////////////////
//
// gl3_process
//
//////////////////////////////////////////////////////////////////////////


const render_process_data& gl3_process::process_data() const
	{ return process_data_; }


void gl3_process::execute
	(const vector2i& unreliable_window_size)
{
	for (render_process_data::canvas_list::const_iterator it
			(process_data_.canvases.first()); it; ++it)
	{
		if (!it)
		{
			LOG_ERROR("List of canvases contains a NULL canvas.");
			throw null_pointer();
		}

		gl3_canvas &canvas =
			object_manager_.object_for_id<gl3_canvas>(*it);

		canvas.execute(unreliable_window_size);
	}
}


gl3_process::gl3_process
	(const covert_object_manager& object_manager,
	 const gl3_api& gl3)
	:
	object_manager_(object_manager), gl3_(gl3)
{ }


void gl3_process::set_data(auto_pointer<covert_data> data)
{
	if (!data)
	{
		LOG_ERROR("Cannot apply NULL data.");
		throw null_pointer();
	}

	auto_pointer<render_process_data> process_data
		(data.try_cast_exact<render_process_data>());

	if (!process_data)
	{
		LOG_ERROR("Incoming data must be a render_process_data.");
		throw type_mismatch();
	}

	process_data_ = *process_data;
}


auto_pointer<covert_data> gl3_process::data()
	{ return process_data_.clone(); }




} /* namespace viral_core */

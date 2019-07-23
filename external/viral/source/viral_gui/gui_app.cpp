/**
 *************************************************************************
 *
 * @file gui_app.cpp
 *
 * GUI application framework, implementation.
 *
 ************************************************************************/


#include "gui_app.hpp"


#include <viral_core/log.hpp>
#include <viral_core/file.hpp>
#include <viral_core/window.hpp>
#include <viral_core/input.hpp>
#include <viral_core/render.hpp>
#include <viral_core/render_resource.hpp>
#include <viral_core/render_process.hpp>
#include <viral_core/render_command.hpp>
#include <viral_core/render_task.hpp>


#include "gui_event.hpp"
#include "gui_style_util.hpp"
#include "gui_stack.hpp"
#include "gui_element_loader.hpp"
#include "gui_style_loader.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_bootstrap
//
//////////////////////////////////////////////////////////////////////////


gui_bootstrap::gui_bootstrap
	(file_context& files,
	 render_task& rtask,
	 gui_shader_cache& shader_cache,
	 gui_material_cache& material_cache,
	 gui_style_cache& style_cache,
	 const string& vertex_shader_path,
	 const string& fragment_shader_path,
	 const string& material_path,
	 const list<string>& style_xml_paths)
	:
	gui_shader_id(new render_shader_id("gui_shader"))
{
	LOG_INFO("Bootstrapping GUI.");


	gui_style_loader style_loader
		("gui_style_loader", files,
		 shader_cache, material_cache, style_cache); 


	render_command_queue qinit;


	// Load shaders from source and commit to rendering system,
	// finally add to cache for use within style and element XML.
	LOG_INFO("Loading GUI shaders: " + vertex_shader_path +
							" and: " + fragment_shader_path);

	render_shader_data shader_data;

	auto_pointer<file> vertex_shader
		(files.open_file(vertex_shader_path, file::read_only));
	shader_data.vertex_shader =
		vertex_shader->read_text();

	shader_data.vertex_shader_virtual_path_hint =
		vertex_shader_path;
	shader_data.vertex_shader_absolute_path_hint =
		files.to_absolute_path(vertex_shader_path);

	auto_pointer<file> fragment_shader
		(files.open_file(fragment_shader_path, file::read_only));		
	shader_data.fragment_shader =
		fragment_shader->read_text();

	shader_data.fragment_shader_virtual_path_hint =
		fragment_shader_path;
	shader_data.fragment_shader_absolute_path_hint =
		files.to_absolute_path(fragment_shader_path);

	qinit.commit(shader_data, gui_shader_id);

	shader_cache.insert(gui_shader_name, gui_shader_id);


	// Create and commit materials required for all GUI styles.
	// Additionally cache materials for use in style and element XML.
	// We prohibit filtering or compression on GUI style materials
	// in order to maintain sharp outlines and readable text.
	LOG_INFO("Loading GUI materials: " + material_path);

	render_texture_2d_data texture_template;
	texture_template.filter = render_texture_data::filter_nearest;
	texture_template.compression = render_texture_data::compression_none;
	texture_template.keep_host_image = false;

	render_material_data material_template;
	material_template.shader = gui_shader_id;
	material_template.blend = render_material_data::blend_alpha;	
	material_template.unlit = true;

	style_loader.create_materials
		(material_path, "png",
		 texture_template, material_template,
		 qinit);

	rtask.execute(std::move(qinit));
	rtask.finish();


	// Load all style XMLs.
	LOG_INFO("Loading GUI styles.");

	for (list<string>::const_iterator it
			(style_xml_paths.first()); it; ++it)
	{
		style_loader.load(*it);
	}
}


const char* const gui_bootstrap::gui_shader_name = "gui_shader";




//////////////////////////////////////////////////////////////////////////
//
// gui_app
//
//////////////////////////////////////////////////////////////////////////


void gui_app::register_event_handler
	(const string& name,
	 auto_pointer<gui_event_handler> event_handler)
{
	register_event_handler
		(element_cache_.entry(name),
		 std::move(event_handler));
}


void gui_app::register_event_handler
	(const shared_instance<gui_element>& element,
	 auto_pointer<gui_event_handler> event_handler)
{
	if (!event_handler)
	{
		LOG_ERROR("Cannot register NULL event handler.");
		throw null_pointer();
	}


	// Fetch list of event handlers for incoming element.
	// Create this list if it does not exist yet.
	event_handler_map::const_iterator handler_list_it
		(event_handlers_.find(element));

	if (!handler_list_it)
	{
		auto_pointer<event_handler_list> handler_list
			(new event_handler_list());

		handler_list_it =
			event_handlers_.insert
				(element, std::move(handler_list));

		// Exception safety:
		// We need not delete any new event handler list
		// in case of later exceptions since it is empty.
	}


	// Add event handler to existing or new handler list.
	// Assert valid list pointer just for safety reasons.
	if (!handler_list_it->value)
	{
		LOG_ERROR("Gui element: " + element().name +
				  " has NULL event handler list.");
		throw assert_failed();
	}

	handler_list_it->value->insert
		(std::move(event_handler)); 
}


void gui_app::remove_event_handlers
	(const string& name) NOTHROW
{
	shared_pointer<gui_element> element
		(element_cache_.find(name));

	if (!element)
	{
		LOG_ERROR("Element: " + name + " not found in element cache.");
		return; // NOTHROW.
	}

	remove_event_handlers(element);
}


void gui_app::remove_event_handlers
	(const weak_pointer<gui_element>& element) NOTHROW
{
	event_handler_map::iterator handler_list_it
		(event_handlers_.find(element));

	if (!handler_list_it)
	{
		LOG_WARN("Element has no event handlers.");
		return; // NOTHROW.
	}

	event_handlers_.remove(handler_list_it);
}


void gui_app::purge_event_handlers() NOTHROW
{
	for (event_handler_map::iterator it
			(event_handlers_.first()); it;)
	{
		event_handler_map::iterator del_it(it);
		++it;

		if (!del_it->key.lock())
			event_handlers_.remove(del_it);
	}
}


void gui_app::clear_event_handlers() NOTHROW
	{ event_handlers_.clear(); }


const float gui_app::timer_timestep_seconds = 0.01f;
const float gui_app::timer_maximum_seconds = 0.5f;


gui_app::gui_app
	(const string& application_name,
	 const string& vertex_shader_path,
	 const string& fragment_shader_path,
	 const string& material_path,
	 const list<string>& style_paths,
	 const string& default_cursor_style,
	 const list<string>& element_paths,
	 const string& root_element)
	:
	application_name(application_name),

	timer_(timer_timestep_seconds, timer_maximum_seconds),
	engine_(application_name),

	shader_cache_(application_name + "_gui_shaders"),
	material_cache_(application_name + "_gui_materials"),
	style_cache_(application_name + "_gui_styles"),
	element_cache_(application_name + "_gui_elements"),

	bootstrap_
		(engine_.files(), engine_.rtask(),
		 shader_cache_, material_cache_, style_cache_,
		 vertex_shader_path,
		 fragment_shader_path,
		 material_path,
		 style_paths),

	gui_(application_name + "_gui",
		 engine_.window(), engine_.input(),
		 *this,
		 style_cache_.entry<gui_cursor_style>(default_cursor_style),
		 true),

	process_id_(new render_process_id(application_name + "_render_process")),
	canvas_id_(new render_canvas_id(application_name + "_render_canvas"))
{
	initialize_elements(element_paths, root_element);
	LOG_INFO("Finished GUI initialization for: " + application_name);
}


void gui_app::execute_unsafe()
	{ execute(); }


void gui_app::logics_hook(gui_modal_interaction* modal_interaction)
{
	/** Default implementation does nothing. */
}


void gui_app::render_hook(render_command_queue& queue)
{
	/** Default implementation does nothing. */
}


void gui_app::execute()
{
	// Perform post-loading operations:
	// Actually show the application window,
	// and discard accumulated startup time.
	initialize_window();
	timer_.restart();

	while (main_loop_iteration(NULL));
}


void gui_app::initialize_elements
	(const list<string>& element_paths, const string& root_element)
{
	LOG_INFO("Loading GUI elements.");

	gui_element_loader element_loader
		(application_name + "_gui_element_loader",
		 gui_, engine_.files(),
		 shader_cache_, material_cache_, style_cache_,
		 element_cache_);

	for (list<string>::const_iterator it(element_paths.first()); it; ++it)
		element_loader.load_elements(*it);

	gui_.modal_stack().insert_layer
		(element_cache_.entry(root_element));
}


void gui_app::initialize_window()
{
	LOG_INFO("Showing application window.");

	engine_.window().set_caption(application_name);
	engine_.window().set_display
		(vector2f(100, 100), vector2f(1024, 768),
		 window_context::no_cursor);
}


bool gui_app::main_loop_iteration
	(gui_modal_interaction* modal_interaction)
{
	/**
	 * An iteration through the main GUI loop,
	 * either for the core application or for
	 * an ongoing modal interaction.
	 *
	 * Returns false to indicate that the program should exit,
	 * which is ignored while a modal interaction is in progress.
	 */

	timer_.try_sleep();
	timer_.update();

	if (!timer_.has_next_timestep())
		return true;

	while (timer_.next_timestep())
	{
		engine_.window().update();
		engine_.input().update();

		if (!modal_interaction &&
			engine_.window().close_flag())
				return false;

		logics(modal_interaction);
	}

	if (!engine_.rtask().has_pending_queue())
		render();

	return true;
}


void gui_app::modal_iteration
	(gui_modal_interaction& modal_interaction)
		{ main_loop_iteration(&modal_interaction); }


void gui_app::logics(gui_modal_interaction* modal_interaction)
{
	logics_hook(modal_interaction);

	if (modal_interaction)
	{
		gui_modal_context::update(*modal_interaction);
		return;
	}

	logics_event_handlers();
}


void gui_app::logics_event_handlers()
{
	// Let GUI update for user input on its own,
	// then run suitable event handlers.
	gui_output_event_list output;
	gui_.update(output);

	for (gui_output_event_list::const_iterator event_it
			(output.first()); event_it; ++event_it)
	{
		if (!*event_it) continue;


		// Find list of handlers for event element.
		event_handler_map::const_iterator handler_list_it
			(event_handlers_.find((*event_it)->source_untyped));

		if (!handler_list_it) continue;
		if (!handler_list_it->value) continue;


		// Execute each handler in handler list.
		for (event_handler_list::const_iterator handler_it
				(handler_list_it->value->first()); handler_it; ++handler_it)
		{
			if (!*handler_it) continue;
			(*handler_it)->handle_event(**event_it);
		}
	}


	// Purge event handlers only if clients
	// had any events for deleting GUI elements.
	if (!output.empty())
		purge_event_handlers();
}


void gui_app::render()
{
	render_command_queue queue;

	render_hook(queue);

	render_process_data process_data;
	process_data.canvases.insert(canvas_id_);
	process_data.show_results = true;
	queue.commit(process_data, process_id_);

	render_canvas_data canvas_data;
	canvas_data.copy_to_window = true;
	canvas_data.layers.insert(gui_.render_layer());
	canvas_data.pixel_size =
		canvas_data.target_window_pixels.size =
			engine_.window().size().to_vector2i();
	queue.commit(canvas_data, canvas_id_);

	gui_.commit_render(queue);

	queue.render(process_id_);
	queue.purge();
	engine_.rtask().execute
		(std::move(queue));
}




} /* namespace viral_gui */

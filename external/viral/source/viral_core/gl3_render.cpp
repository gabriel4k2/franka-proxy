/**
 *************************************************************************
 *
 * @file gl3_render.cpp
 *
 * A render_context based on OpenGL, core version 3, implementation.
 *
 ************************************************************************/


#include "gl3_render.hpp"


#include "log.hpp"
#include "scope_guard.hpp"
#include "integer_operations.hpp"
#include "covert_data.hpp"
#include "ms_window.hpp"
#include "gl3_api.hpp"
#include "gl3_resource.hpp"
#include "gl3_scene.hpp"
#include "gl3_process.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// gl3_render_context
//
//////////////////////////////////////////////////////////////////////////


gl3_render_context::gl3_render_context
	(const string& context_name,
	 window_context& window)
	:
	render_context(context_name),
	window(window),

#if defined(VIRAL__HAVE_MS_WINDOW)	
	render_device_context_(NULL),
	gl_api_context_(NULL)
#endif

{
	LOG_INFO("Using gl3_render_context implementation.");


	// Init OpenGL API context.
	init_gl_api_context();
	SCOPE_GUARD
		(shutdown_gl_on_exception_only,
			(*this, &gl3_render_context::shutdown_gl_api_context));


	// Acquire OpenGL API context for remaining construction.
	// There is no need to use the acquire_api_exclusive() variant,
	// as there must not be concurrent threads during construction.
	acquire_api();
	SCOPE_GUARD
		(release_api_on_any_exit,
			(*this, &gl3_render_context::release_api));


	// The below calls require an initialized OpenGL API context
	// and are undone through member destructors on exception.
	init_gl_driver_info();
	init_gl_api();
	register_object_creators();


	// Success, NOTHROW from here.
	shutdown_gl_on_exception_only.dismiss();
}


gl3_render_context::~gl3_render_context() NOTHROW
{
	LOG_INFO("Destroying.");

	clear_on_destruction();
	shutdown_gl_api_context();
}


auto_pointer<covert_object> gl3_render_context::create_object
	(const shared_pointer<covert_id>& id)
{
	if (!id)
	{
		LOG_ERROR("Cannot create object for NULL ID.");
		throw null_pointer();
	}

	object_creator_map::iterator creator
		(object_creators_.find(typeid(*id)));

	if (!creator || !creator->value)
	{
		LOG_ERROR("No object implemented for ID: " + id->debug_name);
		throw unsupported_feature();
	}

	return (this->*(creator->value))();
}


void gl3_render_context::render
	(const shared_pointer<render_process_id>& process)
{	
	/**
	 * Execute given rendering process.
	 *
	 * Implementation notes:
	 * - We cache the unreliable window_context::size()
	 *		to achieve some consistency against concurrent
	 *		window_context::update()s.
	 * - We show results here to avoid a back-dependency
	 *		from gl3_process to gl3_render_context.
	 */

	gl3_process &process_object =
		object_manager().object_for_id<gl3_process>(process);

	vector2f unreliable_window_size_float(window.size());
	vector2i unreliable_window_size
		(integer_operations::cast_or_throw<int64>
			(unreliable_window_size_float.x),
		 integer_operations::cast_or_throw<int64>
			(unreliable_window_size_float.y));

	process_object.execute
		(unreliable_window_size);

	if (process_object.process_data().show_results)
		show_results();
}


void gl3_render_context::init_gl_driver_info()
{
	LOG_INFO("Driver information:");

	string vendor
		(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
	string renderer
		(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
	string version
		(reinterpret_cast<const char*>(glGetString(GL_VERSION)));

	LOG_INFO("Vendor: " + vendor);
	LOG_INFO("Renderer: " + renderer);
	LOG_INFO("Version: " + version);
}


void gl3_render_context::register_object_creators()
{
	object_creators_.insert
		(typeid(render_texture_id),
		 &gl3_render_context::create_typed_object<gl3_texture>);
	object_creators_.insert
		(typeid(render_shader_id),
		 &gl3_render_context::create_typed_object<gl3_shader>);
	object_creators_.insert
		(typeid(render_material_id),
		 &gl3_render_context::create_typed_object<gl3_material>);
	object_creators_.insert
		(typeid(render_model_id),
		 &gl3_render_context::create_typed_object<gl3_model>);

	object_creators_.insert
		(typeid(render_light_id),
		 &gl3_render_context::create_typed_object<gl3_light>);
	object_creators_.insert
		(typeid(render_puppet_id),
		 &gl3_render_context::create_typed_object<gl3_puppet>);
	object_creators_.insert
		(typeid(render_stack_id),
		 &gl3_render_context::create_typed_object<gl3_stack>);
	object_creators_.insert
		(typeid(render_scene_id),
		 &gl3_render_context::create_typed_object<gl3_scene>);

	object_creators_.insert
		(typeid(render_layer_id),
		 &gl3_render_context::create_typed_object<gl3_layer>);
	object_creators_.insert
		(typeid(render_canvas_id),
		 &gl3_render_context::create_typed_object<gl3_canvas>);
	object_creators_.insert
		(typeid(render_process_id),
		 &gl3_render_context::create_typed_object<gl3_process>);
}


template <typename T>
	auto_pointer<covert_object> gl3_render_context::create_typed_object()
		{ return auto_pointer<covert_object>(new T(object_manager(), *gl3_)); }




//////////////////////////////////////////////////////////////////////////
//
// gl3_render_context, Microsoft Windows implementation
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__HAVE_MS_WINDOW)


void gl3_render_context::acquire_api()
{
	if (!wglMakeCurrent(render_device_context_, gl_api_context_))
	{
		LOG_ERROR("Failed to acquire OpenGL API context.");
		throw api_call_failed();
	}
}


void gl3_render_context::release_api() NOTHROW
{
	if (!wglMakeCurrent(NULL, NULL))
	{
		LOG_ERROR("Failed to release OpenGL API context.");
		return; // NOTHROW
	}
}


void gl3_render_context::show_results()
{
	if (!SwapBuffers(render_device_context_))
	{
		LOG_ERROR("Failed to swap display buffers.");
		throw api_call_failed();
	}
}


void gl3_render_context::init_gl_api_context()
{
	LOG_INFO("Retrieving device context for rendering.");

	ms_window_context* mswin =
		dynamic_cast<ms_window_context*>(&window);

	if (!mswin)
	{
		LOG_ERROR("Expected a ms_window_context as target window.");
		throw type_mismatch();
	}

	render_device_context_ = 
		mswin->render_device_context();


	LOG_INFO("Setting pixel format.");

	// We only use the window back-buffer to display final pixel colors.
	// Any color read-backs work on canvases instead of the window buffer,
	// thus we can use PFD_SWAP_EXCHANGE for a small performance gain.
	// An explicit PFD_DEPTH_DONTCARE is required to suppress depth bits.

	PIXELFORMATDESCRIPTOR format =
		{ sizeof(PIXELFORMATDESCRIPTOR), 1,
		  PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW |
		  PFD_DOUBLEBUFFER | PFD_SWAP_EXCHANGE | PFD_DEPTH_DONTCARE,
		  PFD_TYPE_RGBA, 24,
		  8, 0, 8, 0, 8, 0, 8, 0,
		  0, 0, 0, 0, 0,
		  0, 0,
		  0, 0, 0, 0, 0, 0 };

	int pixel_format =
		ChoosePixelFormat(render_device_context_, &format);

	if (!pixel_format)
	{
		LOG_ERROR("No appropriate pixel format available.");
		throw unsupported_feature();
	}

	if (!SetPixelFormat(render_device_context_, pixel_format, &format))
	{
		LOG_CRITICAL("Couldn't set pixel format.");
		throw system_call_failed();
	}


	LOG_INFO("Creating OpenGL API context.");

	gl_api_context_ =
		wglCreateContext(render_device_context_);

	if (!gl_api_context_)
	{
		LOG_CRITICAL("Couldn't create OpenGL API context.");
		throw system_call_failed();
	}
}


void gl3_render_context::init_gl_api()
{
	/**
	 * Initialize normal and WGL-specific extensions.
	 * The latter actually are encapsulated into a "normal" extension,
	 * but we only check for an appropriate procedure address.
	 *
	 * Once initialized, the gl3_ pointer remains not-@NULL
	 * up to the destruction of the gl3_render_context.
	 */
	LOG_INFO("Parsing OpenGL extensions.");

	string extension_string
		(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)));

	PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB =
		reinterpret_cast<PFNWGLGETEXTENSIONSSTRINGARBPROC>
			(wglGetProcAddress("wglGetExtensionsStringARB"));

	if (wglGetExtensionsStringARB)
	{
		extension_string += " ";
		extension_string += wglGetExtensionsStringARB(render_device_context_);
	}
	else
	{
		LOG_WARN("Failed to fetch Windows-specific OpenGL extensions.");
	}

	gl3_.reset(new gl3_api(extension_string));
}


void gl3_render_context::shutdown_gl_api_context() NOTHROW
{
	wglMakeCurrent(render_device_context_, gl_api_context_);

	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
		LOG_WARN("Some gl error occurred: " + error);

	wglMakeCurrent(NULL, NULL);

	if (gl_api_context_)
	{
		LOG_INFO("Deleting OpenGL API context.");

		if (!wglDeleteContext(gl_api_context_))
			LOG_ERROR("Failed to delete OpenGL API context.");

		gl_api_context_ = NULL;
	}
}


#endif /* defined(VIRAL__HAVE_MS_WINDOW) */




} /* namespace viral_core */

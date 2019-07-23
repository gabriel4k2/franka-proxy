/**
 *************************************************************************
 *
 * @file gl3_render.hpp
 *
 * A render_context based on OpenGL, core version 3.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_RENDER_HPP)
#define INCLUDED__VIRAL_CORE__GL3_RENDER_HPP


#include "build_config.hpp"
#include "gl3_config.hpp"


#include "window_forward.hpp"
#include "gl3_forward.hpp"


#include "auto_pointer.hpp"
#include "tree_map.hpp"
#include "render.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup gl3_render_module OpenGL3 Rendering Module
 * @ingroup render_module
 *
 * Uses the OpenGL API in core version 3 for all rendering.
 *
 ************************************************************************/


/**
 *************************************************************************
 *
 * @class gl3_render_context
 * @ingroup gl3_render_module
 *
 * A render_context based on OpenGL, core version 3.
 *
 * Usage notes:
 * - See gl3_canvas for the main rendering entry point.
 * - See gl3_kernel for details on the shading pipeline.
 *
 * Coordinate system considerations:
 * - All OpenGL images (textures, framebuffers, viewports) expect
 *		a left-handed coordinate system with a bottom-left origin.
 *		This conflicts with image_raw and @ref camera conventions.
 * - OpenGL bottom-up rules only are relevant for interpretation and
 *		display of final window contents. Thus, we can consistently
 *		ignore most OpenGL specifics and use our own conventions.
 * - We explicitly store OpenGL images top-down instead of bottom-up
 *		(e.g. we provide top-left pixel pointers to glTexImage).
 * - We do not explicitly flip texture or framebuffer coordinates
 *		(e.g. shader code refers to a bottom-right texture pixel
 *		as (1, 1), and to a top-left viewport position as (-1, -1)).
 * - Conversely, we must perform an explicit flip for final display
 *		on any target window and on corresponding window read-back.
 * - Since we render with wrong handedness under OpenGL conventions,
 *		front-facing mesh_triangle%s have clockwise winding in OpenGL.
 *
 ************************************************************************/
class gl3_render_context : public render_context
{

	public:

		/**
		 * Create OpenGL3 rendering context that renders
		 * into some @bref{window}. The target window must
		 * remain valid for the entire renderer lifetime.
		 *
		 * @attent Only a single gl3_render_context must be bound
		 * to each window_context to avoid threading-related errors.
		 */
		gl3_render_context
			(const string& context_name,
			 window_context& window);


		~gl3_render_context() NOTHROW override;


		window_context& window;


	private:

		void acquire_api() override;
		void release_api() NOTHROW override;

		auto_pointer<covert_object> create_object
			(const shared_pointer<covert_id>& id) override;

		void render
			(const shared_pointer<render_process_id>& process) override;

		void show_results();

		void init_gl_api_context();
		void init_gl_driver_info();
		void init_gl_api();

		void register_object_creators();
		template <typename T>
			auto_pointer<covert_object> create_typed_object();

		void shutdown_gl_api_context() NOTHROW;


		auto_pointer<gl3_api> gl3_;

		typedef auto_pointer<covert_object>
			(gl3_render_context::*object_creator_function)();

		typedef tree_map<type_info_ref, object_creator_function> object_creator_map;
		object_creator_map object_creators_;


		#if defined(VIRAL__HAVE_MS_WINDOW)
			HDC render_device_context_;
			HGLRC gl_api_context_;
		#endif

		#if !defined(VIRAL__HAVE_MS_WINDOW)
			#error "Not implemented for current window_context back-end."
		#endif
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_RENDER_HPP) */

/**
 *************************************************************************
 *
 * @file gl3_process.hpp
 *
 * OpenGL3 rendering process.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_PROCESS_HPP)
#define INCLUDED__VIRAL_CORE__GL3_PROCESS_HPP


#include "build_config.hpp"


#include "covert_object.hpp"
#include "render_process.hpp"
#include "gl3_kernel.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_layer
 * @ingroup gl3_render_module
 *
 * A rendering layer as used by gl3_render_context.
 *
 ************************************************************************/
class gl3_layer : public covert_object
{
	friend class gl3_render_context;

	public:

		typedef render_layer_id covert_id_type;

		const render_layer_data& layer_data() const;


		/**
		 * Render some scene into the gl3_render_target
		 * of the parent gl3_canvas.
		 */
		void execute(gl3_render_target& canvas_target);


	private:

		gl3_layer
			(const covert_object_manager& object_manager,
			 const gl3_api& gl3);

		void set_data(auto_pointer<covert_data> data) override;
		auto_pointer<covert_data> data() override;

		void update_background(gl3_render_target& canvas_target);
		void execute_kernel(gl3_render_target& canvas_target);


		const covert_object_manager& object_manager_;
		const gl3_api& gl3_;

		render_layer_data layer_data_;

		gl3_kernel kernel_;
};




/**
 *************************************************************************
 *
 * @class gl3_canvas
 * @ingroup gl3_render_module
 *
 * A rendering canvas as used by gl3_render_context.
 *
 ************************************************************************/
class gl3_canvas : public covert_object
{
	friend class gl3_render_context;

	public:

		typedef render_canvas_id covert_id_type;

		const render_canvas_data& canvas_data() const;
		const gl3_render_target& canvas_target() const;


		/**
		 * Render all contained gl3_layers into canvas_target(),
		 * then potentially copy results to the bound window.
		 */
		void execute
			(const vector2i& unreliable_window_size);


		/**
		 * Test if this canvas is currently executing.
		 * Avoids read-write conflicts with canvas textures,
		 * see gl3_texture_object_canvas.
		 */
		bool executing() const NOTHROW;


	private:

		gl3_canvas
			(const covert_object_manager& object_manager,
			 const gl3_api& gl3);

		void set_data(auto_pointer<covert_data> data) override;
		auto_pointer<covert_data> data() override;

		void update_target();
		void execute_layers();
		void prepare_target_texture();

		void try_copy_to_window
			(const vector2i& unreliable_window_size);

		void try_fetch_result_rgba();
		void try_fetch_result_depth();

		void reset_executing() NOTHROW;


		const covert_object_manager& object_manager_;
		const gl3_api& gl3_;

		render_canvas_data canvas_data_;

		auto_pointer<gl3_render_target> canvas_target_;
		bool executing_;
};




/**
 *************************************************************************
 *
 * @class gl3_process
 * @ingroup gl3_render_module
 *
 * A rendering process as used by gl3_render_context.
 *
 ************************************************************************/
class gl3_process : public covert_object
{
	friend class gl3_render_context;

	public:

		typedef render_process_id covert_id_type;

		const render_process_data& process_data() const;


		/** Execute all contained gl3_canvas%es. */
		void execute
			(const vector2i& unreliable_window_size);


	private:

		gl3_process
			(const covert_object_manager& object_manager,
			 const gl3_api& gl3);

		void set_data(auto_pointer<covert_data> data) override;
		auto_pointer<covert_data> data() override;


		const covert_object_manager& object_manager_;
		const gl3_api& gl3_;

		render_process_data process_data_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_PROCESS_HPP) */

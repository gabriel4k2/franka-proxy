/**
 *************************************************************************
 *
 * @file null_render.hpp
 *
 * Null rendering system.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__NULL_RENDER_HPP)
#define INCLUDED__VIRAL_CORE__NULL_RENDER_HPP


#include "build_config.hpp"


#include "auto_pointer.hpp"
#include "covert_object.hpp"
#include "render.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class null_render_context
 * @ingroup render_module
 *
 * A render_context that does not perform any graphics rendering.
 *
 * Usage notes:
 * - A null_render_context does not fully mimic a true render_context.
 *		For example, null_render_context ignores render_command_render
 *		and thus does not perform correct queries of rendering results.
 *
 ************************************************************************/
class null_render_context :
	public render_context
{
	template <typename T> friend class null_render_object;


	public:

		null_render_context
			(const string& context_name);


		~null_render_context() NOTHROW override;


	private:

		void acquire_api() override;
		void release_api() NOTHROW override;

		auto_pointer<covert_object> create_object
			(const shared_pointer<covert_id>& id) override;

		void render
			(const shared_pointer<render_process_id>& process) override;

		void register_object_creators();
		template <typename T>
			auto_pointer<covert_object> create_typed_object();

		static void log_and_throw_null_data();
		static void log_and_throw_invalid_data_type();


		typedef auto_pointer<covert_object>
			(null_render_context::*object_creator_function)();

		typedef tree_map<type_info_ref, object_creator_function> object_creator_map;
		object_creator_map object_creators_;
};




/**
 *************************************************************************
 *
 * @class null_render_object
 * @ingroup render_module
 *
 * A generic covert_object that stores covert_data of type @bref{T}
 * for the null_render_context.
 *
 ************************************************************************/
template <typename T>
	class null_render_object :
		public covert_object
{
	friend class null_render_context;


	public:

		typedef typename T::covert_id_type covert_id_type;


	private:

		null_render_object() { }


		void set_data(auto_pointer<covert_data> data) override
		{
			if (!data)
				null_render_context::log_and_throw_null_data();

			auto_pointer<T> data_typed
				(data.try_cast<T>());

			if (!data_typed)
				null_render_context::log_and_throw_invalid_data_type();

			// Exception safety: NOTHROW from here.
			data_ = std::move(data_typed);
		}


		auto_pointer<covert_data> data() override
			{ return data_ ? data_->clone() : auto_pointer<T>(); }


		auto_pointer<T> data_;
};


/**
 * Back-end objects for null_render_context.
 * @ingroup render_module
 */
//@{
typedef null_render_object<render_texture_data> null_render_texture;
typedef null_render_object<render_shader_data> null_render_shader;
typedef null_render_object<render_material_data> null_render_material;
typedef null_render_object<render_model_data> null_render_model;

typedef null_render_object<render_light_data> null_render_light;
typedef null_render_object<render_puppet_data> null_render_puppet;
typedef null_render_object<render_stack_data> null_render_stack;
typedef null_render_object<render_scene_data> null_render_scene;

typedef null_render_object<render_layer_data> null_render_layer;
typedef null_render_object<render_canvas_data> null_render_canvas;
typedef null_render_object<render_process_data> null_render_process;
//@}




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__NULL_RENDER_HPP) */

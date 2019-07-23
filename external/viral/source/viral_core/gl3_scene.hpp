/**
 *************************************************************************
 *
 * @file gl3_scene.hpp
 *
 * Scene-specific backend objects of OpenGL3.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_SCENE_HPP)
#define INCLUDED__VIRAL_CORE__GL3_SCENE_HPP


#include "build_config.hpp"


#include "covert_object.hpp"
#include "render_scene.hpp"
#include "gl3_uniform.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_scene_object_tag
 * @ingroup gl3_render_module
 *
 * Tag for objects that can be part of a gl3_scene.
 *
 ************************************************************************/
class gl3_scene_object_tag
{
	public:

		virtual ~gl3_scene_object_tag() NOTHROW { }


	protected:

		gl3_scene_object_tag() { }
};




/**
 *************************************************************************
 *
 * @class gl3_light
 * @ingroup gl3_render_module
 *
 * Any scene light supported by gl3_render_context.
 *
 ************************************************************************/
class gl3_light :
	public covert_object,
	public gl3_scene_object_tag
{
	friend class gl3_render_context;

	public:

		typedef render_light_id covert_id_type;

		const render_light_data& light_data() const;

	private:

		gl3_light
			(const covert_object_manager& object_manager,
			 const gl3_api& gl3);

		void set_data(auto_pointer<covert_data> data) override;
		auto_pointer<covert_data> data() override;

		const covert_object_manager& object_manager_;
		const gl3_api& gl3_;

		render_light_data light_data_;
};




/**
 *************************************************************************
 *
 * @class gl3_puppet_data
 * @ingroup gl3_render_module
 *
 * Provides puppet-specific gl3_uniform_value%s.
 *
 * Usage notes:
 * - Puppet-specific gl3_uniform_value%s override both automatic
 *		and material-specific uniform values of the same name.
 *		See gl3_kernel and gl3_uniform_value_stack.
 *
 ************************************************************************/
class gl3_puppet_data :
	public render_puppet_data,
	public gl3_scene_object_tag
{

	public:

		typedef render_puppet_id covert_id_type;


		gl3_puppet_data() { }

		gl3_puppet_data(const gl3_puppet_data& other)
			:
			render_puppet_data(other),
			uniform_values(other.uniform_values)
		{ }

		explicit gl3_puppet_data(const render_puppet_data& other)
			: render_puppet_data(other) { }


		auto_pointer<covert_data> clone() const override
			{ return auto_pointer<covert_data>(new gl3_puppet_data(*this)); }


		gl3_uniform_value_map uniform_values;
};




/**
 *************************************************************************
 *
 * @class gl3_puppet
 * @ingroup gl3_render_module
 *
 * A puppet backend supported by gl3_render_context.
 *
 ************************************************************************/
class gl3_puppet :
	public covert_object,
	public gl3_scene_object_tag
{
	friend class gl3_render_context;

	public:

		typedef render_puppet_id covert_id_type;

		const gl3_puppet_data& puppet_data() const;

	private:

		gl3_puppet
			(const covert_object_manager& object_manager,
			 const gl3_api& gl3);

		void set_data(auto_pointer<covert_data> data) override;
		auto_pointer<covert_data> data() override;

		const covert_object_manager& object_manager_;
		const gl3_api& gl3_;

		auto_pointer<gl3_puppet_data> puppet_data_;
};




/**
 *************************************************************************
 *
 * @class gl3_stack
 * @ingroup gl3_render_module
 *
 * A rendering stack as supported by gl3_render_context.
 *
 ************************************************************************/
class gl3_stack :
	public covert_object,
	public gl3_scene_object_tag
{
	friend class gl3_render_context;

	public:

		typedef render_stack_id covert_id_type;

		const render_stack_data& stack_data() const;

	private:

		gl3_stack
			(const covert_object_manager& object_manager,
			 const gl3_api& gl3);

		void set_data(auto_pointer<covert_data> data) override;
		auto_pointer<covert_data> data() override;

		const covert_object_manager& object_manager_;
		const gl3_api& gl3_;

		render_stack_data stack_data_;
};




/**
 *************************************************************************
 *
 * @class gl3_scene
 * @ingroup gl3_render_module
 *
 * Some rendering scene used by gl3_render_context.
 *
 ************************************************************************/
class gl3_scene : public covert_object
{
	friend class gl3_render_context;

	public:

		typedef render_scene_id covert_id_type;

		const render_scene_data& scene_data() const;

	private:

		gl3_scene
			(const covert_object_manager& object_manager,
			 const gl3_api& gl3);

		void set_data(auto_pointer<covert_data> data) override;
		auto_pointer<covert_data> data() override;

		const covert_object_manager& object_manager_;
		const gl3_api& gl3_;

		render_scene_data scene_data_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_SCENE_HPP) */

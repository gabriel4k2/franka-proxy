/**
 *************************************************************************
 *
 * @file null_render.cpp
 *
 * Null rendering system, implementation.
 *
 ************************************************************************/


#include "null_render.hpp"


#include "log.hpp"
#include "render_resource.hpp"
#include "render_scene.hpp"
#include "render_process.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// null_render_context
//
//////////////////////////////////////////////////////////////////////////


null_render_context::null_render_context
	(const string& context_name)
	:
	render_context(context_name)
{
	LOG_INFO("Using null_render_context implementation.");
	register_object_creators();
}


null_render_context::~null_render_context() NOTHROW
{
	LOG_INFO("Destroying.");
	clear_on_destruction();
}


void null_render_context::acquire_api()
	{ /** No API for null_render_context. */ }
void null_render_context::release_api() NOTHROW
	{ /** No API for null_render_context. */ }


auto_pointer<covert_object> null_render_context::create_object
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


void null_render_context::render
	(const shared_pointer<render_process_id>& process)
		{ /** No rendering in null_render_context. */ }


void null_render_context::register_object_creators()
{
	object_creators_.insert
		(typeid(render_texture_id),
		 &null_render_context::create_typed_object<null_render_texture>);
	object_creators_.insert
		(typeid(render_shader_id),
		 &null_render_context::create_typed_object<null_render_shader>);
	object_creators_.insert
		(typeid(render_material_id),
		 &null_render_context::create_typed_object<null_render_material>);
	object_creators_.insert
		(typeid(render_model_id),
		 &null_render_context::create_typed_object<null_render_model>);

	object_creators_.insert
		(typeid(render_light_id),
		 &null_render_context::create_typed_object<null_render_light>);
	object_creators_.insert
		(typeid(render_puppet_id),
		 &null_render_context::create_typed_object<null_render_puppet>);
	object_creators_.insert
		(typeid(render_stack_id),
		 &null_render_context::create_typed_object<null_render_stack>);
	object_creators_.insert
		(typeid(render_scene_id),
		 &null_render_context::create_typed_object<null_render_scene>);

	object_creators_.insert
		(typeid(render_layer_id),
		 &null_render_context::create_typed_object<null_render_layer>);
	object_creators_.insert
		(typeid(render_canvas_id),
		 &null_render_context::create_typed_object<null_render_canvas>);
	object_creators_.insert
		(typeid(render_process_id),
		 &null_render_context::create_typed_object<null_render_process>);
}


template <typename T>
	auto_pointer<covert_object> null_render_context::create_typed_object()
		{ return auto_pointer<covert_object>(new T()); }


void null_render_context::log_and_throw_null_data()
{
	LOG_ERROR("Cannot apply NULL data.");
	throw null_pointer();
}


void null_render_context::log_and_throw_invalid_data_type()
{
	LOG_ERROR("Incoming data does not match target object type.");
	throw type_mismatch();
}




} /* namespace viral_core */

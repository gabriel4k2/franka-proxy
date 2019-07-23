/**
 *************************************************************************
 *
 * @file gl3_scene.cpp
 *
 * Scene-specific backend objects of OpenGL3, implementation.
 *
 ************************************************************************/


#include "gl3_scene.hpp"


#include <typeinfo>


#include "log.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// gl3_light
//
//////////////////////////////////////////////////////////////////////////


const render_light_data& gl3_light::light_data() const
	{ return light_data_; }


gl3_light::gl3_light
	(const covert_object_manager& object_manager,
	 const gl3_api& gl3)
	:
	object_manager_(object_manager), gl3_(gl3)
{ }


void gl3_light::set_data(auto_pointer<covert_data> data)
{
	if (!data)
	{
		LOG_ERROR("Cannot apply NULL data.");
		throw null_pointer();
	}

	auto_pointer<render_light_data> light_data
		(data.try_cast_exact<render_light_data>());

	if (!light_data)
	{
		LOG_ERROR("Incoming data must be a render_light_data.");
		throw type_mismatch();
	}

	light_data_ = *light_data;
}


auto_pointer<covert_data> gl3_light::data()
	{ return light_data_.clone(); }




//////////////////////////////////////////////////////////////////////////
//
// gl3_puppet
//
//////////////////////////////////////////////////////////////////////////


const gl3_puppet_data& gl3_puppet::puppet_data() const
{
	if (!puppet_data_)
	{
		LOG_ERROR("No internal puppet data assigned.\r\n"
				  "Please commit appropriate render_puppet_data.");
		throw invalid_state();
	}

	return *puppet_data_;
}


gl3_puppet::gl3_puppet
	(const covert_object_manager& object_manager,
	 const gl3_api& gl3)
	:
	object_manager_(object_manager), gl3_(gl3)
{ }


void gl3_puppet::set_data(auto_pointer<covert_data> data)
{
	if (!data)
	{
		LOG_ERROR("Cannot apply NULL data.");
		throw null_pointer();
	}

	if (typeid(*data) == typeid(gl3_puppet_data))
	{
		puppet_data_ =
			data.try_cast_exact<gl3_puppet_data>();
	}
	else if (typeid(*data) == typeid(render_puppet_data))
	{
		auto_pointer<render_puppet_data> puppet_data
			(data.try_cast_exact<render_puppet_data>());
		puppet_data_.reset
			(new gl3_puppet_data(*puppet_data));
	}
	else
	{
		LOG_ERROR("Incoming data has invalid type.");
		throw type_mismatch();
	}
}


auto_pointer<covert_data> gl3_puppet::data()
{
	return puppet_data_ ?
			puppet_data_->clone() :
			auto_pointer<covert_data>();
}




//////////////////////////////////////////////////////////////////////////
//
// gl3_stack
//
//////////////////////////////////////////////////////////////////////////


const render_stack_data& gl3_stack::stack_data() const
	{ return stack_data_; }


gl3_stack::gl3_stack
	(const covert_object_manager& object_manager,
	 const gl3_api& gl3)
	:
	object_manager_(object_manager), gl3_(gl3)
{ }


void gl3_stack::set_data(auto_pointer<covert_data> data)
{
	if (!data)
	{
		LOG_ERROR("Cannot apply NULL data.");
		throw null_pointer();
	}

	auto_pointer<render_stack_data> stack_data
		(data.try_cast_exact<render_stack_data>());

	if (!stack_data)
	{
		LOG_ERROR("Incoming data must be a render_stack_data.");
		throw type_mismatch();
	}

	stack_data_ = *stack_data;
}


auto_pointer<covert_data> gl3_stack::data()
	{ return stack_data_.clone(); }




//////////////////////////////////////////////////////////////////////////
//
// gl3_scene
//
//////////////////////////////////////////////////////////////////////////


const render_scene_data& gl3_scene::scene_data() const
	{ return scene_data_; }


gl3_scene::gl3_scene
	(const covert_object_manager& object_manager,
	 const gl3_api& gl3)
	:
	object_manager_(object_manager), gl3_(gl3)
{ }


void gl3_scene::set_data(auto_pointer<covert_data> data)
{
	if (!data)
	{
		LOG_ERROR("Cannot apply NULL data.");
		throw null_pointer();
	}

	auto_pointer<render_scene_data> scene_data
		(data.try_cast_exact<render_scene_data>());

	if (!scene_data)
	{
		LOG_ERROR("Incoming data must be a render_scene_data.");
		throw type_mismatch();
	}

	scene_data_ = *scene_data;
}


auto_pointer<covert_data> gl3_scene::data()
	{ return scene_data_.clone(); }




} /* namespace viral_core */

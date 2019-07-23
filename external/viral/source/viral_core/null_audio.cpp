/**
 *************************************************************************
 *
 * @file null_audio.cpp
 *
 * Null audio system, implementation.
 *
 ************************************************************************/


#include "null_audio.hpp"


#include "log.hpp"
#include "covert_object.hpp"
#include "audio_graph.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// null_audio_context
//
//////////////////////////////////////////////////////////////////////////


null_audio_context::null_audio_context
	(const string& context_name)
	:
	audio_context(context_name)
{
	LOG_INFO("Using null_audio_context implementation.");
	register_object_creators();
}


null_audio_context::~null_audio_context() NOTHROW
{
	LOG_INFO("Destroying.");
	clear_on_destruction();
}


auto_pointer<covert_object>
	null_audio_context::create_object
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


void null_audio_context::play
	(const audio_command_play& play_command)
{ /** The null_audio_context ignores audio_command_play. */}


void null_audio_context::stop
	(const audio_command_stop& stop_command)
{ /** The null_audio_context ignores audio_command_stop. */}


void null_audio_context::set_master
	(const audio_command_master& master_command)
{ /** The null_audio_context ignores audio_command_master. */}


void null_audio_context::render
	(const audio_command_render& render_command)
{ /** The null_audio_context ignores audio_command_render. */}


void null_audio_context::register_object_creators()
{
	object_creators_.insert
		(typeid(audio_waveform_id),
		 &null_audio_context::create_typed_object<null_audio_waveform>);
	object_creators_.insert
		(typeid(audio_effect_id),
		 &null_audio_context::create_typed_object<null_audio_effect>);
	object_creators_.insert
		(typeid(audio_source_id),
		 &null_audio_context::create_typed_object<null_audio_source>);
	object_creators_.insert
		(typeid(audio_scene_id),
		 &null_audio_context::create_typed_object<null_audio_scene>);
	object_creators_.insert
		(typeid(audio_master_id),
		 &null_audio_context::create_typed_object<null_audio_master>);
}


template <typename T>
	auto_pointer<covert_object> null_audio_context::create_typed_object()
		{ return auto_pointer<covert_object>(new T()); }


void null_audio_context::log_and_throw_null_data()
{
	LOG_ERROR("Cannot apply NULL data.");
	throw null_pointer();
}


void null_audio_context::log_and_throw_invalid_data_type()
{
	LOG_ERROR("Incoming data does not match target object type.");
	throw type_mismatch();
}




} /* namespace viral_core */

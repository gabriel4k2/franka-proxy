/**
 *************************************************************************
 *
 * @file ms_audio_graph.cpp 
 *
 * Audio graphs on Microsoft Windows, implementation.
 *
 ************************************************************************/


#include "ms_audio_graph.hpp"


#if defined(VIRAL__HAVE_MS_AUDIO)


#include "log.hpp"
#include "audio_buffer.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// ms_audio_waveform
//
//////////////////////////////////////////////////////////////////////////


ms_audio_waveform::ms_audio_waveform
	(const covert_object_manager& object_manager)
	:
	object_manager_(object_manager)
{ }


void ms_audio_waveform::set_data(auto_pointer<covert_data> data)
{
	// Validate incoming data.
	if (!data)
	{
		LOG_ERROR("Cannot apply NULL data.");
		throw null_pointer();
	}

	auto_pointer<audio_waveform_data> waveform_data
		(data.try_cast_exact<audio_waveform_data>());

	if (!waveform_data)
	{
		LOG_ERROR("Incoming data must be audio_waveform_data.");
		throw type_mismatch();
	}

	if (!waveform_data->wave)
	{
		LOG_ERROR("Cannot apply data with NULL wave.");
		throw null_pointer();
	}

	// Exception safety: NOTHROW from here.
	waveform_data_ = std::move(waveform_data);
}


auto_pointer<covert_data> ms_audio_waveform::data()
{
	return waveform_data_ ?
				waveform_data_->clone() :
				auto_pointer<audio_waveform_data>();
}




//////////////////////////////////////////////////////////////////////////
//
// ms_audio_effect
//
//////////////////////////////////////////////////////////////////////////


ms_audio_effect::ms_audio_effect
	(const covert_object_manager& object_manager)
	:
	object_manager_(object_manager)
{ }


void ms_audio_effect::set_data(auto_pointer<covert_data> data)
{
	if (!data)
	{
		LOG_ERROR("Cannot apply NULL data.");
		throw null_pointer();
	}

	auto_pointer<audio_effect_data> effect_data
		(data.try_cast_exact<audio_effect_data>());

	if (!effect_data)
	{
		LOG_ERROR("Incoming data must be audio_effect_data.");
		throw type_mismatch();
	}

	effect_data_ = std::move(effect_data);
}


auto_pointer<covert_data> ms_audio_effect::data()
{
	return effect_data_ ?
				effect_data_->clone() :
				auto_pointer<audio_waveform_data>();
}




//////////////////////////////////////////////////////////////////////////
//
// ms_audio_source
//
//////////////////////////////////////////////////////////////////////////


void ms_audio_source::play() NOTHROW
	{ playing_ = true; }
void ms_audio_source::stop() NOTHROW
	{ playing_ = false; }


void ms_audio_source::render
	(audio_buffer& target_buffer)
{ }


ms_audio_source::ms_audio_source
	(const covert_object_manager& object_manager)
	:
	object_manager_(object_manager),
	playing_(false)
{ }


void ms_audio_source::set_data(auto_pointer<covert_data> data)
{
	if (!data)
	{
		LOG_ERROR("Cannot apply NULL data.");
		throw null_pointer();
	}

	auto_pointer<audio_source_data> source_data
		(data.try_cast_exact<audio_source_data>());

	if (!source_data)
	{
		LOG_ERROR("Incoming data must be audio_source_data.");
		throw type_mismatch();
	}

	source_data_ = *source_data;
}


auto_pointer<covert_data> ms_audio_source::data()
	{ return source_data_.clone(); }




//////////////////////////////////////////////////////////////////////////
//
// ms_audio_scene
//
//////////////////////////////////////////////////////////////////////////


void ms_audio_scene::render
	(audio_buffer& target_buffer)
{ }


ms_audio_scene::ms_audio_scene
	(const covert_object_manager& object_manager)
	:
	object_manager_(object_manager)
{ }


void ms_audio_scene::set_data(auto_pointer<covert_data> data)
{
	if (!data)
	{
		LOG_ERROR("Cannot apply NULL data.");
		throw null_pointer();
	}

	auto_pointer<audio_scene_data> scene_data
		(data.try_cast_exact<audio_scene_data>());

	if (!scene_data)
	{
		LOG_ERROR("Incoming data must be audio_scene_data.");
		throw type_mismatch();
	}

	scene_data_ = *scene_data;
}


auto_pointer<covert_data> ms_audio_scene::data()
	{ return scene_data_.clone(); }




//////////////////////////////////////////////////////////////////////////
//
// ms_audio_master
//
//////////////////////////////////////////////////////////////////////////


void ms_audio_master::render
	(audio_buffer& target_buffer)
{
	// Zero out contents of target buffer for additive fetching.
	memory::fill_with_zero_typed
		(target_buffer.data(), target_buffer.sample_count());
}


ms_audio_master::ms_audio_master
	(const covert_object_manager& object_manager)
	:
	object_manager_(object_manager)
{ }


void ms_audio_master::set_data(auto_pointer<covert_data> data)
{
	if (!data)
	{
		LOG_ERROR("Cannot apply NULL data.");
		throw null_pointer();
	}

	auto_pointer<audio_master_data> master_data
		(data.try_cast_exact<audio_master_data>());

	if (!master_data)
	{
		LOG_ERROR("Incoming data must be audio_master_data.");
		throw type_mismatch();
	}

	master_data_ = *master_data;
}


auto_pointer<covert_data> ms_audio_master::data()
	{ return master_data_.clone(); }




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_MS_AUDIO) */

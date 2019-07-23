/**
 *************************************************************************
 *
 * @file audio_loader.cpp
 *
 * Loader for audio-specific covert_data, implementation.
 *
 ************************************************************************/


#include "audio_loader.hpp"


#include "log.hpp"
#include "list.hpp"
#include "xml.hpp"
#include "xml_util.hpp"
#include "waveform_any.hpp"
#include "audio_graph.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// audio_loader
//
//////////////////////////////////////////////////////////////////////////


audio_loader::audio_loader
	(const string& loader_name, file_context& files,
	 covert_data_cache& data_cache, covert_id_cache& id_cache)
	:
	covert_loader
		(loader_name, files,
		 data_cache, id_cache)
{
	register_handler("waveform", static_cast<data_handler>
						(&audio_loader::load_data<audio_waveform_data>));
	register_handler("effect", static_cast<data_handler>
						(&audio_loader::load_data<audio_effect_data>));
	register_handler("source", static_cast<data_handler>
						(&audio_loader::load_data<audio_source_data>));
	register_handler("scene", static_cast<data_handler>
						(&audio_loader::load_data<audio_scene_data>));
	register_handler("texture_2d", static_cast<data_handler>
						(&audio_loader::load_data<audio_master_data>));
}


auto_pointer<waveform_any> audio_loader::load_wave(const string& path)
	{ return waveform_any::load(path, files); }


void audio_loader::to_data_effects_helper
	(const xml_node& node,
	 list<shared_pointer<audio_effect_id>>& effects) const
{
	if (node.has_child("effects"))
	{
		list<shared_pointer<audio_effect_id>> local_effects;

		for (xml_node::const_child_iterator it
				(node.child("effects").first_child()); it; ++it)
		{
			if (it->name() != "effect")
			{
				LOG_ERROR("Effect nodes must be named effect.");
				throw xml_exception();
			}

			local_effects.insert
				(id_cache.entry<audio_effect_id>
					(it->attribute("ref")));
		}

		swap(local_effects, effects);
	}
}


void audio_loader::to_data
	(const xml_node& node, audio_waveform_data& waveform)
{
	if (node.has_child("wave"))
		waveform.wave =
			load_wave(node.child("wave").attribute("file"));
}


void audio_loader::to_data
	(const xml_node& node, audio_effect_data& effect)
{ }


void audio_loader::to_data
	(const xml_node& node, audio_source_data& source)
{
	if (node.has_child("waveform"))
		source.waveform =
			id_cache.entry<audio_waveform_id>
				(node.child("waveform").attribute("ref"));	
	if (node.has_child("scene"))
		source.scene =
			id_cache.entry<audio_scene_id>
				(node.child("scene").attribute("ref"));	

	if (node.has_child("emitter"))
	{
		const string& emitter = node.child("emitter").attribute("value");

		if (emitter == "emitter_point")
			source.emitter = audio_source_data::emitter_point;
		else if (emitter == "emitter_cone")
			source.emitter = audio_source_data::emitter_cone;
		else if (emitter == "emitter_global")
			source.emitter = audio_source_data::emitter_global;
		else
		{
			LOG_ERROR("Unknown emitter type in source: " + emitter);
			throw xml_exception();
		}
	}

	if (node.has_child("volume_pre_effects"))
		source.volume_pre_effects =
			xml_converter::to_float(node.child("volume_pre_effects"));
	if (node.has_child("volume_post_effects"))
		source.volume_post_effects =
			xml_converter::to_float(node.child("volume_post_effects"));

	if (node.has_child("panning_post_effects"))
		source.panning_post_effects =
			xml_converter::to_float(node.child("panning_post_effects"));
	
	if (node.has_child("priority"))
		source.priority =
			xml_converter::to_float(node.child("priority"));	

	if (node.has_child("playing"))
		source.playing =
			xml_converter::to_bool(node.child("playing"));
	if (node.has_child("commit_playing"))
		source.commit_playing =
			xml_converter::to_bool(node.child("commit_playing"));

	if (node.has_child("current_frame"))
		source.current_frame =
			xml_converter::to_int64(node.child("current_frame"));
	if (node.has_child("commit_current_frame"))
		source.commit_current_frame =
			xml_converter::to_bool(node.child("commit_current_frame"));

	if (node.has_child("playback_keeps_alive"))
		source.playback_keeps_alive =
			xml_converter::to_bool(node.child("playback_keeps_alive"));

	if (node.has_child("repeat_count"))
		source.repeat_count =
			xml_converter::to_int64(node.child("repeat_count"));
	if (node.has_child("repeat_endless"))
		source.repeat_endless =
			xml_converter::to_bool(node.child("repeat_endless"));

	if (node.has_child("loop_frame_start"))
		source.loop_frame_start =
			xml_converter::to_int64(node.child("loop_frame_start"));
	if (node.has_child("loop_frame_end"))
		source.loop_frame_end =
			xml_converter::to_int64(node.child("loop_frame_end"));
	if (node.has_child("loop_from_waveform"))
		source.loop_from_waveform =
			xml_converter::to_bool(node.child("loop_from_waveform"));

	if (node.has_child("tail_seconds"))
		source.tail_seconds =
			xml_converter::to_float(node.child("tail_seconds"));

	if (node.has_child("position"))
		source.position =
			xml_converter::to_vector(node.child("position"));
	if (node.has_child("orientation"))
		source.orientation =
			xml_converter::to_rotation(node.child("orientation"));
	if (node.has_child("stereo_distance"))
		source.stereo_distance =
			xml_converter::to_float(node.child("stereo_distance"));

	if (node.has_child("distance_maximum_volume"))
		source.distance_maximum_volume =
			xml_converter::to_float(node.child("distance_maximum_volume"));
	if (node.has_child("attenuation"))
		source.attenuation =
			xml_converter::to_float(node.child("attenuation"));

	if (node.has_child("spot_angle"))
		source.distance_maximum_volume =
			xml_converter::to_float(node.child("spot_angle"));
	if (node.has_child("spot_exponent"))
		source.attenuation =
			xml_converter::to_float(node.child("spot_exponent"));

	to_data_effects_helper(node, source.effects);
}


void audio_loader::to_data
	(const xml_node& node, audio_scene_data& scene)
{
	if (node.has_child("mode"))
	{
		const string& mode = node.child("mode").attribute("value");

		if (mode == "scene_mode_flat")
			scene.mode = audio_scene_data::scene_mode_flat;
		else if (mode == "scene_mode_3d")
			scene.mode = audio_scene_data::scene_mode_3d;
		else
		{
			LOG_ERROR("Unknown emitter type in source: " + mode);
			throw xml_exception();
		}
	}

	if (node.has_child("volume_pre_effects"))
		scene.volume_pre_effects =
			xml_converter::to_float(node.child("volume_pre_effects"));
	if (node.has_child("volume_post_effects"))
		scene.volume_post_effects =
			xml_converter::to_float(node.child("volume_post_effects"));

	if (node.has_child("panning_post_effects"))
		scene.panning_post_effects =
			xml_converter::to_float(node.child("panning_post_effects"));

	if (node.has_child("listener_position"))
		scene.listener_position =
			xml_converter::to_vector(node.child("listener_position"));
	if (node.has_child("listener_orientation"))
		scene.listener_orientation =
			xml_converter::to_rotation(node.child("listener_orientation"));
	if (node.has_child("listener_ear_distance"))
		scene.listener_ear_distance =
			xml_converter::to_float(node.child("listener_ear_distance"));

	to_data_effects_helper(node, scene.effects);
}


void audio_loader::to_data
	(const xml_node& node, audio_master_data& master)
{
	if (node.has_child("scenes"))
	{
		audio_master_data::scene_set scenes;

		for (xml_node::const_child_iterator it
				(node.child("scenes").first_child()); it; ++it)
		{
			if (it->name() != "scene")
			{
				LOG_ERROR("Scene nodes must be named scene.");
				throw xml_exception();
			}

			scenes.insert
				(id_cache.entry<audio_scene_id>
					(it->attribute("ref")));
		}

		swap(scenes, master.scenes);
	}

	if (node.has_child("volume_pre_effects"))
		master.volume_pre_effects =
			xml_converter::to_float(node.child("volume_pre_effects"));
	if (node.has_child("volume_post_effects"))
		master.volume_post_effects =
			xml_converter::to_float(node.child("volume_post_effects"));

	to_data_effects_helper(node, master.effects);
}


template <typename T>
	void audio_loader::load_data
		(const xml_node& node, const string& name)
{
	auto_pointer<T> data(new T());
	to_data(node, *data);
	data_cache.insert
		(name, std::move(data));

	request_covert_id
		<typename T::covert_id_type>(name);
}	




} /* namespace viral_core */

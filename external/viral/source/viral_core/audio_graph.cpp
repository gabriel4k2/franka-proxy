/**
 *************************************************************************
 *
 * @file audio_graph.cpp
 *
 * Audio sources and scenes, implementation.
 *
 ************************************************************************/


#include "audio_graph.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// audio_waveform_data
//
//////////////////////////////////////////////////////////////////////////


audio_waveform_data::audio_waveform_data() { }


audio_waveform_data::audio_waveform_data
	(auto_pointer<waveform_any> wave)
	:
	wave(std::move(wave))
{ }


audio_waveform_data::audio_waveform_data
	(const audio_waveform_data& other)
	:
	wave(other.wave ?
			other.wave->clone() :
			auto_pointer<waveform_any>())
{ }


audio_waveform_data::audio_waveform_data
	(audio_waveform_data&& other)
	:
	wave(std::move(other.wave))
{ }


auto_pointer<covert_data> audio_waveform_data::clone() const
	{ return auto_pointer<covert_data>(new audio_waveform_data(*this)); }




//////////////////////////////////////////////////////////////////////////
//
// audio_effect_data
//
//////////////////////////////////////////////////////////////////////////


audio_effect_data::audio_effect_data() { }


audio_effect_data::audio_effect_data
	(auto_pointer<audio_effect> effect)
	:
	effect(std::move(effect))
{ }


audio_effect_data::audio_effect_data
	(const audio_effect_data& other)
	:
	effect(other.effect ?
				other.effect->clone() :
				auto_pointer<audio_effect>())
{ }


audio_effect_data::audio_effect_data
	(audio_effect_data&& other)
	:
	effect(std::move(other.effect))
{ }


auto_pointer<covert_data> audio_effect_data::clone() const
	{ return auto_pointer<covert_data>(new audio_effect_data(*this)); }




//////////////////////////////////////////////////////////////////////////
//
// audio_source_data
//
//////////////////////////////////////////////////////////////////////////


audio_source_data::audio_source_data()
	:
	emitter(emitter_point),

	volume_pre_effects(1.f),
	volume_post_effects(1.f),
	panning_post_effects(0.f),

	priority(1.f),

	playing(false),
	commit_playing(false),
	current_frame(0),
	commit_current_frame(false),
	playback_keeps_alive(true),

	repeat_count(0),
	repeat_endless(false),
	loop_frame_start(0),
	loop_frame_end(0),
	loop_from_waveform(true),
	tail_seconds(0.1f),

	stereo_distance(0.1f),
	distance_maximum_volume(1.f),
	attenuation(0.1f),
	spot_angle(45.f),
	spot_exponent(1.f)
{ }


audio_source_data::audio_source_data
	(const shared_pointer<audio_waveform_id>& waveform,
	 const shared_pointer<audio_scene_id>& scene,
	 emitter_type emitter,
	 float volume_pre_effects, float volume_post_effects,
	 float panning_post_effects,
	 float priority,
	 bool playing, bool commit_playing,
	 int64 current_frame, bool commit_current_frame,
	 bool playback_keeps_alive,
	 int64 repeat_count, bool repeat_endless,
	 int64 loop_frame_start, int64 loop_frame_end,
	 bool loop_from_waveform,
	 float tail_seconds,
	 const vector& position, const rotation& orientation,
	 float stereo_distance,
	 float distance_maximum_volume, float attenuation,
	 float spot_angle, float spot_exponent,
	 const effect_list& effects)
	:
	waveform(waveform),
	scene(scene),
	emitter(emitter),

	volume_pre_effects(volume_pre_effects),
	volume_post_effects(volume_post_effects),
	panning_post_effects(panning_post_effects),

	priority(priority),

	playing(playing),
	commit_playing(commit_playing),
	current_frame(current_frame),
	commit_current_frame(commit_current_frame),
	playback_keeps_alive(playback_keeps_alive),

	repeat_count(repeat_count),
	repeat_endless(repeat_endless),
	loop_frame_start(loop_frame_start),
	loop_frame_end(loop_frame_end),
	loop_from_waveform(loop_from_waveform),
	tail_seconds(tail_seconds),

	position(position),
	orientation(orientation),
	stereo_distance(stereo_distance),
	distance_maximum_volume(distance_maximum_volume),
	attenuation(attenuation),
	spot_angle(spot_angle),
	spot_exponent(spot_exponent),

	effects(effects)
{ }


auto_pointer<covert_data> audio_source_data::clone() const
	{ return auto_pointer<covert_data>(new audio_source_data(*this)); }




//////////////////////////////////////////////////////////////////////////
//
// audio_scene_data
//
//////////////////////////////////////////////////////////////////////////


audio_scene_data::audio_scene_data()
	:
	mode(scene_mode_3d),
	volume_pre_effects(1.f),
	volume_post_effects(1.f),
	panning_post_effects(0.f),
	listener_ear_distance(0.2f)
{ }


audio_scene_data::audio_scene_data
	(scene_mode mode,
	 float volume_pre_effects,
	 float volume_post_effects,
	 float panning_post_effects,
	 const vector& listener_position,
	 const rotation& listener_orientation,
	 float listener_ear_distance,
	 const effect_list& effects)
	:
	mode(mode),
	volume_pre_effects(volume_pre_effects),
	volume_post_effects(volume_post_effects),
	panning_post_effects(panning_post_effects),
	listener_position(listener_position),
	listener_orientation(listener_orientation),
	listener_ear_distance(listener_ear_distance),
	effects(effects)
{ }


auto_pointer<covert_data> audio_scene_data::clone() const
	{ return auto_pointer<covert_data>(new audio_scene_data(*this)); }




//////////////////////////////////////////////////////////////////////////
//
// audio_master_data
//
//////////////////////////////////////////////////////////////////////////


audio_master_data::audio_master_data()
	:
	volume_pre_effects(1.f),
	volume_post_effects(1.f)
{ }


audio_master_data::audio_master_data
	(const scene_set& scenes,
	 float volume_pre_effects,
	 float volume_post_effects,
	 const effect_list& effects)
	:
	scenes(scenes),
	volume_pre_effects(volume_pre_effects),
	volume_post_effects(volume_post_effects),
	effects(effects)
{ }


auto_pointer<covert_data> audio_master_data::clone() const
	{ return auto_pointer<covert_data>(new audio_master_data(*this)); }




} /* namespace viral_core */

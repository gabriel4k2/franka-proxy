/**
 *************************************************************************
 *
 * @file audio_forward.hpp
 *
 * Forward declarations for the @ref audio_module "Audio Module".
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__AUDIO_FORWARD_HPP)
#define INCLUDED__VIRAL_CORE__AUDIO_FORWARD_HPP


#include "build_config.hpp"


namespace viral_core
{


// Software audio utilities.
class audio_channels;
class audio_rate;

class audio_channels_resampler;
class audio_rate_resampler;

class audio_buffer;
class audio_effect;


// Clientside waveform data.
class waveform_raw;
class waveform_ogg;

class waveform_any;

template <typename T> class waveform_any_typed;
typedef waveform_any_typed<waveform_raw> waveform_any_raw;
typedef waveform_any_typed<waveform_ogg> waveform_any_ogg;


// Generic audio interface classes.
class audio_context;
class audio_task;
class audio_loader;

class audio_waveform_id;
class audio_waveform_data;
class audio_effect_id;
class audio_effect_data;
class audio_source_id;
class audio_source_data;
class audio_scene_id;
class audio_scene_data;
class audio_master_id;
class audio_master_data;

class audio_command_play;
class audio_command_stop;
class audio_command_master;
class audio_command_render;
class audio_command_queue;


// Null audio implementation.
class null_audio_context;
template <typename T> class null_audio_object;

typedef null_audio_object<audio_waveform_data> null_audio_waveform;
typedef null_audio_object<audio_effect_data> null_audio_effect;
typedef null_audio_object<audio_source_data> null_audio_source;
typedef null_audio_object<audio_scene_data> null_audio_scene;
typedef null_audio_object<audio_master_data> null_audio_master;


// Microsoft audio implementation.
class ms_audio_context;
class ms_audio_render_format;
class ms_audio_buffer_queue;
class ms_audio_sink;
class ms_audio_sink_callbacks;

class ms_audio_waveform;
class ms_audio_effect;
class ms_audio_source;
class ms_audio_scene;
class ms_audio_master;




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__AUDIO_FORWARD_HPP) */

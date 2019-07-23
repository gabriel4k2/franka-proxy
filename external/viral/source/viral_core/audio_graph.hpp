/**
 *************************************************************************
 *
 * @file audio_graph.hpp
 *
 * Audio sources and scenes.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__AUDIO_GRAPH_HPP)
#define INCLUDED__VIRAL_CORE__AUDIO_GRAPH_HPP


#include "build_config.hpp"


#include "auto_pointer.hpp"
#include "list.hpp"
#include "tree_set.hpp"
#include "string.hpp"
#include "geo_transform.hpp"
#include "covert_data.hpp"
#include "waveform_any.hpp"
#include "audio_effect.hpp"


namespace viral_core
{


/**
 * IDs for specific audio-internal objects.
 * @ingroup audio_module
 */
//@{
class audio_waveform_id : public covert_id
{
	public:
		audio_waveform_id() { }
		audio_waveform_id(const string& debug_name)
			: covert_id(debug_name) { }
};

class audio_effect_id : public covert_id
{
	public:
		audio_effect_id() { }
		audio_effect_id(const string& debug_name)
			: covert_id(debug_name) { }
};

class audio_source_id : public covert_id
{
	public:
		audio_source_id() { }
		audio_source_id(const string& debug_name)
			: covert_id(debug_name) { }
};

class audio_scene_id : public covert_id
{
	public:
		audio_scene_id() { }
		audio_scene_id(const string& debug_name)
			: covert_id(debug_name) { }
};

class audio_master_id : public covert_id
{
	public:
		audio_master_id() { }
		audio_master_id(const string& debug_name)
			: covert_id(debug_name) { }
};
//@}




/**
 *************************************************************************
 *
 * @class audio_waveform_data
 * @ingroup audio_module
 *
 * Wraps a @ref waveform for use through an audio_waveform_id.
 *
 ************************************************************************/
class audio_waveform_data :
	public covert_data
{
	public:

		typedef audio_waveform_id covert_id_type;

		audio_waveform_data();
		audio_waveform_data(auto_pointer<waveform_any> wave);

		audio_waveform_data(const audio_waveform_data& other);
		audio_waveform_data(audio_waveform_data&& other);

		auto_pointer<covert_data> clone() const override;
		
		auto_pointer<waveform_any> wave;
};




/**
 *************************************************************************
 *
 * @class audio_effect_data
 * @ingroup audio_module
 *
 * Attaches an audio_effect to an audio_effect_id.
 *
 * Implementation notes:
 * - Back-end implementations may replace generic software audio_effect%s
 *		with suitable API-specific variants (e.g. for performance reasons).
 *
 ************************************************************************/
class audio_effect_data :
	public covert_data
{
	public:

		typedef audio_effect_id covert_id_type;

		audio_effect_data();
		audio_effect_data(auto_pointer<audio_effect> effect);

		audio_effect_data(const audio_effect_data& other);
		audio_effect_data(audio_effect_data&& other);

		auto_pointer<covert_data> clone() const override;

		auto_pointer<audio_effect> effect;
};




/**
 *************************************************************************
 *
 * @class audio_source_data
 * @ingroup audio_module
 *
 * Plays an audio_waveform_id in an audio @bref{scene}.
 *
 * Playback notes:
 * - Clients can control playback of an audio source both by commands
 *		(e.g. audio_command_play) and by committing audio_source_data
 *		(e.g. @true @bref{playing} with @true @bref{commit_playing}).
 *		However, commands are more light-weight and thus preferrable.
 * - Each successful audio_command_render advances all audio sources
 *		that play to an audio_scene_id of the current audio_master_id.
 *		All other audio sources do not advance @bref{current_frame}.
 * - Back-ends do not interpolate parameters of their audio sources.
 *		Thus, rapid changes (e.g. in @bref{wave} or @bref{position})
 *		over consecutive audio rendering may cause audible glitches.
 *
 * Timing notes:
 * - Audio rendering need not be exactly synchronized to real-time.
 *		Audio sources particularly may start, stop, play, or repeat
 *		with implementation tolerances (e.g. too slow or too fast).
 * - To avoid glitches (e.g. accumulating delayed one-shot sources),
 *		back-ends still should remain mostly in synch, for instance
 *		by consuming dummy audio even when no device is connected.
 * - Clients should not rely on the back-end @bref{playing} state
 *		(i.e. audio_command_queue::query() for audio_source_data),
 *		but instead should synch with waveform::length_seconds(),
 *		possibly with a small, additional tolerance.
 *
 * Audio source lifetime:
 * - Clients should regularly release unneeded audio_source_id%s
 *		to reduce processing overhead on audio_command_render,
 *		notably for out-of-range or stopped audio sources.
 * - Back-ends keep audio sources with @false @bref{repeat_endless}
 *		and @true @bref{playback_keeps_alive} alive while playing,
 *		even if no respective client-side audio_source_id%s remain.
 *		This relieves clients from explicitly keeping sources alive
 *		during playback (i.e. to avoid cutting off one-shot sounds).
 *
 ************************************************************************/
class audio_source_data :
	public covert_data
{

	public:

		typedef audio_source_id covert_id_type;

		typedef list<shared_pointer<audio_effect_id>> effect_list;


		enum emitter_type
		{
			/** Omni-directional audio. */
			emitter_point,
				
			/** Spot cone with falloff. */
			emitter_cone,

			/** Audio without distance falloff. */
			emitter_global
		};


		audio_source_data();
		audio_source_data
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
			 const effect_list& effects);


		auto_pointer<covert_data> clone() const override;


		shared_pointer<audio_waveform_id> waveform;
		shared_pointer<audio_scene_id> scene;
		emitter_type emitter;

	
		/**
		 * Linear volume factors.
		 * Applied before and after effects, respectively.
		 */
		//@{
		float volume_pre_effects;
		float volume_post_effects;
		//@}


		/**
		 * Panning of this audio source in the audio scene.
		 * Back-ends implicitly clamp pan to [-1, 1] range.
		 */
		float panning_post_effects;


		/**
		 * Priority factor for the audio source.
		 * Back-ends use priority (e.g. alongside volume and distance)
		 * to decide which sources to play if source slots are limited.
		 */
		float priority;


		/**
		 * Playback state of the audio source.
		 *
		 * Usage notes:
		 * - Negative or past-end @bref{current_frame}s are allowed,
		 *		but later playback skips to the nearest valid frame.
		 * - Commits ignore @bref{playing} and @bref{current_frame},
		 *		unless respective commit flags are also set to @true.
		 *		This allows clients to commit audio source parameters
		 *		(e.g. @bref{position}) without interrupting playback
		 *		(e.g. at an unknown back-end @bref{current_frame}).
		 * - Queries for @bref{playing} state and @bref{current_frame}
		 *		are unreliable (e.g. due to concurrent audio rendering),
		 *		and should not be used for client synchronization.
		 * - Queries return arbitrary values for @bref{commit_playing}
		 *		and @bref{commit_current_frame}.
		 */
		//@{
		bool playing;
		bool commit_playing;

		int64 current_frame;
		bool commit_current_frame;
		//@}


		/**
		 * If set, implementations keep this audio source alive
		 * until playback, including repetitions, has finished.
		 * Ignored if @bref{repeat_endless} is set.
		 */
		bool playback_keeps_alive;


		/**
		 * Audio repeating and looping options.
		 *
		 * Usage notes:
		 * - A @bref{repeat_count} of zero specifies no looping.
		 *		The entire source plays, loop points are ignored.
		 * - After @bref{repeat_count} repetitions of the loop,
		 *		the audio source plays past the loop end point
		 *		and continues to the end of the waveform.
		 * - A @true @bref{loop_from_waveform} uses loop points
		 *		from the @ref waveform instead of explicit ones.
		 */
		//@{
		int64 repeat_count;
		bool repeat_endless;

		int64 loop_frame_start;
		int64 loop_frame_end;
		bool loop_from_waveform;
		//@}


		/**
		 * Silence tail to avoid audio cuts in @bref{effects}.
		 *
		 * The audio source delays stopping (e.g. due to audio_command_stop
		 * or due to finishing its @bref{wave}form) for @bref{tail_seconds}
		 * while feeding silence into its @bref{effects}.
		 */
		float tail_seconds;
		

		/**
		 * Positioning of the audio source in a 3D scene.
		 * Only relevant in @bref{scene_mode_3d}.
		 */
		//@{
		vector position;
		rotation orientation;
		float stereo_distance;
		//@}


		/**
		 * Distance for maximum volume and volume falloff.
		 * Only relevant in @bref{scene_mode_3d}.
		 */
		//@{
		float distance_maximum_volume;
		float attenuation;
		//@}


		/**
		 * Parameters for @bref{emitter_spot}.
		 * The @bref{spot_angle} is given in degrees.
		 * Only relevant in @bref{scene_mode_3d}.
		 */
		//@{
		float spot_angle;
		float spot_exponent;
		//@}


		effect_list effects;
};




/**
 *************************************************************************
 *
 * @class audio_scene_data
 * @ingroup audio_module
 *
 * A target for multiple audio sources.
 *
 * Gain considerations:
 * - See audio_master_data for gain considerations.
 *
 * Implementation notes:
 * - Audio scenes do not keep track of contained audio_source_id%s.
 *		This relieves clients from explicitly removing audio sources 
 *		from their audio scenes (e.g. once playback has finished).
 *
 ************************************************************************/
class audio_scene_data :
	public covert_data
{

	public:

		typedef audio_scene_id covert_id_type;

		typedef list<shared_pointer<audio_effect_id>> effect_list;


		enum scene_mode
		{
			/**
			 * The scene acts as a stereo mixer.
			 * Only 2D parameters influence audio output
			 * (e.g. source panning and volume factors).
			 */
			scene_mode_flat,


			/**
			 * The scene acts as a 3D panorama.
			 * Both 2D and 3D parameters have effects
			 * (e.g. source and listener positions).
			 */
			scene_mode_3d
		};


		audio_scene_data();
		audio_scene_data
			(scene_mode mode,
			 float volume_pre_effects,
			 float volume_post_effects,
			 float panning_post_effects,
			 const vector& listener_position,
			 const rotation& listener_orientation,
			 float listener_ear_distance,
			 const effect_list& effects);


		auto_pointer<covert_data> clone() const override;


		scene_mode mode;


		/**
		 * Linear volume factors.
		 * Applied before and after effects, respectively.
		 */
		//@{
		float volume_pre_effects;
		float volume_post_effects;
		//@}


		/**
		 * Panning of this audio scene in the master bus.
		 * Back-ends implicitly clamp pan to [-1, 1] range.
		 */
		float panning_post_effects;


		/**
		 * Parameters of the virtual listener.
		 * Only relevant in @bref{scene_mode_3d}.
		 */
		//@{
		vector listener_position;
		rotation listener_orientation;
		float listener_ear_distance;
		//@}


		effect_list effects;
};




/**
 *************************************************************************
 *
 * @class audio_master_data
 * @ingroup audio_module
 *
 * A master bus collects audio_scene_id%s for simultaneous playback.
 *
 * Usage notes:
 * - Only one audio master bus can be active at a time.
 *
 * Gain considerations:
 * - To avoid distortions, clients must ensure (e.g. with @bref{effects})
 *		that the output of the audio master remains in a [-1, 1] range.
 * - Clients can support the final effects by also keeping audio scenes
 *		and audio sources mostly normalized (e.g. again with effects).
 *
 ************************************************************************/
class audio_master_data :
	public covert_data
{

	public:

		typedef audio_master_id covert_id_type;

		typedef tree_set<shared_pointer<audio_scene_id>> scene_set;
		typedef list<shared_pointer<audio_effect_id>> effect_list;


		audio_master_data();
		audio_master_data
			(const scene_set& scenes,
			 float volume_pre_effects,
			 float volume_post_effects,
			 const effect_list& effects);


		auto_pointer<covert_data> clone() const override;


		scene_set scenes;


		/**
		 * Linear volume factors.
		 * Applied before and after effects, respectively.
		 */
		//@{
		float volume_pre_effects;
		float volume_post_effects;
		//@}


		effect_list effects;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__AUDIO_GRAPH_HPP) */

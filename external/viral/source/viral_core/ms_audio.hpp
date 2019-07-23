/**
 *************************************************************************
 *
 * @file ms_audio.hpp
 *
 * Audio on Microsoft Windows.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MS_AUDIO_HPP)
#define INCLUDED__VIRAL_CORE__MS_AUDIO_HPP


#include "build_config.hpp"


#if defined(VIRAL__HAVE_MS_AUDIO)


#include "audio_forward.hpp"


#include "auto_pointer.hpp"
#include "shared_pointer.hpp"
#include "audio.hpp"
#include "audio_sampling.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class ms_audio_context
 * @ingroup audio_module
 *
 * An audio_context implementation with WASAPI back-end
 * for use on Microsoft Windows.
 *
 ************************************************************************/
class ms_audio_context :
	public audio_context
{
	friend class ms_audio_callback;


	public:

		ms_audio_context(const string& context_name);
		~ms_audio_context() NOTHROW override;


	private:

		auto_pointer<covert_object> create_object
			(const shared_pointer<covert_id>& id) override;

		void play
			(const audio_command_play& play_command) override;
		void stop
			(const audio_command_stop& stop_command) override;

		void set_master
			(const audio_command_master& master_command) override; 

		void render
			(const audio_command_render& render_command) override;

		void register_object_creators();
		template <typename T>
			auto_pointer<covert_object> create_typed_object();


		static const audio_channels_type default_render_channels_;
		static const audio_rate_type default_render_rate_;

		static const float render_time_tolerance_factor_;
		static const int64 target_buffer_count_;
		static const int64 max_frame_count_per_buffer_;

		typedef auto_pointer<covert_object>
			(ms_audio_context::*object_creator_function)();

		typedef tree_map<type_info_ref, object_creator_function> object_creator_map;
		object_creator_map object_creators_;

		weak_pointer<audio_master_id> master_id_;

		auto_pointer<ms_audio_render_format> render_format_;
		auto_pointer<ms_audio_buffer_queue> buffer_queue_;
		auto_pointer<ms_audio_sink> sink_;
};




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_MS_AUDIO) */


#endif /* !defined(INCLUDED__VIRAL_CORE__MS_AUDIO_HPP) */

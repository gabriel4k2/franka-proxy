/**
 *************************************************************************
 *
 * @file ms_audio_graph.hpp 
 *
 * Audio graphs on Microsoft Windows.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MS_AUDIO_GRAPH_HPP)
#define INCLUDED__VIRAL_CORE__MS_AUDIO_GRAPH_HPP


#include "build_config.hpp"


#if defined(VIRAL__HAVE_MS_AUDIO)


#include "audio_forward.hpp"


#include "covert_object.hpp"
#include "audio_graph.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class ms_audio_waveform
 * @ingroup audio_module
 *
 * Stores an audio waveform on Microsoft Windows. 
 *
 ************************************************************************/
class ms_audio_waveform :
	public covert_object
{
	friend class ms_audio_context;

	public:

		typedef audio_waveform_id covert_id_type;
		
	private:

		ms_audio_waveform
			(const covert_object_manager& object_manager);

		void set_data(auto_pointer<covert_data> data) override;
		auto_pointer<covert_data> data() override;

		const covert_object_manager& object_manager_;

		auto_pointer<audio_waveform_data> waveform_data_;
};




/**
 *************************************************************************
 *
 * @class ms_audio_effect
 * @ingroup audio_module
 *
 * Implements an audio effect on Microsoft Windows. 
 *
 ************************************************************************/
class ms_audio_effect :
	public covert_object
{
	friend class ms_audio_context;

	public:

		typedef audio_effect_id covert_id_type;
		
	private:

		ms_audio_effect
			(const covert_object_manager& object_manager);

		void set_data(auto_pointer<covert_data> data) override;
		auto_pointer<covert_data> data() override;

		const covert_object_manager& object_manager_;

		auto_pointer<audio_effect_data> effect_data_;
};




/**
 *************************************************************************
 *
 * @class ms_audio_source
 * @ingroup audio_module
 *
 * Represents an audio source on Microsoft Windows. 
 *
 ************************************************************************/
class ms_audio_source :
	public covert_object
{
	friend class ms_audio_context;


	public:

		typedef audio_source_id covert_id_type;


		void play() NOTHROW;
		void stop() NOTHROW;


		/**
		 * Render the ms_audio_waveform for this audio source.
		 * Adds rendered audio onto the @bref{target_buffer}.
		 */
		void render(audio_buffer& target_buffer);

		
	private:

		ms_audio_source
			(const covert_object_manager& object_manager);

		void set_data(auto_pointer<covert_data> data) override;
		auto_pointer<covert_data> data() override;

		const covert_object_manager& object_manager_;

		audio_source_data source_data_;
		bool playing_;
};




/**
 *************************************************************************
 *
 * @class ms_audio_scene
 * @ingroup audio_module
 *
 * Realizes an audio scene on Microsoft Windows. 
 *
 ************************************************************************/
class ms_audio_scene :
	public covert_object
{
	friend class ms_audio_context;


	public:

		typedef audio_scene_id covert_id_type;


		/**
		 * Render all ms_audio_source%s in this audio scene.
		 * Adds rendered audio onto the @bref{target_buffer}.
		 */
		void render
			(audio_buffer& target_buffer);

		
	private:

		ms_audio_scene
			(const covert_object_manager& object_manager);

		void set_data(auto_pointer<covert_data> data) override;
		auto_pointer<covert_data> data() override;

		const covert_object_manager& object_manager_;

		audio_scene_data scene_data_;
};




/**
 *************************************************************************
 *
 * @class ms_audio_master
 * @ingroup audio_module
 *
 * Realizes an audio master bus on Microsoft Windows. 
 *
 ************************************************************************/
class ms_audio_master :
	public covert_object
{
	friend class ms_audio_context;


	public:

		typedef audio_master_id covert_id_type;


		/**
		 * Render all ms_audio_scene%s in this audio master.
		 * Contents of the @bref{target_buffer} are overwritten.
		 */
		void render
			(audio_buffer& target_buffer);
		

	private:

		ms_audio_master
			(const covert_object_manager& object_manager);

		void set_data(auto_pointer<covert_data> data) override;
		auto_pointer<covert_data> data() override;

		const covert_object_manager& object_manager_;

		audio_master_data master_data_;
};




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_MS_AUDIO) */


#endif /* !defined(INCLUDED__VIRAL_CORE__MS_AUDIO_GRAPH_HPP) */

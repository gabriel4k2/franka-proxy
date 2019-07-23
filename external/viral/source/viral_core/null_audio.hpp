/**
 *************************************************************************
 *
 * @file null_audio.hpp
 *
 * Null audio system.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__NULL_AUDIO_HPP)
#define INCLUDED__VIRAL_CORE__NULL_AUDIO_HPP


#include "build_config.hpp"


#include "auto_pointer.hpp"
#include "covert_object.hpp"
#include "audio.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class null_audio_context
 * @ingroup audio_module
 *
 * An audio_context that does not perform any audio rendering.
 *
 * Usage notes:
 * - A null_audio_context does not completely mimic a true audio_context.
 *		For instance, the null_audio_context ignores audio_command_play
 *		and therefore does not correctly track currently playing sources.
 *
 ************************************************************************/
class null_audio_context :
	public audio_context
{
	template <typename T> friend class null_audio_object;


	public:

		null_audio_context
			(const string& context_name);


		~null_audio_context() NOTHROW override;


	private:

		auto_pointer<covert_object> create_object
			(const shared_pointer<covert_id>& id) override;

		void play(const audio_command_play& play_command) override;
		void stop(const audio_command_stop& stop_command) override;

		void set_master
			(const audio_command_master& master_command) override;

		void render
			(const audio_command_render& render_command) override;

		void register_object_creators();
		template <typename T>
			auto_pointer<covert_object> create_typed_object();

		static void log_and_throw_null_data();
		static void log_and_throw_invalid_data_type();


		typedef auto_pointer<covert_object>
			(null_audio_context::*object_creator_function)();

		typedef tree_map<type_info_ref, object_creator_function> object_creator_map;
		object_creator_map object_creators_;
};




/**
 *************************************************************************
 *
 * @class null_audio_object
 * @ingroup audio_module
 *
 * A generic covert_object that stores covert_data of type @bref{T}
 * for the null_audio_context.
 *
 ************************************************************************/
template <typename T>
	class null_audio_object :
		public covert_object
{
	friend class null_audio_context;


	public:

		typedef typename T::covert_id_type covert_id_type;


	private:

		null_audio_object() { }


		void set_data(auto_pointer<covert_data> data) override
		{
			if (!data)
				null_audio_context::log_and_throw_null_data();

			auto_pointer<T> data_typed
				(data.try_cast<T>());

			if (!data_typed)
				null_audio_context::log_and_throw_invalid_data_type();

			// Exception safety: NOTHROW from here.
			data_ = std::move(data_typed);
		}


		auto_pointer<covert_data> data() override
			{ return data_ ? data_->clone() : auto_pointer<T>(); }


		auto_pointer<T> data_;
};


/**
 * Objects for the null_audio_context graph.
 * @ingroup audio_module
 */
//@{
typedef null_audio_object<audio_waveform_data> null_audio_waveform;
typedef null_audio_object<audio_effect_data> null_audio_effect;
typedef null_audio_object<audio_source_data> null_audio_source;
typedef null_audio_object<audio_scene_data> null_audio_scene;
typedef null_audio_object<audio_master_data> null_audio_master;
//@}




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__NULL_AUDIO_HPP) */

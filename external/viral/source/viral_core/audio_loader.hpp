/**
 *************************************************************************
 *
 * @file audio_loader.hpp
 *
 * Loader for audio-specific covert_data.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__AUDIO_LOADER_HPP)
#define INCLUDED__VIRAL_CORE__AUDIO_LOADER_HPP


#include "build_config.hpp"


#include "container_forward.hpp"
#include "file_forward.hpp"
#include "audio_forward.hpp"


#include "covert_loader.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class audio_loader
 * @ingroup render_module
 *
 * Loads audio-specific covert_data and covert_id%s
 * from some XML source.
 *
 ************************************************************************/
class audio_loader :
	public covert_loader
{

	public:

		audio_loader
			(const string& loader_name, file_context& files,
			 covert_data_cache& data_cache, covert_id_cache& id_cache);


		~audio_loader() NOTHROW override { }


	protected:

		/** Load an external data file via bound @bref{files} context. */
		auto_pointer<waveform_any> load_wave(const string& path);


		/** Parsing utilities. */
		void to_data_effects_helper
			(const xml_node& node,
			 list<shared_pointer<audio_effect_id>>& effects) const;


		/** Load audio-specific covert_data from some XML @bref{node}. */
		//@{
		void to_data(const xml_node& node, audio_waveform_data& waveform);
		void to_data(const xml_node& node, audio_effect_data& effect);
		void to_data(const xml_node& node, audio_source_data& source);
		void to_data(const xml_node& node, audio_scene_data& scene);
		void to_data(const xml_node& node, audio_master_data& master);
		//@}

		
	private:

		template <typename T>
			void load_data
				(const xml_node& node, const string& name);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__AUDIO_LOADER_HPP) */

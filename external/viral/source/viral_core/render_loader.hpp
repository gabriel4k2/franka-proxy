/**
 *************************************************************************
 *
 * @file render_loader.hpp
 *
 * Loader for rendering-specific covert data.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__RENDER_LOADER_HPP)
#define INCLUDED__VIRAL_CORE__RENDER_LOADER_HPP


#include "build_config.hpp"


#include "file_forward.hpp"


#include "covert_loader.hpp"
#include "render_resource.hpp"
#include "render_scene.hpp"
#include "render_process.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class render_loader
 * @ingroup render_module
 *
 * Loads rendering-specific covert_data and covert_id%s
 * from some XML source.
 *
 ************************************************************************/
class render_loader :
	public covert_loader
{

	public:

		render_loader
			(const string& loader_name, file_context& files,
			 covert_data_cache& data_cache, covert_id_cache& id_cache);


		~render_loader() NOTHROW override { }


	protected:

		/** Load an external data file via bound @bref{files} context. */
		//@{
		string load_text_file(const string& path);
		image_rgba load_image_rgba(const string& path);
		model load_model(const string& path);
		//@}


		/** Parsing utilities. */
		//@{
		render_texture_data::wrap_mode
			to_texture_wrap_mode(const string& s) const;
		render_texture_data::filter_mode
			to_texture_filter_mode(const string& s) const;
		render_texture_data::color_space_mode
			to_texture_color_space_mode(const string& s) const;

		void to_data_texture_helper
			(const xml_node& node, render_texture_data& texture) const;
		//@}


		/** Load rendering-specific covert_data from some XML @bref{node}. */
		//@{
		void to_data(const xml_node& node, render_texture_2d_data& texture_2d);
		void to_data(const xml_node& node, render_texture_cubemap_data& texture_cubemap);
		void to_data(const xml_node& node, render_texture_canvas_data& texture_canvas);
		void to_data(const xml_node& node, render_shader_data& shader);
		void to_data(const xml_node& node, render_material_data& material);
		void to_data(const xml_node& node, render_model_data& model);

		void to_data(const xml_node& node, render_light_data& light);
		void to_data(const xml_node& node, render_puppet_data& puppet);
		void to_data(const xml_node& node, render_stack_data& stack);
		void to_data(const xml_node& node, render_scene_data& scene);

		void to_data(const xml_node& node, render_layer_data& layer);
		void to_data(const xml_node& node, render_canvas_data& canvas);
		void to_data(const xml_node& node, render_process_data& process);
		//@}

		
	private:

		template <typename T>
			void load_data
				(const xml_node& node, const string& name);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__RENDER_LOADER_HPP) */

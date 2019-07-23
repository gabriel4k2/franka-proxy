/**
 *************************************************************************
 *
 * @file gl3_loader.hpp
 *
 * Specialized loader for OpenGL3 covert_data.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_LOADER_HPP)
#define INCLUDED__VIRAL_CORE__GL3_LOADER_HPP


#include "build_config.hpp"


#include "gl3_forward.hpp"
#include "render_loader.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_loader
 * @ingroup gl3_render_module
 *
 * A specialized render_loader that supports covert_data specific
 * to OpenGL3, such as gl3_model_data.
 *
 ************************************************************************/
class gl3_loader : public render_loader
{
	public:

		gl3_loader
			(const string& loader_name, file_context& files,
			 covert_data_cache& data_cache, covert_id_cache& id_cache);

		~gl3_loader() NOTHROW override { }


	private:

		void to_uniform_values_helper
			(const xml_node& node, gl3_uniform_value_map& uniform_values);

		void to_data(const xml_node& node, render_shader_data& shader);
		void to_data(const xml_node& node, gl3_material_data& material);
		void to_data(const xml_node& node, gl3_model_data& model);
		void to_data(const xml_node& node, gl3_puppet_data& puppet);

		template <typename T>
			void load_data
				(const xml_node& node, const string& name);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_LOADER_HPP) */
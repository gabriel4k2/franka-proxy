/**
 *************************************************************************
 *
 * @file gl3_resource.hpp
 *
 * Large-scale backend objects of OpenGL3.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_RESOURCE_HPP)
#define INCLUDED__VIRAL_CORE__GL3_RESOURCE_HPP


#include "build_config.hpp"


#include "covert_object.hpp"
#include "render_resource.hpp"
#include "gl3_texture_object.hpp"
#include "gl3_program_object.hpp"
#include "gl3_uniform.hpp"
#include "gl3_mesh.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_texture
 * @ingroup gl3_render_module
 *
 * GPU-stored texture used by gl3_render_context.
 *
 ************************************************************************/
class gl3_texture : public covert_object
{
	friend class gl3_render_context;


	public:

		typedef render_texture_id covert_id_type;
		
		const gl3_texture_object& texture_object() const;


	private:

		gl3_texture
			(const covert_object_manager& object_manager,
			 const gl3_api& gl3);

		void set_data(auto_pointer<covert_data> data) override;
		auto_pointer<covert_data> data() override;


		const covert_object_manager& object_manager_;
		const gl3_api& gl3_;

		auto_pointer<render_texture_data> texture_data_;

		auto_pointer<gl3_texture_object> texture_object_;
};




/**
 *************************************************************************
 *
 * @class gl3_shader
 * @ingroup gl3_render_module
 *
 * GPU-stored shader used by gl3_render_context.
 *
 * Usage notes:
 * - Clients can disable optional shaders (e.g. geometry shaders)
 *		by an empty code string in render_shader_data.
 * - GL3 shaders on their own do not know about any rendering logics.
 *		Selection of suitable shaders is a combination of client-side
 *		responsibility (e.g. with proper render_loader::load_options,
 *		shader to material assignments, client-side uniform choice)
 *		and gl3_kernel implementation.
 *
 ************************************************************************/
class gl3_shader : public covert_object
{
	friend class gl3_render_context;


	public:

		typedef render_shader_id covert_id_type;

		const gl3_program_object& program_object() const;


	private:

		gl3_shader
			(const covert_object_manager& object_manager,
			 const gl3_api& gl3);

		void set_data(auto_pointer<covert_data> data) override;
		auto_pointer<covert_data> data() override;


		const covert_object_manager& object_manager_;
		const gl3_api& gl3_;

		render_shader_data shader_data_;

		auto_pointer<gl3_program_object> program_object_;
};




/**
 *************************************************************************
 *
 * @class gl3_material_data
 * @ingroup gl3_render_module
 *
 * Provides material-specific gl3_uniform_value%s.
 *
 * Usage notes:
 * - Material-specific uniform values override automatic uniform values
 *		of the same name, such as light parameters or camera transforms.
 *		See gl3_kernel and gl3_uniform_value_stack.
 *
 ************************************************************************/
class gl3_material_data : public render_material_data
{

	public:

		typedef render_material_id covert_id_type;


		gl3_material_data() { }

		gl3_material_data(const gl3_material_data& other)
			:
			render_material_data(other),
			uniform_values(other.uniform_values)
		{ }

		explicit gl3_material_data(const render_material_data& other)
			: render_material_data(other) { }


		auto_pointer<covert_data> clone() const override
			{ return auto_pointer<covert_data>(new gl3_material_data(*this)); }


		gl3_uniform_value_map uniform_values;
};




/**
 *************************************************************************
 *
 * @class gl3_material
 * @ingroup gl3_render_module
 *
 * A material as used by gl3_render_context.
 *
 ************************************************************************/
class gl3_material : public covert_object
{
	friend class gl3_render_context;


	public:

		typedef render_material_id covert_id_type;

		const gl3_material_data& material_data() const;


	private:

		gl3_material
			(const covert_object_manager& object_manager,
			 const gl3_api& gl3);

		void set_data(auto_pointer<covert_data> data) override;
		auto_pointer<covert_data> data() override;


		const covert_object_manager& object_manager_;
		const gl3_api& gl3_;

		auto_pointer<gl3_material_data> material_data_;
};




/**
 *************************************************************************
 *
 * @class gl3_model_data
 * @ingroup gl3_render_module
 *
 * Adds OpenGL3-specific, custom mapping from mesh_stream names
 * to layout locations of vertex attribute inputs.
 *
 * Usage notes:
 * - There are default_stream_locations() for all streams
 *		from the mesh_stream_registry.
 * - If clients use custom streams, they must provide a complete
 *		mapping to the layout locations of relevant gl3_shader%s.
 *
 * Implementation notes:
 * - We need a static mapping from named mesh_stream%s and their data
 *		to layout location indices when we upload a mesh to OpenGL.
 *		In particular, this means we cannot dynamically bind streams
 *		to existing, named vertex attribute inputs on rendering.
 *		Rendering instead checks if a gl3_mesh provides all streams
 *		that are required when using a given gl3_program_object.
 *
 ************************************************************************/
class gl3_model_data : public render_model_data
{

	public:

		typedef render_model_id covert_id_type;


		gl3_model_data()
			:
			stream_locations(default_stream_locations())
		{ }


		gl3_model_data(const gl3_model_data& other)
			:
			render_model_data(other),
			stream_locations(other.stream_locations)
		{ }

		gl3_model_data(gl3_model_data&& other)
			:
			render_model_data(std::move(other)),
			stream_locations(std::move(other.stream_locations))
		{ }


		explicit gl3_model_data(const render_model_data& other)
			:
			render_model_data(other),
			stream_locations(default_stream_locations())
		{ }

		explicit gl3_model_data(render_model_data&& other)
			:
			render_model_data(std::move(other)),
			stream_locations(default_stream_locations())
		{ }


		auto_pointer<covert_data> clone() const override
			{ return auto_pointer<covert_data>(new gl3_model_data(*this)); }


		typedef gl3_mesh::stream_location_map stream_location_map;
		static stream_location_map default_stream_locations();
		stream_location_map stream_locations;
};




/**
 *************************************************************************
 *
 * @class gl3_model_group
 * @ingroup gl3_render_module
 *
 * A model_group within a gl3_model.
 *
 ************************************************************************/
class gl3_model_group :
	private non_copyable
{
	friend class gl3_model;


	public:

		const string name;
		gl3_mesh mesh;


	private:

		gl3_model_group
			(const gl3_api& gl3, const model_group& group,
			 const gl3_mesh::stream_location_map& stream_locations,
			 bool transient_hint)
			:
			name(group.name),
			mesh(gl3, group.mesh, stream_locations, transient_hint)
		{ }
};




/**
 *************************************************************************
 *
 * @class gl3_model
 * @ingroup gl3_render_module
 *
 * A GPU-stored @ref model used by gl3_render_context.
 *
 * Usage notes:
 * - Clients must apply gl3_model_data to use custom mesh_stream%s.
 *
 * Implementation notes:
 * - We must keep a copy of some model members (e.g. skeleton, boundings).
 *		Otherwise, this data would become unavailable once we discard
 *		the @bref{model} due to a @false @bref{keep_host_model} flag.
 *
 ************************************************************************/
class gl3_model : public covert_object
{
	friend class gl3_render_context;


	public:

		typedef render_model_id covert_id_type;

		typedef owning_list<gl3_model_group>::const_iterator group_iterator;

		group_iterator first_group() const;
		group_iterator last_group() const;

		const skeleton* skeleton_rig() const;

		const box& bounding_box() const;
		float bounding_radius() const;


	private:

		gl3_model
			(const covert_object_manager& object_manager,
			 const gl3_api& gl3);

		void set_data(auto_pointer<covert_data> data) override;
		auto_pointer<covert_data> data() override;


		const covert_object_manager& object_manager_;
		const gl3_api& gl3_;

		auto_pointer<gl3_model_data> model_data_;

		typedef owning_list<gl3_model_group> group_list;
		group_list groups_;

		auto_pointer<skeleton> skeleton_rig_;

		box bounding_box_;
		float bounding_radius_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_RESOURCE_HPP) */

/**
 *************************************************************************
 *
 * @file render_scene.hpp
 *
 * Holds scene-specific rendering data.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__RENDER_SCENE_HPP)
#define INCLUDED__VIRAL_CORE__RENDER_SCENE_HPP


#include "build_config.hpp"


#include "auto_pointer.hpp"
#include "shared_pointer.hpp"
#include "array.hpp"
#include "tree_set.hpp"
#include "tree_map.hpp"
#include "geo_vector.hpp"
#include "geo_transform.hpp"
#include "covert_data.hpp"
#include "render_resource.hpp"


namespace viral_core
{


/**
 * Tag for IDs that can be inserted into a rendering scene. 
 * @ingroup render_module
 */
class render_scene_object_tag
{
	public:
		virtual ~render_scene_object_tag() NOTHROW { }

	protected:
		render_scene_object_tag() { }
}; 




/**
 * IDs for scenes and contained objects.
 * @ingroup render_module
 */
//@{
class render_light_id :
	public covert_id,
	public render_scene_object_tag
{
	public:
		render_light_id() { }
		render_light_id(const string& debug_name)
			: covert_id(debug_name) { }
};

class render_puppet_id :
	public covert_id,
	public render_scene_object_tag
{
	public:
		render_puppet_id() { }
		render_puppet_id(const string& debug_name)
			: covert_id(debug_name) { }
};

class render_stack_id :
	public covert_id,
	public render_scene_object_tag
{
	public:
		render_stack_id() { }
		render_stack_id(const string& debug_name)
			: covert_id(debug_name) { }
};

class render_scene_id : public covert_id
{
	public:
		render_scene_id() { }
		render_scene_id(const string& debug_name)
			: covert_id(debug_name) { }
};
//@}




/**
 *************************************************************************
 *
 * @class render_light_data
 * @ingroup render_module
 *
 * Defines a light source within a virtual scene.
 *
 * Phong model:
 * - Default light and material parameters suggest a phong model,
 *		where a fragment color derives as:
 *		@verbatim
 *			distance_factor =
 *				1 on parallel lights, otherwise
 *					1 / (1 + light_attenuation * distance * distance)
 *			spot_factor =
 *				1 if not a spot light, otherwise
 *					pow(max(dot(to_light_unit, -light_direction)
 *								- light_spot_angle_cosine, 0)
 *							/ (1 - light_spot_angle_cosine),
 *						light_spot_exponent)
 *			shadow_factor =
 *				1 - light_occlusion * shadow_opacity
 *
 *			diffuse_factor =
 *				max(dot_product(frag_normal_unit, to_light_unit), 0)
 *			specular_factor =
 *				pow(max(dot(to_camera_unit, to_light_reflected_unit), 0),
 *					material_specular_exponent);
 *
 *			ambient =
 *				light_ambient_color * material_ambient_color *
 *				distance_factor * spot_factor
 *			diffuse =
 *				light_diffuse_color * material_diffuse_color *
 *				shadow_factor * distance_factor * spot_factor * diffuse_factor
 *			specular =
 *				light_specular_color * material_specular_color *
 *				shadow_factor * distance_factor * spot_factor * specular_factor
 *
 *			fragment_color =
 *				light_intensity * (ambient + diffuse + specular) +
 *				material_emit_color
 *		@endverbatim
 * - Specific render_context back-ends may implement differing
 *		light-material interactions, see render_shader_data.
 *
 * Usage notes:
 * - Directed emitters (e.g. @bref{emitter_spot}) with identity
 *		@bref{orientation} emit light into positive z direction.
 * - Clients should use light colors (e.g. @bref{diffuse_color})
 *		to model light-material interactions only. In particular,
 *		light colors are not suited for changing light brightness
 *		or decay behaviour.
 * - Clients should use @bref{intensity} to determine maximum
 *		light brightness (i.e. at @bref{position}), and should
 *		use @bref{attenuation} to control decay over distance.
 * - For back-ends that support but a limited number of lights,
 *		a high @bref{priority} helps rate relevant light sources.
 *
 ************************************************************************/
class render_light_data : public covert_data
{
	public:

		typedef render_light_id covert_id_type;


		enum emitter_type
			{ emitter_point, emitter_spot, emitter_parallel }; 


		render_light_data();

		render_light_data
			(emitter_type emitter, bool enabled,
			 float priority, float influence_epsilon,
			 const color& ambient_color,
			 const color& diffuse_color,
			 const color& specular_color,
			 float intensity, float attenuation,
			 float spot_angle, float spot_exponent,
			 bool shadow_enabled,
			 float shadow_priority,
			 float shadow_opacity,
			 float shadow_clip_near_normalized,
			 float shadow_clip_far_normalized);


		auto_pointer<covert_data> clone() const override;


		/**
		 * Determine light influence, the maximum color increment
		 * that default light equations allow for a light source
		 * with given @bref{emitter}, colors and @bref{intensity}
		 * after @bref{attenuation} at a given distance or target.
		 * Influence does not consider priority, spotlight angles,
		 * surface orientation, or shadows.
		 *
		 * For instance, a render_context uses light influences
		 * to select the most relevant lights for some puppet.
		 *
		 * Backends may discard lights that contribute less
		 * than the light @bref{influence_epsilon} dictates.
		 * For point lights, call point_range() to calculate
		 * an upper bound on the respective culling distance.
		 */
		//@{
		float influence
			(float distance) const NOTHROW;
		float influence
			(const vector& target) const NOTHROW;
		float influence
			(const sphere& target) const NOTHROW;

		float point_range() const NOTHROW;
		//@}


		emitter_type emitter;
		bool enabled;
		float priority;
		float influence_epsilon;

		vector position;
		rotation orientation;

		color ambient_color;
		color diffuse_color;
		color specular_color;

		float intensity;
		float attenuation;

		float spot_angle;
		float spot_exponent;

		bool shadow_enabled;
		float shadow_priority;
		float shadow_opacity;
		float shadow_clip_near_normalized;
		float shadow_clip_far_normalized;
};




/**
 *************************************************************************
 *
 * @class render_puppet_data
 * @ingroup render_module
 *
 * Positions a model within a virtual scene.
 *
 * Usage notes:
 * - See render_model_data and render_scene_data.
 *
 ************************************************************************/
class render_puppet_data : public covert_data
{
	public:

		typedef render_puppet_id covert_id_type;

		typedef tree_map<string, shared_pointer<render_material_id>> material_map;


		render_puppet_data();

		render_puppet_data
			(const vector& position,
			 const rotation& orientation,
			 float scale,
			 const shared_pointer<render_model_id>& model,
			 auto_pointer<skeleton_transform> relative_pose,
			 const material_map& materials,
			 bool visible,
			 bool shadow_cast_enabled);

		render_puppet_data
			(const render_puppet_data& other);


		auto_pointer<covert_data> clone() const override;


		vector position;
		rotation orientation;
		float scale;


		shared_pointer<render_model_id> model;


		/**
		 * Relative pose for the @ref skeleton rig in @bref{model}.
		 * A @NULL pose pointer shows the default binding pose.
		 * Ignored by models without a skeleton.
		 */
		auto_pointer<skeleton_transform> relative_pose;


		/**
		 * Material to use for each named mesh_group
		 * in the puppet @bref{model}.
		 * Each mesh_group must have an assigned material,
		 * but any surplus materials are ignored.
		 */
		material_map materials;


		/**
		 * Control puppet-wide visibility:
		 * If @false, suppress rendering of this puppet.
		 * If @true, render any bound @bref{materials}
		 * that are @bref{render_material_data::visible}.
		 */
		bool visible;


		/**
		 * Control puppet-wide shadow casting behaviour:
		 * If @false, this puppet does not cast any shadows.
		 * If @true, cast shadows for @bref{materials} with
		 * @bref{render_material_data::shadow_cast_enabled}.
		 * 
		 * Note puppets can cast shadows even if not @bref{visible}.
		 * This allows to add artistic shadows to a virtual scene.
		 */
		bool shadow_cast_enabled;
};




/**
 *************************************************************************
 *
 * @class render_stack_data
 * @ingroup render_module
 *
 * Renders an array of render_puppet_id%s exactly in given order.
 *
 * Usage notes:
 * - In most cases, rendering back-ends cannot perfectly depth-sort
 *		blended puppets, but instead derive an approximate sorting
 *		from puppet positions. In contrast, render stacks allow for
 *		custom a-priori depth-sorting (e.g. for decals or sky domes).
 * - To avoid duplicate rendering, individual puppets within a stack
 *		need not and should not be inserted into the scene.
 * - A renderer backend may check the @bref{anchor_hint} point in order
 *		to depth-sort stacks against other geometry. 
 * - @attent Render stacks are not particularly efficient, as they
 *		might not partake in some back-end optimizations (e.g. z-prepass
 *		or hierarchical culling). Thus, clients should only use stacks
 *		where required. For example, prefer sorting meshes and triangles
 *		within a render_model_data over multiple stacked puppets.
 *
 ************************************************************************/
class render_stack_data : public covert_data
{
	public:

		typedef render_stack_id covert_id_type;

		typedef array<shared_pointer<render_puppet_id>> puppet_array;


		render_stack_data();
		render_stack_data
			(const puppet_array& puppets,
			 const vector& anchor_hint);

		auto_pointer<covert_data> clone() const override;


		puppet_array puppets;
		vector anchor_hint;
};




/**
 *************************************************************************
 *
 * @class render_scene_data
 * @ingroup render_module
 *
 * Groups various rendering @bref{objects} into a virtual scene.
 *
 * Usage notes:
 * - Renderer back-ends can arbitrarily interpret virtual scenes
 *		(e.g. layer hierarchical structures onto a scene, sort scene
 *		objects to implement depth-correct blending, combine objects
 *		to reduce the number of draw calls).
 *
 ************************************************************************/
class render_scene_data : public covert_data
{
	public:

		typedef render_scene_id covert_id_type;

		typedef tree_set<shared_pointer<render_scene_object_tag>> object_set;


		render_scene_data();
		render_scene_data(const object_set& objects);

		auto_pointer<covert_data> clone() const override;


		object_set objects;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__RENDER_SCENE_HPP) */

/**
 *************************************************************************
 *
 * @file gl3_kernel_scene.hpp
 *
 * Scene parsing in OpenGL3 kernel.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_KERNEL_SCENE_HPP)
#define INCLUDED__VIRAL_CORE__GL3_KERNEL_SCENE_HPP


#include "build_config.hpp"


#include "covert_forward.hpp"
#include "gl3_forward.hpp"


#include "util.hpp"
#include "buffer.hpp"
#include "geo_camera.hpp"
#include "gl3_kernel_util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_kernel_scene
 * @ingroup gl3_render_module
 *
 * Parse the gl3_scene_object_tag%s of a gl3_scene.
 *
 * Usage notes:
 * - Parsing sorts all opaque_object()s from front to back,
 *		and all blend_object()s from back to front.
 * - Parsing sorts shadow_enabled_light()s by shadow priority.
 *
 ************************************************************************/
class gl3_kernel_scene :
	private non_copyable
{

	public:

		gl3_kernel_scene
			(const covert_object_manager& object_manager);


		~gl3_kernel_scene() NOTHROW;


		/**
		 * Parse a @bref{scene} and extract its objects
		 * into per-pass collections for later queries,
		 * using some @bref{cam}era for depth sorting.
		 *
		 * The call clears any preceding parse() results.
		 */
		void parse_scene
			(const gl3_scene& scene,
			 const camera& cam);

	
		/**
		 * Access various objects within the scene.
		 * All object queries throw on invalid indices.
		 * See gl3_kernel_scene for notes on object sorting.
		 */
		//@{
		const gl3_light& light(int64 index) const;
		int64 light_count() const NOTHROW;
		const gl3_light& shadow_enabled_light(int64 index) const;
		int64 shadow_enabled_light_count() const NOTHROW;

		const gl3_scene_object_tag& renderable_object(int64 index) const;
		int64 renderable_object_count() const NOTHROW;
		const gl3_scene_object_tag& opaque_object(int64 index) const;
		int64 opaque_object_count() const NOTHROW;
		const gl3_scene_object_tag& blend_object(int64 index) const;
		int64 blend_object_count() const NOTHROW;
		const gl3_scene_object_tag& shadow_caster(int64 index) const;
		int64 shadow_caster_count() const NOTHROW;
		//@}


		/**
		 * Fetch the index of a shadow_enabled_light()
		 * within the overall light() array.
		 */
		int64 shadow_enabled_light_index(int64 index) const;


	private:

		void apply_camera(const camera& cam);

		void parse_scene_objects(const gl3_scene& scene);
		void parse_light(const gl3_light& light);
		void parse_puppet(const gl3_puppet& puppet);
		void parse_stack(const gl3_stack& stack);

		void sort_objects();
		void clear() NOTHROW;


		const covert_object_manager& object_manager_;

		camera cam_;
		frustum frustum_;

		dynamic_buffer<const gl3_light*> lights_;
		dynamic_buffer<gl3_kernel_priority_index> shadow_enabled_light_indices_;

		dynamic_buffer<const gl3_scene_object_tag*> renderable_objects_;
		dynamic_buffer<gl3_kernel_priority_index> opaque_object_indices_;
		dynamic_buffer<gl3_kernel_priority_index> blend_object_indices_;
		dynamic_buffer<int64> shadow_caster_indices_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_KERNEL_SCENE_HPP )*/
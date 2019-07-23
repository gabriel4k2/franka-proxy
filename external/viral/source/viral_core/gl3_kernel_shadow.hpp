/**
 *************************************************************************
 *
 * @file gl3_kernel_shadow.hpp
 *
 * Management of shadow maps in OpenGL3 kernel.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_KERNEL_SHADOW_HPP)
#define INCLUDED__VIRAL_CORE__GL3_KERNEL_SHADOW_HPP


#include "build_config.hpp"


#include "covert_forward.hpp"
#include "render_forward.hpp"
#include "gl3_forward.hpp"


#include "util.hpp"
#include "owning_array.hpp"
#include "geo_camera.hpp"
#include "gl3_kernel_state.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_kernel_shadow
 * @ingroup gl3_render_module
 *
 * Maps shadow-enabled lights to limited per-kernel shadow maps
 * and renders any in-use shadow maps.
 *
 * Implementation notes:
 * - A gl3_kernel_shadow allocates shadow maps on first use only.
 *		We do not preallocate shadow maps to avoid consuming
 *		excessive memory on gl3_layer%s without shadows.
 * - The shadow kernel implements a redundant depth rendering pass
 *		to avoid a circular dependency on gl3_kernel_render.
 *
 ************************************************************************/
class gl3_kernel_shadow :
	private non_copyable
{
	public:

		gl3_kernel_shadow
			(const covert_object_manager& object_manager,
			 const gl3_api& gl3,
			 const gl3_kernel_scene& kernel_scene);


		~gl3_kernel_shadow() NOTHROW;
	

		/**
		 * Distribute shadow maps over the shadow-enabled lights
		 * in @bref{kernel_scene_} and render assigned shadow maps.
		 *
		 * Shadow mapping accounts for the @bref{scene_camera} 
		 * to improve depth map precision.
		 */
		void render_shadow_maps
			(const camera& scene_camera);


		/**
		 * Fetch index of shadow map assigned to scene @bref{light_index},
		 * or @bref{no_shadow_map_assigned} for lights without shadow map.
		 * The target of the shadow map index depends on the emitter type
		 * of the light (e.g. shadow_map_point() for point lights).
		 */
		int64 shadow_map_index(int64 light_index) const;


		/**
		 * Fetch shadow map of given type and @bref{index}.
		 * Throws if no such shadow map exists.
		 */
		//@{
		const gl3_shadow_map_point& shadow_map_point(int64 index) const;
		const gl3_shadow_map_spot& shadow_map_spot(int64 index) const;
		const gl3_shadow_map_parallel& shadow_map_parallel(int64 index) const;
		//@}


		/**
		 * Fetch number of currently allocated,
		 * but possibly unused shadow maps.
		 */
		//@{
		int64 shadow_map_point_count() const NOTHROW;
		int64 shadow_map_spot_count() const NOTHROW;
		int64 shadow_map_parallel_count() const NOTHROW;
		//@}


		/**
		 * Fetch the number of shadow maps assigned
		 * to lights through shadow_map_index().
		 */
		//@{
		int64 shadow_map_point_used() const NOTHROW;
		int64 shadow_map_spot_used() const NOTHROW;
		int64 shadow_map_parallel_used() const NOTHROW;
		//@}


		/** Index that indicates a light without a shadow map. */
		static const int64 no_shadow_map_assigned = -1;


	private:

		gl3_shadow_map_point&
			assign_shadow_map_point(int64 light_index);
		gl3_shadow_map_spot&
			assign_shadow_map_spot(int64 light_index);
		gl3_shadow_map_parallel&
			assign_shadow_map_parallel(int64 light_index);

		void render_shadow_map_point
			(const render_light_data& light_data,
			 gl3_shadow_map_point& shadow_map);
		void render_shadow_map_spot
			(const render_light_data& light_data,
			 gl3_shadow_map_spot& shadow_map);
		void render_shadow_map_parallel
			(const render_light_data& light_data,
			 gl3_shadow_map_parallel& shadow_map,
			 const camera& scene_camera);

		bool bound_shadow_casters_parallel
			(box& light_local_bounding_box,
			 const matrix& to_light_local);
		void lock_to_texels_parallel
			(box& light_local_bounding_box,
			 const frustum& cascade_frustum,
			 int64 shadow_map_size);			 

		void apply_shadow_map_camera
			(const camera& shadow_map_camera);

		void render_pass_shadow_map();
		void render_puppet(const gl3_puppet& puppet);
		void render_stack(const gl3_stack& stack);


		const covert_object_manager& object_manager_;
		const gl3_api& gl3_;
		const gl3_kernel_scene& kernel_scene_;

		owning_array<gl3_shadow_map_point> shadow_maps_point_;
		owning_array<gl3_shadow_map_spot> shadow_maps_spot_;
		owning_array<gl3_shadow_map_parallel> shadow_maps_parallel_;

		dynamic_buffer<int64> shadow_map_indices_;

		int64 shadow_map_point_used_;
		int64 shadow_map_spot_used_;
		int64 shadow_map_parallel_used_;

		camera shadow_map_camera_;
		frustum shadow_map_frustum_;

		gl3_kernel_state kernel_state_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_KERNEL_SHADOW_HPP) */
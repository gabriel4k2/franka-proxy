/**
 *************************************************************************
 *
 * @file gl3_kernel_light.hpp
 *
 * Per-pass light management in OpenGL3 kernel.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_KERNEL_LIGHT_HPP)
#define INCLUDED__VIRAL_CORE__GL3_KERNEL_LIGHT_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"
#include "gl3_forward.hpp"


#include "util.hpp"
#include "buffer.hpp"
#include "gl3_kernel_util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_kernel_light
 * @ingroup gl3_render_module
 *
 * Manages relevant, pending, and per-pass lights for scene objects.
 *
 ************************************************************************/
class gl3_kernel_light :
	private non_copyable
{
	public:

		gl3_kernel_light
			(const gl3_kernel_scene& kernel_scene,
			 const gl3_kernel_shadow& kernel_shadow);


		~gl3_kernel_light() NOTHROW;


		/**
		 * Cache and prioritize relevant lights
		 * in the @bref{bounding_sphere} of some scene object.
		 */
		void select_relevant_lights
			(const sphere& bounding_sphere);


		/**
		 * Enqueue all lights chosen by select_relevant_lights()
		 * for subsequent use through select_pass_lights().
		 *
		 * Clients may repeatedly call init_pending_lights()
		 * to reuse relevant lights (e.g. over mesh groups),
		 * which avoids expensive and redundant light caching.
		 *
		 * The call clears any precedingly pending lights.
		 */
		void init_pending_lights();


		/**
		 * Check if there are any remaining pending lights
		 * that warrant additive lighting passes.
		 */
		bool has_pending_lights();


		/**
		 * Select high-priority pending lights as pass lights
		 * and remove all selected lights from pending lights.
		 * Update per-pass shadow map slots accordingly.
		 *
		 * If @bref{multipass_shadows} is @true, light selection
		 * defers high-priority lights that exceed shadow slot limits
		 * to subsequent rendering passes.
		 *
		 * If @bref{multipass_shadows} is @false, light selection
		 * always uses all highest-priority lights, but suppresses
		 * any excess shadow maps through pass_shadow_enabled().
		 * This favors light contribution over correct shadows.
		 */
		void select_pass_lights(bool multipass_shadows);


		/**
		 * Lights selected for current pass and their shadow flags.
		 * Shadow flags match render_light_data::shadow_enabled
		 * unless select_pass_lights() ran out of shadow map slots.
		 */
		//@{
		const dynamic_buffer<int64>& pass_light_indices() const NOTHROW;
		const dynamic_buffer<bool>& pass_shadow_enabled() const NOTHROW;
		//@}


		/**
		 * Shadow maps required for lights in current pass,
		 * or gl3_kernel_shadow::no_shadow_map_assigned
		 * for unused shadow map slots.
		 */
		//@{
		int64 pass_shadow_map_point_index() const NOTHROW;
		int64 pass_shadow_map_spot_index() const NOTHROW;
		int64 pass_shadow_map_parallel_index() const NOTHROW;
		//@}


	private:

		const gl3_kernel_scene& kernel_scene_;
		const gl3_kernel_shadow& kernel_shadow_;

		dynamic_buffer<gl3_kernel_priority_index> relevant_light_indices_;
		dynamic_buffer<int64> pending_light_indices_;

		dynamic_buffer<int64> pass_light_indices_;
		dynamic_buffer<bool> pass_shadow_enabled_;

		int64 pass_shadow_map_point_index_;
		int64 pass_shadow_map_spot_index_;
		int64 pass_shadow_map_parallel_index_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_KERNEL_LIGHT_HPP) */
/**
 *************************************************************************
 *
 * @file gl3_kernel_util.hpp
 *
 * Utilities for gl3_kernel and related classes.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_KERNEL_UTIL_HPP)
#define INCLUDED__VIRAL_CORE__GL3_KERNEL_UTIL_HPP


#include "build_config.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_kernel_pass
 * @ingroup gl3_render_module
 *
 * Constants for all rendering passes in gl3_kernel.
 *
 ************************************************************************/
class gl3_kernel_pass :
	private non_constructable
{
	public:

		enum type
		{
			opaque,	
			opaque_multipass_light,
			blend,
			stack,
			shadow_map
		};
};


typedef gl3_kernel_pass::type gl3_kernel_pass_type;




/**
 *************************************************************************
 *
 * @class gl3_kernel_limits
 * @ingroup gl3_render_module
 *
 * Hard limits in a gl3_kernel, such as for skeleton joints
 * or per-pass lights.
 *
 ************************************************************************/
class gl3_kernel_limits :
	private non_constructable
{
	public:

		/** Maximum number of joints supported per skeleton. */
		static const int64 max_skeleton_joints = 16;


		/** Maximum number of shadow maps per type. */
		//@{
		static const int64 max_shadows_point = 8;
		static const int64 max_shadows_spot = 8;
		static const int64 max_shadows_parallel = 2;
		//@}


		/** Shadow map quality settings. */
		//@{
		static const int64 shadow_map_resolution = 1024;
		static const int64 shadow_map_cascade_count = 4;
		//@}


		/** Maximum number of lights supported per pass. */
		static const int64 max_pass_lights = 4;
};




/**
 *************************************************************************
 *
 * @class gl3_kernel_priority_index
 * @ingroup gl3_render_module
 *
 * Priority-sortable index for use in gl3_kernel logics.
 *
 ************************************************************************/
class gl3_kernel_priority_index
{
	public:

		gl3_kernel_priority_index
			(int64 index, float priority)
			:
			index(index), priority(priority)
		{ }


		static bool sort_ascending
			(const gl3_kernel_priority_index& a,
			 const gl3_kernel_priority_index& b)
				{ return a.priority < b.priority; }

		static bool sort_descending
			(const gl3_kernel_priority_index& a,
			 const gl3_kernel_priority_index& b)
				{ return a.priority > b.priority; }


		int64 index;
		float priority;
};


/**
 * Required so that std::sort() calls correct swap.
 * @ingroup gl3_render_module
 */
inline void swap
	(gl3_kernel_priority_index& a,
	 gl3_kernel_priority_index& b)
{
	swap(a.index, b.index);
	swap(a.priority, b.priority);
}




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_KERNEL_UTIL_HPP) */
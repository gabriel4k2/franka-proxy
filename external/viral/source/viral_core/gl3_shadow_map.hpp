/**
 *************************************************************************
 *
 * @file gl3_shadow_map.hpp
 *
 * Shadow map targets for OpenGL3 rendering.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_SHADOW_MAP_HPP)
#define INCLUDED__VIRAL_CORE__GL3_SHADOW_MAP_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "auto_pointer.hpp"
#include "owning_array.hpp"
#include "geo_2d.hpp"
#include "geo_transform.hpp"
#include "gl3_frame_buffer.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_shadow_map_point
 * @ingroup gl3_render_module
 *
 * A cube map with depth values, as required for point light sources.
 *
 ************************************************************************/
class gl3_shadow_map_point :
	private non_copyable
{
	public:

		gl3_shadow_map_point
			(const gl3_api& gl3,
			 int64 size);


		~gl3_shadow_map_point() NOTHROW;


		int64 size() const NOTHROW;


		/**
		 * Transformation from reciprocal of linear z distance
		 * between light and fragment to non-linear, reciprocal
		 * z coordinates in [0 ~ znear, 1 ~ zfar] as stored in
		 * the depth texture of this shadow map.
		 *
		 * In particular,
		 *		depth = (1/z) * z_factor() + z_offset(),
		 * with
		 *		z_factor() == 0.5f * (-2 * znear * zfar / (zfar - znear)),
		 *		z_offset() == 0.5f * ((zfar + znear) / (zfar - znear)) + 0.5f,
		 * replicates a transform of a vector(0, 0, z) with
		 * matrix::perspective().project() to [-1, 1] range
		 * and then applies scale and offset to [0, 1].
		 */
		//@{
		void update_z_transform(const camera& point_camera);
		float z_factor() const NOTHROW;
		float z_offset() const NOTHROW;
		//@}


		/**
		 * Bind given cubemap side of shadow map
		 * as rendering target.
		 *
		 * Automatically selects suitable viewport
		 * and clears current shadow map contents.
		 */
		void prepare_and_bind_target
			(gl3_frame_buffer_attachment_cubemap::cubemap_side side) const;


		/** Bind shadow map to @bref{texture_unit_index}. */
		void bind_texture(GLenum texture_unit_index) const;


		static const GLsizei min_size = 32;
		static const GLsizei max_size = 4096;


	private:

		const gl3_api& gl3_;
		const int64 size_;

		float z_factor_;
		float z_offset_;
		
		auto_pointer<gl3_texture_object_cubemap> depth_texture_;

		typedef owning_array<gl3_frame_buffer> depth_target_array;
		depth_target_array depth_targets_;
};




/**
 *************************************************************************
 *
 * @class gl3_shadow_map_spot
 * @ingroup gl3_render_module
 *
 * A single shadow map for spot lights.
 *
 ************************************************************************/
class gl3_shadow_map_spot :
	private non_copyable
{
	public:

		gl3_shadow_map_spot
			(const gl3_api& gl3,
			 int64 size);


		~gl3_shadow_map_spot() NOTHROW;


		int64 size() const NOTHROW;


		/**
		 * Projective transformation from world space
		 * to coordinates in the current shadow map.
		 * Resulting (u, v, z) coords are in [0,1]^3 
		 * after explicit homogenization.
		 */
		//@{
		void update_world_to_texture(const camera& spot_camera);
		const matrix& world_to_texture() const NOTHROW;
		//@}


		/**
		 * Bind shadow map as rendering target.
		 * Automatically selects suitable viewport
		 * and clears current shadow map contents.
		 */
		void prepare_and_bind_target() const;


		/** Bind shadow map to @bref{texture_unit_index}. */
		void bind_texture(GLenum texture_unit_index) const;


		static const GLsizei min_size = 32;
		static const GLsizei max_size = 4096;


	private:

		const gl3_api& gl3_;
		const int64 size_;

		matrix world_to_texture_;

		auto_pointer<gl3_texture_object_2d> depth_texture_;
		auto_pointer<gl3_frame_buffer> depth_target_;
};




/**
 *************************************************************************
 *
 * @class gl3_shadow_map_parallel
 * @ingroup gl3_render_module
 *
 * A cascade of 2D shadow maps for use with parallel lights.
 *
 ************************************************************************/
class gl3_shadow_map_parallel :
	private non_copyable
{
	public:

		gl3_shadow_map_parallel
			(const gl3_api& gl3,
			 int64 size,
			 int64 cascade_count);


		~gl3_shadow_map_parallel() NOTHROW;


		int64 size() const NOTHROW;
		int64 cascade_count() const NOTHROW;


		/**
		 * Orthographic transformation from world space
		 * to coordinates in some cascade @bref{entry}.
		 * Resulting (u, v, z) coords are in [0,1]^3 
		 * and require no homogenization.
		 */
		//@{
		void update_world_to_texture(const camera& entry_camera, int64 entry);
		matrix world_to_texture(int64 entry) const NOTHROW;
		//@}


		/**
		 * Whether some cascade @bref{entry}
		 * holds valid depth values.
		 */
		//@{
		void update_valid(bool valid, int64 entry);
		bool valid(int64 entry) const NOTHROW;
		//@}


		/**
		 * Bind shadow map at @bref{entry} index
		 * as rendering target.
		 *
		 * Automatically selects suitable viewport
		 * and clears current shadow map contents.
		 */
		void prepare_and_bind_target(int64 entry) const;


		/** Bind shadow map to @bref{texture_unit_index}. */
		void bind_texture(GLenum texture_unit_index) const;


		static const GLsizei min_size = 32;
		static const GLsizei max_size = 4096;
		static const int64 max_cascade_count = 4;


	private:

		const gl3_api& gl3_;
		const int64 size_;
		const int64 cascade_count_;

		dynamic_buffer<matrix> world_to_texture_;
		dynamic_buffer<bool> valid_;

		auto_pointer<gl3_texture_object_2d_array> depth_textures_;

		typedef owning_array<gl3_frame_buffer> depth_target_array;
		depth_target_array depth_targets_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_SHADOW_MAP_HPP) */

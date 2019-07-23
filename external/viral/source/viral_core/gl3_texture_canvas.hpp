/**
 *************************************************************************
 *
 * @file gl3_texture_canvas.hpp
 *
 * Render-to-texture result bridge.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_TEXTURE_CANVAS_HPP)
#define INCLUDED__VIRAL_CORE__GL3_TEXTURE_CANVAS_HPP


#include "build_config.hpp"


#include "covert_forward.hpp"
#include "gl3_forward.hpp"


#include "gl3_texture_object.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_texture_canvas
 * @ingroup gl3_render_module
 *
 * A bridge for using the render-to-texture results of a gl3_canvas
 * through a gl3_texture_object interface.
 *
 * Usage notes:
 * - Clients should only use gl3_texture_canvas::bind().
 *		Other gl3_texture_object calls (e.g. build_mipmaps())
 *		do not make sense for a gl3_texture_canvas bridge
 *		and thus throw on use or fail silently.
 *
 ************************************************************************/
class gl3_texture_canvas :
	public gl3_texture_object
{
	public:

		/**
		 * Create proxy that uses the gl3_render_target
		 * of some @bref{source_canvas} as texture.
		 */
		gl3_texture_canvas
			(const gl3_api& gl3,
			 const covert_object_manager& object_manager,
			 const weak_pointer<render_canvas_id>& source_canvas);


		void build_mipmaps() const override;
		void bind(GLenum texture_unit_index) const override;


	private:

		const covert_object_manager& object_manager_;
		weak_pointer<render_canvas_id> source_canvas_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_TEXTURE_CANVAS_HPP) */
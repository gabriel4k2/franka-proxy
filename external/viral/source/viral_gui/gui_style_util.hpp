/**
 *************************************************************************
 *
 * @file gui_style_util.hpp
 *
 * GUI utility styles.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_STYLE_UTIL_HPP)
#define INCLUDED__VIRAL_GUI__GUI_STYLE_UTIL_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/render_forward.hpp>


#include <viral_core/shared_pointer.hpp>
#include <viral_core/geo_2d.hpp>
#include <viral_core/font.hpp>


#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_cursor_style
 * @ingroup gui_module
 *
 * Style for a cursor rectangle.
 *
 ************************************************************************/
class gui_cursor_style :
	public gui_style
{
	public:

		gui_cursor_style
			(const shared_instance<render_material_id>& material,
			 const rect2f& extents);


		const shared_instance<render_material_id> material;


		/**
		 * Extents of the cursor in GUI coordinates
		 * around the cursor hotspot at (0, 0).
		 */
		const rect2f extents;
};




/**
 *************************************************************************
 *
 * @class gui_font_style
 * @ingroup gui_module
 *
 * A bitmap font as used for in-gui display.
 *
 ************************************************************************/
class gui_font_style :
	public gui_style
{
	public:

		gui_font_style
			(const shared_instance<render_material_id>& material,
			 const shared_instance<font>& f);


		const shared_instance<render_material_id> material;


		/**
		 * Pointer to font layout, never @NULL.
		 * Shared pointer to avoid expensive font copies
		 * in each gui_element or each gui_style.
		 */
		const shared_instance<font> f;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_STYLE_UTIL_HPP) */

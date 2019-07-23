/**
 *************************************************************************
 *
 * @file gui_style_util.cpp
 *
 * GUI utility styles, implementation.
 *
 ************************************************************************/


#include "gui_style_util.hpp"


#include <viral_core/log.hpp>


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_cursor_style
//
//////////////////////////////////////////////////////////////////////////


gui_cursor_style::gui_cursor_style
	(const shared_instance<render_material_id>& material,
	 const rect2f& extents)
	:
	material(material), extents(extents)
{ }




//////////////////////////////////////////////////////////////////////////
//
// gui_font_style
//
//////////////////////////////////////////////////////////////////////////


gui_font_style::gui_font_style
	(const shared_instance<render_material_id>& material,
	 const shared_instance<font>& f)
	:
	material(material), f(f)
{ }




} /* namespace viral_gui */

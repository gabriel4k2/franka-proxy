/**
 *************************************************************************
 *
 * @file gui_tooltip_data.hpp
 *
 * Utilities for gui_tooltip data, implementation.
 *
 ************************************************************************/


#include "gui_tooltip_data.hpp"


#include "gui_tooltip.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_tooltip_data
//
//////////////////////////////////////////////////////////////////////////


gui_tooltip_data::~gui_tooltip_data() NOTHROW { }


gui_tooltip_data::gui_tooltip_data(const string& text)
	: text_(text) { }


void gui_tooltip_data::apply(gui_tooltip& tooltip)
{
	tooltip.set_text(text_);

	// Clear image for base tooltip.
	// Relevant when clients remove the image
	// from an existing tooltip.
	tooltip.clear_image();
}




//////////////////////////////////////////////////////////////////////////
//
// gui_tooltip_data_material
//
//////////////////////////////////////////////////////////////////////////


gui_tooltip_data_material::gui_tooltip_data_material
	(const string& text,
	 const shared_instance<render_material_id>& material,
	 const vector2i& image_size)
	:
	gui_tooltip_data(text),
	material_(material),
	image_size_(image_size)
{ }


void gui_tooltip_data_material::apply(gui_tooltip& tooltip)
{
	/** Basic exception safety only. */
	gui_tooltip_data::apply(tooltip);
	tooltip.set_image(material_, image_size_);
}




//////////////////////////////////////////////////////////////////////////
//
// gui_tooltip_data_image
//
//////////////////////////////////////////////////////////////////////////


gui_tooltip_data_image::gui_tooltip_data_image
	(const string& text,
	 const image_rgba& source_image,
	 const render_material_data& material_data)
	:
	gui_tooltip_data(text),
	source_image_(source_image),
	material_data_(material_data)
{ }

	
void gui_tooltip_data_image::apply(gui_tooltip& tooltip)
{
	/** Basic exception safety only. */
	gui_tooltip_data::apply(tooltip);
	tooltip.set_image(source_image_, material_data_);
}




} /* namespace viral_gui */

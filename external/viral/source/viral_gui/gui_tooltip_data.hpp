/**
 *************************************************************************
 *
 * @file gui_tooltip_data.hpp
 *
 * Utilities for gui_tooltip data.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_TOOLTIP_DATA_HPP)
#define INCLUDED__VIRAL_GUI__GUI_TOOLTIP_DATA_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/shared_pointer.hpp>
#include <viral_core/string.hpp>
#include <viral_core/image_raw.hpp>
#include <viral_core/geo_2d.hpp>
#include <viral_core/render_resource.hpp>


#include "gui_forward.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_tooltip_data
 * @ingroup gui_module
 *
 * A gui_tooltip data entry in a gui_tooltip_area.
 *
 ************************************************************************/
class gui_tooltip_data
{
	friend class gui_tooltip_area;


	public:

		virtual ~gui_tooltip_data() NOTHROW;


	protected:

		gui_tooltip_data(const string& text);

		virtual void apply(gui_tooltip& tooltip);


	private:

		const string text_;
};




/**
 *************************************************************************
 *
 * @class gui_tooltip_data_material
 * @ingroup gui_module
 *
 * A gui_tooltip_data that uses @bref{material_} with @bref{image_size_}.
 *
 ************************************************************************/
class gui_tooltip_data_material :
	public gui_tooltip_data
{
	friend class gui_tooltip_area;


	protected:

		gui_tooltip_data_material
			(const string& text,
			 const shared_instance<render_material_id>& material,
			 const vector2i& image_size);

		void apply(gui_tooltip& tooltip) override;


	private:

		const shared_instance<render_material_id> material_;
		const vector2i image_size_;
};




/**
 *************************************************************************
 *
 * @class gui_tooltip_data_image
 * @ingroup gui_module
 *
 * A gui_tooltip_data which wraps an image_rgba into @bref{material_data_}.
 *
 ************************************************************************/
class gui_tooltip_data_image :
	public gui_tooltip_data
{
	friend class gui_tooltip_area;


	protected:

		gui_tooltip_data_image
			(const string& text,
			 const image_rgba& source_image,
			 const render_material_data& material_data);

		void apply(gui_tooltip& tooltip) override;


	private:

		const image_rgba source_image_;
		const render_material_data material_data_;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_TOOLTIP_DATA_HPP) */

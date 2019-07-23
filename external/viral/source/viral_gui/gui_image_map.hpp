/**
 *************************************************************************
 *
 * @file gui_image_map.hpp
 *
 * Clickable image maps.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_IMAGE_MAP_HPP)
#define INCLUDED__VIRAL_GUI__GUI_IMAGE_MAP_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/image_forward.hpp>


#include "gui_event.hpp"
#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_image_map
 * @ingroup gui_module
 *
 * Shows a clickable gui_image within a gui_frame.
 *
 ************************************************************************/
class gui_image_map :
	public gui_element
{
	public:

		gui_image_map
			(const string& name, gui_context& gui,
			 const shared_instance<gui_image_map_style>& image_map_style,
			 const shared_instance<render_material_id>& material,
			 const vector2i& image_size,
			 bool continuous_events);

		gui_image_map
			(const string& name, gui_context& gui,
			 const shared_instance<gui_image_map_style>& image_map_style,
			 const image_rgba& source_image,
			 const render_material_data& material_data,
			 bool continuous_events);


		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		/** Pass on source material or image to internal gui_image. */
		//@{ 
		void apply_material
			(const shared_instance<render_material_id>& material,
			 const vector2i& image_size);
		void apply_source_image
			(const image_rgba& source_image,
			 const render_material_data& material_data);
		//@}


		const shared_instance<gui_image_map_style> image_map_style;


		/**
		 * If set, send gui_image_map_event%s while dragging.
		 * Otherwise, send events only on click.
		 */
		const bool continuous_events;


	private:

		void realize_layout() override;

		void process_image_map_clicks
			(gui_input_event& input,
			 gui_output_event_list& output);
		void process_image_map_dragging
			(gui_input_event& input,
			 gui_output_event_list& output);

		void create_image_map_event
			(const gui_mouse_event& mouse_input,
			 gui_output_event_list& output,
			 const rect2f& image_arrange);


		static const float clickable_size_epsilon_;

		bool dragging_;

		const shared_instance<gui_image> image_;
		const shared_instance<gui_frame> frame_;
};




/**
 *************************************************************************
 *
 * @class gui_image_map_event
 * @ingroup gui_module
 *
 * Sent when the user clicked some pixel within a gui_image_map.
 *
 ************************************************************************/
class gui_image_map_event :
	public gui_output_event
{
	public:

		gui_image_map_event
			(gui_image_map& source,
			 const vector2f& normalized_position,
			 const vector2f& relative_gui_position)
			:
			gui_output_event(source),

			source(source.fetch_shared_from_parent()
						 .try_cast<gui_image_map>()
						 .instance_or_throw()),

			normalized_position(normalized_position),
			relative_gui_position(relative_gui_position)
		{ }

		const shared_instance<gui_image_map> source;
		const vector2f normalized_position;
		const vector2f relative_gui_position;
};




/**
 *************************************************************************
 *
 * @class gui_image_map_style
 * @ingroup gui_module
 *
 * Styling for a gui_image_map.
 *
 ************************************************************************/
class gui_image_map_style :
	public gui_style
{
	public:

		gui_image_map_style
			(const shared_instance<gui_image_style>& image_style,
			 const shared_instance<gui_frame_style>& frame_style);

		const shared_instance<gui_image_style> image_style;
		const shared_instance<gui_frame_style> frame_style;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_IMAGE_MAP_HPP) */
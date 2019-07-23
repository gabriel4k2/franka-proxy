/**
 *************************************************************************
 *
 * @file gui_value_edit.hpp
 *
 * Combined sliders and text edit boxes.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_VALUE_EDIT_HPP)
#define INCLUDED__VIRAL_GUI__GUI_VALUE_EDIT_HPP


#include <viral_core/build_config.hpp>


#include "gui_element.hpp"
#include "gui_slider.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_value_edit_converter
 * @ingroup gui_module
 *
 * Converts text within a gui_value_edit to slider values and vice versa.
 *
 ************************************************************************/
class gui_value_edit_converter :
	private non_copyable
{
	friend class gui_value_edit;

	public:

		virtual ~gui_value_edit_converter() NOTHROW { }

	private:

		/**
		 * Convert text to value or vice versa.
		 * On failure, both calls can throw an exception
		 * to undo any recent value or text changes.
		 */
		//@{
		virtual float to_value(const string& text) = 0;
		virtual string to_text(float value) = 0;
		//@}
};




/**
 *************************************************************************
 *
 * @class gui_value_edit
 * @ingroup gui_module
 *
 * A gui_slider that shows its current value in a gui_editbox.
 *
 * Layout notes:
 * - A value edit places a growing slider next to a fixed-size edit box.
 *		See both gui_slider and gui_editbox for individual layouting.
 * - To achieve a specific edit box size, the gui_scroll_area_style
 *		for the edit box should @bref{prefer_override_size} alongside
 *		a suitable @bref{override_size}.
 *
 * Text-value conversion:
 * - By default, a value edit interprets text() in its editbox()
 *		as a floating point value() in slider() range. Likewise,
 *		a default value edit converts floating-point slider values
 *		to text with implementation-specific precision.
 * - Clients can use set_converter() to customize value-to-text conversion.
 *		Applications range from values with units to enum-like value names.
 * - In general, the value edit synchronizes its value() and its text().
 *		However, in rare cases (such as code-provoked changes directly
 *		to slider() or editbox(), or repeated failure during conversion),
 *		value() and text() may be inconsistent. In this case, client code
 *		can try resynchronizing by calling either try_text_from_value()
 *		or try_value_from_text().
 *
 ************************************************************************/
class gui_value_edit :
	public gui_element
{
	public:

		gui_value_edit
			(const string& name, gui_context& gui,
			 const shared_instance<gui_value_edit_style>& value_edit_style,
			 orientation_type orientation,
			 gui_slider::snap_mode snap);


		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;
	

		/** Enable or disable user GUI input. */
		//@{
		void set_read_only(bool read_only) NOTHROW;
		bool read_only() const NOTHROW;
		//@}


		/**
		 * Access to text() and value().
		 * Setters synchronize text() and value() through converter()
		 * or default conversion behaviour. On failure, both setters
		 * consume any exceptions and try to revert the operation.
		 * Both setter calls return @false on failure, @true on success.
		 */
		//@{
		bool try_set_value(float value) NOTHROW; 
		float value() const NOTHROW;
		bool try_set_text(const string& text) NOTHROW;
		string text() const NOTHROW;
		//@}


		/**
		 * Try to synchronize current text() and value().
		 * Shorthand for try_set_value(value()) or try_set_text(text()).
		 */
		//@{
		bool try_text_from_value() NOTHROW;
		bool try_value_from_text() NOTHROW;
		//@}


		/**
		 * Set a new text-value @bref{converter}
		 * and destroy any previous converter().
		 * Setting a @NULL converter is valid
		 * and restores default conversions.
		 */
		void set_converter
			(auto_pointer<gui_value_edit_converter> converter) NOTHROW;


		/**
		 * Get current text-value converter,
		 * or @NULL for default conversion.
		 */
		gui_value_edit_converter* converter() NOTHROW;


		gui_slider& slider() NOTHROW;
		const gui_slider& slider() const NOTHROW;
		gui_editbox& editbox() NOTHROW;
		const gui_editbox& editbox() const NOTHROW;


		const shared_instance<gui_value_edit_style> value_edit_style;
		const orientation_type orientation;


	private:

		void realize_layout() override;

		float default_to_value(const string& text);
		string default_to_text(float value);

		void restore_value_and_text
			(float old_value, const string& old_text) NOTHROW;


		static const int64 default_text_digits_;
		static const int64 default_edit_history_step_limit_;
		static const int64 default_edit_history_memory_limit_;

		bool monitor_editbox_edit_cancel_;

		const shared_instance<gui_slider> slider_;
		const shared_instance<gui_editbox> editbox_;

		auto_pointer<gui_value_edit_converter> converter_;
};


/**
 * Notifies clients that the user changed the value of a gui_value_edit.
 * @ingroup gui_module
 */
typedef gui_simple_output_event<gui_value_edit> gui_value_edit_event;




/**
 *************************************************************************
 *
 * @class gui_value_edit_style
 * @ingroup gui_module
 *
 * Styling for a combined gui_value_edit element.
 *
 ************************************************************************/
class gui_value_edit_style :
	public gui_style
{
	
	public:

		gui_value_edit_style
			(const shared_instance<gui_slider_style>& slider_style,
			 const shared_instance<gui_editbox_style>& editbox_style,
			 float slider_edit_spacing);

		const shared_instance<gui_slider_style> slider_style;
		const shared_instance<gui_editbox_style> editbox_style;
		const float slider_edit_spacing;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_VALUE_EDIT_HPP) */

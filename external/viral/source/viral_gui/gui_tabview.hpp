/**
 *************************************************************************
 *
 * @file gui_tabview.hpp
 *
 * GUI tabview element.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_TABVIEW_HPP)
#define INCLUDED__VIRAL_GUI__GUI_TABVIEW_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/util.hpp>
#include <viral_core/array.hpp>
#include <viral_core/tree_set.hpp>


#include "gui_list.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_tabview_header
 * @ingroup gui_module
 *
 * A single header item within a gui_tabview.
 *
 ************************************************************************/
class gui_tabview_header :
	public gui_list_item
{
	friend class gui_tabview;

	private:

		gui_tabview_header
			(const string& caption,
			 const shared_pointer<render_material_id>& icon,
			 const shared_instance<gui_element>& page)
			:
			gui_list_item(caption, icon),
			page(page)
		{ }

		const shared_instance<gui_element> page;
};




/**
 *************************************************************************
 *
 * @class gui_tabview
 * @ingroup gui_module
 *
 * Shows a list of page headers and a single selected page.
 *
 * Layout notes:
 * - A tabview prefers space for the header list, the largest page(),
 *		and the splitter bar in-between headers and pages.
 * - A tabview clips on less space than preferred, and the splitter
 *		allows to distribute excess space between headers and pages.
 * - To enable scrolling on tabview headers, override the preferred size
 *		within gui_tabview_style::header_scroll_area_style. Otherwise,
 *		the tabview prefers size for all headers and clips on less space.
 * - Tabview pages do not automatically provide scrolling, but instead
 *		clients must explicitly create suitable gui_scroll_area%s.
 *
 ************************************************************************/
class gui_tabview :
	public gui_element
{
	public:

		gui_tabview
			(const string& name, gui_context& gui,
			 const shared_instance<gui_tabview_style>& tabview_style,
			 orientation_type orientation);


		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		/**
		 * Insert a new tabview @bref{page} under a header
		 * with @bref{caption} and optional @bref{icon}.
		 * Returns the index of the new page.
		 */
		//@{
		int64 insert_page
			(const string& caption,
			 const shared_instance<gui_element>& page);
		int64 insert_page
			(const string& caption,
			 const shared_pointer<render_material_id>& icon,
			 const shared_instance<gui_element>& page);
		//@}


		/** Retrieve total number of tabview pages */
		int64 page_count() const NOTHROW;

		/** Get index of currently selected page. */
		int64 selected_page() const NOTHROW;


		/**
		 * Fetch, show, or remove page at given, valid @bref{index}.
		 * Silently ignore invalid indices and return @NULL page().
		 */
		//@{
		shared_pointer<gui_element> page(int64 index) const NOTHROW;
		void show_page(int64 index) NOTHROW;
		void remove_page(int64 index) NOTHROW;
		//@}


		/**
		 * Allow or prohibit selection of the page
		 * at @bref{index} by means of user input.
		 * Silently ignores invalid indices.
		 *
		 * Currently visible pages remain visible
		 * even if their selection is prohibited,
		 * unless another page is shown explicitly.
		 * Finally, code (e.g. show_page()) still
		 * can show disabled pages.
		 */
		//@{
		void enable_page(int64 index) NOTHROW;
		void disable_page(int64 index) NOTHROW;
		//@}


		/**
		 * Access the splitter between headers and pages
		 * as by respective gui_split methods.
		 */
		//@{
		void set_split_read_only(bool split_read_only);
		bool split_read_only() const;
		void set_split_normalized(float split_normalized);
		float split_normalized() const;
		//@}


		const shared_instance<gui_tabview_style> tabview_style;
		const orientation_type orientation;


	private:

		void realize_layout() override;


		int64 selected_page_;

		const shared_instance<gui_split> header_page_split_;
		const shared_instance<gui_scroll_area> header_scroll_;
		const shared_instance<gui_list> header_list_;
		const shared_instance<gui_tabview_switch> tabview_switch_;
};




/**
 *************************************************************************
 *
 * @class gui_tabview_switch
 * @ingroup gui_module
 *
 * Switches pages shown within the gui_split of a gui_tabview.
 *
 ************************************************************************/
class gui_tabview_switch :
	public gui_element
{
	friend class gui_tabview;

	public:

		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


	private:

		gui_tabview_switch
			(const string& name, gui_context& gui);

		void realize_layout() override;

		void insert_page(const shared_pointer<gui_element>& page);
		void select_page(const shared_pointer<gui_element>& page) NOTHROW;
		void remove_page(const shared_pointer<gui_element>& page) NOTHROW;


		typedef tree_set<shared_instance<gui_element>> page_set;
		page_set pages_;

		shared_pointer<gui_element> current_page_;
};




/**
 *************************************************************************
 *
 * @class gui_tabview_style
 * @ingroup gui_module
 *
 * Styling for a gui_tabview element.
 *
 ************************************************************************/
class gui_tabview_style :
	public gui_style
{
	public:

		gui_tabview_style
			(const shared_instance<gui_split_style>& split_style,
			 const shared_instance<gui_scroll_area_style>& header_scroll_area_style,
			 const shared_instance<gui_list_style>& header_list_style);

		const shared_instance<gui_split_style> split_style;
		const shared_instance<gui_scroll_area_style> header_scroll_area_style;
		const shared_instance<gui_list_style> header_list_style;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_TABVIEW_HPP) */

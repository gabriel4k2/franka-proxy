/**
 *************************************************************************
 *
 * @file gui_split.hpp
 *
 * GUI splitter panes.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_SPLIT_HPP)
#define INCLUDED__VIRAL_GUI__GUI_SPLIT_HPP


#include <viral_core/build_config.hpp>


#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_split
 * @ingroup gui_module
 *
 * Separates its arrange_area() into two resizeable panes.
 *
 ************************************************************************/
class gui_split :
	public gui_element
{
	public:

		gui_split
			(const string& name, gui_context& gui,
			 const shared_instance<gui_split_style>& split_style,
			 orientation_type orientation);


		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		/** Read-only mode disallows mouse dragging. */
		//@{
		void set_read_only(bool read_only) NOTHROW;
		bool read_only() const NOTHROW;
		//@}


		/**
		 * Access split coefficient, normalized to [0, 1].
		 * Sliently clamps invalid values.
		 */
		//@{
		void set_split_normalized(float split_normalized);
		float split_normalized() const;
		//@}


		void set_pos_content(const shared_pointer<gui_element>& pos_content);
		const shared_pointer<gui_element>& pos_content() const NOTHROW;
		void clear_pos_content() NOTHROW;

		void set_neg_content(const shared_pointer<gui_element>& neg_content);
		const shared_pointer<gui_element>& neg_content() const NOTHROW;
		void clear_neg_content() NOTHROW;


		/**
		 * Not-@NULL linked splits that automatically
		 * synchronize split_normalized() on movement.
		 *
		 * Usage notes:
		 * - The @bref{links} list may contain the current split.
		 * - On-screen splitter positions depend on individual content size,
		 *		and thus need not match even if splitters are linked.
		 */
		//@{
		typedef list<weak_pointer<gui_split>> link_list;
		void set_links(const link_list& links);
		const link_list& links() const;
		//@}
		

		const shared_instance<gui_split_style> split_style;
		const orientation_type orientation;


	private:

		void realize_layout() override;
		void synchronize_links();


		shared_pointer<gui_element> neg_content_;
		shared_pointer<gui_element> pos_content_;

		const shared_instance<gui_knob> splitter_;

		link_list links_;
};


/**
 * Notifies clients that the user moved a GUI split.
 * @ingroup gui_module
 */
typedef gui_simple_output_event<gui_split> gui_split_event;




/**
 *************************************************************************
 *
 * @class gui_split_style
 * @ingroup gui_module
 *
 * Styling for a gui_split pane.
 *
 ************************************************************************/
class gui_split_style :
	public gui_style
{
	public:

		gui_split_style
			(const shared_instance<gui_knob_style>& knob_style,
			 float splitter_size,
			 float min_pane_size,
			 bool ensure_content_size_neg,
			 bool ensure_content_size_pos);


		const shared_instance<gui_knob_style> knob_style;
		const float splitter_size;
		const float min_pane_size;


		/**
		 * Limit minimum pane size to preferred content size
		 * in addition to @bref{minimum_pane_size} and consider
		 * content size in gui_split::preferred_size().
		 */
		//@{
		const bool ensure_content_size_neg;
		const bool ensure_content_size_pos;
		//@}
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_SPLIT_HPP) */
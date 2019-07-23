/**
 *************************************************************************
 *
 * @file gui_grid.hpp
 *
 * GUI grid layout.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_GRID_HPP)
#define INCLUDED__VIRAL_GUI__GUI_GRID_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/array.hpp>


#include "gui_element.hpp"
#include "gui_anchor.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_grid_cell
 * @ingroup gui_module
 *
 * Utility class that stores a single gui_grid entry.
 *
 ************************************************************************/
class gui_grid_cell
{
	friend class gui_grid;

	public:

		gui_grid_cell();

	private:

		vector2f preferred_size() const;

		void layout
			(const rect2f& arrange_area,
			 const rect2f& clip_area);
		void clip_entirely();

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output);
		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue);

		shared_pointer<gui_element> content_;

		vector2i stretch_neg_;
		vector2i stretch_pos_;

		shared_instance<gui_grid_cell_style> grid_cell_style_;
};




/**
 *************************************************************************
 *
 * @class gui_grid
 * @ingroup gui_module
 *
 * Arranges gui_element%s in a constant-sized grid.
 *
 * Usage notes:
 * - A grid is not optimized for a large number of grid cells.
 *		In particular, most grid operations run in O(@bref{size}),
 *		even if many contained elements are clipped.
 *
 * Layout notes:
 * - A grid reports preferred_size() over all contained GUI elements.
 *		Any excess size within the arrange_area() of a grid distributes
 *		over grid cells based on individual row or column weights.
 * - If the parent element hands down a smaller arrange_area(),
 *		content is clipped and does not scale down any further.
 *		A parent gui_scroll_area resolves this problem.
 * - Cells with gui_spacer%s enforce white space within a grid.
 *
 ************************************************************************/
class gui_grid :
	public gui_element
{

	public:

		gui_grid
			(const string& name, gui_context& gui,
			 const vector2i& size);


		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		/**
		 * Insert new @bref{content} at grid @bref{position}.
		 * Replaces any existing content at this position.
		 */
		void insert_content
			(const shared_pointer<gui_element>& content,
			 const vector2i& position);


		/**
		 * Fetch content at given grid position,
		 * or @NULL if there is no content there.
		 */
		shared_pointer<gui_element> content
			(const vector2i& position) const NOTHROW;
		

		/**
		 * Remove content at grid @bref{position} or all content.
		 * Silently ignore errors for @ref NOTHROW behaviour.
		 */
		//@{
		void remove_content(const vector2i& position) NOTHROW;
		void clear_content() NOTHROW;
		//@}


		/**
		 * Make any content() at @bref{position} arrange over
		 * @bref{stretch_pos} adjacent cells to the bottom-right
		 * or @bref{stretch_neg} adjacent cells to the top-left.
		 *
		 * Usage notes:
		 * - Cell content only requests its preferred_size()
		 *		for the row and column at @bref{position},
		 *		and does not consider any stretching.
		 * - On invalid parameters, setters throw an exception,
		 *		while queries return a default, zero stretch.
		 */
		//@{
		void set_stretch_pos
			(const vector2i& position, const vector2i& stretch_pos);
		vector2i stretch_pos
			(const vector2i& position) const NOTHROW;		

		void set_stretch_neg
			(const vector2i& position, const vector2i& stretch_neg);
		vector2i stretch_neg
			(const vector2i& position) const NOTHROW;

		void set_stretch_left(const vector2i& position, int64 stretch_left);
		void set_stretch_right(const vector2i& position, int64 stretch_right);
		void set_stretch_up(const vector2i& position, int64 stretch_up);
		void set_stretch_down(const vector2i& position, int64 stretch_down);
		//@}


		/**
		 * Set the style of some individual grid cell
		 * at a valid @bref{position}.
		 */
		void set_grid_cell_style
			(const vector2i& position,
			 const shared_instance<gui_grid_cell_style>& cell_style);


		/**
		 * Access positive weight of an existing grid row or column.
		 *
		 * Usage notes:
		 * - Given additional @bref{n} pixels in arrange_area(),
		 *		each row or column grows by @bref{n} * @bref{weight} /
		 *		@bref{sum_over_all_weights} pixels.
		 * - A (default) weight of 0 indicates constant-size elements.
		 *		Weight queries also return 0 on invalid indices.
		 */
		//@{
		void set_row_weight(int64 index, float weight);
		float row_weight(int64 index) const NOTHROW;
		void set_column_weight(int64 index, float weight);
		float column_weight(int64 index) const NOTHROW;
		//@}


		const vector2i size;


	private:

		void realize_layout() override;

		bool position_valid(const vector2i& position) const NOTHROW;


		typedef array<gui_grid_cell> cell_array;
		cell_array cells_;

		typedef array<float> weight_array;
		weight_array row_weights_;
		weight_array column_weights_;
};




/**
 *************************************************************************
 *
 * @class gui_grid_cell_style
 * @ingroup gui_module
 *
 * Styling for individual cells in a gui_grid.
 *
 ************************************************************************/
class gui_grid_cell_style :
	public gui_style
{
	public:

		gui_grid_cell_style
			(const vector2f& padding_00,
			 const vector2f& padding_22,
			 gui_anchor::anchor_type horizontal_anchor,
			 gui_anchor::anchor_type vertical_anchor);
		

		/**
		 * Padding around the content of the grid cell
		 * to the top-left and bottom-right.
		 */
		//@{
		const vector2f padding_00;
		const vector2f padding_22;
		//@}


		/**
		 * Anchoring of content in a grid cell,
		 * as by a gui_anchor element.
		 */
		//@{
		const gui_anchor::anchor_type horizontal_anchor;
		const gui_anchor::anchor_type vertical_anchor;
		//@}
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_GRID_HPP) */
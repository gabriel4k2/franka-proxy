/**
 *************************************************************************
 *
 * @file gui_grid.cpp
 *
 * GUI grid layout, implementation.
 *
 ************************************************************************/


#include "gui_grid.hpp"


#include <viral_core/log.hpp>
#include <viral_core/buffer.hpp>


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_grid_cell
//
//////////////////////////////////////////////////////////////////////////


gui_grid_cell::gui_grid_cell()
	:
	grid_cell_style_
		(new gui_grid_cell_style
			(vector2f(), vector2f(),
			 gui_anchor::anchor_fill, gui_anchor::anchor_fill))
{ }


vector2f gui_grid_cell::preferred_size() const
{
	// Note that padding and anchoring
	// is opaque for the parent gui_grid,
	// as we hand on padded content size.
	return
		grid_cell_style_().padding_00 +
		grid_cell_style_().padding_22 +
		(content_ ? content_->preferred_size() : vector2f());
}


void gui_grid_cell::layout
	(const rect2f& arrange_area,
	 const rect2f& clip_area)
{
	if (!content_) return;

	rect2f padded_arrange
		(arrange_area.pos + grid_cell_style_().padding_00,
		 arrange_area.size - grid_cell_style_().padding_00
						   - grid_cell_style_().padding_22);

	if (padded_arrange.size.min() < 0)
	{
		content_->clip_entirely();
		return;
	}

	rect2f anchored_arrange
		(gui_anchor::build_layout
			(content_->preferred_size(), padded_arrange,
			 grid_cell_style_().horizontal_anchor,
			 grid_cell_style_().vertical_anchor));

	content_->layout(anchored_arrange, clip_area);
}


void gui_grid_cell::clip_entirely()
	{ if (content_) content_->clip_entirely(); }


void gui_grid_cell::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
		{ if (content_) content_->process_input(input, output); }

void gui_grid_cell::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
		{ if (content_) content_->commit_render(packer, queue); }




//////////////////////////////////////////////////////////////////////////
//
// gui_grid
//
//////////////////////////////////////////////////////////////////////////


gui_grid::gui_grid
	(const string& name, gui_context& gui,
	 const vector2i& size)
	:
	gui_element(name, gui),
	size(size)
{
	cells_.resize(size.x * size.y);
	row_weights_.resize(size.y);
	column_weights_.resize(size.x);
}


vector2f gui_grid::preferred_size() const
{
	vector2f preferred;

	// Sum maximum horizontal size required for each column.
	for (int64 x = 0; x < size.x; ++x)
	{
		float column_max = 0;
		for (int64 y = 0; y < size.y; ++y)
			column_max =
				max(column_max,
					cells_[y * size.x + x].preferred_size().x);
	
		preferred.x += column_max;
	}

	// Sum maximum vertical size required for each row.
	for (int64 y = 0; y < size.y; ++y)
	{
		float row_max = 0;
		for (int64 x = 0; x < size.x; ++x)
			row_max =
				max(row_max,
					cells_[y * size.x + x].preferred_size().y);

		preferred.y += row_max;
	}
	
	return preferred;
}


void gui_grid::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	for (int64 i = 0; i < cells_.size(); ++i)
		cells_[i].process_input(input, output);
}


void gui_grid::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;

	for (int64 i = 0; i < cells_.size(); ++i)
		cells_[i].commit_render(packer, queue);
}


void gui_grid::insert_content
	(const shared_pointer<gui_element>& content,
	 const vector2i& position)
{
	if (!position_valid(position))
	{
		LOG_ERROR("Cannot insert content at invalid index: " +
				  position.x + " " + position.y);
		throw bounds_exceeded();
	}

	if (content)
		insert_child(content.instance_or_throw());

	// Exception safety: NOTHROW from here.
	int64 index = position.y * size.x + position.x;

	if (cells_[index].content_)
		remove_child(cells_[index].content_.instance_or_terminate());

	cells_[index].content_ = content;
	want_layout();
}


shared_pointer<gui_element> gui_grid::content
	(const vector2i& position) const NOTHROW
{
	if (!position_valid(position))
	{
		LOG_WARN("Querying for content at invalid index: " +
				 position.x + " " + position.y);
		return shared_pointer<gui_element>(); // NOTHROW
	}

	return cells_[position.y * size.x + position.x].content_;
}


void gui_grid::remove_content(const vector2i& position) NOTHROW
{
	if (!position_valid(position))
	{
		LOG_ERROR("Cannot remove content at invalid index: " +
				  position.x + " " + position.y);
		return; // NOTHROW
	}

	int64 index = position.y * size.x + position.x;

	if (cells_[index].content_)
		remove_child(cells_[index].content_.instance_or_terminate());

	cells_[index].content_.reset();
	want_layout();	
}


void gui_grid::clear_content() NOTHROW
{
	for (int64 i = 0; i < size.x * size.y; ++i)
	{
		if (cells_[i].content_)
			remove_child(cells_[i].content_.instance_or_terminate());

		cells_[i].content_.reset();
	}

	want_layout();
}


void gui_grid::set_stretch_pos
	(const vector2i& position, const vector2i& stretch_pos)
{
	if (!position_valid(position))
	{
		LOG_ERROR("Cannot set stretching at invalid index: " +
				  position.x + " " + position.y);
		throw bounds_exceeded();
	}

	if (!position_valid(position + stretch_pos))
	{
		LOG_ERROR("Cannot stretch to invalid index.");
		throw bounds_exceeded();
	}

	cells_[position.y * size.x + position.x].stretch_pos_ = stretch_pos;
	want_layout();
}


vector2i gui_grid::stretch_pos
	(const vector2i& position) const NOTHROW
{
	if (!position_valid(position))
	{
		LOG_ERROR("Cannot fetch stretching at invalid index: " +
				  position.x + " " + position.y);
		return vector2i();
	}

	return
		cells_[position.y * size.x + position.x].stretch_pos_;
}


void gui_grid::set_stretch_neg
	(const vector2i& position, const vector2i& stretch_neg)
{
	if (!position_valid(position))
	{
		LOG_ERROR("Cannot set stretching at invalid index: " +
				  position.x + " " + position.y);
		throw bounds_exceeded();
	}

	if (!position_valid(position - stretch_neg))
	{
		LOG_ERROR("Cannot stretch to invalid index.");
		throw bounds_exceeded();
	}

	cells_[position.y * size.x + position.x].stretch_neg_ = stretch_neg;
	want_layout();
}


vector2i gui_grid::stretch_neg
	(const vector2i& position) const NOTHROW
{
	if (!position_valid(position))
	{
		LOG_ERROR("Cannot fetch stretching at invalid index: " +
				  position.x + " " + position.y);
		return vector2i();
	}

	return
		cells_[position.y * size.x + position.x].stretch_neg_;
}



void gui_grid::set_stretch_left(const vector2i& position, int64 stretch_left)
	{ set_stretch_neg(position, vector2i(stretch_left, stretch_neg(position).y)); }
void gui_grid::set_stretch_right(const vector2i& position, int64 stretch_right)
	{ set_stretch_pos(position, vector2i(stretch_right, stretch_pos(position).y)); }

void gui_grid::set_stretch_up(const vector2i& position, int64 stretch_up)
	{ set_stretch_neg(position, vector2i(stretch_neg(position).x, stretch_up)); }
void gui_grid::set_stretch_down(const vector2i& position, int64 stretch_down)
	{ set_stretch_pos(position, vector2i(stretch_pos(position).x, stretch_down)); }


void gui_grid::set_grid_cell_style
	(const vector2i& position,
	 const shared_instance<gui_grid_cell_style>& grid_cell_style)
{
	if (!position_valid(position))
	{
		LOG_ERROR("Cannot set cell style at invalid index: " +
				  position.x + " " + position.y);
		throw bounds_exceeded();
	}

	cells_[position.y * size.x + position.x].grid_cell_style_ =
		grid_cell_style;

	want_layout();
}


void gui_grid::set_row_weight(int64 index, float weight)
{
	if (index < 0 || index >= size.y)
	{
		LOG_ERROR("Row index outside grid bounds.");
		throw bounds_exceeded();
	}

	if (weight < 0)
	{
		LOG_ERROR("Cannot set negative row weight.");
		throw invalid_parameter();
	}

	row_weights_[index] = weight;
	want_layout();
}


float gui_grid::row_weight(int64 index) const NOTHROW
{
	return (index < 0 || index >= size.y) ?
				0 : row_weights_[index];
}


void gui_grid::set_column_weight(int64 index, float weight)
{
	if (index < 0 || index >= size.x)
	{
		LOG_ERROR("Column index outside grid bounds.");
		throw bounds_exceeded();
	}

	if (weight < 0)
	{
		LOG_ERROR("Cannot set negative column weight.");
		throw invalid_parameter();
	}

	column_weights_[index] = weight;
	want_layout();
}


float gui_grid::column_weight(int64 index) const NOTHROW
{
	return (index < 0 || index >= size.x) ?
				0 : column_weights_[index];
}


void gui_grid::realize_layout()
{
	// Clip entire grid on invalid layout.
	if (layout_invalid())
	{
		for (int64 y = 0; y < size.y; ++y)
		for (int64 x = 0; x < size.x; ++x)
			cells_[y * size.x + x].clip_entirely();

		return;
	}


	// Layout valid, so arrange grid cells.
	rect2f ar_area(arrange_area());
	rect2f cl_area(clip_area());

	vector2f pref_size(preferred_size());
	vector2f additional_size
		((ar_area.size - pref_size).max(vector2f()));


	// Determine total column and row weights.
	float row_weight_sum = 0;
	for (int64 i = 0; i < row_weights_.size(); ++i)
		row_weight_sum += row_weights_[i];

	float column_weight_sum = 0;
	for (int64 i = 0; i < column_weights_.size(); ++i)
		column_weight_sum += column_weights_[i];


	// Determine size of each row and each column by maximum
	// size of respective padded content and column weight.
	dynamic_buffer<float> row_sizes;
	row_sizes.resize(size.y);
	memory::fill_with_zero_typed
		(row_sizes.data(), row_sizes.size());

	dynamic_buffer<float> column_sizes;
	column_sizes.resize(size.x);
	memory::fill_with_zero_typed
		(column_sizes.data(), column_sizes.size());

	for (int64 y = 0; y < size.y; ++y)
	for (int64 x = 0; x < size.x; ++x)
	{
		vector2f pref_size_cell
			(cells_[y * size.x + x].preferred_size());

		float cell_size_y = pref_size_cell.y;
		float cell_size_x = pref_size_cell.x;

		if (row_weights_[y] != 0)
			cell_size_y +=
				additional_size.y * row_weights_[y] / row_weight_sum;

		if (column_weights_[x] != 0)
			cell_size_x +=
				additional_size.x * column_weights_[x] / column_weight_sum;
		
		row_sizes[y] = max(row_sizes[y], cell_size_y);
		column_sizes[x] = max(column_sizes[x], cell_size_x);
	}


	// Arrange individual cells within the grid.
	float y_current = 0;
	for (int64 y = 0; y < size.y; ++y)
	{
		float x_current = 0;
		for (int64 x = 0; x < size.x; ++x)
		{
			gui_grid_cell& cell = cells_[y * size.x + x];

			rect2f cell_arrange
				(ar_area.pos + vector2f(x_current, y_current),
				 vector2f(column_sizes[x], row_sizes[y]));

			for (int64 xs = x - cell.stretch_neg_.x; xs <= x + cell.stretch_pos_.x; ++xs)
			{
				if (xs < 0 || xs >= size.x) continue;
				if (xs == x) continue;
				if (xs < x) cell_arrange.pos.x -= column_sizes[xs];					
				cell_arrange.size.x += column_sizes[xs];
			}

			for (int64 ys = y - cell.stretch_neg_.y; ys <= y + cell.stretch_pos_.y; ++ys)
			{
				if (ys < 0 || ys >= size.y) continue;
				if (ys == y) continue;
				if (ys < y) cell_arrange.pos.y -= row_sizes[ys];
				cell_arrange.size.y += row_sizes[ys];
			}

			rect2f cell_clip;
			if (cell_arrange.try_intersect(cl_area, cell_clip))
				cell.layout(cell_arrange, cell_clip);
			else
				cell.clip_entirely();

			x_current += column_sizes[x];
		}

		y_current += row_sizes[y];
	}
}


bool gui_grid::position_valid(const vector2i& position) const NOTHROW
{
	return position.x >= 0 && position.x < size.x &&
		   position.y >= 0 && position.y < size.y;
}




//////////////////////////////////////////////////////////////////////////
//
// gui_grid_cell_style
//
//////////////////////////////////////////////////////////////////////////


gui_grid_cell_style::gui_grid_cell_style
	(const vector2f& padding_00,
	 const vector2f& padding_22,
	 gui_anchor::anchor_type horizontal_anchor,
	 gui_anchor::anchor_type vertical_anchor)
	:
	padding_00(padding_00),
	padding_22(padding_22),
	horizontal_anchor(horizontal_anchor),
	vertical_anchor(vertical_anchor)
{
	if (padding_00.min() < 0 || padding_22.min() < 0)
	{
		LOG_ERROR("Padding of grid cells must not be negative.");
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */

/**
 *************************************************************************
 *
 * @file gui_label.cpp
 *
 * GUI labels with static text, implementation.
 *
 ************************************************************************/


#include "gui_label.hpp"


#include <viral_core/log.hpp>
#include <viral_core/mesh.hpp>


#include "gui.hpp"
#include "gui_render.hpp"
#include "gui_style_util.hpp"
#include "gui_anchor.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_label
//
//////////////////////////////////////////////////////////////////////////


gui_label::gui_label
	(const string& name, gui_context& gui,
	 const shared_instance<gui_label_style>& label_style,
	 const string& text)
	:
	gui_element(name, gui),
	label_style(label_style),
	text_(text)
{
	update_unclipped_text_extents();
}


vector2f gui_label::preferred_size() const
	{ return unclipped_text_extents_; }


void gui_label::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{ /** No action. */ }


void gui_label::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;
	pack_render_mesh(packer);
}


void gui_label::set_text(const string& text)
{
	// Early out to avoid overhead
	// of repeated render_mesh_ updates.
	if (text_ == text) return;

	text_ = text;

	update_unclipped_text_extents();
	render_mesh_.reset();
	want_layout();
}

const string& gui_label::text() const
	{ return text_; }

void gui_label::clear_text() NOTHROW
{
	/** A @ref NOTHROW variant of set_text(). */

	// Early out to avoid overhead
	// of repeated render_mesh_ updates.
	if (text_.empty()) return;

	text_.clear();

	update_unclipped_text_extents();
	render_mesh_.reset();
	want_layout();
}


void gui_label::realize_layout()
	{ render_mesh_.reset(); }


void gui_label::update_unclipped_text_extents() NOTHROW
{
	unclipped_text_extents_ =
		label_style().font_style().f().calculate_line_integral_extents(text_);
}


void gui_label::update_render_mesh()
{
	if (render_mesh_) return;


	// Apply ellipsis to any single-line text
	// so it adapts to available arrange space.
	rect2f ar_area(arrange_area());

	int64 unclipped_char_count =
		text_.contains('\n') ?
			text_.size() :
			label_style().font_style().f().find_letter_index(text_, ar_area.size);
				// Finds first partial letter, index of this letter
				// is the number of fully visible characters.

	string text_with_ellipsis
		((unclipped_char_count >= text_.size()) ? text_ :
			(text_.substring(0, max(0, unclipped_char_count - 3)) + "..."));


	// Anchor incoming text (potentially after ellipsis)
	// within available arrangement area. If enabled,
	// apply pixel-integral font position.
	vector2f text_size
		(label_style().font_style().f().calculate_line_integral_extents
			(text_with_ellipsis));

	vector2f anchor
		(gui_anchor::build_layout
			(text_size, ar_area, label_style().anchor_style()).pos);

	if (gui.pixel_integral_rendering)
		anchor = anchor.floor();


	// Typeset text to current clipping area.
	// Typesetting uses a local font coordinate system,
	// so we must clip in this system as well.
	// Finally, transfer mesh back to GUI space
	// for subsequent mesh packing.
	rect2f clip_area_in_font_coordinates
		(clip_area().pos - anchor, clip_area().size);

	mesh new_render_mesh
		(label_style().font_style().f().typeset_mesh
			(text_with_ellipsis, clip_area_in_font_coordinates));

	int64 vertex_count = new_render_mesh.vertex_count();
	for (int64 i = 0; i < vertex_count; ++i)
		new_render_mesh.vertices()[i] +=
			vector(anchor.x, anchor.y, 0);

	new_render_mesh.rebuild_boundings();


	// Exception safety:
	// Apply offhand copy. NOTHROW from here.
	render_mesh_.reset
		(new mesh(std::move(new_render_mesh)));
}


void gui_label::pack_render_mesh(gui_render_packer& packer)
{
	update_render_mesh();
	packer.pack_mesh
		(*render_mesh_, label_style().font_style().material);
}




//////////////////////////////////////////////////////////////////////////
//
// gui_label_style
//
//////////////////////////////////////////////////////////////////////////


gui_label_style::gui_label_style
	(const shared_instance<gui_font_style>& font_style,
	 const shared_instance<gui_anchor_style>& anchor_style)
	:
	font_style(font_style),
	anchor_style(anchor_style)
{ }




} /* namespace viral_gui */
/**
 *************************************************************************
 *
 * @file gui_frame.cpp
 *
 * Frames around other GUI elements, implementation.
 *
 ************************************************************************/


#include "gui_frame.hpp"


#include <viral_core/log.hpp>
#include <viral_core/integer_operations.hpp>
#include <viral_core/mesh.hpp>


#include "gui.hpp"
#include "gui_event.hpp"
#include "gui_render.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_frame
//
//////////////////////////////////////////////////////////////////////////


gui_frame::gui_frame
	(const string& name, gui_context& gui,
	 const shared_instance<gui_frame_style>& frame_style)
	:
	gui_element(name, gui),
	frame_style(frame_style)
{ }


vector2f gui_frame::preferred_size() const
{
	return frame_size_for_content_size
			(content_ ? content_->preferred_size() : vector2f());
}


void gui_frame::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	if (content_)
		content_->process_input(input, output);

	// Consume all mouse input within borders
	// to prohibit clicking through the frame.
	// Also acquire key focus on hot clicks.
	if (!input.hot) return;
	if (layout_invalid()) return;
	if (typeid(input) != typeid(gui_mouse_event)) return;

	gui_mouse_event& mouse_event =
		static_cast<gui_mouse_event&>(input);

	rect2f border_area;
	rect2f border_clip;
	if (!build_border_layout
			(content_ ? content_->preferred_size() : vector2f(),
			 border_area, border_clip)) return;

	if (border_clip.contains(mouse_event.cursor))
	{
		if (mouse_event.button == gui_mouse_event::left_button &&
			mouse_event.event == gui_mouse_event::button_click)
				acquire_key_focus();

		input.hot = false;
	}
}


void gui_frame::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;

	pack_frame(packer);

	if (content_)
		content_->commit_render(packer, queue);
}


void gui_frame::set_content
	(const shared_pointer<gui_element>& content)
{
	if (content)
		insert_child(content.instance_or_throw());

	// Exception safety: NOTHROW from here.
	if (content_)
		remove_child(content_.instance_or_terminate());

	content_ = content;
	want_layout();
}

const shared_pointer<gui_element>&
	gui_frame::content() const NOTHROW
		{ return content_; }

void gui_frame::clear_content() NOTHROW
{
	if (content_)
		remove_child(content_.instance_or_terminate());

	content_.reset();
	want_layout();
}


vector2f gui_frame::frame_size_for_content_size
	(const vector2f& content_size) const
{
	// Determine size required only for display of
	// frame margin and borders, as these always show.
	// Note negative margins need not be allocated,
	// but cancel out (always positive) border size.
	vector2f margin_borders
		((frame_style().margin_00 + frame_style().border_00 +
		  frame_style().border_22 + frame_style().margin_22).max(vector2f()));


	// Determine size required for padding and content.
	// Again, negative padding overlaps margin, borders,
	// and potentially parent area, so need not be considered.
	// However, negative padding cancels out content size.
	vector2f padding_content
		((frame_style().padding_00 +
		  frame_style().padding_22 +
		  content_size).max(vector2f()));


	return
		margin_borders + padding_content;
}


bool gui_frame::build_border_layout
	(const vector2f& content_size,
	 rect2f& arrange, rect2f& clip) const
{
	if (layout_invalid()) return false;


	// Determine minimum arrange area required
	// for given content and surrounding frame.
	rect2f ar_area(arrange_area());
	ar_area.size =
		ar_area.size.max
			(frame_size_for_content_size(content_size));

	if (ar_area.size.min() <= 0)
		return false;


	// Build border arrangement area and
	// corresponding clipping area.
	rect2f arrange_temp
		(ar_area.pos + frame_style().margin_00,
		 ar_area.size -
			(frame_style().margin_00 + frame_style().margin_22));

	if (arrange_temp.size.min() <= 0)
		return false;

	rect2f clip_temp;
	if (!arrange_temp.try_intersect(clip_area(), clip_temp))
		return false;


	// Success: Hand areas back to caller.
	arrange = arrange_temp;
	clip = clip_temp;
	return true;
}


bool gui_frame::build_content_layout
	(const vector2f& content_size,
	 rect2f& arrange, rect2f& clip) const
{
	if (layout_invalid()) return false;


	// As build_border_layout(), but additionally
	// remove space for border and padding.
	rect2f ar_area(arrange_area());
	ar_area.size =
		ar_area.size.max
			(frame_size_for_content_size(content_size));

	if (ar_area.size.min() <= 0)
		return false;

	rect2f arrange_temp
		(ar_area.pos + 
			frame_style().margin_00 + frame_style().border_00 +
			frame_style().padding_00,
		 ar_area.size -
			(frame_style().margin_00 + frame_style().border_00 +
			 frame_style().padding_00 + frame_style().padding_22 +
			 frame_style().border_22 + frame_style().margin_22));

	if (arrange_temp.size.min() <= 0)
		return false;

	rect2f clip_temp;
	if (!arrange_temp.try_intersect(clip_area(), clip_temp))
		return false;

	arrange = arrange_temp;
	clip = clip_temp;
	return true;
}


void gui_frame::realize_layout()
{
	// If present, position content within frame
	// inside style-specific margin and padding.
	if (!content_) return;

	rect2f content_arrange;
	rect2f content_clip;

	if (build_content_layout
			(content_->preferred_size(), content_arrange, content_clip))
	{
		content_->layout(content_arrange, content_clip);
	}
	else
		content_->clip_entirely();
}


void gui_frame::pack_frame(gui_render_packer& packer)
{
	// Build 16 boundary vertices for the 9 frame areas.
	// Occupy either granted size or preferred size,
	// whichever is larger, then clip on mesh creation.
	vector2f pos(arrange_area().pos);
	vector2f size(arrange_area().size.max(preferred_size()));

	vector2f center_size
		(size - (frame_style().margin_00 + frame_style().border_00 +
				 frame_style().border_22 + frame_style().margin_22));
		
	vector2f diagonal_points[4] =
		{ pos + frame_style().margin_00,
		  diagonal_points[0] + frame_style().border_00,
		  diagonal_points[1] + center_size,
		  diagonal_points[2] + frame_style().border_22 };

	vector2f frame_points[16];
	for (int64 y = 0; y < 4; ++y)
	for (int64 x = 0; x < 4; ++x)
	{
		frame_points[y * 4 + x] =
			vector2f(diagonal_points[x].x, diagonal_points[y].y);
	}


	// Build normalized texture coordinate rectangles
	// for each tile from pixel parameters in frame style.
	const vector2i pixel_sizes[3] =
		{ frame_style().pixel_size_00, 
		  frame_style().pixel_size_11,
		  frame_style().pixel_size_22 };

	vector2f material_pixel_size
		(frame_style().material_pixel_size.to_vector2f());

	rect2i pixel_texcoords[9];
	rect2f frame_texcoords[9];
	int64 index = 0;
	for (int64 y = 0; y < 3; ++y)
	for (int64 x = 0; x < 3; ++x, ++index)
	{
		pixel_texcoords[index] =
			rect2i(x == 0 ? frame_style().pixel_offset.x :
						pixel_texcoords[index - 1].end_x(),
				   y == 0 ? frame_style().pixel_offset.y :
						pixel_texcoords[index - 3].end_y(),
				   pixel_sizes[x].x, pixel_sizes[y].y);

		frame_texcoords[index] =
			(pixel_texcoords[index].to_rect2f());

		frame_texcoords[index].pos /= material_pixel_size;
		frame_texcoords[index].size /= material_pixel_size;
	}


	// Create all frame rectangles:
	// - We only use a single material for the entire frame
	//		to reduce the number of draw calls. Hence, we must
	//		subdivide the center area and the border areas
	//		if these exceed our tile size.
	// - We bound maximum subdivisions to prefer graphics
	//		artifacts over excessive triangle counts.
	vector2i center_subdiv
		(1 + min(max_subdivisions_,
				 max(0, integer_operations::cast_or_clamp<int64>
							((center_size.x - gui_render_util::render_epsilon) /
							 frame_style().size_per_tile.x))),
		 1 + min(max_subdivisions_,
				 max(0, integer_operations::cast_or_clamp<int64>
							((center_size.y - gui_render_util::render_epsilon) /
							 frame_style().size_per_tile.y))));

	dynamic_buffer<rect2f> extents;
	extents.reserve((2 + center_subdiv.x) * (2 + center_subdiv.y));
	dynamic_buffer<rect2f> texcoords;
	texcoords.reserve((2 + center_subdiv.x) * (2 + center_subdiv.y));

	for (int64 y = 0; y < 3; ++y)
	for (int64 x = 0; x < 3; ++x)
	{
		vector2f area_start(frame_points[y * 4 + x]);
		vector2f area_end(frame_points[(y + 1) * 4 + x + 1]);
		rect2f area_texcoords(frame_texcoords[y * 3 + x]);


		// For each subdivision, create extents and texture coords.	
		// We only subdivide into tiling directions, and apply
		// a fixed subdivision of one into non-tiling directions.
		int64 xsubdiv = ((x == 1) ? center_subdiv.x : 1);
		int64 ysubdiv = ((y == 1) ? center_subdiv.y : 1);

		for (int64 sy = 0; sy < ysubdiv; ++sy)
		for (int64 sx = 0; sx < xsubdiv; ++sx)
		{
			vector2f tile_start
				(area_start +
				 vector2i(sx, sy).to_vector2f() * frame_style().size_per_tile);


			// As center_size need not be a multiple of tile_size,
			// we may have to show partial tiles. In this case,
			// we also must adjust tile texture coordinates.
			vector2f tile_size
				((sx < xsubdiv - 1) ? frame_style().size_per_tile.x :
									(area_end.x - tile_start.x),
				 (sy < ysubdiv - 1) ? frame_style().size_per_tile.y :
									(area_end.y - tile_start.y));

			vector2f texture_factor
				((x != 1 || sx < xsubdiv - 1) ?
					1 : (tile_size.x / frame_style().size_per_tile.x),
				 (y != 1 || sy < ysubdiv - 1) ?
					1 : (tile_size.y / frame_style().size_per_tile.y));

			extents.push_back
				(rect2f(tile_start, tile_size));
			texcoords.push_back
				(rect2f(area_texcoords.pos,
						area_texcoords.size * texture_factor));
		}
	}


	// Create a mesh from all frame rectangles,
	// then pack that mesh into GUI rendering.
	mesh msh
		(gui_render_util::create_mesh_from_rects
			(extents.data(), texcoords.data(), extents.size(),
			 clip_area(), true, gui.pixel_integral_rendering));

	packer.pack_mesh
		(msh, frame_style().material);
}


const int64 gui_frame::max_subdivisions_ = 64;




//////////////////////////////////////////////////////////////////////////
//
// gui_frame_style
//
//////////////////////////////////////////////////////////////////////////


gui_frame_style::gui_frame_style
	(const vector2f& margin_00, const vector2f& margin_22,
	 const vector2f& border_00, const vector2f& border_22,
	 const vector2f& padding_00, const vector2f& padding_22,
	 const shared_instance<render_material_id>& material,
	 const vector2i& material_pixel_size,
	 const vector2i& pixel_offset,
	 const vector2i& pixel_size_00,
	 const vector2i& pixel_size_11,
	 const vector2i& pixel_size_22,
	 const vector2f& size_per_tile)
	:
	margin_00(margin_00),
	margin_22(margin_22),
	border_00(border_00),
	border_22(border_22),
	padding_00(padding_00),
	padding_22(padding_22),
	material(material),	
	material_pixel_size(material_pixel_size),
	pixel_offset(pixel_offset),
	pixel_size_00(pixel_size_00),
	pixel_size_11(pixel_size_11),
	pixel_size_22(pixel_size_22),
	size_per_tile(size_per_tile)
{
	if (border_00.min() <= 0 ||
		border_22.min() <= 0)
	{
		LOG_ERROR("Frame borders must be greater than zero.");
		throw invalid_parameter();
	}

	if (material_pixel_size.min() < 1)
	{
		LOG_ERROR("Material pixel size must be greater than zero.");
		throw invalid_parameter();
	}

	if (size_per_tile.min() <= 0)
	{
		LOG_ERROR("Frame size per tile must be greater than zero.");
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */

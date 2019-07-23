/**
 *************************************************************************
 *
 * @file font.cpp
 *
 * Font implementation.
 *
 ************************************************************************/


#include "font.hpp"


#include "log.hpp"
#include "file.hpp"
#include "file_util.hpp"
#include "xml.hpp"
#include "model.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//	font
//
//////////////////////////////////////////////////////////////////////////


font font::load(const xml_node& root)
{
	// Fetch general font data.
	if (root.name() != "font")
	{
		LOG_ERROR("Root node tag of a BMFont XML must be font");
		throw xml_exception();
	}

	const xml_node& info = root.child("info");

	string name
		(info.attribute("face") + " " + info.attribute("size"));

	if (info.attribute("bold") != "0") name += " bold";
	if (info.attribute("italic") != "0") name += " italic";

	const xml_node& common = root.child("common");

	float line_height = common.attribute("lineHeight").to_float();
	float base_distance = common.attribute("base").to_float();

	float unitscale_x = 1 / common.attribute("scaleW").to_float();
	float unitscale_y = 1 / common.attribute("scaleH").to_float();


	// Fetch individual letters.
	// Transform texcoords from BMFont space (pixels)
	// to our internal space (normalized units), see
	// mesh_stream_registry::texcoords_stream_name.
	letter_map letters;
	const xml_node& chars = root.child("chars");

	for (xml_node::const_child_iterator cit(chars.first_child()); cit; ++cit)
	{
		if (cit->name() != "char") continue;

		int32 id = cit->attribute("id").to_int32();
	
		float x = cit->attribute("x").to_float();
		float y = cit->attribute("y").to_float();

		float width = cit->attribute("width").to_float();
		float height = cit->attribute("height").to_float();

		float xoffset = cit->attribute("xoffset").to_float();
		float yoffset = cit->attribute("yoffset").to_float();

		float xadvance = cit->attribute("xadvance").to_float();

		rect2f extents
			(xoffset, -base_distance + yoffset,
			 width, height);

		rect2f texcoords
			(x * unitscale_x, y * unitscale_y,
			 width * unitscale_x, height * unitscale_y);

		font_letter letter
			(extents, texcoords, xadvance);

		letters.try_insert(id, letter);
	}


	// Fetch optional kerning pairs.
	kerning_pair_map kerning_pairs;

	if (root.has_child("kernings"))
	{
		const xml_node& kernings = root.child("kernings");

		for (xml_node::const_child_iterator kit(kernings.first_child()); kit; ++kit)
		{
			if (kit->name() != "kerning") continue;

			font_kerning_pair pair
				(kit->attribute("first").to_int32(),
				 kit->attribute("second").to_int32());

			float amount =
				kit->attribute("amount").to_float();

			kerning_pairs.try_insert(pair, amount);
		}
	}


	return font
			(name, letters, kerning_pairs,
			 line_height, base_distance, '?');		
}


font font::load(file& input)
	{ return load(*xml_node::load(input)); }
font font::load(const string& path, file_context &files)
	{ return load(*xml_node::load(path, files)); }


mesh font::typeset_mesh
	(const string& text) const
		{ return typeset_mesh_helper(text, rect2f(), false); }

mesh font::typeset_mesh
	(const string& text, const rect2f& clip) const
		{ return typeset_mesh_helper(text, clip, true); }


model font::typeset_model
	(const string& text, const string& group_name) const
		{ return typeset_model_helper(text, group_name, rect2f(), false); }

model font::typeset_model
	(const string& text, const string& group_name,
	 const rect2f& clip) const
		{ return typeset_model_helper(text, group_name, clip, true); }


vector2f font::calculate_line_integral_extents
	(const string& text) const NOTHROW
{
	vector2f text_extents;

	vector2f base(0, base_distance);
	float line_size_x = 0;

	for (int64 index = 0; index < text.size(); ++index)
	{
		rect2f extents, texcoords;	

		typeset_result result =
			typeset_letter(text, index, base,
						   rect2f(), false,
						   extents, texcoords);


		// Determine width of each individual row,
		// and maximum width over all rows.
		// Ignore other typeset_result cases.
		if (result == typeset_success)
		{
			line_size_x =
				max(line_size_x, extents.end_x());
		}
		else if (result == typeset_line_break)
		{
			text_extents.x = max(text_extents.x, line_size_x);
			text_extents.y += line_height;
			line_size_x = 0;
		}
	}


	// Update for last line (terminating \0).
	text_extents.x = max(text_extents.x, line_size_x);
	text_extents.y += line_height;
	
	return text_extents;
}


bool font::find_letter_extents
	(const string& text, int64 index,
	 const rect2f& clip, bool enable_clip,
	 rect2f& extents) const NOTHROW
{
	// Early-out if text does not contain index.
	if (index < 0 || index >= text.size())
		return false;


	// Typeset text and search for letter at index.
	vector2f base(0, base_distance);
	for (int64 i = 0; i < text.size(); ++i)
	{
		rect2f result_extents, ignore_texcoords;	

		typeset_result result =
			typeset_letter(text, i, base, // Attention: i instead of index.
						   clip, enable_clip,
						   result_extents, ignore_texcoords);

		if (i != index)
			continue; // Continue until we reach target index.

		if (result != typeset_success)
			return false; // Target is newline, ignored, or clipped.

		extents = result_extents;
		return true;
	}


	// Should never be reached:
	// The above typesetting loop finds any letter
	// with index >= 0 and index < text.size().
	// No exception to maintain NOTHROW.
	LOG_ERROR("Corrupt program flow in find_letter_extents().");
	return false;
}


int64 font::find_letter_index
	(const string& text, const vector2f& position) const NOTHROW
{
	vector2f base(0, base_distance);
	float lower_line_bound_y = line_height;


	// Whether position maps to a character
	// on the current line, rationale below.
	bool on_this_line =
		(text.seek('\n') != string::invalid_index) ?
			(position.y < lower_line_bound_y) : true;


	for (int64 index = 0; index < text.size(); ++index)
	{
		rect2f result_extents, ignore_texcoords;	

		typeset_result result =
			typeset_letter(text, index, base,
						   rect2f(), false,
						   result_extents, ignore_texcoords);


		// Skip characters until we arrive at the correct line.
		// Since we have processed all preceding lines already,
		// we need only check against lower y bound of each line.
		// If we arrive at the last line, we must match on it.
		if (!on_this_line)
		{
			if (result == typeset_line_break)
			{
				lower_line_bound_y += line_height;
				on_this_line =
					(text.seek('\n', index + 1) != string::invalid_index) ?
						(position.y < lower_line_bound_y) : true;
			}

			continue;
		}
		

		// Matching character must be on this line.
		// Since we test characters from left to right, we only need
		// to check the right-hand border of each character.
		// First char also covers positions to the left of the line.
		if (result == typeset_success &&
			position.x < result_extents.end_x())
				return index;
	

		// No matching character found before line break,
		// so our position is to the right of this line.
		// In this case, clamp to the newline character.
		if (result == typeset_line_break)
			return index;
	}

	
	// No matching character found on last line.
	// Thus, clamp to terminating \0.
	return text.size();
}


mesh font::typeset_mesh_helper
	(const string& text,
	 const rect2f& clip, bool enable_clip) const
{
	// Preallocate a worst-case guess
	// for the required data streams.
	mesh m;

	mesh_stream_vector& vertices = m.allocate_vertices(text.size() * 4);
	mesh_stream_triangle& triangles = m.allocate_triangles(text.size() * 2);
	mesh_stream_vector& normal_stream = m.allocate_normals();
	mesh_stream_vector& texcoords_stream = m.allocate_texcoords();


	// Perform actual typesetting and
	// build mesh contents.
	vector2f base(0, base_distance);

	int64 dest_strm = 0;
	int64 dest_tri = 0;
	int64 numrects = 0;

	for (int64 index = 0; index < text.size(); ++index)
	{
		// Typeset a single letter to determine
		// its (potentially clipped) extents and
		// texture coordinates.
		rect2f extents, texcoords;

		typeset_result result =
			typeset_letter(text, index, base,
						   clip, enable_clip,
						   extents, texcoords);

		if (result != typeset_success)
			continue;
		

		// No clipping or just a partial clip:
		// Store letter and texture rect in streams.
		vertices[dest_strm] = vector(extents.pos.x, extents.pos.y, 0);
		vertices[dest_strm + 1] = vector(extents.end_x(), extents.pos.y, 0);
		vertices[dest_strm + 2] = vector(extents.end_x(), extents.end_y(), 0);
		vertices[dest_strm + 3] = vector(extents.pos.x, extents.end_y(), 0);

		normal_stream[dest_strm] = normal_stream[dest_strm + 1] =
			normal_stream[dest_strm + 2] = normal_stream[dest_strm + 3] =
				vector(0, 0, -1);

		texcoords_stream[dest_strm] = vector(texcoords.pos.x, texcoords.pos.y, 0);
		texcoords_stream[dest_strm + 1] = vector(texcoords.end_x(), texcoords.pos.y, 0);
		texcoords_stream[dest_strm + 2] = vector(texcoords.end_x(), texcoords.end_y(), 0);
		texcoords_stream[dest_strm + 3] = vector(texcoords.pos.x, texcoords.end_y(), 0);

		triangles[dest_tri] =
			mesh_triangle(dest_strm, dest_strm + 3, dest_strm + 1);
		triangles[dest_tri + 1] =
			mesh_triangle(dest_strm + 1, dest_strm + 3, dest_strm + 2);

		dest_strm += 4;
		dest_tri += 2;
		++numrects;
	}


	// Shrink back to actual number of rectangles
	// that remained after clipping.
	vertices.resize(numrects * 4);
	triangles.resize(numrects * 2);
	normal_stream.resize(numrects * 4);
	texcoords_stream.resize(numrects * 4);


	// Finalize and return mesh.
	m.validate();
	m.rebuild_boundings();

	return std::move(m);
}


model font::typeset_model_helper
	(const string& text, const string& group_name,
	 const rect2f& clip, bool enable_clip) const
{
	model result;

	result.insert_group
		(group_name,
		 typeset_mesh_helper(text, clip, enable_clip));

	result.validate();
	result.rebuild_boundings();

	return std::move(result);
}


font::typeset_result font::typeset_letter
	(const string& text,
	 const int64 index, vector2f& base,
	 const rect2f& clip, bool enable_clip,
	 rect2f& extents, rect2f& texcoords) const NOTHROW
{
	/**
	 * Utility that typesets the letter at @bref{index} in @bref{text}
	 * to a @bref{base} position, then advances the @bref{base} position
	 * to the next letter.
	 *
	 * On @bref{typeset_success}, @bref{extents} and @bref{texcoords}
	 * hold a potentially @bref{clip}ped rectangle for the letter
	 * and associated texture coordinates.
	 *
	 * On other return codes, the letter does not have valid extents
	 * (e.g. the letter is a newline or the letter is clipped entirely).
	 * In this case, @bref{extents} and @bref{texcoords} remain unchanged,
	 * but the @bref{base} position still moves to the next letter.
	 *
	 * @attent Clients must be careful not to forego @ref NOTHROW
	 * by implicitly creating @bref{text} (e.g. from const char*).
	 */


	// Ignore out-of-range characters.
	if (index < 0 || index >= text.size())
		return typeset_ignore;


	// Handle line breaks.
	// Ignore first character of multi-character line breaks,
	// otherwise skip character and start a new line.
	if (index != text.size() - 1 &&
		text[index] == '\r' &&
		text[index + 1] == '\n')
	{
		return typeset_ignore;
	}

	if (text[index] == '\n')
	{
		base.x = 0;
		base.y += line_height;
		return typeset_line_break;
	}


	// Fetch letter from map of all known letters.
	// Try to use fallback letter on failure.
	letter_map::const_iterator it
		(letters.find(static_cast<unsigned char>(text[index])));

	if (!it)
	{
		it = letters.find(fallback_letter);
		if (!it) return typeset_ignore;
	}


	// Place letter into typesetting system,
	// and advance base to next letter.
	extents = it->value.extents;
	texcoords = it->value.texcoords;

	extents.pos += base;
	base.x += it->value.increment;


	// Adjust kerning distance specific to current
	// letter pair if any such entry exists.		
	if (index > 0)
	{
		font_kerning_pair pair
			(static_cast<unsigned char>(text[index - 1]),
			 static_cast<unsigned char>(text[index]));

		kerning_pair_map::const_iterator kit
			(kerning_pairs.find(pair));

		if (kit)
		{
			extents.pos.x += kit->value;
			base.x += kit->value;
		}
	}


	// Optionally perform clipping and store results.
	// In order to support partial clipping,
	// we must adapt texture coordinates as well.
	if (enable_clip)
	{
		rect2f extents_clipped, texcoords_clipped;

		if (!extents.try_intersect(clip, extents_clipped))
			return typeset_clip_entirely;

		texcoords_clipped =
			(extents == extents_clipped) ?
				texcoords :
				extents_clipped.apply_linear_map(extents, texcoords);

		extents = extents_clipped;
		texcoords = texcoords_clipped;
	}


	return typeset_success;
}




} /* namespace viral_core */ 

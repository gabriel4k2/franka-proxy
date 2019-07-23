/**
 *************************************************************************
 *
 * @file input.cpp
 *
 * General input facilities, implementation.
 *
 ************************************************************************/


#include "input.hpp"


#include "log.hpp"
#include "tree_map.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//	input_context
//
//////////////////////////////////////////////////////////////////////////


input_context::input_context(const string& context_name)
	: context_name(context_name)
		{ LOG_INFO("Creating: " + context_name); }

input_context::~input_context() NOTHROW
	{ LOG_INFO("Destroyed: " + context_name); }


input_source_name_map input_context::get_default_source_names() const
{
	/**
	 * Implementation notes:	
	 * - Exploits the fact that blocks of sources are contiguous.
	 * - Assumes that enums and int32 offsets do not cause overflow.
	 * - @attent Must be modified in case of new input sources.
	 */

	input_source_name_map names;

	safe_insert_source_name(names, input_source::none, "none");

	for (int32 i = 0; i < 12; ++i)
		safe_insert_source_name
			(names,
			 static_cast<input_source_type>(input_source::k_f1 + i),
			 string("F") + (i + 1));

	for (int32 i = 0; i < 10; ++i)
		safe_insert_source_name
			(names,
			 static_cast<input_source_type>(input_source::k_0 + i),
			 string(i));

	for (int32 i = 0; i < 26; ++i)
		safe_insert_source_name
			(names,
			 static_cast<input_source_type>(input_source::k_a + i),
			 string(static_cast<char>('A' + i)));

	safe_insert_source_name(names, input_source::k_comma, ",");
	safe_insert_source_name(names, input_source::k_period, ".");
	safe_insert_source_name(names, input_source::k_slash, "/");
	safe_insert_source_name(names, input_source::k_semicolon, ";");
	safe_insert_source_name(names, input_source::k_apostrophe, "'");

	safe_insert_source_name(names, input_source::k_left_bracket, "[");
	safe_insert_source_name(names, input_source::k_right_bracket, "]");

	safe_insert_source_name(names, input_source::k_minus, "-");
	safe_insert_source_name(names, input_source::k_equals, "=");
	safe_insert_source_name(names, input_source::k_backslash, "\\");
	safe_insert_source_name(names, input_source::k_grave, "~");
	safe_insert_source_name(names, input_source::k_angle_bracket, "<");

	safe_insert_source_name(names, input_source::k_shift, "shift");
	safe_insert_source_name(names, input_source::k_control, "ctrl");
	safe_insert_source_name(names, input_source::k_alt, "alt");

	safe_insert_source_name(names, input_source::k_left_shift, "left shift");
	safe_insert_source_name(names, input_source::k_right_shift, "right shift");
	safe_insert_source_name(names, input_source::k_left_control, "left ctrl");
	safe_insert_source_name(names, input_source::k_right_control, "right ctrl");
	safe_insert_source_name(names, input_source::k_left_alt, "left alt");
	safe_insert_source_name(names, input_source::k_right_alt, "right alt");

	safe_insert_source_name(names, input_source::k_caps_lock, "caps lock");

	safe_insert_source_name(names, input_source::k_escape, "esc");
	safe_insert_source_name(names, input_source::k_space, "space");
	safe_insert_source_name(names, input_source::k_tab, "tab");
	safe_insert_source_name(names, input_source::k_return, "enter");
	safe_insert_source_name(names, input_source::k_backspace, "backspace");

	safe_insert_source_name(names, input_source::k_insert, "insert");
	safe_insert_source_name(names, input_source::k_home, "home");
	safe_insert_source_name(names, input_source::k_page_up, "page up");
	safe_insert_source_name(names, input_source::k_delete, "delete");
	safe_insert_source_name(names, input_source::k_end, "end");
	safe_insert_source_name(names, input_source::k_page_down, "page down");

	safe_insert_source_name(names, input_source::k_up, "up");
	safe_insert_source_name(names, input_source::k_down, "down");
	safe_insert_source_name(names, input_source::k_left, "left");
	safe_insert_source_name(names, input_source::k_right, "right");

	for (int32 i = 0; i < 10; ++i)
		safe_insert_source_name
			(names,
			 static_cast<input_source_type>(input_source::k_num0 + i),
			 string("numpad ") + i);

	safe_insert_source_name(names, input_source::k_num_enter, "numpad enter");
	safe_insert_source_name(names, input_source::k_num_plus, "numpad +");
	safe_insert_source_name(names, input_source::k_num_minus, "numpad -");
	safe_insert_source_name(names, input_source::k_num_multiply, "numpad *");
	safe_insert_source_name(names, input_source::k_num_divide, "numpad /");
	safe_insert_source_name(names, input_source::k_num_period, "numpad .");

	for (int32 i = 0; i < 3; ++i)
		safe_insert_source_name
			(names,
			 static_cast<input_source_type>(input_source::m_axis0 + i),
			 string("mouse axis ") + (i + 1));

	safe_insert_source_name(names, input_source::m_absolute_x, "mouse X position");
	safe_insert_source_name(names, input_source::m_absolute_y, "mouse Y position");

	for (int32 i = 0; i < 5; ++i)
		safe_insert_source_name
			(names,
			 static_cast<input_source_type>(input_source::m_button0 + i),
			 string("mouse button") + (i + 1));

	for (int32 j = 0; j < 4; ++j)
	{
		for (int32 i = 0; i < 8; ++i)
			safe_insert_source_name
				(names,
				 static_cast<input_source_type>(input_source::j0_axis0 + j * 24 + i),
				 string("joy") + (j + 1) + " axis" + (i + 1));

		for (int32 i = 0; i < 16; ++i)
			safe_insert_source_name
				(names,
				 static_cast<input_source_type>(input_source::j0_button0 + j * 24 + i),
				 string("joy") + (j + 1) + " button" + (i + 1));
	}

	return names;
}


void input_context::safe_insert_source_name
	(input_source_name_map& map,
	 input_source_type value, const string& name)
{
	/**
	 * Insert @bref{value} to @bref{name} pair into @bref{map}
	 * of input source names.
	 *
	 * Verifies that value is valid and has not been provided yet.
	 */

	if (value < 0 || value >= input_source::count)
	{
		LOG_ERROR("Input source: " + value + " is invalid.");
		throw assert_failed();
	}

	input_source_type source_type =
		static_cast<input_source_type>(value);

	if (map.contains(source_type))
	{
		LOG_ERROR("Input source: " + value + " already has a name.");
		throw assert_failed();
	}

	map.insert(source_type, name);
}




} /* namespace viral_core */

/**
 *************************************************************************
 *
 * @file xml_util.cpp
 *
 * XML utility implementation.
 *
 ************************************************************************/


#include "xml_util.hpp"


#include "log.hpp"
#include "integer_checks.hpp"
#include "auto_pointer.hpp"
#include "xml.hpp"
#include "color.hpp"
#include "geo_2d.hpp"
#include "geo_vector.hpp"
#include "geo_transform.hpp"
#include "geo_primitive.hpp"
#include "geo_volume.hpp"
#include "geo_camera.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// xml_util
//
//////////////////////////////////////////////////////////////////////////


bool xml_util::is_xml_name_valid(const string& name)
{
	int64 size = name.size();

	if (size == 0) return false;

	for (int64 i = 0; i < size; ++i)
	{
		char chr = name[i];

		if (chr >= 'a' && chr <= 'z') continue;
		if (chr >= 'A' && chr <= 'Z') continue;
		if (chr == ':' || chr == '_') continue;

		if (i != 0)
		{
			if (chr >= '0' && chr <= '9') continue;
			if (chr == '-' || chr == '.') continue;
		}

		return false;
	}

	return true;
}


string xml_util::escape_entities(const string& text)
{
	int64 size = text.size();

	string result;
	result.reserve(size);

	for (int64 i = 0; i < size; ++i)
	{
		char chr = text[i];

		if (chr == '<') result += "&lt;";
		else if (chr == '>') result += "&gt;";
		else if (chr == '&') result += "&amp;";
		else if (chr == '\"') result += "&quot;";
		else if (chr == '\'') result += "&apos;";
		else result.append(chr);
	}

	return result;
}


string xml_util::unescape_entities(const string& text)
{
	int64 size = text.size();

	string result;
	result.reserve(size);

	for (int64 i = 0; i < size; )
	{
		if (text[i] == '&')
		{
			if (integer_checks::is_add_safe(i, 3) &&
				i + 3 < size)	// 4-character entities.
				
			{
				if (memory::are_blocks_equal_typed(text.data() + i, "&lt;", 4))
				{
					result.append('<');
					i += 4;
					continue;
				}
				else if (memory::are_blocks_equal_typed(text.data() + i, "&gt;", 4))
				{
					result.append('>');
					i += 4;
					continue;
				}
			}

			if (integer_checks::is_add_safe(i, 4) &&
				i + 4 < size)	// 5-character entities.
			{
				if (memory::are_blocks_equal_typed(text.data() + i, "&amp;", 5))
				{
					result.append('&');
					i += 5;
					continue;
				}
			}

			if (integer_checks::is_add_safe(i, 5) &&
				i + 5 < size)	// 6-character entities.
			{
				if (memory::are_blocks_equal_typed(text.data() + i, "&quot;", 6))
				{
					result.append('\"');
					i += 6;
					continue;
				}
				if (memory::are_blocks_equal_typed(text.data() + i, "&apos;", 6))
				{
					result.append('\'');
					i += 6;
					continue;
				}
			}
		}


		// Not an escaped entity.
		result.append(text[i]);
		++i;
	}

	return result;
}


string xml_util::escape_whitespace(const string& text)
{
	int64 size = text.size();

	string result;
	result.reserve(size);

	for (int64 i = 0; i < size; ++i)
	{
		if (text[i] == ' ') result += "\\s";
		else if (text[i] == '\t') result += "\\t";
		else if (text[i] == '\r') result += "\\r";
		else if (text[i] == '\n') result += "\\n";
		else if (text[i] == '\\') result += "\\\\";
		else result.append(text[i]);
	}

	return result;
}


string xml_util::unescape_whitespace(const string& text)
{
	int64 size = text.size();

	string result;
	result.reserve(size);

	for (int64 i = 0; i < size; ++i)
	{
		// Directly copy non-escaped characters to output.
		if (text[i] != '\\')
		{			
			result.append(text[i]);
			continue;
		}

		// Escape sequence, look at next character.
		++i;

		if (i >= size) break; 
		else if (text[i] == 's') result.append(' ');
		else if (text[i] == 't') result.append('\t');
		else if (text[i] == 'r') result.append('\r');
		else if (text[i] == 'n') result.append('\n');	
		else result.append(text[i]);
	}

	return result;
}


string xml_util::unescape_whitespace_prioritized(const string& text)
{
	int64 size = text.size();

	string temp;
	temp.reserve(size);

	dynamic_buffer<bool> esc_ws;
	esc_ws.reserve(size);


	// See unescape_whitespace(), but keep track
	// of whitespace that came from recognized
	// escape sequences.
	for (int64 i = 0; i < size; ++i)
	{
		if (text[i] != '\\')
		{			
			temp.append(text[i]);
			esc_ws.push_back(false);
			continue;
		}

		++i;

		if (i >= size) break; 
		else if (text[i] == 's')
			{ temp.append(' '); esc_ws.push_back(true); }
		else if (text[i] == 't')
			{ temp.append('\t'); esc_ws.push_back(true); }
		else if (text[i] == 'r')
			{ temp.append('\r'); esc_ws.push_back(true); }
		else if (text[i] == 'n')
			{ temp.append('\n'); esc_ws.push_back(true); }
		else
			{ temp.append(text[i]); esc_ws.push_back(false); }
	}


	// Remove any coherent block of unescaped whitespace
	// that has contact to a whitespace escape sequence.
	int64 tempsize = temp.size();

	string result;
	result.reserve(tempsize);

	for (int64 i = 0; i < tempsize; ++i)
	{
		if (esc_ws[i] ||
			(temp[i] != ' ' && temp[i] != '\t' &&
			 temp[i] != '\r' && temp[i] != '\n'))
		{
			result.push_back(temp[i]);
			continue;
		}


		// Unescaped whitespace, so find block limits.
		// Then check characters before and after block
		// to decide whether to copy or to discard.
		int64 s = i;
		for (; i < tempsize - 1; ++i)
		{
			if (esc_ws[i + 1] ||
				(temp[i + 1] != ' ' && temp[i + 1] != '\t' &&
				 temp[i + 1] != '\r' && temp[i + 1] != '\n'))
					break;
		}
		
		if ((s <= 0 || !esc_ws[s - 1]) &&
			(i >= tempsize - 1 || !esc_ws[i + 1]))
		{
			for (int64 k = s; k <= i; ++k)
				result.push_back(temp[k]);
		}
	}


	return result;
}




//////////////////////////////////////////////////////////////////////////
//
// xml_converter, conversion XML to object instances
//
//////////////////////////////////////////////////////////////////////////


bool xml_converter::to_bool(const xml_node &node)
{
	const string& value = node.attribute("value");

	if (value == "true") return true;
	if (value == "false") return false;

	LOG_ERROR("Booleans support only true and false, not: " + value);
	throw xml_exception();
}


int32 xml_converter::to_int32(const xml_node &node)
	{ return node.attribute("value").to_int32(); }
int64 xml_converter::to_int64(const xml_node &node)
	{ return node.attribute("value").to_int64(); }
float xml_converter::to_float(const xml_node &node)
	{ return node.attribute("value").to_float(); }


vector2i xml_converter::to_vector2i(const xml_node &node)
{
	return vector2i
		(node.attribute("x").to_int64(),
		 node.attribute("y").to_int64());
}


vector2f xml_converter::to_vector2f(const xml_node &node)
{
	return vector2f
		(node.attribute("x").to_float(),
		 node.attribute("y").to_float());
}


rect2i xml_converter::to_rect2i(const xml_node &node)
{
	return rect2i
		(to_vector2i(node.child("pos")),
		 to_vector2i(node.child("size")));
}


rect2f xml_converter::to_rect2f(const xml_node &node)
{
	return rect2f
		(to_vector2f(node.child("pos")),
		 to_vector2f(node.child("size")));
}


vector xml_converter::to_vector(const xml_node &node)
{
	return vector
		(node.attribute("x").to_float(),
		 node.attribute("y").to_float(),
		 node.attribute("z").to_float());
}


rotation xml_converter::to_rotation(const xml_node &node)
{
	if (node.attribute("mode") == "identity")
		return rotation();

	if (node.attribute("mode") == "axis_angle")
		return rotation
			(to_vector(node),
			 node.attribute("angle").to_float());

	if (node.attribute("mode") == "euler")
		return
			rotation(vector(0, 0, 1), node.attribute("roll").to_float()) *
			rotation(vector(1, 0, 0), node.attribute("pitch").to_float()) *
			rotation(vector(0, 1, 0), node.attribute("yaw").to_float());

	if (node.attribute("mode") == "quaternion")
		return rotation::quaternion
			(node.attribute("w").to_float(),
			 node.attribute("x").to_float(),
			 node.attribute("y").to_float(),
			 node.attribute("z").to_float()).normalized();

	LOG_ERROR("Unknown mode attribute in xml rotation.");
	throw xml_exception();
}


matrix xml_converter::to_matrix(const xml_node &node)
{
	return matrix
		(node.attribute("c00").to_float(), node.attribute("c01").to_float(),
		 node.attribute("c02").to_float(), node.attribute("c03").to_float(),

		 node.attribute("c10").to_float(), node.attribute("c11").to_float(),
		 node.attribute("c12").to_float(), node.attribute("c13").to_float(),

		 node.attribute("c20").to_float(), node.attribute("c21").to_float(),
		 node.attribute("c22").to_float(), node.attribute("c23").to_float(),

		 node.attribute("c30").to_float(), node.attribute("c31").to_float(),
		 node.attribute("c32").to_float(), node.attribute("c33").to_float());
}


line xml_converter::to_line(const xml_node &node)
{
	return line
		(to_vector(node.child("origin")),
		 to_vector(node.child("direction")));
}


segment xml_converter::to_segment(const xml_node &node)
{
	return segment
		(to_vector(node.child("start")),
		 to_vector(node.child("end")));
}


triangle xml_converter::to_triangle(const xml_node &node)
{
	return triangle
		(to_vector(node.child("a")),
		 to_vector(node.child("b")),
		 to_vector(node.child("c")));
}


box xml_converter::to_box(const xml_node &node)
{
	return box
		(to_vector(node.child("min")),
		 to_vector(node.child("max")));
}


sphere xml_converter::to_sphere(const xml_node &node)
{
	return sphere
		(to_vector(node.child("position")),
		 node.attribute("radius").to_float());
}


camera xml_converter::to_camera(const xml_node &node)
{
	const string& projection_value =
		node.attribute("projection");

	camera::projection_mode projection;
	if (projection_value == "perspective")
		projection = camera::perspective;
	else if (projection_value == "orthographic")
		projection = camera::orthographic;
	else
	{
		LOG_ERROR("Unknown camera projection mode: " + projection_value);
		throw xml_exception();
	}

	return camera
		(to_vector(node.child("position")),
		 to_rotation(node.child("orientation")),
		 projection,
		 node.attribute("field_of_view_x").to_float(),
		 node.attribute("aspect_y_to_x").to_float(),
		 node.attribute("near_plane").to_float(),
		 node.attribute("far_plane").to_float(),
		 to_vector(node.child("post_translate")),
		 to_vector(node.child("post_scale")));
}


color xml_converter::to_color(const xml_node &node)
{
	return
		color
			(node.attribute("r").to_float(),
			 node.attribute("g").to_float(),
			 node.attribute("b").to_float(),
			 node.find_attribute("a") ?
				node.attribute("a").to_float() : 0);
}




//////////////////////////////////////////////////////////////////////////
//
// xml_converter, conversion object instances to XML
//
//////////////////////////////////////////////////////////////////////////


auto_pointer<xml_node> xml_converter::to_xml(bool b)
{
	auto_pointer<xml_node> result(new xml_node("bool"));
	result->insert_attribute("value", b ? "true" : "false");
	return result;
}


auto_pointer<xml_node> xml_converter::to_xml(int32 i)
{
	auto_pointer<xml_node> result(new xml_node("int32"));
	result->insert_attribute("value", string(i));
	return result;
}


auto_pointer<xml_node> xml_converter::to_xml(int64 i)
{
	auto_pointer<xml_node> result(new xml_node("int64"));
	result->insert_attribute("value", string(i));
	return result;
}


auto_pointer<xml_node> xml_converter::to_xml(float f)
{
	auto_pointer<xml_node> result(new xml_node("float"));
	result->insert_attribute("value", string(f));
	return result;
}


auto_pointer<xml_node> xml_converter::to_xml(const vector2i& v)
{
	auto_pointer<xml_node> result(new xml_node("vector2i"));
	result->insert_attribute("x", string(v.x));
	result->insert_attribute("y", string(v.y));
	return result;
}


auto_pointer<xml_node> xml_converter::to_xml(const vector2f& v)
{
	auto_pointer<xml_node> result(new xml_node("vector2f"));
	result->insert_attribute("x", string(v.x));
	result->insert_attribute("y", string(v.y));
	return result;
}


auto_pointer<xml_node> xml_converter::to_xml(const rect2i& r)
{
	auto_pointer<xml_node> result(new xml_node("rect2i"));
	result->insert_child("pos", to_xml(r.pos));
	result->insert_child("size", to_xml(r.size));
	return result;
}


auto_pointer<xml_node> xml_converter::to_xml(const rect2f& r)
{
	auto_pointer<xml_node> result(new xml_node("rect2f"));
	result->insert_child("pos", to_xml(r.pos));
	result->insert_child("size", to_xml(r.size));
	return result;
}


auto_pointer<xml_node> xml_converter::to_xml(const vector& v)
{
	auto_pointer<xml_node> result(new xml_node("vector"));
	result->insert_attribute("x", string(v.x));
	result->insert_attribute("y", string(v.y));
	result->insert_attribute("z", string(v.z));
	return result;
}


auto_pointer<xml_node> xml_converter::to_xml(const rotation& r)
{
	auto_pointer<xml_node> result(new xml_node("rotation"));
	result->insert_attribute("mode", "quaternion");
	result->insert_attribute("w", string(r.w));
	result->insert_attribute("x", string(r.x));
	result->insert_attribute("y", string(r.y));
	result->insert_attribute("z", string(r.z));
	return result;
}


auto_pointer<xml_node> xml_converter::to_xml(const matrix& m)
{
	auto_pointer<xml_node> result(new xml_node("matrix"));

	result->insert_attribute("c00", string(m.c00)); result->insert_attribute("c01", string(m.c01));
	result->insert_attribute("c02", string(m.c02)); result->insert_attribute("c03", string(m.c03));

	result->insert_attribute("c10", string(m.c10)); result->insert_attribute("c11", string(m.c11));
	result->insert_attribute("c12", string(m.c12)); result->insert_attribute("c13", string(m.c13));

	result->insert_attribute("c20", string(m.c20)); result->insert_attribute("c21", string(m.c21));
	result->insert_attribute("c22", string(m.c22)); result->insert_attribute("c23", string(m.c23));

	result->insert_attribute("c30", string(m.c30)); result->insert_attribute("c31", string(m.c31));
	result->insert_attribute("c32", string(m.c32)); result->insert_attribute("c33", string(m.c33));

	return result;
}


auto_pointer<xml_node> xml_converter::to_xml(const line& l)
{
	auto_pointer<xml_node> result(new xml_node("line"));
	result->insert_child("origin", to_xml(l.origin));
	result->insert_child("direction", to_xml(l.direction));
	return result;
}


auto_pointer<xml_node> xml_converter::to_xml(const segment& s)
{
	auto_pointer<xml_node> result(new xml_node("segment"));
	result->insert_child("start", to_xml(s.start));
	result->insert_child("end", to_xml(s.end));
	return result;
}


auto_pointer<xml_node> xml_converter::to_xml(const triangle& t)
{
	auto_pointer<xml_node> result(new xml_node("triangle"));
	result->insert_child("a", to_xml(t.a));
	result->insert_child("b", to_xml(t.b));
	result->insert_child("c", to_xml(t.c));
	return result;
}


auto_pointer<xml_node> xml_converter::to_xml(const box& b)
{
	auto_pointer<xml_node> result(new xml_node("box"));
	result->insert_child("min", to_xml(b.min));
	result->insert_child("max", to_xml(b.max));
	return result;
}


auto_pointer<xml_node> xml_converter::to_xml(const sphere& s)
{
	auto_pointer<xml_node> result(new xml_node("sphere"));
	result->insert_child("position", to_xml(s.position));
	result->insert_attribute("radius", string(s.radius));
	return result;
}


auto_pointer<xml_node> xml_converter::to_xml(const camera& c)
{
	auto_pointer<xml_node> result(new xml_node("camera"));

	result->insert_child("position", to_xml(c.position));
	result->insert_child("orientation", to_xml(c.orientation));

	if (c.projection == camera::perspective)
		result->insert_attribute("projection", "perspective");
	else if (c.projection == camera::orthographic)
		result->insert_attribute("projection", "orthographic");
	else
	{
		LOG_ERROR("Unknown camera projection mode: " +
				  static_cast<int32>(c.projection));
		throw xml_exception();
	}

	result->insert_attribute("field_of_view_x", string(c.field_of_view_x));
	result->insert_attribute("aspect_y_to_x", string(c.aspect_y_to_x));
	result->insert_attribute("near_plane", string(c.near_plane));
	result->insert_attribute("far_plane", string(c.far_plane));

	result->insert_child("post_translate", to_xml(c.post_translate));
	result->insert_child("post_scale", to_xml(c.post_scale));
	
	return result;
}


auto_pointer<xml_node> xml_converter::to_xml(const color& c)
{
	auto_pointer<xml_node> result(new xml_node("color"));
	result->insert_attribute("r", string(c.r));
	result->insert_attribute("g", string(c.g));
	result->insert_attribute("b", string(c.b));
	result->insert_attribute("a", string(c.a));
	return result;
}




} /* namespace viral_core */

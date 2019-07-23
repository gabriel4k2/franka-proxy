/**
 *************************************************************************
 *
 * @file xml_util.hpp
 *
 * XML utility header.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__XML_UTIL_HPP)
#define INCLUDED__VIRAL_CORE__XML_UTIL_HPP


#include "build_config.hpp"


#include "container_forward.hpp"
#include "string_forward.hpp"
#include "file_forward.hpp"
#include "image_forward.hpp"
#include "geo_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class xml_util
 * @ingroup file_module
 *
 * Generic XML utilities.
 *
 ************************************************************************/
class xml_util :
	private non_constructable
{

	public:

		/**
		 * Test if an XML tag or attribute @bref{name} is valid.
		 * 
		 * Valid names:
		 * - are not empty.
		 * - start with a character from 'a'-'z', 'A'-'Z', ':', '_'.
		 * - contain only 'a'-'z', 'A'-'Z', '0'-'9', ':', '_', '-', '.'.
		 */
		static bool is_xml_name_valid(const string& name);


		/**
		 * Escape and unescape XML entities in element text
		 * and attribute values.
		 *
		 * Both escaping and unescaping:
		 * - handle all five XML entities ("<", ">", "&", "\"", "'").
		 * - copy everything else to output (e.g. letters, digits, punctuation).
		 * In particular, unescaping does not report invalid XML entities,
		 * but copies these directly to output.
		 *
		 * Note XML parsing in general assumes its characters
		 * in UTF-8, so strings should be coded appropriately.
		 */
		//@{
		static string escape_entities(const string& text);
		static string unescape_entities(const string& text);
		//@}


		/**
		 * Mask or unmask all whitespace in element text or attribute values
		 * so that XML saving and loading accurately preserve the whitespace.
		 *
		 * Usage notes:
		 * - Escaping uses the backslash '\\' character as an escape mark.
		 *		Supported sequences are '\\s', '\\t', '\\r', '\\n', '\\\\',
		 *		for original characters ' ', '\\t', '\\r', '\\n', and '\\'.
		 * - Unescaping ignores the leading backslash on any other sequence
		 *		and silently writes the next character to output. Likewise,
		 *		unescaping copies over any already unescaped whitespace.
		 * - @attent Clients must explicitly call escape_whitespace() and
		 *		unescape_whitespace() before saving and after loading XML.
		 *		See xml_node documentation for details.
		 */
		//@{
		static string escape_whitespace(const string& text);
		static string unescape_whitespace(const string& text);
		//@}


		/**
		 * Similar to unescape_whitespace(), but discards any block of 
		 * already unescaped whitespace adjacent to recognized whitespace
		 * escape sequences. This is useful for precise text formatting
		 * without the need to mask each single blank.
		 */
		static string unescape_whitespace_prioritized(const string& text);
};




/**
 *************************************************************************
 *
 * @class xml_converter
 * @ingroup file_module
 *
 * Intuitive conversion in-between xml_node%s and common data types.
 *
 * XML examples:
 * @verbatim
 * <some_float_value value="192.522" />
 * <some_vector x="1" y="2" z="3" />
 *
 * <extended_color r="0.5" g="0.1" b="0.9" a="10"
 *				   color_space="srgb" />
 *
 * <my_box>
 *     <min x="1" y="2" z=3" />
 *     <max x="5" y="3" z=7" />
 *     <my_box_contents> ... </my_box_contents>
 * </my_box>
 * @endverbatim
 *
 * XML mapping conventions:
 * - By convention, instances of most classes correspond to xml_node%s.
 *		Member variables of an instance either map to XML attributes
 *		(intrinsic variables, strings), or to subordinate xml_node%s
 *		(class-typed variables).
 * - Attribute names and child node names match with the names
 *		of associated class members. For root level xml_node%s,
 *		XML names are unspecified and default to class names.
 *		Clients should replace default names with member names
 *		when inserting children nodes into parents.
 * - To allow for transparent extensibility, additional attributes
 *		and subordinate nodes are allowed, even if these do not match
 *		with any existing members.
 *
 ************************************************************************/
class xml_converter :
	private non_constructable
{
	public:

		/** Parse xml @bref{node} into some output type. */
		//@{
		static bool to_bool(const xml_node &node);
		static int32 to_int32(const xml_node &node);
		static int64 to_int64(const xml_node &node);
		static float to_float(const xml_node &node);

		static vector2i to_vector2i(const xml_node &node);
		static vector2f to_vector2f(const xml_node &node);
		static rect2i to_rect2i(const xml_node &node);
		static rect2f to_rect2f(const xml_node &node);

		static vector to_vector(const xml_node &node);
		static rotation to_rotation(const xml_node &node);
		static matrix to_matrix(const xml_node &node);

		static line to_line(const xml_node &node);
		static segment to_segment(const xml_node &node);
		static triangle to_triangle(const xml_node &node);
		static box to_box(const xml_node &node);
		static sphere to_sphere(const xml_node &node);
		static camera to_camera(const xml_node &node);

		static color to_color(const xml_node &node);
		//@}


		/** Convert data to XML and return not-@NULL result. */
		//@{
		static auto_pointer<xml_node> to_xml(bool b);
		static auto_pointer<xml_node> to_xml(int32 i);
		static auto_pointer<xml_node> to_xml(int64 i);
		static auto_pointer<xml_node> to_xml(float f);

		static auto_pointer<xml_node> to_xml(const vector2i& v);
		static auto_pointer<xml_node> to_xml(const vector2f& v);
		static auto_pointer<xml_node> to_xml(const rect2i& r);
		static auto_pointer<xml_node> to_xml(const rect2f& r);

		static auto_pointer<xml_node> to_xml(const vector& v);
		static auto_pointer<xml_node> to_xml(const rotation& r);
		static auto_pointer<xml_node> to_xml(const matrix& m);

		static auto_pointer<xml_node> to_xml(const line& l);
		static auto_pointer<xml_node> to_xml(const segment& s);
		static auto_pointer<xml_node> to_xml(const triangle& t);
		static auto_pointer<xml_node> to_xml(const box& b);
		static auto_pointer<xml_node> to_xml(const sphere& s);
		static auto_pointer<xml_node> to_xml(const camera& c);

		static auto_pointer<xml_node> to_xml(const color& c);
		//@}
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__XML_UTIL_HPP) */


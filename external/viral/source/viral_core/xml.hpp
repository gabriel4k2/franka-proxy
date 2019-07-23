/**
 *************************************************************************
 *
 * @file xml.hpp
 *
 * Header for in-memory XML tree structure.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__XML_HPP)
#define INCLUDED__VIRAL_CORE__XML_HPP


#include "build_config.hpp"


#include "file_forward.hpp"


#include "buffer.hpp"
#include "list.hpp"
#include "tree_map.hpp"
#include "string.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class xml_node
 * @ingroup file_module
 *
 * A single node within an XML tree.
 *
 * XML usage notes:
 * - An XML node owns its attributes and children nodes. These in turn
 *		remain valid but for the lifetime of their parent node.
 * - XML tag and attribute names must match up with the C identifier
 *		naming convention (e.g. so that save() produces valid XML).
 * - Attribute values and element text may contain arbitrary characters.
 *		Saving and loading automatically translate between characters
 *		and XML entities as required (see xml_util::escape_entities()).
 * - XML IDs and NODEREFs are not intrinsically supported, but provided
 *		as normal attributes. See create_id_to_node_map() for alternatives.
 *
 * Saving and loading:
 * - XML saving and XML loading invoke string::compact() on element text
 *		and on attibute values. This trims leading and trailing whitespace
 *		and compresses any interior whitespace to at most a single space.
 *		Furthermore, saving introduces additional formatting whitespace.
 *		If an application requires exact whitespace, it must explicitly
 *		use escaping sequences (e.g. with xml_util::escape_whitespace()).
 * - Loading discards the order of element text to subordinate elements
 *		in the XML file, and saving uses an arbitrary order. Generally,
 *		elements which hold relevant text should not have any children.
 * - XML loading is realized via the Expat parser, which requires
 *		@ref VIRAL__HAVE_EXPAT to be defined.
 *
 * Implementation notes:
 * - Node names are constant after creation to avoid inconsistencies
 *		in parent-to-child lookup maps (i.e. as by changing the names
 *		of existing children nodes).
 * - Only xml_node%s which have been created through insert_child()
 *		have not-@NULL parent() pointers. This in turn implies that
 *		client-constructed xml_node%s never have any parents.
 * - XML nodes do not support move operations (e.g. move constructors),
 *		because a @ref NOTHROW move would require moving const names.
 *		Instead, XML nodes use auto_pointer%s to avoid deep copies.
 * 
 ************************************************************************/
class xml_node
{

	public:

		typedef tree_map<string, string> attribute_map;
		typedef attribute_map::iterator attribute_iterator;
		typedef attribute_map::const_iterator const_attribute_iterator;

		typedef list<xml_node> child_list;
		typedef list<xml_node>::iterator child_iterator;
		typedef list<xml_node>::const_iterator const_child_iterator;


		/**
		 * Create new, parent-less XML node,
		 * and fill with copy of given content.
		 */
		//@{
		explicit xml_node
			(const string& name);

		xml_node
			(const string& name,
			 const string& text);

		xml_node
			(const string& name,
			 const string& text,			
			 const attribute_map& attributes,
			 const child_list& children);
		//@}


		/**
		 * Create new, parent-less XML node from @bref{other} node.
		 * Optionally, use another @bref{name} for the new XML node,
		 * and throw validation_failed() if this name is not valid.
		 * Not-@NULL auto pointers transfer ownership over children,
		 * attributes, and text without expensive, explicit copying.
		 */
		//@{
		xml_node(const xml_node& other);
		xml_node(const string& name, const xml_node& other);
		xml_node(auto_pointer<xml_node> other);
		xml_node(const string& name, auto_pointer<xml_node> other);
		//@}


		const string& name() const NOTHROW;
		xml_node* parent() NOTHROW;
		const xml_node* parent() const NOTHROW;


		void set_text(const string& text);
		const string& text() const NOTHROW;


		/**
		 * Replace all node attributes with new @bref{attributes}.
		 * Throws on any invalid names in @bref{attributes}.
		 */
		void set_attributes(const attribute_map& attributes);


		/**
		 * Add an attribute with a unique @bref{name} and
		 * some @bref{value}, by default an empty string.
		 * Returns an iterator to the new attribute entry.
		 * Throws if the node already has another attribute
		 * with @bref{name}, or if @bref{name} is invalid.
		 */
		//@{
		attribute_iterator insert_attribute(const string& name);
		attribute_iterator insert_attribute(const string& name, const string& value);
		//@}


		attribute_iterator first_attribute() NOTHROW;
		const_attribute_iterator first_attribute() const NOTHROW;
		attribute_iterator last_attribute() NOTHROW;	
		const_attribute_iterator last_attribute() const NOTHROW;	

		int64 attribute_count() const NOTHROW;
		bool has_attributes() const NOTHROW;
		bool has_attribute(const string& name) const NOTHROW;


		/**
		 * Look for an attribute with given @bref{name}.
		 * On failure, return a past-end iterator (find versions),
		 * or throw an exception (declarative versions).
		 */
		//@{	
		attribute_iterator find_attribute(const string& name) NOTHROW;
		const_attribute_iterator find_attribute(const string& name) const NOTHROW;

		string& attribute(const string& name);
		const string& attribute(const string& name) const;
		//@}


		/**
		 * If @bref{name}d attribute exists, place its value
		 * into @bref{output} and return @true. Otherwise,
		 * do not change @bref{output} and return @false.
		 */		
		bool try_get_attribute(const string& name, string& output) const;


		/**
		 * Delete attribute at valid @bref{it}erator.
		 * In doing so, the iterator becomes invalid.
		 */
		void remove_attribute(const attribute_iterator& it) NOTHROW;


		/** Replace all node children with new @bref{children}. */
		void set_children(const child_list& children);


		/**
		 * Create a new child node (as by respective constructors),
		 * and append that node to the end of the children list.
		 */
		//@{
		child_iterator insert_child(const string& name);
		child_iterator insert_child(const xml_node& other);
		child_iterator insert_child(const string& name, const xml_node& other);
		child_iterator insert_child(auto_pointer<xml_node> other);
		child_iterator insert_child(const string& name, auto_pointer<xml_node> other);
		//@}


		child_iterator first_child() NOTHROW;
		const_child_iterator first_child() const NOTHROW;
		child_iterator last_child() NOTHROW;
		const_child_iterator last_child() const NOTHROW;

		int64 child_count() const NOTHROW;
		int64 child_count(const string& name) const NOTHROW;

		bool has_children() const NOTHROW;
		bool has_child(const string& name) const NOTHROW;
		bool has_child(const string& name, int64 index) const NOTHROW;


		/**
		 * Look for a child node with given @bref{name},
		 * either at index zero or at an explicit @bref{index}.
		 * On failure, return a past-end iterator (find versions),
		 * or throw an exception (declarative versions).
		 */
		//@{	
		child_iterator find_child(const string& name) NOTHROW;
		child_iterator find_child(const string& name, int64 index) NOTHROW;
		const_child_iterator find_child(const string& name) const NOTHROW;
		const_child_iterator find_child(const string& name, int64 index) const NOTHROW;

		xml_node& child(const string& name);
		xml_node& child(const string& name, int64 index);
		const xml_node& child(const string& name) const;
		const xml_node& child(const string& name, int64 index) const;
		//@}


		/**
		 * If @bref{name}d child node exists at index zero
		 * or at an explicit @bref{index}, place its text
		 * into @bref{output} and return @true. Otherwise,
		 * do not change @bref{output} and return @false.
		 */		
		//@{
		bool try_get_child_text(const string& name, string& output) const;
		bool try_get_child_text(const string& name, int64 index, string& output) const;
		//@}


		/**
		 * Delete children subtree at valid @bref{it}erator.
		 * In doing so, the iterator becomes invalid.
		 */
		void remove_child(const child_iterator& it) NOTHROW;


		/**
		 * Delete all attributes and all children,
		 * then reset the node text(). Node name()		
		 * and node parent() remain unchanged.
		 */
		void clear() NOTHROW;


		/**
		 * Save XML branch represented by this node
		 * into some output target.
		 */
		//@{
		void save(const string& path, file_context& files) const;
		void save(file& output) const;
		//@}


		/**
		 * Create not-@NULL XML tree by parsing text
		 * from various sources.
		 */
		//@{
		static auto_pointer<xml_node> load
			(const string& path, file_context& files);
		static auto_pointer<xml_node> load
			(file& input);
		static auto_pointer<xml_node> load
			(const string& xml_text);
		//@}


		/**
		 * Recursively insert XML nodes with an ID attribute
		 * into the provided ID to node lookup @bref{map}.
		 * This call does not initially clear the lookup map.
		 * @attent Duplicate IDs are ignored completely.
		 */
		void create_id_to_node_map
				(tree_map<string, const xml_node&>& map) const;


		/**
		 * Verify internal consistency of data structure.
		 * Throws validation_failed() on any errors.
		 *
		 * Validation exists only for testing purposes.
		 * Clients should never need to call validate(),
		 * as XML nodes directly throw on any errors
		 * instead of becoming invalid.
		 */
		void validate() const;


	private:

		void validate_name() const;
		void validate_attributes() const;
		void validate_children_list_to_lookup() const;
		void validate_children_lookup_to_list() const;
		void validate_children_lookup() const;
		void validate_recursive() const;

		void swap_content_from_temporary(xml_node& other) NOTHROW;

		void recursive_save(file& output, int64 depth) const;


		static const int max_string_size_expat_ = 1073741824; // ~1GB

		const string name_;
		xml_node* parent_;

		string text_;
		attribute_map attributes_;

		child_list children_;
		typedef tree_map<string, dynamic_buffer<child_iterator>> child_map;
		child_map child_lookup_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__XML_HPP) */


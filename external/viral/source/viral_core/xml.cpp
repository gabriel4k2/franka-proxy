/**
 *************************************************************************
 *
 * @file xml.cpp
 *
 * In-memory XML tree implementation.
 *
 ************************************************************************/


#include "xml.hpp"


#if defined(VIRAL__HAVE_EXPAT)

	// On Windows, we need to tell expat it is statically linked,
	// otherwise, we geht nasty __dllimport magic.
	#if defined(VIRAL__OS_TARGET_WINDOWS)
		#define XML_STATIC
	#endif

    #include <expat.h>

#endif


#include "exception.hpp"
#include "log.hpp"
#include "scope_guard.hpp"
#include "util.hpp"
#include "file.hpp"
#include "file_util.hpp"
#include "xml_util.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// Expat-specific helpers
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__HAVE_EXPAT)

	/**
	 * Detail namespace for binding to the expat library.
	 * @ingroup file_module
	 */
	namespace expat_detail
	{

		/**
		 * Contains the current expat XML parsing stack.
		 * @ingroup file_module
		 */
		struct xml_parsing_state :
			private non_copyable
		{
			xml_parsing_state()
				: current_node(NULL) { }

			auto_pointer<xml_node> root;
			xml_node* current_node;
		};


		void start_element
			(void *userdata, const XML_Char* name, const XML_Char** attrs)
		{
			/**
			 * Add new element to the end of our node stack,
			 * so any subordinate elements and text are properly attached.
			 * For exception safety, the root of our stack is connected
			 * to an auto_pointer.
			 */

			xml_parsing_state &state =
				*reinterpret_cast<xml_parsing_state*>(userdata);

			if (!state.current_node)
			{
				state.root.reset(new xml_node(name));
				state.current_node = state.root.get();
			}
			else
			{
				state.current_node =				
					&*(state.current_node->insert_child(name));
			}

			// Use pointers to avoid any overflow,
			// as opposed to int64 indices.
			while (attrs[0] != NULL)
			{
				state.current_node->insert_attribute(attrs[0], attrs[1]);
				attrs += 2;
			}
		}


		void end_element
			(void *userdata, const XML_Char* name)
		{
			/**
			 * End currently pending XML element on the node stack.
			 * Use this opportunity to discard irrelevant whitespace.
			 */

			xml_parsing_state &state =
				*reinterpret_cast<xml_parsing_state*>(userdata);

			if (!state.current_node) return;
			
			state.current_node->set_text
				(state.current_node->text().compact());
			state.current_node =
				state.current_node->parent();
		}


		void character_data_handler
			(void *userdata, const XML_Char* s, int len)
		{
			/**
			 * Append incoming (not \0-terminated) XML text.
			 * In this context, note that the character handler
			 * may be called multiple times on each element.
			 */

			xml_parsing_state &state =
				*reinterpret_cast<xml_parsing_state*>(userdata);

			if (!state.current_node) return;

			state.current_node->set_text
				(state.current_node->text() + string(s, len));
		}
	}

#endif




//////////////////////////////////////////////////////////////////////////
//
// xml_node
//
//////////////////////////////////////////////////////////////////////////


xml_node::xml_node(const string& name)
	: name_(name), parent_(NULL)
		{ validate_name(); }

xml_node::xml_node
	(const string& name, const string& text)
		: name_(name), parent_(NULL), text_(text)
			{ validate_name(); }


xml_node::xml_node
	(const string& name,
	 const string& text,			
	 const attribute_map& attributes,
	 const child_list& children)
	:
	name_(name), parent_(NULL), text_(text)
{
	validate_name();			

	// Setters perform additional checks.
	set_attributes(attributes);
	set_children(children);
}


xml_node::xml_node(const xml_node& other)
	:
	name_(other.name_),
	parent_(NULL),
	text_(other.text_)	
{
	validate_name(); // Name should already be valid.

	// Setters perform additional checks.
	set_attributes(other.attributes_);
	set_children(other.children_);
}


xml_node::xml_node(const string& name, const xml_node& other)
	:
	name_(name),
	parent_(NULL),
	text_(other.text_)	
{
	validate_name();

	// Setters perform additional checks.
	set_attributes(other.attributes_);
	set_children(other.children_);
}


xml_node::xml_node(auto_pointer<xml_node> other)
	:
	name_(other ? other->name_ : string()), // Proper error handling below.
	parent_(NULL)
{
	if (!other)
	{
		LOG_ERROR("Cannot create XML node from NULL source node.");
		throw null_pointer();
	}

	validate_name(); // Name should already be valid.
	swap_content_from_temporary(*other);
}


xml_node::xml_node(const string& name, auto_pointer<xml_node> other)
	:
	name_(name),
	parent_(NULL)
{
	if (!other)
	{
		LOG_ERROR("Cannot create XML node from NULL source node.");
		throw null_pointer();
	}

	validate_name();
	swap_content_from_temporary(*other);
}


const string& xml_node::name() const NOTHROW
	{ return name_; }
xml_node* xml_node::parent() NOTHROW
	{ return parent_; }
const xml_node* xml_node::parent() const NOTHROW
	{ return parent_; }


void xml_node::set_text(const string& text)
	{ text_ = text; }
const string& xml_node::text() const NOTHROW
	{ return text_; }


void xml_node::set_attributes(const attribute_map& attributes)
{
	for (attribute_map::const_iterator it(attributes.first()); it; ++it)
	{
		if (!xml_util::is_xml_name_valid(it->key))
		{
			LOG_ERROR("Name: " + it->key + " is not valid for XML attribute.");
			throw invalid_parameter();
		}
	}

	attributes_ = attributes;
}


xml_node::attribute_iterator xml_node::insert_attribute(const string& name)
	{ return insert_attribute(name, string()); }

xml_node::attribute_iterator xml_node::insert_attribute
	(const string& name, const string& value)
{
	if (!xml_util::is_xml_name_valid(name))
	{
		LOG_ERROR("Name: " + name + " is not valid for XML attribute.");
		throw invalid_parameter();
	}

	if (attributes_.contains(name))
	{
		LOG_ERROR("XML attribute: " + name + " already exists.");
		throw xml_exception();
	}

	return attributes_.insert(name, value);
}


xml_node::attribute_iterator xml_node::first_attribute() NOTHROW
	{ return attributes_.first(); }
xml_node::const_attribute_iterator xml_node::first_attribute() const NOTHROW
	{ return attributes_.first(); }
xml_node::attribute_iterator xml_node::last_attribute() NOTHROW
	{ return attributes_.last(); }
xml_node::const_attribute_iterator xml_node::last_attribute() const NOTHROW
	{ return attributes_.last(); }

int64 xml_node::attribute_count() const NOTHROW
	{ return attributes_.size(); }
bool xml_node::has_attributes() const NOTHROW
	{ return !attributes_.empty(); }
bool xml_node::has_attribute(const string& name) const NOTHROW
	{ return find_attribute(name); }


xml_node::attribute_iterator xml_node::find_attribute
	(const string& name) NOTHROW
		{ return attributes_.find(name); }
xml_node::const_attribute_iterator xml_node::find_attribute
	(const string& name) const NOTHROW
		{ return attributes_.find(name); }


string& xml_node::attribute(const string& name)
{
	attribute_iterator find(attributes_.find(name));

	if (!find)
	{
		LOG_ERROR("Mandatory XML attribute: " + name + " is missing.");
		throw xml_exception();
	}

	return find->value;
}


const string& xml_node::attribute(const string& name) const
{
	const_attribute_iterator find(attributes_.find(name));

	if (!find)
	{
		LOG_ERROR("Mandatory XML attribute: " + name + " is missing.");
		throw xml_exception();
	}

	return find->value;
}


bool xml_node::try_get_attribute
	(const string& name, string& output) const
{
	const_attribute_iterator it(find_attribute(name));
	if (!it) return false;
	output = it->value;
	return true;
}


void xml_node::remove_attribute(const attribute_iterator& it) NOTHROW
	{ attributes_.remove(it); }


void xml_node::set_children(const child_list& children)
{
	/**
	 * Implementation notes:
	 * - We must insert all children individually to assign parents
	 *		and to build the by-name lookup structure.
	 * - To guarantee strong exception safety, we must swap out
	 *		data structures for current children and child lookup.
	 *		Then, we can identically restore these on any exception.
	 */


	// Swapping out is NOTHROW.
	child_list swapout_children;
	swap(children_, swapout_children);
	SCOPE_GUARD
		(children_guard,
			(children_, &child_list::swap_with,
			 guard_by_reference(swapout_children)));

	child_map swapout_child_lookup;
	swap(child_lookup_, swapout_child_lookup);
	SCOPE_GUARD
		(child_lookup_guard,
			(child_lookup_, &child_map::swap_with,
			 guard_by_reference(swapout_child_lookup)));


	// Any child insertion may fail,
	// e.g. when out of memory.
	for (const_child_iterator it(children.first()); it; ++it)
		insert_child(*it);


	// NOTHROW from here.
	child_lookup_guard.dismiss();
	children_guard.dismiss();
}


xml_node::child_iterator xml_node::insert_child(const string& name)
	{ return insert_child(auto_pointer<xml_node>(new xml_node(name))); }

xml_node::child_iterator xml_node::insert_child(const xml_node& other)
	{ return insert_child(auto_pointer<xml_node>(new xml_node(other))); }

xml_node::child_iterator xml_node::insert_child(const string& name, const xml_node& other)
	{ return insert_child(auto_pointer<xml_node>(new xml_node(name, other))); }

xml_node::child_iterator xml_node::insert_child(auto_pointer<xml_node> other)
	{ return insert_child(other ? other->name() : string(), std::move(other)); }

xml_node::child_iterator xml_node::insert_child
	(const string& name, auto_pointer<xml_node> other)
{
	/**
	 * Implementation notes:
	 * - We use both a list and a tree to allow for fast
	 *		sequential and by-name access to children nodes.
	 * - Since there can be multiple non-consecutive children
	 *		of the same name, we must use an intermediate
	 *		buffer for all children of a given name.
	 * - We implement all other insert_child() variants
	 *		through auto_pointer transfers to minimize
	 *		code redundancy for the minor runtime costs
	 *		of a few additional heap allocations.
	 */


	// Create and initialize new children node.
	// We cannot use the auto_pointer constructor,
	// since the list creates another node copy.
	if (!other)
	{
		LOG_ERROR("Cannot insert NULL child node.");
		throw null_pointer();
	}

	child_iterator it
		(children_.push_back(xml_node(name)));
	SCOPE_GUARD
		(del_guard_list, (children_, &child_list::pop_back));

	it->parent_ = this;
	it->swap_content_from_temporary(*other);


	// Try to add children node to existing lookup buffer.
	child_map::iterator bufit(child_lookup_.find(it->name_));
	if (bufit)
	{
		bufit->value.push_back(it);
		del_guard_list.dismiss();
		return it;
	}


	// Create new lookup buffer and insert child there.
	// Ugly function cast helps compiler to select the correct remove().
	bufit =
		child_lookup_.insert(it->name_, dynamic_buffer<child_iterator>());		

	SCOPE_GUARD
		(del_guard_buffer,
			(child_lookup_, &child_map::remove_by_iterator, bufit));

	bufit->value.push_back(it);

	del_guard_buffer.dismiss();
	del_guard_list.dismiss();
	return it;
}


xml_node::child_iterator xml_node::first_child() NOTHROW
	{ return children_.first(); }
xml_node::const_child_iterator xml_node::first_child() const NOTHROW
	{ return children_.first(); }
xml_node::child_iterator xml_node::last_child() NOTHROW
	{ return children_.last(); }
xml_node::const_child_iterator xml_node::last_child() const NOTHROW
	{ return children_.last(); }


int64 xml_node::child_count() const NOTHROW
	{ return children_.size(); }

int64 xml_node::child_count(const string& name) const NOTHROW
{
	child_map::const_iterator find(child_lookup_.find(name));
	if (!find) return 0;
	return find->value.size();
}


bool xml_node::has_children() const NOTHROW
	{ return !children_.empty(); }
bool xml_node::has_child(const string& name) const NOTHROW
	{ return find_child(name, 0); }
bool xml_node::has_child(const string& name, int64 index) const NOTHROW
	{ return find_child(name, index); }


xml_node::child_iterator xml_node::find_child
	(const string& name) NOTHROW
		{ return find_child(name, 0); }

xml_node::child_iterator xml_node::find_child
	(const string& name, int64 index) NOTHROW
{
	child_map::iterator it(child_lookup_.find(name));

	if (!it || index < 0 || index >= it->value.size())
		return child_iterator();

	return it->value[index];
}

xml_node::const_child_iterator xml_node::find_child
	(const string& name) const NOTHROW
		{ return find_child(name, 0); }

xml_node::const_child_iterator xml_node::find_child
	(const string& name, int64 index) const NOTHROW
{
	child_map::const_iterator it(child_lookup_.find(name));

	if (!it || index < 0 || index >= it->value.size())
		return const_child_iterator();

	return it->value[index];
}


xml_node& xml_node::child(const string& name)
	{ return child(name, 0); }

xml_node& xml_node::child(const string& name, int64 index)
{
	child_iterator find(find_child(name, index));

	if (!find)
	{
		LOG_ERROR("Mandatory child node: " + name + " is missing.");
		throw xml_exception();
	}

	return *find;
}

const xml_node& xml_node::child(const string& name) const
	{ return child(name, 0); }

const xml_node& xml_node::child(const string& name, int64 index) const
{
	const_child_iterator find(find_child(name, index));

	if (!find)
	{
		LOG_ERROR("Mandatory child node: " + name + " is missing.");
		throw xml_exception();
	}

	return *find;
}


bool xml_node::try_get_child_text
	(const string& name, string& output) const
		{ return try_get_child_text(name, 0, output); }

bool xml_node::try_get_child_text
	(const string& name, int64 index, string& output) const
{
	const_child_iterator it(find_child(name, index));
	if (!it) return false;
	output = it->text_;
	return true;
}


void xml_node::remove_child(const child_iterator& it) NOTHROW
{
	/**
	 * Implementation notes:
	 * - This call removes the child from the children list
	 *		and from the by-name lookup map. Potentially,
	 *		any empty lookup buffer must be deleted as well.
	 * - For valid input iterators, the index lookup buffer
	 *		always exists. It contains each iterator to any
	 *		child of a given name exactly once.
	 * - The order of the lookup buffer must be maintained
	 *		to enable index-based children queries.
	 */

	if (!it) return;

	child_map::iterator lookup
		(child_lookup_.find(it->name_));

	if (!lookup) return;

	dynamic_buffer<child_iterator>& entries =
		lookup->value;

	int64 index = 0;
	while (entries[index] != it) ++index;
	entries.remove_ordered(index);

	if (entries.empty())
		child_lookup_.remove(lookup);

	children_.remove(it);
}


void xml_node::clear() NOTHROW
{
	text_.clear();
	attributes_.clear();
	children_.clear();
	child_lookup_.clear();
}


void xml_node::save
	(const string& path, file_context& files) const
{
	auto_pointer<file> output
		(files.open_file(path, file::read_write_truncate));
	save(*output);
}

void xml_node::save(file& output) const
	{ recursive_save(output, 0); }


auto_pointer<xml_node> xml_node::load
	(const string& path, file_context& files)
{
	auto_pointer<file> input
		(files.open_file(path, file::read_only));
	return load(*input);
}

auto_pointer<xml_node> xml_node::load(file& input)
	{ return load(input.read_text()); }

auto_pointer<xml_node> xml_node::load(const string& xml_text)
{

#if defined(VIRAL__HAVE_EXPAT)

	// Initialize parser, handlers and state, and shutdown guard.
	XML_Parser parser(XML_ParserCreate(NULL));

	if (!parser)
	{
		LOG_ERROR("Failed to create XML parser instance.");
		throw xml_exception(); 
	}

	SCOPE_GUARD
		(parser_guard, (&XML_ParserFree, parser));
	

	expat_detail::xml_parsing_state state;
	XML_SetUserData(parser, &state);


	XML_SetStartElementHandler
		(parser, &expat_detail::start_element);
	XML_SetEndElementHandler
		(parser, &expat_detail::end_element);
	XML_SetCharacterDataHandler
		(parser, &expat_detail::character_data_handler);


	// Ensure that xml text fits into int-typed size of Expat.
	// Auto promotion to larger type.
	if (xml_text.size() > max_string_size_expat_)
	{
		LOG_ERROR("XML exceeds maximum supported size for Expat.");
		throw bounds_exceeded();
	}

	int valid_size =
		integer_operations::cast_or_throw<int>(xml_text.size());


	// Parse and return result on success.
	XML_Status retcode =
		XML_Parse(parser, xml_text.data(), valid_size, true);

	if (retcode != XML_STATUS_OK)
	{
		const char* msg = XML_ErrorString(XML_GetErrorCode(parser));
		int64 line = XML_GetCurrentLineNumber(parser);
		int64 column = XML_GetCurrentColumnNumber(parser);

		LOG_ERROR("Failed to parse XML data from file.");
		LOG_ERROR("Expat reported error: " + msg +
				  " in line: " + line + " on column: " + column);

		throw xml_exception();
	}

	if (!state.root)
	{
		LOG_CRITICAL("XML reading produced empty tree, but reported no error.");
		throw assert_failed();
	}

	return std::move(state.root);

#else

	LOG_ERROR("Not built with support for Expat / XML loading.");
	throw unsupported_feature();

#endif

}


void xml_node::create_id_to_node_map(tree_map<string, const xml_node&>& map) const
{
	const_attribute_iterator idit(find_attribute("id"));

	if (idit && idit->value != "")
		map.try_insert(idit->value, *this);

	for (const_child_iterator cit(first_child()); cit; ++cit)
		cit->create_id_to_node_map(map);
}


void xml_node::validate() const
{
	validate_name();
	validate_attributes();
	validate_children_list_to_lookup();
	validate_children_lookup_to_list();
	validate_children_lookup();
	validate_recursive();
}


void xml_node::validate_name() const
{
	if (!xml_util::is_xml_name_valid(name_))
	{
		LOG_ERROR("Name: " + name_ + " is not valid for XML nodes.");
		throw validation_failed();
	}
}


void xml_node::validate_attributes() const
{
	for (attribute_map::const_iterator it(attributes_.first()); it; ++it)
	{
		if (!xml_util::is_xml_name_valid(it->key))
		{
			LOG_ERROR("Name: " + it->key + " is not valid for XML attributes.");
			throw validation_failed();
		}
	}
}


void xml_node::validate_children_list_to_lookup() const
{
	/**
	 * Ensure that a matching lookup buffer contains
	 * each child from the child list exactly once.
	 */

	for (child_list::const_iterator it(children_.first()); it; ++it)
	{
		child_map::const_iterator bufferit
			(child_lookup_.find(it->name_));

		if (!bufferit)
		{
			LOG_ERROR("Child: " + it->name_ + " has no matching lookup buffer.");
			throw validation_failed();
		}

		bool child_in_lookup = false;

		for (int64 i = 0; i < bufferit->value.size(); ++i)
		{
			if (bufferit->value[i] != it) continue;

			if (child_in_lookup)
			{
				LOG_ERROR("Lookup buffer contains child: " + it->name_ + " twice.");
				throw validation_failed();
			}

			child_in_lookup = true;
		}	

		if (!child_in_lookup)
		{
			LOG_ERROR("Lookup buffer does not contain child: " + it->name_);
			throw validation_failed();
		}
	}
}


void xml_node::validate_children_lookup_to_list() const
{
	/**
	 * Ensure that each lookup buffer entry maps
	 * to exactly one child from the children list.
	 */

	for (child_map::const_iterator it(child_lookup_.first()); it; ++it)
	for (int64 i = 0; i < it->value.size(); ++i)
	{	
		if (!it->value[i])
		{
			LOG_ERROR("Lookup buffer contains invalid iterator.");
			throw validation_failed();
		}

		bool child_in_list = false;

		for (child_list::const_iterator childit(children_.first()); childit; ++childit)
		{
			if (childit != it->value[i]) continue;

			if (child_in_list)
			{
				LOG_ERROR("Children list contains child: " + it->key + " twice.");
				throw validation_failed();
			}

			child_in_list = true;
		}
	
		if (!child_in_list)
		{
			LOG_ERROR("Children list does not contain child: " + it->key);
			throw validation_failed();
		}
	}
}


void xml_node::validate_children_lookup() const
{
	/**
	 * Ensure that all lookup buffers are non-empty
	 * and contain only children of matching names.
	 */

	for (child_map::const_iterator it(child_lookup_.first()); it; ++it)
	{
		if (it->value.size() == 0)
		{
			LOG_ERROR("Lookup buffer may not be empty.");
			throw validation_failed();
		}

		for (int64 i = 0; i < it->value.size(); ++i)
		{	
			if (it->key != it->value[i]->name_)
			{
				LOG_ERROR("Lookup buffer has child of mismatched name.");
				throw validation_failed();
			}
		}
	}
}


void xml_node::validate_recursive() const
{
	for (child_list::const_iterator it(children_.first()); it; ++it)
	{
		if (it->parent_ != this)	
		{
			LOG_ERROR("Child in XML node has corrupt parent pointer.");
			throw validation_failed();
		}

		it->validate();
	}
}


void xml_node::swap_content_from_temporary(xml_node& other) NOTHROW
{
	/**
	 * Transfer content from temporary XML node.
	 * Upward hierarchy remains unchanged
	 * (i.e. this->name_ and this->parent_).
	 */

	swap(text_, other.text_);
	swap(attributes_, other.attributes_);
	swap(children_, other.children_);
	swap(child_lookup_, other.child_lookup_);

	for (child_iterator it(children_.first()); it; ++it)
		it->parent_ = this;
}


void xml_node::recursive_save(file& output, int64 depth) const
{
	string indent;
	indent.append('\t', depth);


	// Save opening tag.
	output.write_text(indent);
	output.write_text("<");
	output.write_text(name_);


	// Save all attributes
	const_attribute_iterator ita(first_attribute());
	while (ita)
	{
		output.write_text(" ");
		output.write_text(ita->key);
		output.write_text("=\"");
		output.write_text(xml_util::escape_entities(ita->value));
		output.write_text("\"");

		++ita;
	}


	// Save text, children, and closing tag.
	// Optimized formatting for text-less tags
	// and tags without further children.
	if (text_ == "" && !has_children())
	{
		output.write_text("/>\r\n");
	}
	else
	{
		output.write_text(">");
		output.write_text(xml_util::escape_entities(text_.compact()));

		if (has_children())
		{
			output.write_text("\r\n");
	
			const_child_iterator itc(first_child());	
			while (itc)
			{
				itc->recursive_save(output, depth + 1);
				++itc;
			}

			output.write_text(indent);
		}

		output.write_text("</");
		output.write_text(name_);
		output.write_text(">\r\n");
	}
}




} /* namespace viral_core */

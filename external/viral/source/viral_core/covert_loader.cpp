/**
 *************************************************************************
 *
 * @file covert_loader.cpp
 *
 * Loader for covert objects, implementation.
 *
 ************************************************************************/


#include "covert_loader.hpp"


#include "log.hpp"
#include "file.hpp"
#include "boolean_term.hpp"
#include "xml.hpp"
#include "xml_util.hpp"


namespace viral_core 
{


//////////////////////////////////////////////////////////////////////////
//
// covert_loader
//
//////////////////////////////////////////////////////////////////////////


covert_loader::~covert_loader() NOTHROW
	{ LOG_INFO("Destroyed: " + loader_name); }


void covert_loader::load(const xml_node& root)
{
	if (root.name() != "data")
	{
		LOG_ERROR("Root node of XML data must be named: data.");
		throw xml_exception();
	}

	xml_node local_root(root);
	evaluate_conditions(local_root, true);

	for (xml_node::child_iterator it(local_root.first_child()); it; ++it)
	{
		const string& data_type =
			it->attribute("type");

		data_handler_map::const_iterator handler
			(data_handlers_.find(data_type));

		if (!handler)
		{
			LOG_ERROR("Unknown type of data: " + data_type);
			throw xml_exception();
		}

		(this->*(handler->value))(*it, it->name());
	}
}


void covert_loader::load(file& source_file)
	{ load(*xml_node::load(source_file)); }
void covert_loader::load(const string& path)
	{ load(*xml_node::load(path, files)); }


covert_loader::covert_loader
	(const string& loader_name, file_context& files,
	 covert_data_cache& data_cache, covert_id_cache& id_cache)
	:
	loader_name(loader_name),
	files(files),
	data_cache(data_cache),
	id_cache(id_cache)
{
	LOG_INFO("Creating: " + loader_name);
}


void covert_loader::register_handler
	(const string& data_type, data_handler handler)
		{ data_handlers_.insert(data_type, handler); }


void covert_loader::evaluate_conditions
	(xml_node& node, bool covert_data_level) const
{
	/**
	 * Evaluate @bref{condition} tags on the first two levels only.
	 * This avoids attribute naming conflicts with unrelated XML modules.
	 */

	for (xml_node::child_iterator it(node.first_child()); it; )
	{
		xml_node::child_iterator child(it);
		++it;

		xml_node::attribute_iterator condition
			(child->find_attribute("condition"));

		if (condition &&
			!boolean_term::evaluate(condition->value, load_options))
		{
			node.remove_child(child);
			continue;
		}

		if (covert_data_level)
			evaluate_conditions(*child, false);
	}
}


void covert_loader::throw_id_type_mismatch(const string& name)
{
	LOG_ERROR("An ID with: " + name +
			  " with mismatching type is already registered");
	throw type_mismatch();
}




} /* namespace viral_core */
/**
 *************************************************************************
 *
 * @file collada_import.cpp
 *
 * Collada importer implementation.
 *
 ************************************************************************/


#include "collada_import.hpp"


#include "file.hpp"
#include "xml.hpp"
#include "mesh_util.hpp"
#include "collada_parser.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// collada_importer
//
//////////////////////////////////////////////////////////////////////////


model collada_importer::import_model
	(const xml_node& collada_root, model::model_usage_hint usage)
{
	collada_parser parser(collada_root, usage);
	return std::move(parser.result_model);
}


model collada_importer::import_model
	(const string& colladatext, model::model_usage_hint usage)
{
	auto_pointer<xml_node> root(xml_node::load(colladatext));
	return import_model(*root, usage);
}


model collada_importer::import_model
	(file& input, model::model_usage_hint usage)
		{ return import_model(input.read_text(), usage); }


model collada_importer::import_model
	(const string& path, file_context& files,
	 model::model_usage_hint usage)
{
	auto_pointer<file> input
		(files.open_file(path, file::read_only));
	return import_model(*input, usage);
}




} /* namespace viral_core */

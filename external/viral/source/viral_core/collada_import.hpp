/**
 *************************************************************************
 *
 * @file collada_import.hpp
 *
 * Collada importer interface.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__COLLADA_IMPORT_HPP)
#define INCLUDED__VIRAL_CORE__COLLADA_IMPORT_HPP


#include "build_config.hpp"


#include "file_forward.hpp"


#include "util.hpp"
#include "model.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class collada_importer
 * @ingroup mesh_module
 *
 * Imports @ref model%s from the XML-based Collada format.
 *
 * General notes:
 * - Clients should prefer the proprietary, binary model format
 *		of the framework (.vmsh files) over Collada import where
 *		possible for performance and stability reasons.
 * - Due to the open nature of the Collada standard, import is wonky
 *		and may not work with all exported files.
 * - Collada import is safe for an arbitrary number of threads at a time.
 *
 * Import traits:
 * - The importer does not extract material properties from Collada.
 *		These must explicitly be provided by another means.
 * - Names of model_group%s (e.g. for material assignment) are generated
 *		as @bref{geometry_id}.@bref{material}.@bref{counter}, where:
 *			- @bref{geometry_id} is the name of the Collada polygon soup,
 *			- @bref{material} is the name of the originally assigned material,
 *			- @bref{counter} is a one-based index that ensures uniqueness
 *				of each generated name.
 * - The @ref mesh class does not support Collada's individual indexing
 *		per vertex attribute (e.g. triangles with separate texcoord
 *		and normal indices). Therefore, import creates a new vertex
 *		for each and every triangle corner by duplicating all indexed
 *		vertex attributes. Later on, any redundant vertices created
 *		by this process are purged by model::optimize(). 
 * - The @ref model class does not support multiple skeleton rigs.
 *		Hence, the importer throws on instances of multiple skeletons.
 *		Multiple controller instances on the same skeleton are valid,
 *		and create multiple meshes in the result model.
 * - Import employs model::validate() to ensure that the resulting
 *		in-memory representation is fully valid.
 *
 ************************************************************************/
class collada_importer :
	private non_constructable
{
	public:

		/** Import a @ref model from various Collada XML sources. */
		//@{
		static model import_model
			(const xml_node& collada_root, model::model_usage_hint usage);
		static model import_model
			(const string& collada_text, model::model_usage_hint usage);
		static model import_model
			(file& input, model::model_usage_hint usage);
		static model import_model
			(const string& path, file_context& files,
			 model::model_usage_hint usage);
		//@}
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__COLLADA_IMPORT_HPP) */

/**
 *************************************************************************
 *
 * @file covert_loader.hpp
 *
 * Loading of covert objects.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__COVERT_LOADER_HPP)
#define INCLUDED__VIRAL_CORE__COVERT_LOADER_HPP


#include "build_config.hpp"


#include "file_forward.hpp"


#include "util.hpp"
#include "string.hpp"
#include "tree_set.hpp"
#include "covert_data.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class covert_loader
 * @ingroup covert_module
 *
 * Loads covert_data from some XML source.
 *
 * Basic XML layout:
 * - Example (reduced for simplicity):
 *		@verbatim
 *			<data>
 *				<wood_texture type="texture_2d">
 *					<texture_2d file="graphics/wood.tga" />
 *				</wood_texture>
 *				<diffuse_surface type="shader" condition="gl3">
 *					<vertex_shader file="shaders/diffuse.glvs" />
 *					<fragment_shader file="shaders/diffuse.glfs" />
 *				</diffuse_surface>
 *				<wood_material type="material">
 *					<texture ref="wood_texture" />
 *					<shader condition="gl3 && high_detail" ref="diffuse_surface" />
 *					<diffuse_color r="1" g="0.7" b="0.8" />
 *					<specular_exponent value="8" />
 *				</wood_material>
 *			</data>
 *		@endverbatim
 *
 * XML overview:
 * - Each first-level child of the XML root specifies a covert_data.
 *		The mandatory @bref{type} attribute determines the type
 *		of covert_data. The node tag gives a cache name for both
 *		the covert_data and its covert_id. Node contents determine
 *		members of the covert_data.
 * - By default, loading creates both covert_data and covert_id%s,
 *		then inserts these into @bref{data_cache} and @bref{id_cache}.
 *		If respective cache entries exist already, loading instead
 *		reuses IDs and replaces any data. Loading fails if the type
 *		of an existing ID and incoming data do not match.
 * - Optional @bref{condition} attributes specify a boolean_term
 *		on either data nodes or direct children nodes. The loader
 *		evaluates the conditions for current @bref{load_options}
 *		and discards any XML node whose condition is not met.
 * - Full specifications for all available covert_data types and
 *		their XML form exceed the scope of this documentation.
 *		Consult xml_converter and covert_loader implementations.
 *
 * ID and data management:
 * - References to covert_id%s within covert_data members resolve
 *		through the @bref{id_cache}. Loading fails if a referenced
 *		ID does not exist or does not have the required type.
 * - Clients may separately modify bound caches, for instance
 *		to link an XML file with code-generated covert_id%s.
 * - Loading operations do not commit created covert_data,
 *		but clients must explicitly do this after loading.
 *		Thus, intermediate commits can reduce memory load on
 *		large-scale data (e.g. image_raw%s or @ref model%s).
 *
 * General usage notes:
 * - A covert_loader only provides basic exception safety.
 *		In particular, caches may contain partially loaded
 *		covert_data or covert_id%s after any exception.
 * - For robust loading of additional data, clients should
 *		create a new covert_loader with new, offhand caches.
 *		These caches can then be discarded easily for full
 *		rollback after any exception.
 * - Unlike most subsystems, a covert_loader is not threadsafe
 *		and must only be used by one thread at a time.
 *
 * Implementation notes:
 * - Subsystems must derive from covert_loader and must register_handler()s 
 *		for supported, subsystem-specific covert_data on construction.
 *		The base covert_loader does not support any default data types.
 *
 ************************************************************************/
class covert_loader :
	private non_copyable
{

	public:

		virtual ~covert_loader() NOTHROW;


		/**
		 * Deserialize all covert_data from some XML source,
		 * such as a given virtual_path or an in-memory XML tree.
		 * Place results into construction-bound caches.
		 */
		//@{
		void load(const xml_node& root);
		void load(file& source_file);
		void load(const string& path);
		//@}


		const string loader_name;
		file_context& files;

		covert_data_cache& data_cache;
		covert_id_cache& id_cache;

		typedef tree_set<string> load_option_set;
		load_option_set load_options;


	protected:

		covert_loader
			(const string& loader_name, file_context& files,
			 covert_data_cache& data_cache, covert_id_cache& id_cache);


		typedef void (covert_loader::*data_handler)
			(const xml_node& node, const string& name);


		/**
		 * Register a new handler for some type of covert_data.
		 * Implementations can overwrite existing handlers in order
		 * to create implementation-specific covert_data.
		 */
		void register_handler
			(const string& data_type, data_handler handler);


		/**
		 * Create new covert_id of type @bref{T} and with @bref{name}
		 * or ascertain that any existing ID is of type @bref{T}.
		 */
		template <typename T>
			void request_covert_id(const string& name)
		{
			if (id_cache.find_exact<T>(name)) return;
			if (id_cache.contains(name))
				throw_id_type_mismatch(name);

			shared_instance<T> id(new T(name));
			id_cache.insert(name, id);
		}

		
	private:

		void evaluate_conditions
			(xml_node& node, bool covert_data_level) const;

		void throw_id_type_mismatch(const string& name);


		typedef tree_map<string, data_handler> data_handler_map;
		data_handler_map data_handlers_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__COVERT_LOADER_HPP) */


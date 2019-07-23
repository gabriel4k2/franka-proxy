/**
 *************************************************************************
 *
 * @file model.hpp
 *
 * Header for CPU-side models.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MODEL_HPP)
#define INCLUDED__VIRAL_CORE__MODEL_HPP


#include "build_config.hpp"


#include "file_forward.hpp"


#include "string.hpp"
#include "tree_map.hpp"
#include "owning_list.hpp"
#include "mesh.hpp"
#include "skeleton.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class model_group
 * @ingroup mesh_module
 *
 * Binds a name to some @ref mesh for use in a @ref model.
 *
 ************************************************************************/
class model_group :
	private non_copyable
{
	friend class model;

	public:

		const string name;
		::viral_core::mesh mesh;

	private:

		model_group(const string& name)
			: name(name) { }

		model_group(const string& name, const ::viral_core::mesh& mesh)
			: name(name), mesh(mesh) { }

		model_group(const string& name, ::viral_core::mesh&& mesh)
			: name(name), mesh(std::move(mesh)) { }

		model_group(const model_group& other)
			: name(other.name), mesh(other.mesh) { }
};




/**
 *************************************************************************
 *
 * @class model
 * @ingroup mesh_module
 *
 * Contains multiple @ref mesh%es.
 *
 * Data structure:
 * - Each model consists of ordered and uniquely named mesh_group%s.
 *		Each group in turn holds a @ref mesh with geometry data.
 * - Usually, there is one group for each component of the model.
 *		For instance, a group might correspond to an individually
 *		textured model part or a special collision zone.
 * - Clients can associate the name of a mesh_group within a model
 *		to user data, such as a material name or a collision ID.
 * - Group order within a model is independent from group names and
 *		provides individual mesh sorting, e.g. for rendering.
 *
 * General usage notes:	
 * - @attent Clients must explicitly call rebuild_boundings()
 *		after creating or updating model contents on their own.
 *		This enables correct model culling (e.g. in rendering).
 *		Clients may also want to validate() resulting models.
 * - For performance reasons, model contents (e.g. mesh_stream sizes
 *		or mesh_triangle indices) are not validate()d before each use.
 *		Thus, invalid model contents (e.g. due to client-side updates)
 *		might go undetected and might crash the application.
 *
 ************************************************************************/
class model
{

	public:

		typedef owning_list<model_group>::const_iterator group_iterator;


		/** Hint on the desired final usage of a model. */
		enum model_usage_hint
			{ any_usage, rendering_usage, physics_usage };


		/** Create model without any geometry. */
		model();


		/**
		 * Copy or move construct from @bref{other} model.
		 * Move construction leaves a valid, empty source.
		 */
		//@{
		model(const model& other);
		model(model&& other) NOTHROW;
		//@}


		/** Create not-@NULL heap copy of model and all its groups. */
		auto_pointer<model> clone() const;


		/**
		 * Copy or move assign of @bref{other} model.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty source.
		 */
		//@{
		model& operator =(const model& other);
		model& operator =(model&& other) NOTHROW;
		//@}


		void swap_with(model& other) NOTHROW;


		/**
		 * Insert a new @bref{name}d model_group into the model
		 * and optionally initialize it with given @bref{m}esh,
		 * or throw if there already is a group of given name.
		 */
		//@{
		group_iterator insert_group(const string& name);
		group_iterator insert_group(const string& name, const mesh& m);
		group_iterator insert_group(const string& name, mesh&& m);
		//@}


		int64 group_count() const;
		group_iterator first_group() const;
		group_iterator last_group() const;
		group_iterator find_group(const string& name) const;
		void remove_group(const string& name) NOTHROW;


		/**
		 * Apply a new @bref{skeleton_rig} to this model,
		 * destroying any previously assigned skeleton_rig().
		 * Setting a @NULL rig clears any rig assignment.
		 */
		void set_skeleton_rig
			(auto_pointer<skeleton> skeleton_rig) NOTHROW;


		/**
		 * Fetch current @ref skeleton, or return @NULL
		 * if the model currently has no skeleton rig.
		 */		
		const skeleton* skeleton_rig() const NOTHROW;


		/**
		 * Epsilon-less boundings over all contained meshes.
		 * After any model updates, invoke rebuild_boundings()
		 * to call mesh::rebuild_boundings() on contained meshes
		 * and to update bounding_box() and bounding_radius().
		 */
		//@{
		const box& bounding_box() const;
		float bounding_radius() const;
		void rebuild_boundings();
		//@}


		/**
		 * Potentially slow optimization of model geometry.
		 * Calls mesh::optimize() for all contained meshes,
		 * but does not automatically rebuild_boundings(). 
		 */
		void optimize();


		/**
		 * Call mesh::validate() on all contained meshes,
		 * and check model-wide content (e.g. joint indices).
		 * Validation is rather efficient and its overhead
		 * should be in the order of model initialization.
		 */
		void validate() const;


		/**
		 * Load model from various sources.
		 *
		 * General usage:
		 * - Loading may individually interpret the @bref{usage} hint,
		 *		for example to ignore irrelevant mesh_stream%s.
		 * - Loading already calls rebuild_boundings() and validate(),
		 *		clients do not have to perform redundant calls.
		 *
		 * File formats:
		 * - Collada .dae files: Due to slow parsing (see collada_importer)
		 *		and implicit call to model::optimize() (see mesh_optimize)
		 *		best suited as an import-only format.
		 * - Framework .vms files: Custom, binary, and block-based format.
		 *		Directly stores the contents of models, mesh%es and any
		 *		encapsuled mesh streams. Optimized for fast loading.
		 * - If no explicit file format is specified, the file format
		 *		is determined by the extension on the input file.
		 */
		//@{
		static model load
			(const string& path, file_context& files,
			 model_usage_hint usage);

		static model load_dae
			(file& input, model_usage_hint usage);
		static model load_vmsh
			(file& input, model_usage_hint usage);
		//@}


		/**
		 * Reset all model contents to an empty model(),
		 * including model groups, skeleton rig, and boundings.
		 */
		void clear() NOTHROW;


	private:

		group_iterator insert_group_helper
			(auto_pointer<model_group> group);


		typedef owning_list<model_group> group_list;
		group_list groups_;

		typedef tree_map<string, group_list::iterator> group_map;
		group_map group_lookup_;

		auto_pointer<skeleton> skeleton_rig_;

		box bounding_box_;
		float bounding_radius_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MODEL_HPP) */

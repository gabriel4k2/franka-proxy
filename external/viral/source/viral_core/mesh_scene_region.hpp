/**
 *************************************************************************
 *
 * @file mesh_scene_region.hpp
 *
 * Region tests on a mesh_scene.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MESH_SCENE_REGION_HPP)
#define INCLUDED__VIRAL_CORE__MESH_SCENE_REGION_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "bounding_tree.hpp"
#include "mesh_scene.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class mesh_scene_region_test
 * @ingroup mesh_module
 *
 * A bounding_region test for use in mesh_scene_region.
 *
 ************************************************************************/
class mesh_scene_region_test :
	private non_copyable
{
	template <typename H, typename R, typename T, typename X>
		friend class bounding_region;


	public:

		mesh_scene_region_test();


	private:

		bool try_node_region
			(const mesh_scene& scene,
			 const mesh_scene::const_node_iterator& node,
			 const sphere& s) const;
		bool try_node_region
			(const mesh_scene& scene,
			 const mesh_scene::const_node_iterator& node,
			 const box& b) const;

		bool try_leaf_region
			(const mesh_scene& scene,
			 const mesh_scene::const_node_iterator& node,
			 const sphere& s,
			 bounding_region_no_extended_result& extended_result) const;
		bool try_leaf_region
			(const mesh_scene& scene,
			 const mesh_scene::const_node_iterator& node,
			 const box& b,
			 bounding_region_no_extended_result& extended_result) const;
};




/**
 *************************************************************************
 *
 * @class mesh_scene_region_entry
 * @ingroup mesh_module
 *
 * A single entry in a mesh_scene_region_result.
 *
 ************************************************************************/
class mesh_scene_region_entry
{

	public:

		mesh_scene_region_entry
			(const mesh_scene::const_node_iterator& closest_scene_node,
			 const mesh_scene::object_iterator& closest_scene_object) NOTHROW
			:
			closest_scene_node(closest_scene_node),
			closest_scene_object(closest_scene_object)
		{ }


		/**
		 * Node and object in mesh_scene
		 * that are contained in a region.
		 */
		//@{
		const mesh_scene::const_node_iterator closest_scene_node;
		const mesh_scene::object_iterator closest_scene_object;
		//@}

};




/**
 *************************************************************************
 *
 * @class mesh_scene_region_result
 * @ingroup mesh_module
 *
 * Stores mesh_scene_region results in form of node iterators
 * into the mesh_scene.
 *
 ************************************************************************/
class mesh_scene_region_result
{
	friend class mesh_scene_region;


	public:

		typedef array<mesh_scene_region_entry> entry_array;
		const entry_array entries;


	private:

		mesh_scene_region_result() { }

		mesh_scene_region_result(const entry_array& entries)
			: entries(entries) { }
};




/**
 *************************************************************************
 *
 * @class mesh_scene_region
 * @ingroup mesh_module
 *
 * Find meshes in a mesh_scene that are fully inside a given region.
 *
 * Implementation notes:
 * - Current region queries only consider mesh_triangle%s
 *		and ignore any mesh_line%s or mesh_point%s.
 *
 ************************************************************************/
class mesh_scene_region :
	private non_constructable
{

	public:

		/**
		 * Find all meshes that are completely contained
		 * in a given @bref{s}phere or @bref{b}ox.
		 *
		 * Usage notes:
		 * - On any error, mesh_scene_region tests throw an exception.
		 * - See bounding_region for generic notes.
		 */
		//@{
		static mesh_scene_region_result region
			(const mesh_scene& scene,
			 const sphere& s);
		static mesh_scene_region_result region
			(const mesh_scene& scene,
			 const box& b);
		//@}


	private:

		typedef bounding_region
					<mesh_scene, sphere,
					 mesh_scene_region_test,
					 bounding_region_no_extended_result>
							bounding_region_mesh_scene_sphere;
		typedef bounding_region
					<mesh_scene, box,
					 mesh_scene_region_test,
					 bounding_region_no_extended_result>
							bounding_region_mesh_scene_box;

		template <typename T>
			static mesh_scene_region_result
				wrap_region_result(const T& region_result);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MESH_SCENE_REGION_HPP) */

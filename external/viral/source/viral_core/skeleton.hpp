/**
 *************************************************************************
 *
 * @file skeleton.hpp
 *
 * Low-level support for skeletal animation.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__SKELETON_HPP)
#define INCLUDED__VIRAL_CORE__SKELETON_HPP


#include "build_config.hpp"


#include "array.hpp"
#include "tree_map.hpp"
#include "string.hpp"
#include "geo_transform.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class skeleton_index
 * @ingroup mesh_module
 *
 * Joint indices within a @ref skeleton for use in a mesh_stream.
 *
 ************************************************************************/
class skeleton_index
{
	public:

		/**
		 * Maximum supported number of joints per vertex.
		 * Four joints is an ample number for smooth deformations,
		 * fits in a single entry of back-end rendering streams,
		 * and avoids dynamic memory allocation per vertex.
		 */
		static const int64 max_joints_per_vertex = 4;


		/**
		 * Set @bref{unused_index} together with a zero joint weight
		 * if there are less than @bref{max_joints_per_vertex}.
		 * We cannot use -1 without explicit handling in shaders.
		 * For a skeleton without joints, index 0 is invalid as well,
		 * but then, model::validate() throws on joint index checks.
		 */
		static const int64 unused_index = 0;


		skeleton_index() NOTHROW
			: a(unused_index), b(unused_index),
			  c(unused_index), d(unused_index) { }

		skeleton_index
			(int64 a, int64 b, int64 c, int64 d) NOTHROW
				: a(a), b(b), c(c), d(d) { }


		bool operator ==(const skeleton_index& other) const NOTHROW;
		bool operator !=(const skeleton_index& other) const NOTHROW;


		union
		{
			int64 C[max_joints_per_vertex];
			struct { int64 a, b, c, d; };
		};
};




/**
 *************************************************************************
 *
 * @class skeleton_weight
 * @ingroup mesh_module
 *
 * Weights for joints in an associated skeleton_index.
 *
 ************************************************************************/
class skeleton_weight
{
	public:

		static const int64 max_weights_per_vertex =
			skeleton_index::max_joints_per_vertex;

		skeleton_weight() NOTHROW
			: a(0), b(0), c(0), d(0) { }

		skeleton_weight
			(float a, float b, float c, float d) NOTHROW
				: a(a), b(b), c(c), d(d) { }


		bool operator ==(const skeleton_weight& other) const NOTHROW;
		bool operator !=(const skeleton_weight& other) const NOTHROW;


		union
		{
			float C[max_weights_per_vertex];
			struct { float a, b, c, d; };
		};
};




/**
 *************************************************************************
 *
 * @class skeleton_joint
 * @ingroup mesh_module
 *
 * A single joint within a @ref skeleton.
 *
 ************************************************************************/
class skeleton_joint
{

	public:

		skeleton_joint
			(const string& name,
			 int64 parent, int64 first_child, int64 child_count,
			 const si_transform& joint_to_parent,
			 const si_transform& binding_to_joint);


		/** Maximum number of allowed children per joint. */
		static const int64 max_child_count = 128;


		/**
		 * Name of the joint.
		 * Must be unique within any owning @ref skeleton.
		 */
		const string name;


		/**
		 * Parent and children indices within skeleton::joints().
		 * Children occupy @bref{child_count} contiguous indices,
		 * starting with the @bref{first_child} index.
		 */
		//@{
		const int64 parent;
		const int64 first_child;
		const int64 child_count;
		//@}


		/**
		 * Relative transforms from a joint
		 * to its parent in binding pose.
		 */
		//@{
		const si_transform joint_to_parent;
		const matrix joint_to_parent_matrix;
		//@}


		/**
		 * Transforms from world-space binding pose
		 * to the joint-local coordinate system.
		 */
		//@{
		const si_transform binding_to_joint;
		const matrix binding_to_joint_matrix;	
		//@}
};


/**
 * An array of skeleton_joint%s for use with a @ref skeleton.
 * @ingroup mesh_module
 */
typedef array<skeleton_joint> skeleton_joint_array;


/**
 * An array of skeleton_joint transforms,
 * with an entry for each skeleton_joint
 * of some target @ref skeleton.
 */
typedef array<si_transform> skeleton_transform;


/**
 * A matrix-based variant of skeleton_transform.
 * Usually only used for efficient vertex transforms,
 * but with numerical robustness and handling issues.
 */
typedef array<matrix> skeleton_transform_matrix;




/**
 *************************************************************************
 *
 * @class skeleton
 * @ingroup mesh_module
 *
 * Stores a hierarchy of skeleton_joint%s.
 *
 ************************************************************************/
class skeleton
{
	public:

		/**
		 * Create a skeleton from given @bref{joints}.
		 * The root joint must be the first entry in the @bref{joints} array.
		 * Constructors automatically validate() resulting skeletons.
		 */
		//@{
		skeleton(const skeleton_joint_array& joints);
		skeleton(skeleton_joint_array&& joints);
		//@}


		const skeleton_joint_array& joints() const NOTHROW;


		/**
		 * Fetch joint by @bref{index} or @bref{name}.
		 * Throws if there is no such joint.
		 */
		//@{
		const skeleton_joint& joint(int64 index) const;
		const skeleton_joint& joint(const string& name) const;
		//@}


		/**
		 * Build a skeleton_transform that maps a per-joint
		 * @bref{relative_pose} to an @bref{absolute_pose}.
		 *
		 * The resulting @bref{absolute_pose} transforms
		 * from model-space through @bref{binding_to_joint},
		 * @bref{relative_pose}, and finally parent poses.
		 * Thus coordinates end up back in model-space.
		 *
		 * The call clears any existing @bref{absolute_pose}.
		 * On any error, @bref{absolute_pose} remains empty.
		 */
		void build_absolute_pose
			(const skeleton_transform& relative_pose,
			 skeleton_transform& absolute_pose) const;


		/**
		 * Utility variants of build_absolute_pose()
		 * that create to-pose transformation matrices
		 * for model-space vertices and normals.
		 */
		//@{
		void build_absolute_pose_matrix
			(const skeleton_transform& relative_pose,
			 skeleton_transform_matrix& absolute_pose_matrix) const;
		void build_absolute_normals_matrix
			(const skeleton_transform& relative_pose,
			 skeleton_transform_matrix& absolute_normals_matrix) const;
		//@}


		/**
		 * Check the skeleton and its joint hierarchy,
		 * and throw validation_failed on any errors
		 * (e.g. invalid indices, maximum height exceeded).
		 *
		 * Validation is rather efficient and its overhead
		 * should be in the order of joint initialization.
		 */
		void validate() const;


		/**
		 * Index used for the parent of the root joint
		 * and for the first child of each leaf joint.
		 *
		 * @attent Do not use this in skeleton_index,
		 * use skeleton_index::unused_index instead.
		 */
		static const int64 invalid_index = -1;


		/**
		 * Maximum hierarchy height allowed by validate().
		 * Avoids stack overflow errors on recursion.
		 */
		static const int64 maximum_height = 128;


	private:

		void build_absolute_pose
			(const skeleton_transform& relative_pose,
			 skeleton_transform& absolute_pose,
			 int64 joint_index) const;

		void init_joint_name_to_index();

		void validate
			(int64 joint_index, int64 parent_index,
			 int64 parent_height, int64& joints_visited) const;


		skeleton_joint_array joints_;

		typedef tree_map<string, int64> joint_name_to_index_map;
		joint_name_to_index_map joint_name_to_index_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__SKELETON_HPP) */

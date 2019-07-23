/**
 *************************************************************************
 *
 * @file model_util.hpp
 *
 * Utilities for models.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MODEL_UTIL_HPP)
#define INCLUDED__VIRAL_CORE__MODEL_UTIL_HPP


#include "build_config.hpp"


#include "mesh_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class model_util
 * @ingroup mesh_module
 *
 * Holds various static utility methods that work on the @ref mesh%es
 * stored inside a @ref model.
 *
 ************************************************************************/
class model_util :
	private non_constructable
{

	public:

		/**
		 * Flatten all model_group%s of the provided @bref{m}odel
		 * into a single @ref mesh.
		 *
		 * Usage notes:
		 * - Joining tries to merge all mesh_stream%s with same name,
		 *		same type, and same semantic (e.g. per-triangle stream)
		 *		over all the model_group%s in the input @bref{m}odel.
		 * - Any mesh_stream%s that mismatch in type over all groups
		 *		or that occur only in some groups are silently ignored
		 *		and are subsequently missing from the output mesh.
		 * - Joining automatically adapts vertex indices in mesh_point%s,
		 *		mesh_line%s, and mesh_triangle%s to the merged vertices.
		 * - Joining silently ignores any mesh_stream%s with custom types,
		 *		but correctly copies custom streams of predefined types.
		 * - Empty input models (i.e. models without model_group%s)
		 *		result in empty meshes.
		 *
		 * Implementation notes:
		 * - Joining always validate()s the resulting mesh before return,
		 *		which requires less time than actual joining.
		 */
		static mesh join_model_groups(const model& m);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MODEL_UTIL_HPP) */

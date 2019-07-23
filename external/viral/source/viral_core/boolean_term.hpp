/**
 *************************************************************************
 *
 * @file boolean_term.hpp
 *
 * Evaluation of boolean terms.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOOLEAN_TERM_HPP)
#define INCLUDED__VIRAL_CORE__BOOLEAN_TERM_HPP


#include "build_config.hpp"


#include "string_forward.hpp"
#include "container_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class boolean_term
 * @ingroup string_module
 *
 * Evaluates boolean terms for given variable values.
 *
 * Usage notes:
 * - Boolean terms use popular C++ notation.
 * - Variables must be C++ identifiers (i.e. [a-zA-Z0-9_]+).
 * - Supported operators are !, ||, and &&.
 * - Evaluation order and precedence rules match those of C++.
 * - Arbitrary whitespace can separate identifiers and operators.
 *
 * Example terms:
 * @verbatim render_shadows @endverbatim
 * @verbatim high_quality_audio   &&   3d_audio @endverbatim
 * @verbatim debug_mode && !(windows || unix) @endverbatim
 * @verbatim
 *		high_quality_shaders && (gl3_supported||gl4_supported)
 * @endverbatim
 * @verbatim
 *		direct_x && dx11_supported ||
 *		open_gl  && gl3_supported  && !gl3_core
 * @endverbatim
 *
 ************************************************************************/
class boolean_term :
	private non_constructable
{

	public:

		typedef tree_set<string> option_set;


		/**
		 * Evaluate @bref{term} with variable values given
		 * in @bref{options}.
		 *
		 * Each variable included in the @bref{options} set
		 * evaluates to @true. Variables not contained in
		 * @bref{options} are regarded as @false.
		 *
		 * @bref{term} must be a valid, non-empty boolean 
		 * term. Otherwise, parsing throws a syntax_error.
		 */
		static bool evaluate
			(const string& term, const option_set& options);


	private:

		static bool evaluate_variable
			(const string& term, int64& parsing_position,
			 const option_set& options);
		static bool evaluate_not
			(const string& term, int64& parsing_position,
			 const option_set& options);
		static bool evaluate_and
			(const string& term, int64& parsing_position,
			 const option_set& options);
		static bool evaluate_or
			(const string& term, int64& parsing_position,
			 const option_set& options, bool is_root_term);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOOLEAN_TERM_HPP) */

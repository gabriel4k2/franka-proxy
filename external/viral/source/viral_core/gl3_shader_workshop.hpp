/**
 *************************************************************************
 *
 * @file gl3_shader_workshop.hpp
 *
 * Creating and preprocessing GLSL shaders.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_SHADER_WORKSHOP_HPP)
#define INCLUDED__VIRAL_CORE__GL3_SHADER_WORKSHOP_HPP


#include "build_config.hpp"


#include "file_forward.hpp"


#include "util.hpp"
#include "list.hpp"
#include "string.hpp"
#include "tree_map.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_shader_bricks
 * @ingroup gl3_render_module
 * 
 * Holds ready-made bricks of GLSL source code, such as uniform blocks
 * or lighting utility functions.
 *
 * Usage notes:
 * - Clients should use a gl3_shader_workshop to add shader bricks
 *		into existing GLSL shader code.
 * - In contrast to entirely hand-written shaders, gl3_shader_bricks
 *		avoid redundancy and always match gl3_kernel requirements.
 *		Thus, clients should use shader bricks where possible.
 *
 ************************************************************************/
class gl3_shader_bricks :
	private non_constructable
{

	public:

		static string camera_uniforms();
		static string skeleton_uniforms();
		static string material_uniforms();
		static string light_uniforms();
		static string shadow_uniforms();
};




/**
 *************************************************************************
 *
 * @class gl3_shader_workshop
 * @ingroup gl3_render_module
 *
 * Performs various preprocessing actions on GLSL source code,
 * such as resolving gl3_shader_bricks or include directives.
 *
 * Shader preprocessing:
 * - Specific directives control preprocessing by gl3_shader_workshop.
 *		All directives must be on standalone lines in the form of
 *			\#VIRAL__{DIRECTIVE} {params}
 *		with arbitrary whitespace before, between, and after
 *		individual directive components.
 * - The directive \#VIRAL__INCLUDE "include_file" pastes
 *		the contents of a given virtual_path @bref{include_file}
 *		on the input @bref{files} context into the shader source.
 *		The shader workshop recursively runs on included files.
 * - The directive \#VIRAL__SHADER_BRICK "brick_name" inserts
 *		a ready-made code snippet with given @bref{brick_name}
 *		from gl3_shader_bricks into the shader source. 
 * - Preprocessing removes all single- and multi-line comments,
 *		including any directives within these comments.
 *
 * Usage notes:
 * - The rendering backend (e.g. gl3_program_object) does not apply
 *		gl3_shader_workshop to incoming GLSL sources automatically.
 *		When required, clients must explicitly pass shader sources
 *		(e.g. in render_shader_data) through a gl3_shader_workshop.
 * - By default, a gl3_loader applies the gl3_shader_workshop
 *		to all incoming shader sources. Clients can suppress
 *		this behaviour with a @false "shader_workshop" attribute.
 *
 ************************************************************************/
class gl3_shader_workshop :
	private non_copyable
{

	public:

		enum shader_type
			{ vertex_shader, fragment_shader };


		/**
		 * Preprocess @bref{shader} source code
		 * of given shader @bref{type}.
		 *
		 * Optionally, resolve include directives
		 * through @bref{files} context.
		 */
		//@{
		static string preprocess
			(const string& shader, shader_type type);
		static string preprocess
			(const string& shader, shader_type type,
			 file_context& files);
		//@}


	private:

		gl3_shader_workshop
			(const string& shader,
			 shader_type type,
			 file_context* files,
			 int64 current_include_depth);


		static tree_map<string, string> create_shader_bricks();

		void prepare_lines
			(const string& shader);

		void resolve_directives();
		void resolve_directive
			(const list<string>& line_terms,
			 list<string>::iterator current_line);			
		void resolve_include
			(const string& source_file,
			 list<string>::iterator include_line);
		void resolve_shader_brick
			(const string& shader_brick,
			 list<string>::iterator brick_line);

		void replace_line
			(const string& new_lines,
			 list<string>::iterator old_line);

		void build_result();


		const shader_type type_;
		file_context* const files_;

		list<string> lines_;

		static const int64 max_include_depth_ = 8;
		int64 current_include_depth_;		

		typedef tree_map<string, string> shader_brick_map;
		const shader_brick_map shader_bricks_;

		string result_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_SHADER_WORKSHOP_HPP) */

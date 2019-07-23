/**
 *************************************************************************
 *
 * @file gl3_shader_workshop.cpp
 *
 * Creating and preprocessing GLSL shaders, implementation.
 *
 ************************************************************************/


#include "gl3_shader_workshop.hpp"


#include "log.hpp"
#include "integer_checks.hpp"
#include "file.hpp"
#include "render_scene.hpp"
#include "gl3_kernel_util.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// gl3_shader_bricks
//
//////////////////////////////////////////////////////////////////////////


string gl3_shader_bricks::camera_uniforms()
{
	return
		"// gl3_shader_bricks::camera_uniforms()\n"
		"#define VIRAL__CAMERA_UNIFORMS\n"
		"uniform mat4 projection;\n"
		"uniform vec3 camera_position;\n"
		"uniform mat4 world_to_camera;\n"
		"uniform mat4 model_to_world;\n"
		"\n"
		"\n";
}


string gl3_shader_bricks::skeleton_uniforms()
{
	return
		string() +
	"// gl3_shader_bricks::skeleton_uniforms()\n"
	"#define VIRAL__SKELETON_UNIFORMS\n"
	"#define VIRAL__MAX_SKELETON_JOINTS " +
				 string(gl3_kernel_limits::max_skeleton_joints) + "\n"
	"uniform mat4 skeleton_absolute_pose[VIRAL__MAX_SKELETON_JOINTS];\n"
	"uniform int skeleton_joint_count;\n"
	"\n"
	"\n";
}


string gl3_shader_bricks::material_uniforms()
{
	return
		"// gl3_shader_bricks::material_uniforms()\n"
		"#define VIRAL__MATERIAL_UNIFORMS\n"
		"uniform sampler2D texture;\n"
		"uniform sampler2D normal_map;\n"
		"\n"
		"uniform vec4 material_emissive_color;\n"
		"uniform vec4 material_ambient_color;\n"
		"uniform vec4 material_diffuse_color;\n"
		"uniform vec4 material_specular_color; \n"
		"uniform float material_specular_exponent;\n"
		"\n"
		"\n";
}


string gl3_shader_bricks::light_uniforms()
{
	return
		string() +
	"// gl3_shader_bricks::light_uniforms()\n"
	"#define VIRAL__LIGHT_UNIFORMS\n"
	"#define VIRAL__MAX_PASS_LIGHTS " +
			string(gl3_kernel_limits::max_pass_lights) + "\n"
	"\n"
	"#define VIRAL__EMITTER_POINT " +
			string(render_light_data::emitter_point) + "\n"
	"#define VIRAL__EMITTER_SPOT " +
			string(render_light_data::emitter_spot) + "\n"
	"#define VIRAL__EMITTER_PARALLEL " +
			string(render_light_data::emitter_parallel) + "\n"
	"\n"
	"uniform int light_emitter[VIRAL__MAX_PASS_LIGHTS];\n"
	"uniform vec3 light_position[VIRAL__MAX_PASS_LIGHTS];\n"
	"uniform vec3 light_direction[VIRAL__MAX_PASS_LIGHTS];\n"
	"\n"
	"uniform vec4 light_ambient_color[VIRAL__MAX_PASS_LIGHTS];\n"
	"uniform vec4 light_diffuse_color[VIRAL__MAX_PASS_LIGHTS];\n"
	"uniform vec4 light_specular_color[VIRAL__MAX_PASS_LIGHTS];\n"
	"\n"
	"uniform float light_intensity[VIRAL__MAX_PASS_LIGHTS];\n"
	"uniform float light_attenuation[VIRAL__MAX_PASS_LIGHTS];\n"
	"\n"
	"uniform float light_spot_cosine[VIRAL__MAX_PASS_LIGHTS];\n"
	"uniform float light_spot_exponent[VIRAL__MAX_PASS_LIGHTS];\n"
	"\n"
	"uniform int light_shadow_enabled[VIRAL__MAX_PASS_LIGHTS];\n"
	"uniform float light_shadow_opacity[VIRAL__MAX_PASS_LIGHTS];\n"
	"\n"
	"uniform int light_count;\n"
	"\n"
	"\n";
}


string gl3_shader_bricks::shadow_uniforms()
{
	return
		string() +
	"// gl3_shader_bricks::shadow_uniforms()\n"
	"#define VIRAL__SHADOW_UNIFORMS\n"
	"#define VIRAL__SHADOW_MAP_CASCADE_COUNT" +
			string(gl3_kernel_limits::shadow_map_cascade_count) + "\n"
	"\n"
	"uniform samplerCube shadow_map_point;\n"
	"uniform sampler2D shadow_map_spot;\n"
	"uniform sampler2DArray shadow_map_parallel;\n"
	"\n"
	"uniform float shadow_point_z_factor;\n"
	"uniform float shadow_point_z_offset;\n"
	"uniform mat4 shadow_spot_world_to_texture;\n"
	"uniform mat4 shadow_parallel_world_to_texture[VIRAL__SHADOW_MAP_CASCADE_COUNT];\n"
	"uniform int shadow_parallel_valid[VIRAL__SHADOW_MAP_CASCADE_COUNT];\n"
	"\n"
	"\n";
}




//////////////////////////////////////////////////////////////////////////
//
// gl3_shader_workshop
//
//////////////////////////////////////////////////////////////////////////


string gl3_shader_workshop::preprocess
	(const string& shader, shader_type type)
{
	gl3_shader_workshop workshop(shader, type, NULL, 0);
	return workshop.result_;
}


string gl3_shader_workshop::preprocess
	(const string& shader, shader_type type,
	 file_context& files)
{
	gl3_shader_workshop workshop(shader, type, &files, 0);
	return workshop.result_;
}


gl3_shader_workshop::gl3_shader_workshop
	(const string& shader,
	 shader_type type,
	 file_context* files,
	 int64 current_include_depth)
	:
	type_(type),
	files_(files),
	current_include_depth_(current_include_depth),
	shader_bricks_(create_shader_bricks())
{
	prepare_lines(shader);
	resolve_directives();
	build_result();
}


tree_map<string, string>
	gl3_shader_workshop::create_shader_bricks()
{
	tree_map<string, string> shader_bricks;
	shader_bricks.insert("camera_uniforms", gl3_shader_bricks::camera_uniforms());
	shader_bricks.insert("skeleton_uniforms", gl3_shader_bricks::skeleton_uniforms());
	shader_bricks.insert("material_uniforms", gl3_shader_bricks::material_uniforms());
	shader_bricks.insert("light_uniforms", gl3_shader_bricks::light_uniforms());
	shader_bricks.insert("shadow_uniforms", gl3_shader_bricks::shadow_uniforms());
	return shader_bricks;
}


void gl3_shader_workshop::prepare_lines
	(const string& shader)
{
	/**
	 * Prepare some @bref{shader} for use in gl3_shader_workshop:
	 * remove comments, make line-ends canonical, split lines.
	 */

	string shader_canonical(shader);
	int64 shader_size = shader_canonical.size();

	bool in_multi_line_comment = false;
	bool in_single_line_comment = false;

	for (int64 i = 0; i < shader_size; ++i)
	{
		char curchar = shader_canonical[i];		
		char nextchar =
			(i < shader_size - 1) ?
				shader_canonical[i + 1] : '\0'; 


		// Check for start and end of multi-line comments.
		if (!in_multi_line_comment &&
			!in_single_line_comment &&
			curchar == '/' &&
			nextchar == '*')
		{
			in_multi_line_comment = true;

			shader_canonical[i] = ' ';
			shader_canonical[i + 1] = ' ';

			++i; // and ++i on continue, consume /* marker.
			continue;
		}

		if (in_multi_line_comment &&
			curchar == '*' &&
			nextchar == '/')
		{
			in_multi_line_comment = false;

			shader_canonical[i] = ' ';
			shader_canonical[i + 1] = ' ';

			++i; // and ++i on continue, consume */ marker.
			continue;
		}


		// Check for start and end of single-line comments.
		if (!in_multi_line_comment &&
			!in_single_line_comment &&
			curchar == '/' &&
			nextchar == '/')
		{
			in_single_line_comment = true;

			shader_canonical[i] = ' ';
			shader_canonical[i + 1] = ' ';

			++i; // and ++i on continue, consume // marker.
			continue;
		}

		if (in_single_line_comment &&
			curchar == '\n')
		{
			// Don't remove \n, otherwise we merge two lines.
			in_single_line_comment = false;
			continue;
		}

		
		// Make any line ends canonical,
		// and remove all comments.
		if (curchar == '\r' ||
			in_multi_line_comment ||
			in_single_line_comment)
				shader_canonical[i] = ' ';
	}

	if (in_multi_line_comment)
	{
		LOG_ERROR("Multi-line GLSL comment does not end in shader: " + shader);
		throw invalid_parameter();
	}


	// Split canonical string into lines
	// for later directive parsing.
	lines_.clear();
	shader_canonical.split('\n', lines_);
}


void gl3_shader_workshop::resolve_directives()
{
	list<string>::iterator next_line(lines_.first());
	while (next_line)
	{
		list<string>::iterator current_line(next_line);
		++next_line;


		// Fast early-out on any unrelated lines.
		// We assume that every line with a #VIRAL__ tag
		// is a workshop directive.
		if (!current_line->contains("#VIRAL__"))
			continue;		


		// Split line into individual directive terms,
		// then resolve the directive.
		string line_string
			(current_line->trim().compact());

		list<string> line_terms;
		line_string.split(' ', line_terms);

		resolve_directive
			(line_terms, current_line);
	}
}


void gl3_shader_workshop::resolve_directive
	(const list<string>& line_terms,
	 list<string>::iterator current_line)
{
	/**
	 * Check @bref{line_terms} of @bref{current_line}
	 * for preprocessing directives and execute these.
	 */

	if (line_terms.empty())
	{
		LOG_ERROR("Empty shader directive satisfied pretest.")
		throw assert_failed();
	}

	string directive(*line_terms.first());
	if (directive == "#VIRAL__INCLUDE")
	{
		if (line_terms.size() != 2)
		{
			LOG_ERROR("Invalid usage of #VIRAL__INCLUDE.");
			throw syntax_error();
		}

		resolve_include
			(line_terms.last()->unenquote(), current_line);
	}
	else if (directive == "#VIRAL__SHADER_BRICK")
	{
		if (line_terms.size() != 2)
		{
			LOG_ERROR("Invalid usage of #VIRAL__INCLUDE.");
			throw syntax_error();
		}

		resolve_shader_brick
			(*line_terms.last(), current_line);
	}
	else
	{
		LOG_ERROR("Unknown #VIRAL directive: " + directive);
		throw syntax_error();
	}
}


void gl3_shader_workshop::resolve_include
	(const string& source_path,
	 list<string>::iterator include_line)
{
	/**
	 * Replace inclusion directive on @bref{include_line}
	 * with contents of file at virtual @bref{source_path}.
	 * Recursively applies preprocessing to included files.
	 */

	if (!files_)
	{
		LOG_ERROR("Shader includes not supported in current context.");
		throw unsupported_feature();
	}

	if (current_include_depth_ + 1 > max_include_depth_)
	{
		LOG_ERROR("Include of: " + source_path +
				  " exceeds maximum depth: " + max_include_depth_);
		throw bounds_exceeded();
	}

	auto_pointer<file> source_file
		(files_->open_file(source_path, file::read_only));

	string include_shader
		(string("// Include start: ")  + source_path + "\n" +
		 source_file->read_text() +
		 string("// Include end: ") + source_path + "\n");
	
	gl3_shader_workshop include_workshop
		(include_shader, type_, files_,
		 current_include_depth_ + 1);

	replace_line
		(include_workshop.result_, include_line);
}


void gl3_shader_workshop::resolve_shader_brick
	(const string& shader_brick,
	 list<string>::iterator brick_line)
{
	/**
	 * Replace shader brick directive on @bref{brick_line}
	 * with code of respective @bref{shader_brick}.
	 */

	shader_brick_map::const_iterator find
		(shader_bricks_.find(shader_brick));

	if (!find)
	{
		LOG_ERROR("Unknown shader brick: " + shader_brick);
		throw not_found();
	}

	string brick_source
		(string("// Shader brick: ") + shader_brick + "\n" +
		 find->value);

	replace_line
		(brick_source, brick_line);
}


void gl3_shader_workshop::replace_line
	(const string& new_lines,
	 list<string>::iterator old_line)
{
	/**
	 * Replace @bref{old_line} with one or more lines
	 * from newline-separated @bref{new_lines} string.
	 */

	string new_lines_normalized(new_lines);
	new_lines_normalized.remove_all('\r');

	list<string> new_lines_list;
	new_lines_normalized.split('\n', new_lines_list);

	for (list<string>::const_iterator it
			(new_lines_list.last()); it; --it)
	{
		lines_.insert_after(old_line, *it);
	}

	lines_.remove(old_line);
}


void gl3_shader_workshop::build_result()
{
	int64 result_size = 0;
	for (list<string>::const_iterator it(lines_.first()); it; ++it)
		result_size =
			integer_operations::add_or_throw(result_size, it->size());

	result_.reserve(result_size);
	for (list<string>::const_iterator it(lines_.first()); it; ++it)
	{		
		result_ += *it;
		result_.append('\n');
	}
}




} /* namespace viral_core */

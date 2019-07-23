/**
 *************************************************************************
 *
 * @file gl3_api.cpp
 *
 * Capabilities and functions for OpenGL3, implementation.
 *
 ************************************************************************/


#include "gl3_api.hpp"


#include "string.hpp"
#include "log.hpp"
#include "list.hpp"
#include "tree_set.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// gl3_api utilities
//
//////////////////////////////////////////////////////////////////////////


/**
 * Namespace with utilities for gl3_api.
 * @ingroup gl3_render_module
 */
namespace gl3_api_detail
{

	#if defined(VIRAL__HAVE_MS_WINDOW)

		#define GET_PROC_ADDRESS(procname) \
			gl3_api_detail::get_proc_address(procname, #procname)

		template <typename T>
			void get_proc_address(T *&fnptr, const char* proc_name)
		{
			fnptr =
				reinterpret_cast<T*>(wglGetProcAddress(proc_name));

			if (!fnptr)
			{
				LOG_CRITICAL("Failed to retrieve function from OpenGL driver: " + proc_name);
				throw system_call_failed();
			}
		}

	#endif


	/** Create a list of required OpenGL3 extensions. */
	list<string> required_extensions()
	{
		list<string> result;
		result.push_back("GL_ARB_texture_non_power_of_two");
		result.push_back("GL_ARB_multitexture");
		result.push_back("GL_EXT_texture_array");
		result.push_back("GL_ARB_texture_cube_map");
		result.push_back("GL_ARB_seamless_cube_map");
		result.push_back("GL_EXT_texture3D");
		result.push_back("GL_EXT_texture_sRGB");
		result.push_back("GL_ARB_texture_compression");
		result.push_back("GL_ARB_texture_float");
		result.push_back("GL_ARB_depth_texture");

		result.push_back("GL_ARB_draw_buffers");
		result.push_back("GL_ARB_framebuffer_object");
		result.push_back("GL_ARB_framebuffer_sRGB");

		result.push_back("GL_ARB_vertex_buffer_object");
		result.push_back("GL_ARB_vertex_array_object");

		result.push_back("GL_ARB_shading_language_100");
		result.push_back("GL_ARB_fragment_shader");
		result.push_back("GL_ARB_geometry_shader4");
		result.push_back("GL_ARB_vertex_shader");
		result.push_back("GL_ARB_shader_objects");
		result.push_back("GL_ARB_explicit_attrib_location");

		return result;
	}
}




//////////////////////////////////////////////////////////////////////////
//
// gl3_api
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__HAVE_MS_WINDOW)

	gl3_api::gl3_api(const string& extension_string)
	{
		check_extensions(extension_string);
		check_capabilities();
		initialize_functions();
	}


	void gl3_api::check_extensions(const string& extension_string)
	{
		// Create lookup set for incoming extensions.
		list<string> extension_list;
		extension_string.compact().to_upper().split(' ', extension_list);

		tree_set<string> extension_set;
		for (list<string>::const_iterator it(extension_list.first()); it; ++it)	
			extension_set.insert(*it);


		// Check required extensions against available extensions.
		list<string> required_extensions
			(gl3_api_detail::required_extensions());

		string supported_extensions;
		string unsupported_extensions;
		bool any_unsupported_extension = false;

		for (list<string>::const_iterator it(required_extensions.first()); it; ++it)
		{
			string curext(it->to_upper());

			if (extension_set.contains(curext))
			{
				supported_extensions = (supported_extensions == "") ?
						curext : (supported_extensions + " " + curext);
			}
			else
			{
				unsupported_extensions = (unsupported_extensions == "") ?
						curext : (unsupported_extensions + " " + curext);
				any_unsupported_extension = true;
			}
		}

		if (supported_extensions != "")
			LOG_INFO("Supported extensions: " + supported_extensions);
		if (unsupported_extensions != "")
			LOG_WARN("Unsupported extensions: " + unsupported_extensions);

		if (any_unsupported_extension)
		{
			LOG_ERROR("Target device does not support required OpenGL3 extensions.");
			throw unsupported_feature();
		}
	}

#endif


#if !defined(VIRAL__HAVE_MS_WINDOW)
	#error "Not implemented for current window_context back-end."
#endif


void gl3_api::check_capabilities()
{
	// Query other OpenGL capabilities
	int max_texture_image_units;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &max_texture_image_units);

	int max_draw_buffers;
	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &max_draw_buffers);

	LOG_INFO("Maximum texture image units: " + max_texture_image_units);
	LOG_INFO("Maximum draw buffers: " + max_draw_buffers);

	if (max_texture_image_units < texture_image_units ||
		max_draw_buffers < draw_buffers)
	{
		LOG_ERROR("Target device does not support required OpenGL3 capabilities.");
		throw unsupported_feature();
	}
}


void gl3_api::initialize_functions()
{
	// Multitexturing
	GET_PROC_ADDRESS(glActiveTexture);
	GET_PROC_ADDRESS(glTexImage3D);


	// Frame buffer objects
	GET_PROC_ADDRESS(glDrawBuffers);

	GET_PROC_ADDRESS(glGenRenderbuffers);
	GET_PROC_ADDRESS(glBindRenderbuffer);
	GET_PROC_ADDRESS(glDeleteRenderbuffers);
	GET_PROC_ADDRESS(glRenderbufferStorage);

	GET_PROC_ADDRESS(glGenFramebuffers);
	GET_PROC_ADDRESS(glBindFramebuffer);
	GET_PROC_ADDRESS(glDeleteFramebuffers);
	GET_PROC_ADDRESS(glCheckFramebufferStatus);

	GET_PROC_ADDRESS(glFramebufferTexture2D);
	GET_PROC_ADDRESS(glFramebufferTextureLayer);
	GET_PROC_ADDRESS(glFramebufferRenderbuffer);

	GET_PROC_ADDRESS(glBlitFramebuffer);
	GET_PROC_ADDRESS(glGenerateMipmap);


	// Vertex buffer objects
	GET_PROC_ADDRESS(glGenBuffers);
	GET_PROC_ADDRESS(glBindBuffer);
	GET_PROC_ADDRESS(glDeleteBuffers);
		
	GET_PROC_ADDRESS(glBufferData);
	GET_PROC_ADDRESS(glBufferSubData);
	GET_PROC_ADDRESS(glMapBuffer);
	GET_PROC_ADDRESS(glUnmapBuffer);

	GET_PROC_ADDRESS(glGenVertexArrays);
	GET_PROC_ADDRESS(glBindVertexArray);
	GET_PROC_ADDRESS(glDeleteVertexArrays);


	// Shaders
	GET_PROC_ADDRESS(glCreateShader);
	GET_PROC_ADDRESS(glShaderSource);
	GET_PROC_ADDRESS(glCompileShader);
	GET_PROC_ADDRESS(glGetShaderiv);
	GET_PROC_ADDRESS(glGetShaderInfoLog);
	GET_PROC_ADDRESS(glDeleteShader);

	GET_PROC_ADDRESS(glCreateProgram);
	GET_PROC_ADDRESS(glAttachShader);
	GET_PROC_ADDRESS(glDetachShader);
	GET_PROC_ADDRESS(glLinkProgram);
	GET_PROC_ADDRESS(glValidateProgram);
	GET_PROC_ADDRESS(glUseProgram);
	GET_PROC_ADDRESS(glGetProgramiv);
	GET_PROC_ADDRESS(glGetProgramInfoLog);
	GET_PROC_ADDRESS(glDeleteProgram);

	GET_PROC_ADDRESS(glGetActiveAttrib);
	GET_PROC_ADDRESS(glGetAttribLocation);
	GET_PROC_ADDRESS(glBindAttribLocation);

	GET_PROC_ADDRESS(glEnableVertexAttribArray);
	GET_PROC_ADDRESS(glDisableVertexAttribArray);
	GET_PROC_ADDRESS(glVertexAttribPointer);
	GET_PROC_ADDRESS(glVertexAttribIPointer);

	GET_PROC_ADDRESS(glGetActiveUniform);
	GET_PROC_ADDRESS(glGetUniformLocation);

	GET_PROC_ADDRESS(glUniform1f); GET_PROC_ADDRESS(glUniform2f);
	GET_PROC_ADDRESS(glUniform3f); GET_PROC_ADDRESS(glUniform4f);
	GET_PROC_ADDRESS(glUniform1i); GET_PROC_ADDRESS(glUniform2i);
	GET_PROC_ADDRESS(glUniform3i); GET_PROC_ADDRESS(glUniform4i);
	GET_PROC_ADDRESS(glUniform1fv); GET_PROC_ADDRESS(glUniform2fv);
	GET_PROC_ADDRESS(glUniform3fv); GET_PROC_ADDRESS(glUniform4fv);
	GET_PROC_ADDRESS(glUniform1iv); GET_PROC_ADDRESS(glUniform2iv);
	GET_PROC_ADDRESS(glUniform3iv); GET_PROC_ADDRESS(glUniform4iv);

	GET_PROC_ADDRESS(glUniformMatrix2fv);
	GET_PROC_ADDRESS(glUniformMatrix3fv);
	GET_PROC_ADDRESS(glUniformMatrix4fv);
}




} /* namespace viral_core */

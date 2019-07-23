/**
 *************************************************************************
 *
 * @file gl3_api.hpp
 *
 * Capabilities and functions for OpenGL3.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_API_HPP)
#define INCLUDED__VIRAL_CORE__GL3_API_HPP


#include "build_config.hpp"
#include "gl3_config.hpp"


#include "string_forward.hpp"
#include "gl3_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_api
 * @ingroup gl3_render_module
 *
 * Verifies required OpenGL3 capabilities and exposes associated
 * OpenGL3 functions.
 *
 * Notes:
 * - Even systems with native support for core OpenGL3 use function
 *		pointers from the gl3_api object. This avoids system-specific
 *		code paths in rendering logics.
 * - The back-end must support all employed OpenGL3 capabilities
 *		and extensions, which currently implies version 3.30.
 *		This again avoids multiple paths through rendering code.
 *
 ************************************************************************/
class gl3_api :
	private non_copyable
{
	friend class gl3_render_context;

	public:

		// Texturing
		static const GLsizei max_texture_2d_size = 4096;
		static const GLsizei max_texture_2d_layer_count = 256;
		static const GLsizei max_texture_cube_map_size = 4096;

		PFNGLACTIVETEXTUREPROC glActiveTexture;
		static const GLenum texture_image_units = 16;

		PFNGLTEXIMAGE3DPROC glTexImage3D;


		// Frame buffer objects
		static const GLsizei max_frame_buffer_size = 4096;

		PFNGLDRAWBUFFERSPROC glDrawBuffers;
		static const GLint draw_buffers = 4;

		PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
		PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
		PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
		PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;

		PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
		PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
		PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
		PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;

		PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
		PFNGLFRAMEBUFFERTEXTURELAYERPROC glFramebufferTextureLayer;
		PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;

		PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer;
		PFNGLGENERATEMIPMAPPROC glGenerateMipmap;


		// Vertex buffer objects
		PFNGLGENBUFFERSPROC glGenBuffers;
		PFNGLBINDBUFFERPROC glBindBuffer;
		PFNGLDELETEBUFFERSPROC glDeleteBuffers;
		
		PFNGLBUFFERDATAPROC glBufferData;
		PFNGLBUFFERSUBDATAPROC glBufferSubData;
		PFNGLMAPBUFFERPROC glMapBuffer;
		PFNGLUNMAPBUFFERPROC glUnmapBuffer;

		PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
		PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
		PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;


		// Shaders
		PFNGLCREATESHADERPROC glCreateShader;
		PFNGLSHADERSOURCEPROC glShaderSource;
		PFNGLCOMPILESHADERPROC glCompileShader;
		PFNGLGETSHADERIVPROC glGetShaderiv;
		PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
		PFNGLDELETESHADERPROC glDeleteShader;

		PFNGLCREATEPROGRAMPROC glCreateProgram;
		PFNGLATTACHSHADERPROC glAttachShader;
		PFNGLDETACHSHADERPROC glDetachShader;
		PFNGLLINKPROGRAMPROC glLinkProgram;
		PFNGLVALIDATEPROGRAMPROC glValidateProgram;
		PFNGLUSEPROGRAMPROC glUseProgram;
		PFNGLGETPROGRAMIVPROC glGetProgramiv;
		PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
		PFNGLDELETEPROGRAMPROC glDeleteProgram;

		PFNGLGETACTIVEATTRIBPROC glGetActiveAttrib;
		PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
		PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;

		PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
		PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
		PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
		PFNGLVERTEXATTRIBIPOINTERPROC glVertexAttribIPointer;

		PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform;
		PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;

		PFNGLUNIFORM1FPROC glUniform1f; PFNGLUNIFORM2FPROC glUniform2f;
		PFNGLUNIFORM3FPROC glUniform3f; PFNGLUNIFORM4FPROC glUniform4f;
		PFNGLUNIFORM1IPROC glUniform1i; PFNGLUNIFORM2IPROC glUniform2i;
		PFNGLUNIFORM3IPROC glUniform3i; PFNGLUNIFORM4IPROC glUniform4i;
		PFNGLUNIFORM1FVPROC glUniform1fv; PFNGLUNIFORM2FVPROC glUniform2fv;
		PFNGLUNIFORM3FVPROC glUniform3fv; PFNGLUNIFORM4FVPROC glUniform4fv;
		PFNGLUNIFORM1IVPROC glUniform1iv; PFNGLUNIFORM2IVPROC glUniform2iv;
		PFNGLUNIFORM3IVPROC glUniform3iv; PFNGLUNIFORM4IVPROC glUniform4iv;

		PFNGLUNIFORMMATRIX2FVPROC glUniformMatrix2fv;
		PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv;
		PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;


	private:

		#if defined(VIRAL__HAVE_MS_WINDOW)

			/**
			 * Initialize OpenGL3 api from system OpenGL headers
			 * and from incoming @bref{extension_string}.
			 * The extension string must hold all OS-independent
			 * and all OS-specific extensions.
			 */
			gl3_api(const string& extension_string);

			void check_extensions(const string& extension_string);
		#endif

		#if !defined(VIRAL__HAVE_MS_WINDOW)
			#error "Not implemented for current window_context back-end."
		#endif

		
		void check_capabilities();
		void initialize_functions();
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_API_HPP) */

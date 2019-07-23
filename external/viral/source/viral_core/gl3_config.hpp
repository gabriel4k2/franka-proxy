/**
 *************************************************************************
 *
 * @file gl3_config.hpp
 *
 * Unified config header for OpenGL 3, include instead of OpenGL headers.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_CONFIG_HPP)
#define INCLUDED__VIRAL_CORE__GL3_CONFIG_HPP


#include "build_config.hpp"


#if defined(VIRAL__HAVE_GL3_RENDER)


// Most systems require an intricate include order
// for OpenGL to compile properly under all conditions.
// gl3_config.hpp realizes this include order.


#if defined(VIRAL__OS_TARGET_WINDOWS)

	// On Windows, the system only provides basic GL1.1,
	// so we pull GL3 from external extension headers.
	#include <windows.h>
	#include <gl/gl.h>

	#include <glext.h>
	#include <wglext.h>

#endif


#if defined(VIRAL__OS_TARGET_UNIX)

	// On Linux, use the system-wide OpenGL library,
	// which provides basic GL1 plus GL3 extensions.
	#include <GL/gl.h>
	#include <GL/glext.h>

#endif


#if !defined(VIRAL__OS_TARGET_WINDOWS) && \
	!defined(VIRAL__OS_TARGET_UNIX)
		#error "No GL configuration for current build target."
#endif




#endif /* defined(VIRAL__HAVE_GL3_RENDER) */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_CONFIG_HPP) */

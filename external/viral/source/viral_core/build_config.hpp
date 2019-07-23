/**
 *************************************************************************
 *
 * @file build_config.hpp
 *
 * Configuration of the current build parameters, external libraries,
 * and compiler-specific settings.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BUILD_CONFIG_HPP)
#define INCLUDED__VIRAL_CORE__BUILD_CONFIG_HPP




/** @cond VIRAL__DOXYGEN_IGNORE */




//////////////////////////////////////////////////////////////////////////
//
//	Auto configuration.
//
//////////////////////////////////////////////////////////////////////////


// Various Visual C++ versions.
#if defined(_MSC_VER)

	#define VIRAL__COMPILER_VISUALCPP
	#define VIRAL__COMPILER_STRING "COMPILER_VISUALCPP"

	#define VIRAL__OS_TARGET_WINDOWS
	#define VIRAL__OS_TARGET_STRING "OS_TARGET_WINDOWS"

	#if defined(_WIN64)
		#define VIRAL__MACHINE_TARGET_X64
		#define VIRAL__MACHINE_TARGET_STRING "MACHINE_TARGET_X64"
	#else
		#error "Unknown VC++ machine target."
		#error "See build_config.hpp for supported targets."
	#endif

	#if defined(_DEBUG)
		#define VIRAL__BUILD_MODE_DEBUG
		#define VIRAL__BUILD_MODE_STRING "BUILD_MODE_DEBUG"
	#else
		#define VIRAL__BUILD_MODE_RELEASE
		#define VIRAL__BUILD_MODE_STRING "BUILD_MODE_RELEASE"
	#endif

#endif


// GNU C++ compiler on OSX and Linux.
#if defined(__GNUC__)

	#define VIRAL__COMPILER_GCC
	#define VIRAL__COMPILER_STRING "COMPILER_GCC"

	#if defined(__linux__)
		#define VIRAL__OS_TARGET_UNIX
		#define VIRAL__OS_TARGET_STRING "OS_TARGET_LINUX"
	#else
		#error "Unknown GCC OS target."
		#error "See build_config.hpp for supported targets."
	#endif

	#if defined(__x86_64)
		#define VIRAL__MACHINE_TARGET_X64
		#define VIRAL__MACHINE_TARGET_STRING "MACHINE_TARGET_X64"
	#else
		#error "Unknown GCC machine target."
		#error "See build_config.hpp for supported targets."
	#endif

	#if defined(DEBUG)
		#define VIRAL__BUILD_MODE_DEBUG
		#define VIRAL__BUILD_MODE_STRING "BUILD_MODE_DEBUG"
	#else
		#define VIRAL__BUILD_MODE_RELEASE
		#define VIRAL__BUILD_MODE_STRING "BUILD_MODE_RELEASE"
	#endif

#endif


// Configuration success test.
#if !defined(VIRAL__COMPILER_STRING) || \
	!defined(VIRAL__MACHINE_TARGET_STRING) || \
	!defined(VIRAL__OS_TARGET_STRING) || \
	!defined(VIRAL__BUILD_MODE_STRING)

	#error "Auto configuration failed."
	#error "See build_config.hpp for supported options."

#endif




//////////////////////////////////////////////////////////////////////////
//
//	Visual C++ specific.
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__COMPILER_VISUALCPP)


	// Request windows features:
	// - Win7 (> 0x0601) for networking
	// - WinXP (> 0x0501) for threading and synching
	// - ie5 (> 0x0500) for some shell stuff
	#if !defined(_WIN32_WINNT)
		#define _WIN32_WINNT 0x0601
	#endif
	#if !defined(_WIN32_IE)
		#define _WIN32_IE 0x0500
	#endif


	// Don't never ever use the ugly windows max / min macros.
	#if !defined(NOMINMAX)
		#define NOMINMAX
	#endif


	// Do not allow windows.h to include legacy winsock.h.
	// Otherwise, inclusion of winsock2 fails later on.
	#define _WINSOCKAPI_


	// MSVC typedefs for fixed-width integer types.
	typedef signed __int8 int8;
	typedef unsigned __int8 uint8;

	typedef signed __int16 int16;
	typedef unsigned __int16 uint16;

	typedef signed __int32 int32;
	typedef unsigned __int32 uint32;
	
	typedef signed __int64 int64;
	typedef unsigned __int64 uint64;


	// MSVC pointer-width integer.
	#if defined(_WIN64)
		typedef unsigned __int64 pointer_int;
	#else
		typedef unsigned int pointer_int;
	#endif


	// Visual C++ supports pointer aliasing hints.
	#define VIRAL__RESTRICT_PTR __restrict


	// Enable some additional warnings
	// that are off even on /W4 level.
	#pragma warning(default:4242) // Conversion with data loss
	#pragma warning(default:4254) // Conversion with data loss
	#pragma warning(default:4265) // Missing virtual destructor
	#pragma warning(default:4287) // Sign mismatch on constant
	#pragma warning(default:4365) // Sign mismatch on conversion
	#pragma warning(default:4388) // Signed / unsigned mismatch
	#pragma warning(default:4826) // Conversion with sign extension
	#pragma warning(default:5039) // Exceptions in extern C calls


	// Disable some of the W4 warnings
	// that are unreasonable or outdated.
	#pragma warning(disable:4100) // Unused formal parameter
	#pragma warning(disable:4127) // if constexpr suggestion
	#pragma warning(disable:4201) // Nameless structs/unions


	// Visual C++ 2015 and above support noexcept.
	#if _MSC_VER >= 1900
		#define NOTHROW noexcept
	#else
		#define NOTHROW
	#endif


	// Commonplace NULL define, here so we don't have
	// to pull in a standard header.
	#if !defined(NULL)
		#define NULL 0
	#endif


	// Default configuration for external libs in Visual C++.
	#if !defined(VIRAL__CUSTOM_EXTERNAL_LIBS)
		#define VIRAL__HAVE_CAMERON_GJK
		#define VIRAL__HAVE_EXPAT
		#define VIRAL__HAVE_LIBJPEG
		#define VIRAL__HAVE_LIBOGG
		#define VIRAL__HAVE_LIBPNG
		#define VIRAL__HAVE_LIBVORBIS
		#define VIRAL__HAVE_ZLIB

		#define VIRAL__USE_MS_TIMER
		#define VIRAL__USE_MS_THREAD

		#define VIRAL__HAVE_MS_FILE		
		#define VIRAL__HAVE_MS_NETWORK
		#define VIRAL__HAVE_MS_WINDOW
		#define VIRAL__HAVE_MS_INPUT
		#define VIRAL__HAVE_MS_AUDIO		
		#define VIRAL__HAVE_GL3_RENDER
	#endif


#endif




//////////////////////////////////////////////////////////////////////////
//
//	GCC specific.
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__COMPILER_GCC)


	// GCC typedefs for fixed-width integer types.
	typedef signed char int8;
	typedef unsigned char uint8;

	typedef signed short int16;
	typedef unsigned short uint16;

	typedef signed int int32;
	typedef unsigned int uint32;
	
	typedef signed long long int64;
	typedef unsigned long long uint64;


	// GCC pointer-width integer.
	#if defined(__x86_64)
		typedef unsigned long long pointer_int;
	#else
		typedef unsigned int pointer_int;
	#endif


	// GCC supports pointer aliasing hints.
	#define VIRAL__RESTRICT_PTR __restrict


	// Evaluate NOTHROW tag as noexcept(),
	// which gives us sort-of compile time
	// exception verification.
	#define NOTHROW noexcept


	// Commonplace NULL define, here so we don't have
	// to pull in a standard header.
	#if !defined(NULL)
		#define NULL 0
	#endif


	// Default configuration for external libs in GCC.
	#if !defined(VIRAL__CUSTOM_EXTERNAL_LIBS)
		#define VIRAL__HAVE_CAMERON_GJK
		#define VIRAL__HAVE_EXPAT
		#define VIRAL__HAVE_LIBJPEG
		#define VIRAL__HAVE_LIBOGG
		#define VIRAL__HAVE_LIBPNG
		#define VIRAL__HAVE_LIBVORBIS
		#define VIRAL__HAVE_ZLIB

		#define VIRAL__USE_CPP11_TIMER
		#define VIRAL__USE_CPP11_THREAD

		#define VIRAL__HAVE_POSIX_FILE		
		#define VIRAL__HAVE_POSIX_NETWORK
		#define VIRAL__HAVE_X11_WINDOW
		#define VIRAL__HAVE_X11_INPUT
		#define VIRAL__HAVE_PORT_AUDIO		
		#define VIRAL__HAVE_GL3_RENDER
	#endif


#endif




//////////////////////////////////////////////////////////////////////////
//
//	Check external library configuration.
//
//////////////////////////////////////////////////////////////////////////


#if (defined(VIRAL__USE_MS_TIMER) ? 1 : 0) + \
	(defined(VIRAL__USE_CPP11_TIMER) ? 1 : 0) != 1
	#error "No or more than one timer implementation selected."
#endif

#if (defined(VIRAL__USE_MS_THREAD) ? 1 : 0) + \
	(defined(VIRAL__USE_CPP11_THREAD) ? 1 : 0) != 1
	#error "No or more than one thread implementation selected."
#endif


#if defined(VIRAL__HAVE_MS_INPUT) && !defined(VIRAL__HAVE_MS_WINDOW)
	#error "VIRAL__HAVE_MS_INPUT requires VIRAL__HAVE_MS_WINDOW."
#endif

#if defined(VIRAL__HAVE_X11_INPUT) && !defined(VIRAL__HAVE_X11_WINDOW)
	#error "VIRAL__HAVE_X11_INPUT requires VIRAL__HAVE_X11_WINDOW."
#endif




/** @endcond */ // VIRAL__DOXYGEN_IGNORE




//////////////////////////////////////////////////////////////////////////
//
// Doxygen documentation control.
//
// The defines below enable compile-time switches for all external libs,
// all compilers, all target machine paths and all target build modes
// when doxygen is running. This serves for generating a complete
// documentation for all compile pathes.
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__DOXYGEN_DOCUMENTATION)


	/**
	 * Macro that is set only if doxygen runs,
	 * and which enables all potential compile pathes through code.
	 */
	#define VIRAL__DOXYGEN_DOCUMENTATION


	/** Compiler define flags and associated name string. */
	//@{
	#define VIRAL__COMPILER_VISUALCPP
	#define VIRAL__COMPILER_GCC
	#define VIRAL__COMPILER_STRING "CURRENT_COMPILER"
	//@}


	/** Target machine flags and associated name string. */
	//@{
	#define VIRAL__MACHINE_TARGET_X64
	#define VIRAL__MACHINE_TARGET_STRING "CURRENT_MACHINE_TARGET"
	//@}


	/** Target OS flags and associated name string. */
	//@{
	#define VIRAL__OS_TARGET_WINDOWS
	#define VIRAL__OS_TARGET_UNIX
	#define VIRAL__OS_TARGET_STRING "CURRENT_OS_TARGET"
	//@}


	/** Build mode flags and name string. */
	//@{
	#define VIRAL__BUILD_MODE_DEBUG
	#define VIRAL__BUILD_MODE_RELEASE
	#define VIRAL__BUILD_MODE_STRING "CURRENT_BUILD_MODE"
	//@}


	/**
	 * Typedefs for fixed-width integer types.
	 * These solve problems with varying widths
	 * over systems (e.g. long on GCC vs MSVC).
	 */
	//@{
	typedef signed __int8 int8;
	typedef unsigned __int8 uint8;

	typedef signed __int16 int16;
	typedef unsigned __int16 uint16;

	typedef signed __int32 int32;
	typedef unsigned __int32 uint32;
	
	typedef signed __int64 int64;
	typedef unsigned __int64 uint64;
	//@}


	/**
	 * Unsigned integer the size of a pointer
	 * on the target architecture.
	 */
	typedef unsigned int pointer_int;


	/**
	 * C-style hint for non-aliasing pointer
	 * to support low-level optimizations.
	 */
	#define VIRAL__RESTRICT_PTR __restrict


	/**
	 * This define, available for method declarations and definitions,
	 * indicates that a method provides an explicit NOTHROW guarantee.
	 * See the @ref exception_module "Exception Module" for details.
	 *
	 * Ideally, we would like exception specifiers to be verified
	 * at compile-time, which the C++ language doesn't let us do.
	 * Hence, the actual realization depends on the target compiler.
	 *
	 * For Doxygen documentation purposes, all non-throwing methods
	 * have an explicit noexcept specifier.
	 */
	#define NOTHROW noexcept


	/**
	 * Configure compile-time support for external libraries.
	 *
	 * Usage notes:
	 * - By default, build_config.hpp selects suitable options
	 *		for external libraries of the current build target.
	 *		Build tools can define VIRAL__CUSTOM_EXTERNAL_LIBS
	 *		to override options for external libraries.
	 * - External libraries marked as VIRAL__HAVE_* are optional
	 *		and can freely be included or excluded from compiling
	 *		(e.g. see VIRAL__HAVE_ZLIB or VIRAL__HAVE_GL3_RENDER).
	 *		Use of excluded libraries throws at runtime.
	 * - External libraries marked as VIRAL__USE_* are required
	 *		and mutually exclusive (e.g. VIRAL__USE_CPP11_TIMER).
	 *		Builds must choose exactly one variant.
	 */
	//@{
	#define VIRAL__CUSTOM_EXTERNAL_LIBS

	#define VIRAL__HAVE_CAMERON_GJK
	#define VIRAL__HAVE_EXPAT
	#define VIRAL__HAVE_LIBJPEG
	#define VIRAL__HAVE_LIBOGG
	#define VIRAL__HAVE_LIBPNG
	#define VIRAL__HAVE_LIBVORBIS
	#define VIRAL__HAVE_ZLIB

	#define VIRAL__USE_MS_TIMER
	#define VIRAL__USE_CPP11_TIMER

	#define VIRAL__USE_MS_THREAD
	#define VIRAL__USE_CPP11_THREAD

	#define VIRAL__HAVE_MS_FILE
	#define VIRAL__HAVE_POSIX_FILE

	#define VIRAL__HAVE_MS_NETWORK
	#define VIRAL__HAVE_POSIX_NETWORK

	#define VIRAL__HAVE_MS_WINDOW
	#define VIRAL__HAVE_X11_WINDOW

	#define VIRAL__HAVE_MS_INPUT
	#define VIRAL__HAVE_X11_INPUT

	#define VIRAL__HAVE_MS_AUDIO
	#define VIRAL__HAVE_PORT_AUDIO

	#define VIRAL__HAVE_GL3_RENDER
	//@}


	/**
	 * Setting this define disables all logging
	 * with the logging macros for a speed benefit.
	 */
	#define VIRAL__DISABLE_LOGGING_MACROS
	#undef VIRAL__DISABLE_LOGGING_MACROS


#endif




#endif /* !defined(INCLUDED__VIRAL_CORE__BUILD_CONFIG_HPP) */

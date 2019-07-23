/**
 *************************************************************************
 *
 * @file exception.hpp
 *
 * Header for the Exception Module.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__EXCEPTION_HPP)
#define INCLUDED__VIRAL_CORE__EXCEPTION_HPP


#include "build_config.hpp"


#include <exception>


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup exception_module Exception Module
 *
 * Deals with application-wide exception support and
 * holds several default exception classes.
 *
 * <b>
 * The entire Viral Framework is based on exceptions for error handling,
 * and follows the exception guidelines below. Any client code must be aware
 * of exceptions and should follow the exception guidelines.
 * </b>
 *
 * Exception safety:
 * - All methods are aware of exceptions, and provide at least basic
 * 		 exception safety: If an exception happens at any point in any
 *		 method, the method leaves all touched objects in a valid, yet
 *		 indeterminate state. No resources are leaked. Open iterators
 *		 to any potentially modified objects become invalid.
 * - Stronger exception safety is documented where applicable.
 * - See the @ref scope_guard_module "Scope Guard Module" for an automatic
 *		mechanism to ensure exception safety.
 * - Care is required with exceptions in complicated expressions.
 *		No assumptions shall be made on the order of evaluation in-between
 *		sequence points. For instance, the following code is not exception
 *		safe due to arbitrary order of function parameter evaluation:
 *			@code f(throwing + operation, auto_pointer(new xyz()); @endcode
 *		Most importantly, new()ed objects must always be wrapped into
 *		an auto_pointer or shared_pointer in a standalone statement.
 *
 * Non-throwing methods:
 * - Methods that explicitly never throw an exception are marked with
 *		a @ref NOTHROW tag. All other methods potentially can throw
 *		any exception.
 * - No exceptions must escape from destructors, cleanup methods, exception
 *		copy constructors and exception assignment operators to avoid a call
 *		to std::terminate(). In other words, these methods must provide
 *		@ref NOTHROW guarantees.
 *
 * Exception catching and logging:
 * - Exceptions should be caught at a very high level, in a handler
 *		but for the exception or std::exception classes.
 * - Exceptions don't log on their own, but logging is mostly performed
 *		before throwing and after catching an exception.
 * - Low-level exceptions (memory, containers, strings) must not be logged
 *		to avoid recursive throwing.
 *
 ************************************************************************/




/**
 *************************************************************************
 *
 * @class exception
 * @ingroup exception_module
 *
 * Groups together all custom exceptions thrown by the entire framework,
 * so these may be caught at a single place within high level code.
 *
 ************************************************************************/
class exception: public std::exception
{
	public:

		exception() { }
		~exception() NOTHROW override { }


		/**
		 * Retrieve (static, constant) description string via
		 * the standard C++ exception interface.
		 */
		const char* what() const NOTHROW override
			{ return "General exception"; }
};




/**
 *************************************************************************
 *
 * Indicates that some framework method
 * ran out of dynamic, low-level memory.
 * @ingroup exception_module 
 *
 * @attent
 * C++-style allocations using the @bref{new} operator still
 * throw std::bad_alloc on failure.
 *
 ************************************************************************/
class out_of_memory: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Available memory exceeded."; }
};


/**
 *************************************************************************
 * Thrown if an internal bug or inconsistency of some kind is recognized.
 * @ingroup exception_module 
 ************************************************************************/
class assert_failed: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Internal code assertion failed."; }
};


/**
 *************************************************************************
 * An exception thrown if some self-test assertion fails.
 * @ingroup exception_module
 ************************************************************************/
class test_failed: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Self-test failed."; }
};


/**
 *************************************************************************
 * Thrown if an integer overflow occurred in some checked operation.
 * @ingroup exception_module
 ************************************************************************/
class integer_overflow: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Integer overflow."; }
};


/**
 *************************************************************************
 * Thrown if some pointer must not be @NULL, but was @NULL.
 * @ingroup exception_module
 ************************************************************************/
class null_pointer: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "NULL pointer where not-NULL pointer was required."; }
};


/**
 *************************************************************************
 * Indicates that some method parameter (possibly encapsulated in an
 * object state) contained an invalid value.
 * @ingroup exception_module 
 ************************************************************************/
class invalid_parameter: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Invalid parameter provided."; }
};


/**
 *************************************************************************
 * Thrown if an operation is invoked on an incompatible or invalid
 * object state.
 * @ingroup exception_module 
 ************************************************************************/
class invalid_state: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Invalid state for operation."; }
};


/**
 *************************************************************************
 * Thrown if a requested operation is invalid.
 * @ingroup exception_module 
 ************************************************************************/
class invalid_operation: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Invalid operation requested."; }
};


/**
 *************************************************************************
 * Thrown whenever an object is used in an invalid context, such as
 * alongside a mismatching implementation class.
 * @ingroup exception_module
 ************************************************************************/
class invalid_context: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Object used in an invalid context."; }
};


/**
 *************************************************************************
 * Indicates that some variable or array size exceeded its minimum or
 * maximum allowed value.
 * @ingroup exception_module
 ************************************************************************/
class bounds_exceeded: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Out-of-bounds value detected."; }
};


/**
 *************************************************************************
 * Thrown if an object that requires explicit initialization was not
 * correctly initialized before use.
 * @ingroup exception_module
 ************************************************************************/
class not_initialized: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Object not correctly initialized."; }
};


/**
 *************************************************************************
 * Indicates that a mandatory object within some kind of query
 * was not found.
 * @ingroup exception_module
 ************************************************************************/
class not_found: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Object not found."; }
};


/**
 *************************************************************************
 * An operation on polymorphic objects expected a specific type,
 * but found another, mismatching one.
 * @ingroup exception_module
 ************************************************************************/
class type_mismatch: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Object type mismatch."; }
};


/**
 *************************************************************************
 * Thrown whenever some input data stream for a method or some input
 * file is corrupt or deviates from the required format.
 * @ingroup exception_module
 ************************************************************************/
class corrupt_data: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Provided data corrupt."; }
};


/**
 *************************************************************************
 * Indicates that some feature called by client code currently is not
 * supported.
 * @ingroup exception_module
 ************************************************************************/
class unsupported_feature: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Unsupported feature used."; }
};


/**
 *************************************************************************
 * Thrown if the insertion of some object into a name lookup registry
 * failed.
 * @ingroup exception_module
 ************************************************************************/
class object_registration_failed: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Object registration failed."; }
};


/**
 *************************************************************************
 * Thrown either by a constructor or by a factory if the creation of an 
 * object fails for some unspecified reason.
 * @ingroup exception_module
 ************************************************************************/
class object_creation_failed: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Object creation failed."; }
};


/**
 *************************************************************************
 * Thrown if writing to or reading from a file fails in low-level IO
 * operations.
 * @ingroup exception_module
 ************************************************************************/
class file_operation_failed: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "File operation failed."; }
};


/**
 *************************************************************************
 * Thrown if a required folder could not be created.
 * @ingroup exception_module
 ************************************************************************/
class folder_creation_failed: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Folder creation failed."; }
};


/**
 *************************************************************************
 * Thrown if some low-level, OS-specific method fails.
 * @ingroup exception_module
 ************************************************************************/
class system_call_failed: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Low-level system call failed."; }
};


/**
 *************************************************************************
 * Thrown if some general API method fails.
 * @ingroup exception_module
 ************************************************************************/
class api_call_failed: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "API call failed."; }
};


/**
 *************************************************************************
 * Thrown whenever some operation (e.g. a thread of threaded_task)
 * was terminated by an unhandled exception.
 * @ingroup exception_module
 ************************************************************************/
class unhandled_exception: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Operation terminated by unhandled exception."; }
};


/**
 *************************************************************************
 * Thrown by various data compression or data decompression routines in
 * case of unspecified errors.
 * @ingroup exception_module
 ************************************************************************/
class compression_failed: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Compression or decompression failed."; }
};


/**
 *************************************************************************
 * Indicates failure while handling XML data.
 * @ingroup exception_module
 ************************************************************************/
class xml_exception: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "XML data handling failed."; }
};


/**
 *************************************************************************
 * Thrown if explicit validation of some object fails.
 * @ingroup exception_module
 ************************************************************************/
class validation_failed: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Object validation failed."; }
};


/**
 *************************************************************************
 * Caused by invalid syntax within input data.
 * @ingroup exception_module
 ************************************************************************/
class syntax_error: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Invalid syntax in input data."; }
};


/**
 *************************************************************************
 * Thrown if some network transfer fails.
 * @ingroup exception_module
 ************************************************************************/
class network_exception: public exception
{
	public:
		const char* what() const NOTHROW override
			{ return "Network transfer failed."; }
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__EXCEPTION_HPP) */

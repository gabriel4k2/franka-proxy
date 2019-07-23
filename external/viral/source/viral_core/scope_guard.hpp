/**
 *************************************************************************
 *
 * @file scope_guard.hpp
 *
 * Automated scope guard template header.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__SCOPE_GUARD_HPP)
#define INCLUDED__VIRAL_CORE__SCOPE_GUARD_HPP


#include "build_config.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup scope_guard_module Scope Guard Module
 * @ingroup exception_module
 *
 * A mechanism for automatic code execution on scope exit,
 * such as for cleanup or rollback operations.
 *
 * General usage:
 * - Scope guards replace verbose and slow try-catchall-rethrow blocks
 *		that contain cleanup or rollback code.
 * - Function guards use a client-specified static function.
 * - Object guards use a client-specified member method on some object.
 *
 * Usage overview:
 * - Clients create a scope guard on the stack with @ref SCOPE_GUARD.
 *		On guard creation, clients specify the later cleanup call,
 *		its parameters, and a (this) reference for member methods.
 * - Usually, a scope guard maintains specifiers of its parameters
 *		(e.g. it copies values or pointers and it keeps references).
 * - Clients can guard_by_reference() to enforce passing parameters
 *		by reference. This allows cleanup to modify original variables,
 *		and is required for exception safety on parameters that throw
 *		on copy construction or assignment (see exception safety below).
 * - On scope exit, the scope guard destructor triggers cleanup.
 *		Clients can optionally call scope_guard_base::dismiss()
 *		to disable this behaviour, such as for disabling rollback
 *		on operation success.
 *
 * Exception safety:
 * - @attent Scope guard construction must not throw, otherwise
 *		the guard action might be skipped. Parameters that throw
 *		on copy construction or on assignment must be wrapped into
 *		non-throwing reference holders with guard_by_reference().
 * - @attent As scope guards may work during stack unwinding,
 *		cleanup functions must not throw to avoid std::terminate().
 *		Notably, cleanup functions must mirror guard_by_reference() 
 *		and must accept copy-throwing parameters by reference.
 * - Rollback on exception need not be absolute. For instance,
 *		invalidated iterators usually cannot be reverted back
 *		to the original element.
 *
 * Usage examples:
 *
 * - Automatic release of a C memory resource via a function guard:
 *  @code
 *	{
 *		void *buffer = malloc(16);
 *		SCOPE_GUARD(guard_buffer, (free, buffer));
 *		// The scope guard is not dismissed in here.
 *		// Thus, the buffer object is always freed as per a call to
 *		// free(buffer), even on normal exit of the surrounding scope.
 *		// ...
 *	}
 *  @endcode
 *
 * - Guarding by reference allows cleanup code to modify a parameter:
 *	@code
 *	void decrement_counter(int &refcounter) { --refcounter; }
 *	// ...
 *	{
 *		int my_refcounter;
 *		SCOPE_GUARD(guard,
 *			(decrement_counter, guard_by_reference(my_refcounter)));
 *		// Decrements my_refcounter on scope exit.
 *		// ...
 *	}
 *  @endcode
 *
 * - Object deletion with utility function guard_delete:
 *  @code
 *	{
 *		my_object *object = new my_object();
 *		SCOPE_GUARD(object_guard, (&guard_delete<my_object>, object));
 *		// The object is deleted as per "delete object" on scope exit.
 *		// ...
 *	}
 *  @endcode
 *
 * - Inverse database rollback operations on exception only,
 *		using an object guard and by-reference input of
 *		throw-on-copy string objects:
 *  @code
 *	{
 *		string item_a, item_b;
 *
 *		my_database.insert(item_a);
 *		SCOPE_GUARD(guard_item_a,
 *			(my_database, &database_class::remove, guard_by_reference(item_a)));
 *  
 *		my_database.insert(item_b);
 *		SCOPE_GUARD(guard_item_b,
 *			(my_database, &database_class::remove, guard_by_reference(item_b)));
 *  
 *		// If the scope is left here (as due to an exception),
 *		// both items automatically are removed again, as through
 *		// my_database.remove(item_b), then my_database.remove(item_a)
 *  
 *		my_database.link(item_a, item_b);
 *		SCOPE_GUARD(guard_link,
 *			(my_database, &database_class::unlink,
 *			 guard_by_reference(item_a), guard_by_reference(item_b)));
 *  
 *		// If the scope is left in here, all guards are destroyed,
 *		// and in their destructor undo the associated database
 *		// actions in inverse order. In particular, both items
 *		// first are unlinked, and then destroyed.
 *  
 *		guard_link.dismiss();
 *		guard_item_b.dismiss();
 *		guard_item_a.dismiss();
 *  
 *		// Guards have been dismissed, so cleanup is disabled here.
 *	}
 *	@endcode
 *
 * Origins:
 * The scope guard implementation follows the one given by Alexandrescu
 * in Modern C++.
 *
 ************************************************************************/


template <class P> class scope_guard_reference_holder;


/**
 *************************************************************************
 *
 * @class scope_guard_base
 * @ingroup scope_guard_module
 *
 * Basic scope guard functionality, including all methods
 * that are visible to scope guard clients.
 *
 * Notes:
 * - See the @ref scope_guard_module "Scope Guard Module" for an overview
 *		over the entire scope guard system.
 *
 * Implementation notes:
 * - Binding an instance of a class derived from scope_guard_base
 *		(i.e. as returned by create_guard()) to a const reference
 *		for the parent scope_guard_base class extends the lifetime
 *		of the instance to the scope of the const reference.
 * - Notably, the non-virtual destructor of the derived class runs,
 *		even if the reference only points to a parent class type.
 *		This seems like magic, but is standard C++, also known
 *		as the "most important const".
 * - While lifetime extension is vital for scope guard functionality,
 *		it requires mutable hacks to work around reference const-ness.
 *
 ************************************************************************/
class scope_guard_base
{

	public:

		/**
		 * Dismiss the scope guard, so that cleanup
		 * is not performed on destruction anymore.
		 */
		void dismiss() const NOTHROW { enabled_ = false; }
		

		/**
		 * Execute the client cleanup function on destruction
		 * unless explicitly dimiss()ed.
		 */
		~scope_guard_base() NOTHROW { }


	protected:

		scope_guard_base() NOTHROW
			: enabled_(true) { }


		/**
		 * Copy constructor for passing back the scope guard
		 * from the create_guard() methods.
		 */
		scope_guard_base(const scope_guard_base& other) NOTHROW
			: enabled_(other.enabled_)
				{ other.enabled_ = false; }


		/**
		 * Execute the cleanup function of a scope guard
		 * implementation T, unless the guard has already
		 * been dismissed.
		 */
		template <typename T>
			void execute_unless_dismissed(T &implementation) NOTHROW
		{
			if (enabled_)
			{
				implementation.execute();
				enabled_ = false;
			}
		}


	private:

		mutable bool enabled_;
};




/**
 *************************************************************************
 *
 * Create a scope guard with a variable @bref{name} and @bref{params}
 * by calling an appropriate create_guard() variant.
 *
 * @ingroup scope_guard_module
 *
 ************************************************************************/
#define SCOPE_GUARD(name, params) \
	const scope_guard_base& name = create_guard params; \
	::viral_core::unused_on_purpose(name);




/**
 *************************************************************************
 *
 * Create a scope_guard_reference_holder proxy for passing a provided,
 * potentially-throwing-on-copy parameter by @bref{reference} into
 * a scope guard object.
 *
 * @ingroup scope_guard_module
 *
 ************************************************************************/
template <class P>
	scope_guard_reference_holder<P>
		guard_by_reference(P& reference) NOTHROW
			{ return scope_guard_reference_holder<P>(reference); }




/**
 *************************************************************************
 *
 * Created with guard_by_reference() to pass modifyable parameters of type
 * @bref{P} into a scope guard while surpressing any copy-construction or
 * assignment operator exceptions.
 *
 * @ingroup scope_guard_module
 *
 ************************************************************************/
template <class P>
	class scope_guard_reference_holder
{

	friend scope_guard_reference_holder<P> guard_by_reference<P>(P&) NOTHROW;

	public:

		operator P& () const NOTHROW { return reference_; }

	private:

		scope_guard_reference_holder(P& reference) NOTHROW
			: reference_(reference) { }

		P& reference_;
};




/**
 *************************************************************************
 *
 * Object deletion function for use in a scope guard.
 * @ingroup scope_guard_module
 *
 ************************************************************************/
template <typename T>
	void guard_delete(T *object) { delete object; }




/**
 *************************************************************************
 *
 * Assign @bref{other} value to @bref{value} through guard_by_reference().
 * @attent Assignment on @bref{T} must be @ref NOTHROW.
 *
 * @ingroup scope_guard_module
 *
 ************************************************************************/
template <typename T>
	void guard_assign(T& value, const T& other)
		{ value = other; }




//////////////////////////////////////////////////////////////////////////
//
// Implementation classes below.
//
// Scope guard implementation classes store client-provided parameters
// on the stack. There are specific classes for each parameter count,
// and for object methods or static functions.
//
//////////////////////////////////////////////////////////////////////////


/** @cond DOXYGEN_IGNORE */


template <typename F>
	class scope_guard_function_0 : public scope_guard_base
{
	friend class scope_guard_base;

	public:

		/**
		 * Execute cleanup and destroy scope guard.
		 * Non-virtual on purpose.
		 *
		 * The entire (confusing) call graph on destruction is:
		 * - ~scope_guard_function_x
		 * - scope_guard_base::execute_unless_dismissed<scope_guard_function_x>(*specialized)
		 * - if (enabled_) specialized.scope_guard_function_x::execute()
		 */
		~scope_guard_function_0() NOTHROW
			{ execute_unless_dismissed(*this); }


		/** Create scope guard for provided parameters. */
		static scope_guard_function_0<F> create
			(F guard_function) NOTHROW
		{
			return scope_guard_function_0<F>
				(guard_function);
		}


	private:

		/** Private constructor called by the static create methods. */
		scope_guard_function_0
			(F guard_function) NOTHROW
			:
			guard_function_(guard_function)
		{ }


		/** Execute cleanup method, guard has not been dismissed. */
		void execute() NOTHROW
			{ guard_function_(); }


		F guard_function_;
};


template <typename F, typename A>
	class scope_guard_function_1 : public scope_guard_base
{
	friend class scope_guard_base;

	public:

		~scope_guard_function_1() NOTHROW
			{ execute_unless_dismissed(*this); }

		static scope_guard_function_1<F, A> create
			(F guard_function, const A param_a) NOTHROW
		{
			return scope_guard_function_1<F, A>
				(guard_function, param_a);
		}

	private:

		scope_guard_function_1
			(F guard_function, const A param_a) NOTHROW
			:
			guard_function_(guard_function),
			param_a_(param_a)
		{ }

		void execute() NOTHROW
			{ guard_function_(param_a_); }

		F guard_function_;
		const A param_a_;
};


template <typename F, typename A, typename B>
	class scope_guard_function_2 : public scope_guard_base
{
	friend class scope_guard_base;

	public:

		~scope_guard_function_2() NOTHROW
			{ execute_unless_dismissed(*this); }

		static scope_guard_function_2<F, A, B> create
			(F guard_function, const A param_a, const B param_b) NOTHROW
		{
			return scope_guard_function_2<F, A, B>
				(guard_function, param_a, param_b);
		}

	private:

		scope_guard_function_2
			(F guard_function, const A param_a, const B param_b) NOTHROW
			:
			guard_function_(guard_function),
			param_a_(param_a),
			param_b_(param_b)
		{ }

		void execute() NOTHROW
			{ guard_function_(param_a_, param_b_); }

		F guard_function_;
		const A param_a_;
		const B param_b_;
};


template <typename F, typename A, typename B, typename C>
	class scope_guard_function_3 : public scope_guard_base
{
	friend class scope_guard_base;

	public:

		~scope_guard_function_3() NOTHROW
			{ execute_unless_dismissed(*this); }

		static scope_guard_function_3<F, A, B, C> create
			(F guard_function, const A param_a, const B param_b, const C param_c) NOTHROW
		{
			return scope_guard_function_3<F, A, B, C>
				(guard_function, param_a, param_b, param_c);
		}

	private:

		scope_guard_function_3
			(F guard_function, const A param_a, const B param_b, const C param_c) NOTHROW
			:
			guard_function_(guard_function),
			param_a_(param_a),
			param_b_(param_b),
			param_c_(param_c)
		{ }

		void execute() NOTHROW
			{ guard_function_(param_a_, param_b_, param_c_); }

		F guard_function_;
		const A param_a_;
		const B param_b_;
		const C param_c_;
};


template <class O, typename F>
	class scope_guard_object_0 : public scope_guard_base
{
	friend class scope_guard_base;

	public:

		~scope_guard_object_0() NOTHROW
			{ execute_unless_dismissed(*this); }

		static scope_guard_object_0<O, F> create
			(O &guard_object, F guard_function) NOTHROW
		{
			return scope_guard_object_0<O, F>
				(guard_object, guard_function);
		}

	private:

		scope_guard_object_0
			(O &guard_object, F guard_function) NOTHROW
			:
			guard_object_(guard_object),
			guard_function_(guard_function)
		{ }

		void execute() NOTHROW
			{ (guard_object_.*guard_function_)(); }

		O &guard_object_;
		F guard_function_;
};


template <class O, typename F, typename A>
	class scope_guard_object_1 : public scope_guard_base
{
	friend class scope_guard_base;

	public:

		~scope_guard_object_1() NOTHROW
			{ execute_unless_dismissed(*this); }

		static scope_guard_object_1<O, F, A> create
			(O &guard_object, F guard_function, const A param_a) NOTHROW
		{
			return scope_guard_object_1<O, F, A>
				(guard_object, guard_function, param_a);
		}

	private:

		scope_guard_object_1
			(O &guard_object, F guard_function, const A param_a) NOTHROW
			:
			guard_object_(guard_object),
			guard_function_(guard_function),
			param_a_(param_a)
		{ }

		void execute() NOTHROW
			{ (guard_object_.*guard_function_)(param_a_); }

		O &guard_object_;
		F guard_function_;
		const A param_a_;
};


template <class O, typename F, typename A, typename B>
	class scope_guard_object_2 : public scope_guard_base
{
	friend class scope_guard_base;

	public:

		~scope_guard_object_2() NOTHROW
			{ execute_unless_dismissed(*this); }	

		static scope_guard_object_2<O, F, A, B> create
			(O &guard_object, F guard_function, const A param_a, const B param_b) NOTHROW
		{
			return scope_guard_object_2<O, F, A, B>
				(guard_object, guard_function, param_a, param_b);
		}

	private:

		scope_guard_object_2
			(O &guard_object, F guard_function, const A param_a, const B param_b) NOTHROW
			:
			guard_object_(guard_object),
			guard_function_(guard_function),
			param_a_(param_a),
			param_b_(param_b)
		{ }

		void execute() NOTHROW
			{ (guard_object_.*guard_function_)(param_a_, param_b_); }

		O &guard_object_;
		F guard_function_;
		const A param_a_;
		const B param_b_;
};


template <class O, typename F, typename A, typename B, typename C>
	class scope_guard_object_3 : public scope_guard_base
{
	friend class scope_guard_base;

	public:

		~scope_guard_object_3() NOTHROW
			{ execute_unless_dismissed(*this); }	

		static scope_guard_object_3<O, F, A, B, C> create
			(O &guard_object, F guard_function, const A param_a, const B param_b, const C param_c) NOTHROW
		{
			return scope_guard_object_3<O, F, A, B, C>
				(guard_object, guard_function, param_a, param_b, param_c);
		}

	private:

		scope_guard_object_3
			(O &guard_object, F guard_function, const A param_a, const B param_b, const C param_c) NOTHROW
			:
			guard_object_(guard_object),
			guard_function_(guard_function),
			param_a_(param_a),
			param_b_(param_b),
			param_c_(param_c)
		{ }

		void execute() NOTHROW
			{ (guard_object_.*guard_function_)(param_a_, param_b_, param_c_); }

		O &guard_object_;
		F guard_function_;
		const A param_a_;
		const B param_b_;
		const C param_c_;
};


/** @endcond */




//////////////////////////////////////////////////////////////////////////
//
// Scope guard creation methods below.
//
// The scope guard creation methods resolve to any of the function
// or object guard classes, depending on the provided parameters.
// Parameter count and guard type thus need not be set explicitly
// by the client, but are automatically selected by overloading.
//
// The large number of template parameters serves two purposes:
// - Member method pointers need to be qualified fully to discern
//		object guard creation from function guard creation.
// - Compilers do not implicitly cast types in template parameters,
//		so there need to be separate parameters for input variables
//		and for parameters of object methods.
//
//////////////////////////////////////////////////////////////////////////


/** @cond DOXYGEN_IGNORE */


template <typename F>
	scope_guard_function_0<F> create_guard
		(F guard_function) NOTHROW
{
	return scope_guard_function_0<F>::create
		(guard_function);
}


template <typename F, typename A>
	scope_guard_function_1<F, A> create_guard
		(F guard_function, const A param_a) NOTHROW
{
	return scope_guard_function_1<F, A>::create
		(guard_function, param_a);
}


template <typename F, typename A, typename B>
	scope_guard_function_2<F, A, B> create_guard
		(F guard_function, const A param_a, const B param_b) NOTHROW
{
	return scope_guard_function_2<F, A, B>::create
		(guard_function, param_a, param_b);
}


template <typename F, typename A, typename B, typename C>
	scope_guard_function_3<F, A, B, C> create_guard
		(F guard_function, const A param_a, const B param_b, const C param_c) NOTHROW
{
	return scope_guard_function_3<F, A, B, C>::create
		(guard_function, param_a, param_b, param_c);
}


template <typename R, typename O1, typename O2>
	scope_guard_object_0<O2, R (O1::*)()> create_guard
		(O2& guard_object, R (O1::*guard_function)()) NOTHROW
{
	return scope_guard_object_0<O2, R (O1::*)()>::create
		(guard_object, guard_function);
}


template <typename R, typename O1, typename O2, typename A1, typename A2>
	scope_guard_object_1<O2, R (O1::*)(A1), A2> create_guard
		(O2& guard_object, R (O1::*guard_function)(A1), const A2 param_a) NOTHROW
{
	return scope_guard_object_1<O2, R (O1::*)(A1), A2>::create
		(guard_object, guard_function, param_a);
}


template <typename R, typename O1, typename O2, typename A1, typename A2, typename B1, typename B2>
	scope_guard_object_2<O2, R (O1::*)(A1, B1), A2, B2> create_guard
		(O2& guard_object, R (O1::*guard_function)(A1, B1), const A2 param_a, const B2 param_b) NOTHROW
{
	return scope_guard_object_2<O2, R (O1::*)(A1, B1), A2, B2>::create
		(guard_object, guard_function, param_a, param_b);
}


template <typename R, typename O1, typename O2, typename A1, typename A2, typename B1, typename B2, typename C1, typename C2>
	scope_guard_object_3<O2, R (O1::*)(A1, B1, C1), A2, B2, C2> create_guard
		(O2& guard_object, R (O1::*guard_function)(A1, B1, C1), const A2 param_a, const B2 param_b, const C2 param_c) NOTHROW
{
	return scope_guard_object_3<O2, R (O1::*)(A1, B1, C1), A2, B2, C2>::create
		(guard_object, guard_function, param_a, param_b, param_c);
}


/** @endcond */




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__SCOPE_GUARD_HPP) */

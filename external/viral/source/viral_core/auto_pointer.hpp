/**
 *************************************************************************
 *
 * @file auto_pointer.hpp
 *
 * Header for exclusive, object-owning pointers.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__AUTO_POINTER_HPP)
#define INCLUDED__VIRAL_CORE__AUTO_POINTER_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "exception.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup container_module Container Module
 *
 * Provides general pointer and container classes,
 * such as auto pointers, buffers, sets, or maps.
 *
 * General usage notes:
 * - Containers, pointers, and iterators follow the usual semantics.
 *		In particular, dereferencing an invalid pointer or iterator
 *		or accessing an invalid container element yields undefined
 *		behaviour (mostly crashing).
 * - All containers enforce an implementation-specific maximum size.
 *		See container_limits for details.
 * - On insertion, all containers except for buffers and arrays allocate
 *		single objects and do not invalidate any iterators or pointers.
 * - On deletion, all containers except for buffers and arrays only
 *		invalidate iterators or pointers to the removed element.
 * - @attent The state and behaviour of a single container is undefined
 *		during operations on that container. This includes constructing
 *		or copying of containers and thus copy constructors, destructors,
 *		comparison operators or hashing of any contained elements.
 *
 * Implementation notes:
 * - The main implementation focus was to keep container requirements
 *		for attached types as low as possible (no intermediate copies,
 *		no default constructors, no assignment operator).
 * - We do not use the STL for various reasons (bugs in VS2010,
 *		usage restrictions, feature bloat, compile time bloat,
 *		no useable POD buffers, feature-less string interface, ...)
 * - @attent For limited STL compatibility, such as range-based for loops,
 *		some containers implement STL queries for iterators (e.g. begin()). 
 *		Resulting @ref iterator%s do not fully match their STL counterparts,
 *		e.g. bool tests and operator --() instead of end() and rbegin().
 *
 ************************************************************************/




/**
 *************************************************************************
 *
 * @class auto_pointer
 * @ingroup container_module
 *
 * Holds exclusive, transferable ownership over a single,
 * potentially @NULL, object of type @bref{T}.
 *
 ************************************************************************/
template <typename T>
	class auto_pointer :
		private bool_convert<auto_pointer<T>>
{

	public:

		auto_pointer() NOTHROW : object_(NULL) { }


		/**
		 * Accept ownership over (potentially @NULL)
		 * @bref{object} pointer.
		 */
		explicit auto_pointer(T* object) NOTHROW
			: object_(object) { }


		/**
		 * Pass ownership from @bref{other} auto_pointer,
		 * which in turn becomes @NULL.
		 */
		//@{
		auto_pointer(auto_pointer&& other) NOTHROW
			: object_(other.release_ownership()) { }

		template <typename U>
			auto_pointer(auto_pointer<U>&& other) NOTHROW
				: object_(other.release_ownership()) { }
		//@}


		~auto_pointer() NOTHROW { delete object_; }


		/**
		 * Delete currently stored object, reset to @NULL,
		 * and optionally own new @bref{object}.
		 */
		//@{
		void reset() NOTHROW
			{ delete object_; object_ = NULL; }
		void reset(T *object) NOTHROW
			{ delete object_; object_ = object; }
		//@}


		/**
		 * Transfer ownership from @bref{other} pointer,
		 * which deletes any currently associated object,
		 * then reset @bref{other} to @NULL.
		 */
		//@{
		auto_pointer& operator =(auto_pointer&& other) NOTHROW
		{
			if (this == &other) return *this;

			delete object_;
			object_ = other.release_ownership();
			return *this;
		}

		template <typename U>
			auto_pointer& operator =(auto_pointer<U>&& other) NOTHROW 
				{ delete object_; object_ = other.release_ownership(); return *this; }
		//@}


		void swap_with(auto_pointer& other) NOTHROW
			{ swap(object_, other.object_); }


		/**
		 * Try to transfer ownership to a pointer of type @bref{D}.
		 * Return @NULL and keep ownership if types are
		 * incompatible or if the initial pointer is @NULL.
		 *
		 * In general, prefer dynamic over exact casts.
		 * Exact casts are much faster, but only work
		 * for exact object types.
		 */
		//@{
		template <typename D>		
			auto_pointer<D> try_cast() NOTHROW
		{
			D* dyncast = dynamic_cast<D*>(object_);
			if (!dyncast) return auto_pointer<D>();

			object_ = NULL;
			return auto_pointer<D>(dyncast);
		} 

		template <typename D>		
			auto_pointer<D> try_cast_exact() NOTHROW
		{
			if (!object_) return auto_pointer<D>();
			if (typeid(*object_) != typeid(D)) return auto_pointer<D>();

			D* exact = static_cast<D*>(object_);
	
			object_ = NULL;
			return auto_pointer<D>(exact);
		} 
		//@}


		/**
		 * Dereference target object, or throw on @NULL pointer.
		 * Optionally cast to type @bref{T} and throw on failure.
		 * See try_cast() and try_cast_exact().
		 */
		//@{		
		T& object() const
		{
			if (!object_) throw null_pointer();
			return *object_;
		}

		template <typename D>
			D& object() const
		{
			if (!object_) throw null_pointer();
			D* dyncast = dynamic_cast<D*>(object_);
			if (!dyncast) throw type_mismatch();
			return *dyncast;
		}

		template <typename D>
			D& object_exact() const
		{
			if (!object_) throw null_pointer();
			if (typeid(*object_) != typeid(D)) throw type_mismatch();
			return static_cast<D&>(*object_);
		}
		//@}


		/**
		 * Internal pointer access.
		 *
		 * @attent Dereferencing does not check for @NULL pointers.
		 * Use object() for automatic null_pointer exceptions.
		 */
		//@{
		T* get() const NOTHROW { return object_; }
		T& operator *() const NOTHROW { return *object_; }
		T* operator ->() const NOTHROW { return object_; }
		//@}


		/**
		 * Release any currently stored object from ownership,
		 * reset to @NULL, and return native object pointer.
		 *
		 * Implementation notes:
		 * - Opposed to containers that release as auto_pointer%s,
		 *		we explicitly use ownership in the function name,
		 *		because we drop to unmanaged, native pointers.		
		 */
		T* release_ownership() NOTHROW
		{
			T* ret = object_;
			object_ = NULL;
			return ret;
		}


		/** Test if an internal object currently is assigned. */
		operator bool_conversion_type() const NOTHROW
			{ return object_ ? &auto_pointer::bool_conversion_is_true : NULL; }


	private:

		T *object_;
};


/**
 * Create a not-@NULL heap copy of an @bref{object}.
 *
 * To support cloning, object types must implement
 * @code auto_pointer<T> T::clone() const @endcode.
 * This method must never return @NULL, but may throw.
 *
 * Clients should use clone_dynamic() or clone_exact()
 * for object hierarchies with a virtual clone() method.
 *
 * @ingroup container_module
 */
template <typename T>
	auto_pointer<T> clone(const T& object)
		{ return object.clone(); }


// Realize clone semantics for intrinsic @bref{value}s.
//@{
inline auto_pointer<bool> clone(bool value)
	{ return auto_pointer<bool>(new bool(value)); }
inline auto_pointer<int8> clone(int8 value)
	{ return auto_pointer<int8>(new int8(value)); }
inline auto_pointer<int32> clone(int32 value)
	{ return auto_pointer<int32>(new int32(value)); }
inline auto_pointer<uint32> clone(uint32 value)
	{ return auto_pointer<uint32>(new uint32(value)); }
inline auto_pointer<int64> clone(int64 value)
	{ return auto_pointer<int64>(new int64(value)); }
inline auto_pointer<uint64> clone(uint64 value)
	{ return auto_pointer<uint64>(new uint64(value)); }
inline auto_pointer<float> clone(float value)
	{ return auto_pointer<float>(new float(value)); }
inline auto_pointer<double> clone(double value)
	{ return auto_pointer<double>(new double(value)); }
//@}


/**
 * Create object clone(), then cast to type @bref{D}.
 * Throws on incompatible types or a @NULL clone().
 * In the former case, instantly deletes the clone.
 *
 * Useful for derived objects whose virtual clone()
 * returns base type.
 *
 * In general, prefer dynamic over exact cloning.
 * Exact cloning is more efficient, but requires
 * @bref{T} as the exact type of @bref{object}.
 */
//@{
template <typename T, typename D = T>
	auto_pointer<D> clone_dynamic(const T& object)
{
	auto_pointer<D> clone_pointer
		(object.clone().template try_cast<D>());

	if (!clone_pointer)
		throw type_mismatch();

	return clone_pointer;
}

template <typename T, typename D = T>
	auto_pointer<D> clone_exact(const T& object)
{
	auto_pointer<D> clone_pointer
		(object.clone().template try_cast_exact<D>());

	if (!clone_pointer)
		throw type_mismatch();

	return clone_pointer;
}
//@}




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__AUTO_POINTER_HPP) */

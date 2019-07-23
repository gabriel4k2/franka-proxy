/**
 *************************************************************************
 *
 * @file image_any.hpp
 *
 * Image hierarchy.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__IMAGE_ANY_HPP)
#define INCLUDED__VIRAL_CORE__IMAGE_ANY_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"
#include "image_forward.hpp"


#include "util.hpp"
#include "auto_pointer.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class image_any
 * @ingroup image_module
 *
 * Adapter that merges image types into a class hierarchy.
 *
 ************************************************************************/
class image_any
{

	public:

		virtual ~image_any() NOTHROW;


		/** Create not-@NULL duplicate of this image_any. */
		virtual auto_pointer<image_any> clone() const = 0;

	
		void swap_with(image_any& other) NOTHROW;


		/** Fetch various image properties. */
		//@{
		virtual const vector2i& size() const NOTHROW = 0;

		virtual unsigned char* untyped_data() NOTHROW = 0;
		virtual const unsigned char* untyped_data() const NOTHROW = 0;
		virtual int64 untyped_data_byte_count() const NOTHROW = 0;

		virtual int64 bytes_per_pixel() const NOTHROW = 0;
		virtual int64 bytes_per_component() const NOTHROW = 0;
		virtual int64 components_per_pixel() const NOTHROW = 0;
		//@}
};




/**
 *************************************************************************
 *
 * @class image_any_raw
 * @ingroup image_module
 *
 * Integrates an image_raw of image semantics @bref{S}
 * into the image_any hierarchy.
 *
 ************************************************************************/
template <typename S>
	class image_any_raw :
		public image_any
{

	public:

		typedef image_raw<S> image_type;


		image_any_raw() { }


		image_any_raw(const image_type& image)
			: image(image) { }
		image_any_raw(image_type&& image) NOTHROW
			: image(std::move(image)) { }


		/**
		 * Copy or move construct from @bref{other} image wrapper.
		 * Move construction leaves a valid, empty source.
		 */
		//@{
		image_any_raw(const image_any_raw& other)
			:
			image_any(other),
			image(other.image)
		{ }

		image_any_raw(image_any_raw&& other) NOTHROW
			:
			image_any(std::move(other)),
			image(std::move(other.image))
		{ }
		//@}


		auto_pointer<image_any> clone() const override
			{ return auto_pointer<image_any>(new image_any_raw(*this)); }


		/**
		 * Copy or move assignment of @bref{other} image wrapper.
		 * Move assignment leaves a valid, empty source.
		 */
		//@{
		image_any_raw& operator =(const image_any_raw& other)
		{
			if (&other == this) return *this;

			// Strong exception safety.
			image_any_raw offhand_copy(other);
			swap(*this, offhand_copy);
			return *this;
		}

		image_any_raw& operator =(image_any_raw&& other) NOTHROW
		{
			if (&other == this) return *this;

			// Exception safety: NOTHROW.
			image_any_raw offhand_move(std::move(other));
			swap(*this, offhand_move);
			return *this;
		}
		//@}


		void swap_with(image_any_raw& other) NOTHROW
		{
			swap<image_any>(*this, other);
			swap(image, other.image);
		}


		const vector2i& size() const NOTHROW override
			{ return image.size(); }

		unsigned char* untyped_data() NOTHROW override
			{ return image.untyped_data(); }
		const unsigned char* untyped_data() const NOTHROW override
			{ return image.untyped_data(); }
		int64 untyped_data_byte_size() const NOTHROW override
			{ return image.untyped_data_byte_size(); }

		int64 bytes_per_pixel() const NOTHROW override
			{ return image_type::bytes_per_pixel; }
		int64 bytes_per_component() const NOTHROW override
			{ return image_type::bytes_per_component; }
		int64 components_per_pixel() const NOTHROW override
			{ return image_type::components_per_pixel; }


		image_type image; 
};


/**
 * A series of image_any wrappers for common image types.
 * @ingroup image_module
 */
//@{
typedef image_any_raw<image_semantics_binary> image_any_binary;
typedef image_any_raw<image_semantics_greyscale> image_any_greyscale;
typedef image_any_raw<image_semantics_rgba> image_any_rgba;
typedef image_any_raw<image_semantics_hsva> image_any_hsva;
typedef image_any_raw<image_semantics_depth> image_any_depth;
//@}




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__IMAGE_ANY_HPP) */

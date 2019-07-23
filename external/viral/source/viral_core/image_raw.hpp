/**
 *************************************************************************
 *
 * @file image_raw.hpp
 *
 * Raw images.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__IMAGE_RAW_HPP)
#define INCLUDED__VIRAL_CORE__IMAGE_RAW_HPP


#include "build_config.hpp"


#include "auto_pointer.hpp"
#include "buffer.hpp"
#include "geo_2d.hpp"
#include "image_semantics.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup image_module Image Module
 *
 * Provides image-related functionality such as image and color classes.
 *
 ************************************************************************/


/**
 *************************************************************************
 *
 * @class image_raw_errors
 * @ingroup image_module
 *
 * Provides static error handlers for raw images,
 * which avoids header dependencies on logging.
 *
 ************************************************************************/
class image_raw_errors :
	private non_constructable
{
	template <typename T> friend class image_raw;

	private:

		static void log_and_throw_max_size_exceeded();
		static void log_and_throw_invalid_buffer_size();
		static void log_and_throw_invalid_pixel_access();
};




/**
 *************************************************************************
 *
 * @class image_raw
 * @ingroup image_module
 *
 * Uncompressed 2D image with a per-pixel semantics @bref{S}.
 *
 * Data layout:
 * - Image coordinates run left-to-right in x and top-to-bottom in y.
 *		The top-left corner is at (0, 0), and the bottom-right corner
 *		is at (@bref{size.x} - 1, @bref{size.y} - 1).
 * - The internal data buffer holds @bref{size.x} * @bref{size.y} pixels.
 *		Pixels are stored row-first, top-to-bottom, with pixel components
 *		and pixel component layout as defined by the semantics @bref{S}.
 * - An image with a size() of (0, 0) and an empty data buffer is valid
 *		and is used as an empty default image. Other empty (pixel-less)
 *		images are valid as well (e.g. if only @bref{size.x} is zero).
 *
 * Usage notes:
 * - All operations (notably, all failing operations) always leave
 *		the data buffer() consistent with the size() of the image.
 *		To this end, most operations free() the image on failure.
 *
 * Implementation notes:
 * - Raw images assume that all components of the @bref{pixel_type}
 *		have the same fundamental @bref{component_type}.
 * - Raw images keep pixels of @bref{pixel_type} in a dynamic_buffer.
 *		Thus, pixel types must satisfy dynamic_buffer requirements.
 *
 ************************************************************************/
template <typename S>
	class image_raw
{

	public:

		/** Properties of the image semantics @bref{S}. */
		//@{
		typedef S image_semantics;

		typedef typename S::pixel_type pixel_type;
		typedef typename S::component_type component_type;

		static const int64 bytes_per_pixel = S::bytes_per_pixel;
		static const int64 bytes_per_component = S::bytes_per_component;
		static const int64 components_per_pixel = S::components_per_pixel;
		//@}


		/** Storage of pixels in this image. */
		typedef dynamic_buffer<pixel_type> data_buffer;


		/** Create empty image with size (0, 0). */
		image_raw() NOTHROW { }


		/**
		 * Allocate image to given @bref{size},
		 * but do not initialize pixel components.
		 */
		image_raw(const vector2i& size)
			:
			size_(size)
		{
			if (size_.min() < 0 || size_.max() > max_size)
				image_raw_errors::log_and_throw_max_size_exceeded();

			buffer_.resize(size_.x * size_.y);
		}


		/**
		 * Create image from given @bref{size} and data @bref{buffer}.
		 * Moving leaves an empty source @bref{buffer}.
		 */
		//@{
		image_raw(const vector2i& size, const data_buffer &buffer)
			:
			size_(size)
		{
			if (size_.min() < 0 || size_.max() > max_size)
				image_raw_errors::log_and_throw_max_size_exceeded();

			if (buffer.size() != size_.x * size_.y)
				image_raw_errors::log_and_throw_invalid_buffer_size();

			// Avoid copy on invalid buffer sizes:
			// Copy buffer as last operation.
			buffer_ = buffer;
		}


		image_raw(const vector2i& size, data_buffer&& buffer)
			:
			size_(size)
		{
			if (size_.min() < 0 || size_.max() > max_size)
				image_raw_errors::log_and_throw_max_size_exceeded();

			if (buffer.size() != size_.x * size_.y)
				image_raw_errors::log_and_throw_invalid_buffer_size();

			// Strong exception safety:
			// Move buffer as last operation.
			swap(buffer_, buffer);
		}
		//@}


		/**
		 * Copy or move construct from @bref{other} image.
		 * Move construction leaves a valid, empty source.
		 */
		//@{
		image_raw(const image_raw& other)
			:
			buffer_(other.buffer_),
			size_(other.size_)
		{
			// No need to validate sizes.
			// Original must have been valid.
		}


		image_raw(image_raw&& other) NOTHROW
		{
			// Swap ensures defaulted other.
			swap(*this, other);

			// No need to validate sizes.
			// Original must have been valid,
			// and default is valid as well.
		}
		//@}


		/** Create not-@NULL duplicate of this image. */
		auto_pointer<image_raw> clone() const
			{ return auto_pointer<image_raw>(new image_raw(*this)); }


		/**
		 * Copy or move assignment of @bref{other} image.
		 * Move assignment leaves a valid, empty source.
		 */
		//@{
		image_raw& operator =(const image_raw& other)
		{
			if (&other == this) return *this;

			// Strong exception safety.
			image_raw offhand_copy(other);
			swap(*this, offhand_copy);
			return *this;
		}

		image_raw& operator =(image_raw&& other) NOTHROW
		{
			if (&other == this) return *this;

			// Exception safety: NOTHROW.
			image_raw offhand_move(std::move(other));
			swap(*this, offhand_move);
			return *this;
		}
		//@}


		void swap_with(image_raw& other) NOTHROW
		{
			swap(buffer_, other.buffer_);
			swap(size_, other.size_);
		}


		/**
		 * Access pixel at (x, y).
		 * Checks pixel coordinates and unsuited
		 * for performance-critical operations.
		 */
		pixel_type& at(int64 x, int64 y)
		{
			if (x < 0 || x >= size_.x ||
				y < 0 || y >= size_.y)
					image_raw_errors::log_and_throw_invalid_pixel_access();

			return buffer_[y * size_.x + x];
		}

		const pixel_type& at(int64 x, int64 y) const
		{
			if (x < 0 || x >= size_.x ||
				y < 0 || y >= size_.y)
					image_raw_errors::log_and_throw_invalid_pixel_access();

			return buffer_[y * size_.x + x];
		}
		//@}


		const data_buffer& buffer() const NOTHROW
			{ return buffer_; }

		pixel_type* data() NOTHROW
			{ return buffer_.data(); }
		const pixel_type* data() const NOTHROW
			{ return buffer_.data(); }

		const vector2i& size() const NOTHROW
			{ return size_; }


		/** Access to byte-wise, untyped image data. */
		//@{
		unsigned char* untyped_data() NOTHROW
			{ return reinterpret_cast<unsigned char*>(buffer_.data()); }
		const unsigned char* untyped_data() const NOTHROW
			{ return reinterpret_cast<const unsigned char*>(buffer_.data()); }

		int64 untyped_data_byte_size() const NOTHROW
			{ return buffer_.size() * sizeof(pixel_type); }
		//@}


		/**
		 * Move out data buffer of image,
		 * then reset to free()d state.
		 */
		data_buffer move_buffer() NOTHROW
		{
			size_ = vector2i();

			// Moving frees original buffer_.
			return std::move(buffer_);
		}


		/** Flip image horizontally or vertically. */
		//@{
		void flip_horizontal() NOTHROW
		{
			for (int64 y = 0; y < size_.y; ++y)
			for (int64 x = 0; x < size_.x / 2; ++x)
			{
				int64 src = y * size_.x + x;
				int64 dst = y * size_.x + (size_.x - 1 - x);

				for (int64 c = 0; c < components_per_pixel; ++c)
					swap(buffer_[src].c[c], buffer_[dst].c[c]);
			}
		}

		void flip_vertical() NOTHROW
		{
			for (int64 y = 0; y < size_.y / 2; ++y)
			for (int64 x = 0; x < size_.x; ++x)
			{
				int64 src = y * size_.x + x;
				int64 dst = (size_.y - 1 - y) * size_.x + x;

				for (int64 c = 0; c < components_per_pixel; ++c)
					swap(buffer_[src].c[c], buffer_[dst].c[c]);
			}
		}
		//@}


		/**
		 * Set component @bref{c} to @bref{value} for each pixel.
		 * Ignored if @bref{c} is an invalid component.
		 */
		void set_component
			(int64 c, const component_type& value) NOTHROW
		{
			if (c < 0 || c >= components_per_pixel)
				return;

			for (int64 y = 0; y < size_.y; ++y)
			for (int64 x = 0; x < size_.x; ++x)
				{ buffer_[y * size_.x + x].c[c] = value; }
		}


		/**
		 * Swap components @bref{c1} and @bref{c2} for each pixel.
		 * Ignored if either component is invalid.
		 */
		void swap_components(int64 c1, int64 c2) NOTHROW
		{
			if (c1 < 0 || c1 >= components_per_pixel)
				return;
			if (c2 < 0 || c2 >= components_per_pixel)
				return;

			for (int64 y = 0; y < size_.y; ++y)
			for (int64 x = 0; x < size_.x; ++x)
			{
				swap(buffer_[y * size_.x + x].c[c1],
					 buffer_[y * size_.x + x].c[c2]);
			}
		}


		/** Reset size to (0, 0) and free data buffer. */
		void free() NOTHROW
		{
			buffer_.free();
			size_ = vector2i();
		}


		/**
		 * Maximum supported image extents.
		 *
		 * An image at (@bref{max_size}, @bref{max_size})
		 * must not cause an overflow on its data_buffer
		 * or on untyped_data_byte_size().
		 */
		static const int64 max_size = 8192;


	private:

		data_buffer buffer_;
		vector2i size_;
};


/**
 * Raw images with common semantics.
 * @ingroup image_module
 */
//@{
typedef image_raw<image_semantics_binary> image_binary;
typedef image_raw<image_semantics_greyscale> image_greyscale;
typedef image_raw<image_semantics_rgba> image_rgba;
typedef image_raw<image_semantics_hsva> image_hsva;
typedef image_raw<image_semantics_depth> image_depth;
//@}




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__IMAGE_RAW_HPP) */

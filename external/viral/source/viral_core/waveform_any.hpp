/**
 *************************************************************************
 *
 * @file waveform_any.hpp
 *
 * Polymorphic waveforms.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__WAVEFORM_ANY_HPP)
#define INCLUDED__VIRAL_CORE__WAVEFORM_ANY_HPP


#include "build_config.hpp"


#include "string_forward.hpp"
#include "file_forward.hpp"
#include "audio_forward.hpp"


#include "auto_pointer.hpp"
#include "audio_sampling.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class waveform_any
 * @ingroup audio_module
 *
 * Base class for any waveform in CPU memory.
 *
 ************************************************************************/
class waveform_any
{

	public:

		virtual ~waveform_any() NOTHROW;


		/** Create not-@NULL duplicate of waveform. */
		virtual auto_pointer<waveform_any> clone() const = 0;


		void swap_with(waveform_any& other) NOTHROW;


		/** Fetch waveform properties. */
		//@{
		virtual audio_channels_type channels() const NOTHROW = 0;
		virtual audio_rate_type rate() const NOTHROW = 0;

		virtual int64 channel_count() const NOTHROW = 0;
		virtual int64 frames_per_second() const NOTHROW = 0;

		virtual int64 sample_count() const NOTHROW = 0;
		virtual int64 frame_count() const NOTHROW = 0;
		//@}


		/** Approximate play time in (fractional) seconds. */
		virtual float approximate_length_seconds() const NOTHROW = 0;


		/**
		 * Load waveform from virtual @bref{path} on @bref{files}.
		 * Figures out waveform type based on the file extension.
		 */
		static auto_pointer<waveform_any> load
			(const string& path, file_context& files);
};




/**
 *************************************************************************
 *
 * @class waveform_any_typed
 * @ingroup audio_module
 *
 * Directly stores a waveform of type @bref{T}.
 *
 ************************************************************************/
template <typename T>
	class waveform_any_typed :
		public waveform_any
{

	public:

		waveform_any_typed() NOTHROW { }


		waveform_any_typed(const T& waveform)
			: waveform(waveform) { }
		waveform_any_typed(T&& waveform) NOTHROW
			: waveform(std::move(waveform)) { }


		/**
		 * Copy or move construct from @bref{other} waveform wrapper.
		 * Move construction leaves a valid, empty source.
		 */
		//@{
		waveform_any_typed(const waveform_any_typed& other)
			: waveform(other.waveform) { }
		waveform_any_typed(waveform_any_typed&& other) NOTHROW
			: waveform(std::move(other.waveform)) { }
		//@}


		auto_pointer<waveform_any> clone() const override
		{
			return auto_pointer<waveform_any>
						(new waveform_any_typed(*this));
		}


		/**
		 * Copy or move assignment of @bref{other} waveform wrapper.
		 * Move assignment leaves a valid, empty source.
		 */
		//@{
		waveform_any_typed& operator =(const waveform_any_typed& other)
		{
			if (&other == this) return *this;

			// Strong exception safety.
			waveform_any_typed offhand_copy(other);
			swap(*this, offhand_copy);
			return *this;
		}

		waveform_any_typed& operator =(waveform_any_typed&& other) NOTHROW
		{
			if (&other == this) return *this;

			// Exception safety: NOTHROW.
			waveform_any_typed offhand_move(std::move(other));
			swap(*this, offhand_move);
			return *this;
		}
		//@}


		void swap_with(waveform_any_typed& other) NOTHROW
		{
			swap<waveform_any>(*this, other);
			swap(waveform, other.waveform);
		}


		audio_channels_type channels() const NOTHROW override
			{ return waveform.channels(); }
		audio_rate_type rate() const NOTHROW override
			{ return waveform.rate(); }

		int64 channel_count() const NOTHROW override
			{ return waveform.channel_count(); }
		int64 frames_per_second() const NOTHROW override
			{ return waveform.frames_per_second(); }

		int64 sample_count() const NOTHROW override
			{ return waveform.sample_count(); }
		int64 frame_count() const NOTHROW override
			{ return waveform.frame_count(); }

		float approximate_length_seconds() const NOTHROW override
			{ return waveform.approximate_length_seconds(); }


		T waveform;
};


/**
 * A series of waveform_any wrappers for common waveform types.
 * @ingroup audio_module
 */
//@{
typedef waveform_any_typed<waveform_raw> waveform_any_raw;
typedef waveform_any_typed<waveform_ogg> waveform_any_ogg;
//@}




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__WAVEFORM_ANY_HPP) */

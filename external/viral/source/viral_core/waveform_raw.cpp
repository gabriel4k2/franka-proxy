/**
 *************************************************************************
 *
 * @file waveform_raw.cpp
 *
 * Implementation for uncompressed host-memory waveforms.
 *
 ************************************************************************/


#include "waveform_raw.hpp"


#include "log.hpp"
#include "auto_pointer.hpp"
#include "tree_map.hpp"
#include "file.hpp"
#include "riff_util.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// waveform_raw
//
//////////////////////////////////////////////////////////////////////////


waveform_raw::waveform_raw() NOTHROW
	:
	channels_(audio_channels::channels_unknown),
	rate_(audio_rate::rate_unknown)
{
	// No need to validate().
	// Default waveform is always valid.
}


waveform_raw::waveform_raw
	(audio_channels_type channels,
	 audio_rate_type rate)
	:
	channels_(channels),
	rate_(rate)
{
	validate();
}


waveform_raw::waveform_raw
	(audio_channels_type channels,
	 audio_rate_type rate,
	 int64 frame_count)
	:
	channels_(channels),
	rate_(rate)
{
	buffer_.resize(frame_count);
	validate();
}


waveform_raw::waveform_raw
	(const data_buffer& buffer,
	 audio_channels_type channels,
	 audio_rate_type rate)
	:
	buffer_(buffer), 
	channels_(channels),
	rate_(rate)
{
	validate();
}


waveform_raw::waveform_raw
	(data_buffer&& buffer,
	 audio_channels_type channels,
	 audio_rate_type rate)
	:
	buffer_(std::move(buffer)),
	channels_(channels),
	rate_(rate)
{
	validate();
}


waveform_raw::waveform_raw(const waveform_raw& other)
	:
	buffer_(other.buffer_),
	channels_(other.channels_),
	rate_(other.rate_)
{
	// No need to validate().
	// Original must have been valid.
}


waveform_raw::waveform_raw(waveform_raw&& other) NOTHROW
	:
	channels_(audio_channels_type::channels_unknown),
	rate_(audio_rate_type::rate_unknown)
{
	// Swap ensures defaulted other.
	swap(*this, other);

	// No need to validate().
	// Original must have been valid,
	// and default is valid as well.
}


auto_pointer<waveform_raw> waveform_raw::clone() const
	{ return auto_pointer<waveform_raw>(new waveform_raw(*this)); }


waveform_raw&
	waveform_raw::operator =(const waveform_raw& other)
{
	if (&other == this) return *this;

	// Strong exception safety.
	waveform_raw offhand_copy(other);
	swap(*this, offhand_copy);
	return *this;
}


waveform_raw&
	waveform_raw::operator =(waveform_raw&& other) NOTHROW
{
	if (&other == this) return *this;

	// Exception safety: NOTHROW.
	waveform_raw offhand_move(std::move(other));
	swap(*this, offhand_move);
	return *this;
}


void waveform_raw::swap_with(waveform_raw& other) NOTHROW
{
	swap(buffer_, other.buffer_);
	swap_assign(channels_, other.channels_);
	swap_assign(rate_, other.rate_);
}


const waveform_raw::data_buffer&
	waveform_raw::buffer() const NOTHROW
		{ return buffer_; }

int16* waveform_raw::data() NOTHROW
	{ return buffer_.data(); }
const int16* waveform_raw::data() const NOTHROW
	{ return buffer_.data(); }


audio_channels_type waveform_raw::channels() const NOTHROW
	{ return channels_; }
audio_rate_type waveform_raw::rate() const NOTHROW
	{ return rate_; }


int64 waveform_raw::channel_count() const NOTHROW
	{ return audio_channels::to_channel_count(channels_); }
int64 waveform_raw::frames_per_second() const NOTHROW
	{ return audio_rate::to_frames_per_second_int64(rate_); }


int64 waveform_raw::sample_count() const NOTHROW
	{ return buffer_.size(); }

int64 waveform_raw::frame_count() const NOTHROW
{
	if (channels_ == audio_channels::channels_unknown ||
		rate_ == audio_rate::rate_unknown)
			return 0;

	// Division has no remainder, see validate().
	return buffer_.size() / channel_count();
}


float waveform_raw::approximate_length_seconds() const NOTHROW
{
	if (channels_ == audio_channels::channels_unknown ||
		rate_ == audio_rate::rate_unknown)
			return 0.f;

	return static_cast<float>(frame_count()) /
		   static_cast<float>(frames_per_second());
}


void waveform_raw::clear() NOTHROW
{
	buffer_.clear();
	channels_ = audio_channels::channels_unknown;
	rate_ = audio_rate::rate_unknown;
}


waveform_raw waveform_raw::load
	(const string& path, file_context& files)
{
	auto_pointer<file> input
		(files.open_file(path, file::read_only));

	return load(*input);
}


waveform_raw waveform_raw::load(file& input)
{
	// Parse RIFF structure of WAVE file.
	riff_util::chunk_id_to_offset_map chunk_id_to_offset;
	riff_util::check_file_and_map_chunks
		(input, "WAVE", chunk_id_to_offset);


	// Seek to format chunk and check basic format.
	// We require at least a WAVEFORMATEX PCM variant,
	// though some exporters seem to miss cbSize.
	tree_map<string, int64>::const_iterator fmt_chunk_it
		(chunk_id_to_offset.find("FMT "));

	if (!fmt_chunk_it)
	{
		LOG_ERROR("Wave file has no FMT chunk.");
		throw corrupt_data();
	}

	input.seek
		(fmt_chunk_it->value, file::seek_begin);

	uint32 fmt_length = 0;
	input.read(fmt_length);

	if (fmt_length < 16) // sizeof(WAVEFORMATEX) - sizeof(cbSize)
	{
		LOG_ERROR("Wave file has legacy or corrupt FMT chunk.");
		throw corrupt_data();
	}

	uint16 format_tag = 0;
	input.read(format_tag);

	if (format_tag != 1)
	{
		LOG_ERROR("Wave file does not have PCM format.");
		throw unsupported_feature();
	}


	// Fetch channel count and sampling rate.
	// Check if these match with supported variants.
	// Widening does not cause any overflow.
	uint16 channel_count_narrow = 0;
	input.read(channel_count_narrow);
	int64 channel_count = channel_count_narrow;

	audio_channels_type channels =
		audio_channels::to_channels(channel_count);

	if (channels == audio_channels::channels_unknown)
	{
		LOG_ERROR("Unsupported channel type in wave file.");
		throw unsupported_feature();
	}

	uint32 frames_per_second_narrow = 0;
	input.read(frames_per_second_narrow);
	int64 frames_per_second = frames_per_second_narrow;

	audio_rate_type rate =
		audio_rate::to_rate(frames_per_second);

	if (rate == audio_rate::rate_unknown)
	{
		LOG_ERROR("Unsupported sampling rate in wave file.");
		throw unsupported_feature();
	}


	// Skip average bytes per second
	// and sample alignment in container.
	input.seek(4, file::seek_current);
	input.seek(2, file::seek_current);


	// Check sample bit depth.
	uint16 bits_per_sample_narrow = 0;
	input.read(bits_per_sample_narrow);
	int64 bits_per_sample = bits_per_sample_narrow;

	if (bits_per_sample != 8 && bits_per_sample != 16)
	{
		LOG_ERROR("Unsupported bit depth in wave file.");
		throw unsupported_feature();
	}
	

	// Seek to chunk with waveform data.
	tree_map<string, int64>::const_iterator data_chunk_it
		(chunk_id_to_offset.find("DATA"));

	if (!data_chunk_it)
	{
		LOG_ERROR("Wave file has no data chunk.");
		throw corrupt_data();
	}

	input.seek
		(data_chunk_it->value, file::seek_begin);


	// Fetch data length in bytes,
	// then derive number of samples.
	uint32 data_length_narrow = 0;
	input.read(data_length_narrow);
	int64 data_length = data_length_narrow;

	if (data_length % (bits_per_sample / 8) != 0)
	{
		LOG_ERROR("Data length is missing a partial sample.");
		throw corrupt_data();
	}

	int64 sample_count =
		data_length / (bits_per_sample / 8);

	if (sample_count % channel_count != 0)
	{
		LOG_ERROR("No sample per frame and channel in wave file.");
		throw corrupt_data();
	}


	// Read data in one call for efficiency.
	// Possibly convert to 16 bit samples.
	dynamic_buffer<int16> buffer;
	buffer.resize(sample_count);

	if (bits_per_sample == 8)
	{
		dynamic_buffer<unsigned char> buffer_char;
		buffer_char.resize(sample_count);
		input.read_bytes
			(buffer_char.data(),
			 buffer_char.size() * size_of<unsigned char>());

		for (int64 i = 0; i < buffer_char.size(); ++i)
		{
			// Operation order avoids integer overflows
			// and minimizes rounding errors.
			int32 sample = static_cast<int32>(buffer_char[i]);
			int32 sample_renorm = (sample - 128) * 32767 / 128;
			buffer[i] = static_cast<int16>(sample_renorm);			
		}
	}
	else if (bits_per_sample == 16)
	{
		input.read_bytes
			(buffer.data(),
			 buffer.size() * size_of<int16>());
	}
	else
	{
		LOG_ERROR("Conversion not implemented for wave bit depth.");
		throw assert_failed();
	}


	// Success.
	return waveform_raw
				(std::move(buffer), channels, rate);
}


void waveform_raw::validate()
{
	if (channels_ < 0 || channels_ >= audio_channels::channels_count)
	{
		LOG_ERROR("Invalid channels.");
		throw invalid_parameter();
	}

	if (rate_ < 0 || rate_ >= audio_rate::rate_count)
	{
		LOG_ERROR("Invalid rate.");
		throw invalid_parameter();
	}

	if (channels_ == audio_channels::channels_unknown ||
		rate_ == audio_rate::rate_unknown)
	{
		if (!buffer_.empty())
		{
			LOG_ERROR("Waveforms with unknown channels or rate must be empty.");
			throw invalid_parameter();
		}
	}
	else
	{
		if (channel_count() == 0)
		{
			LOG_ERROR("Known channels constant has zero channel count.");
			throw assert_failed();
		}

		if (frames_per_second() == 0)
		{
			LOG_ERROR("Known rate constant has zero frames per second.");
			throw assert_failed();
		}

		if (buffer_.size() > max_sample_count)
		{
			LOG_ERROR("Too many samples.");
			throw invalid_parameter();
		}

		if (buffer_.size() % channel_count() != 0)
		{
			LOG_ERROR("Not a sample per channel and frame.");
			throw invalid_parameter();
		}
	}
}


/** Current limit is 10 minutes of 7.1 channel audio at 96khz. */
const int64 waveform_raw::max_sample_count =
	10 * 60 * 8 * 96000;




} /* namespace viral_core */

/**
 *************************************************************************
 *
 * @file waveform_ogg.cpp
 *
 * Waveforms with OGG compression, implementation.
 *
 ************************************************************************/


#include "waveform_ogg.hpp"


#include "file.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// waveform_ogg
//
//////////////////////////////////////////////////////////////////////////


waveform_ogg::waveform_ogg() NOTHROW
{
	// No need to validate().
	// Default waveform is always valid.
}


waveform_ogg::waveform_ogg
	(const data_buffer& buffer)
	:
	buffer_(buffer)
{
	validate();
}


waveform_ogg::waveform_ogg
	(data_buffer&& buffer)
	:
	buffer_(std::move(buffer))
{
	validate();
}


waveform_ogg::waveform_ogg(const waveform_ogg& other)
	:
	buffer_(other.buffer_)
{
	// No need to validate().
	// Original must have been valid.
}


waveform_ogg::waveform_ogg(waveform_ogg&& other) NOTHROW
{
	// Swap ensures defaulted other.
	swap(*this, other);

	// No need to validate().
	// Original must have been valid,
	// and default is valid as well.
}


auto_pointer<waveform_ogg> waveform_ogg::clone() const
	{ return auto_pointer<waveform_ogg>(new waveform_ogg(*this)); }


waveform_ogg&
	waveform_ogg::operator =(const waveform_ogg& other)
{
	if (&other == this) return *this;

	// Strong exception safety.
	waveform_ogg offhand_copy(other);
	swap(*this, offhand_copy);
	return *this;
}


waveform_ogg&
	waveform_ogg::operator =(waveform_ogg&& other) NOTHROW
{
	if (&other == this) return *this;

	// Exception safety: NOTHROW.
	waveform_ogg offhand_move(std::move(other));
	swap(*this, offhand_move);
	return *this;
}


void waveform_ogg::swap_with(waveform_ogg& other) NOTHROW
	{ swap(buffer_, other.buffer_); }


const waveform_ogg::data_buffer&
	waveform_ogg::buffer() const NOTHROW
		{ return buffer_; }

unsigned char* waveform_ogg::data() NOTHROW
	{ return buffer_.data(); }
const unsigned char* waveform_ogg::data() const NOTHROW
	{ return buffer_.data(); }


audio_channels_type waveform_ogg::channels() const NOTHROW
	{ return audio_channels::channels_unknown; }
audio_rate_type waveform_ogg::rate() const NOTHROW
	{ return audio_rate::rate_unknown; }

int64 waveform_ogg::channel_count() const NOTHROW
	{ return 0; }
int64 waveform_ogg::frames_per_second() const NOTHROW
	{ return 0; }

int64 waveform_ogg::sample_count() const NOTHROW
	{ return 0; }
int64 waveform_ogg::frame_count() const NOTHROW
	{ return 0; }

float waveform_ogg::approximate_length_seconds() const NOTHROW
	{ return 0; }


void waveform_ogg::clear() NOTHROW
	{ buffer_.clear(); }


waveform_ogg waveform_ogg::load
	(const string& path, file_context& files)
{
	auto_pointer<file> input
		(files.open_file(path, file::read_only));

	return load(*input);
}


waveform_ogg waveform_ogg::load(file& input)
{
	// Read entire ogg file into data buffer.
	// The waveform_ogg constructor validate()s data consistency.
	int64 size = input.find_size();

	data_buffer buffer;
	buffer.resize(size);
	input.read_bytes(buffer.data(), size);

	return waveform_ogg(std::move(buffer));
}


void waveform_ogg::validate()
{
}




} /* namespace viral_core */

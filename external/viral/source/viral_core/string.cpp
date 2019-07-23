/**
 *************************************************************************
 *
 * @file string.cpp
 *
 * String utilities implementation.
 *
 ************************************************************************/


#include "string.hpp"


#include <stdio.h>
#include <string.h>


#include "integer_operations.hpp"
#include "auto_pointer.hpp"
#include "container_limits.hpp"
#include "list.hpp"
#include "hash_util.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//  string_buffer
//
//////////////////////////////////////////////////////////////////////////

		
string_buffer::string_buffer() NOTHROW
	:
	data_(default_empty_literal_),
	size_(0),
	capacity_(0)
{ }


string_buffer::string_buffer(string_buffer&& source) NOTHROW
	:
	data_(default_empty_literal_),
	size_(0),
	capacity_(0)
{
	swap(*this, source);
}


string_buffer::~string_buffer() NOTHROW
	{ free(); }


void string_buffer::reserve(int64 request_capacity)
{
	/**
	 * Implementation notes:
	 * - Compared to dynamic_buffer::reserve(), we limit capacity 
	 *		to container_limits::max_size - 1. This gives leeway
	 *		for storing the terminating, but uncounted \0.
	 */


	// Validate request.
	if (request_capacity < 0)
		throw invalid_parameter();
	if (request_capacity >= container_limits<char>::max_size)
		throw bounds_exceeded();

	
	// Early out if we have enough storage.
	if (request_capacity <= capacity_)
		return;


	// Extrapolate future capacity requirements based on current capacity.
	// This avoids frequent reallocations on many push_back() operations.
	// Additionally clamp extrapolation to container size limits.
	int64 grow_capacity =
		capacity_ / 2; // Never overflows.

	int64 extrapolate_capacity =
		(!integer_checks::is_add_safe(capacity_, grow_capacity) ||
		 capacity_ + grow_capacity >= container_limits<char>::max_size) ?
			container_limits<char>::max_size - 1 :
			(capacity_ + grow_capacity);
				

	// Directly use capacity request if it exceeds the extrapolation,
	// and use a fixed lower capacity limit to avoid dense reallocations
	// on the first few entries.
	int64 new_capacity =
		max(32, max(extrapolate_capacity, request_capacity));


	// Allocate actual string data.
	// This includes the terminating, but uncounted \0.
	data_ =		
		data_ == default_empty_literal_ ?
		memory::alloc_typed<char>(new_capacity + 1) :
		memory::realloc_typed(data_, new_capacity + 1);


	// Ensure \\0-terminated string.
	// This is a no-op, except when reserve()ing from bypass to heap,
	// which requires copying the \\0 of the default_empty_literal_.
	data_[size_] = '\0';


	// Update current capacity.
	// Do not count the terminating \0.
	capacity_ = new_capacity;
}


void string_buffer::resize(int64 new_size)
{
	reserve(new_size);

	// Exception safety: NOTHROW from here.
	size_ = new_size;

	// Ensure that string remains zero-terminated,
	// but do not write to default literal.
	if (data_ != default_empty_literal_)
		data_[size_] = '\0';
}


int64 string_buffer::size() const NOTHROW
	{ return size_; }
int64 string_buffer::capacity() const NOTHROW
	{ return capacity_; }


const char* string_buffer::data() const NOTHROW
	{ return data_; }
char* string_buffer::data() NOTHROW
	{ return data_; }

		
const char& string_buffer::operator [](int64 index) const NOTHROW
	{ return data_[index]; }
char& string_buffer::operator [](int64 index) NOTHROW
	{ return data_[index]; }


const char& string_buffer::at(int64 index) const
{
	if (index < 0 || index > size_) // Allow \0 access.
		throw invalid_parameter();

	return data_[index];
}

char& string_buffer::at(int64 index)
{
	if (index < 0 || index > size_) // Allow \0 access.
		throw invalid_parameter();

	return data_[index];
}


void string_buffer::swap_with(string_buffer& other) NOTHROW
{
	swap(data_, other.data_);
	swap(size_, other.size_);
	swap(capacity_, other.capacity_);
}


void string_buffer::clear() NOTHROW
{
	if (data_ != default_empty_literal_)
		data_[0] = '\0';

	size_ = 0;
}


void string_buffer::free() NOTHROW
{
	if (data_ != default_empty_literal_)
	{
		memory::free(data_);
		data_ = default_empty_literal_;
	}

	size_ = 0;
	capacity_ = 0;
}


/**
 * Shared, empty literal for string_buffer%s.
 * Non-const to enable a non-const string_buffer::data_.
 * This causes no conflicts, as clients anyways must not
 * write to the terminating \\0.
 */
char* string_buffer::default_empty_literal_ = "";




//////////////////////////////////////////////////////////////////////////
//
//  string
//
//////////////////////////////////////////////////////////////////////////


string::string() NOTHROW
	{ /** See string_buffer() for NOTHROW initialization to "". */ }

string::string(const string& source)
	{ set(source.data(), source.size()); }

string::string(string&& source) NOTHROW
	: buffer_(std::move(source.buffer_)) { }

string::string(const char *nt_string)
	{ set(nt_string, size(nt_string)); }

string::string(const char* array, int64 count)
	{ set(array, count); }

string::string(char ascii)
	{ set(&ascii, 1); }


string::string(int32 value)
{
	// Maximum length value: -2^31 = -2147483648\0.
	// Minimum 12 chars, some overhead for safety.
	static const int64 buffer_chars = 16;
	char buffer[buffer_chars + 1];

	#if defined(VIRAL__COMPILER_VISUALCPP)

		errno_t errorcode =
			_itoa_s(value, buffer, buffer_chars, 10);
		if (errorcode != 0)
			throw assert_failed();

	#elif defined(VIRAL__COMPILER_GCC)
		sprintf(buffer, "%d", value);
	#else
		#error "string(int32) not implemented for compiler."
	#endif

	buffer[buffer_chars] = '\0'; // Add additional \0 for safety.
	set(buffer, size(buffer));
}


string::string(uint32 value)
{
	// Maximum length value: 2^32 = 4294967296\0.
	// Minimum 11 chars, some overhead for safety.
	static const int64 buffer_chars = 16;
	char buffer[buffer_chars + 1];

	#if defined(VIRAL__COMPILER_VISUALCPP)

		errno_t errorcode =
			_i64toa_s(value, buffer, buffer_chars, 10);
		if (errorcode != 0)
			throw assert_failed();

	#elif defined(VIRAL__COMPILER_GCC)
		sprintf(buffer, "%u", value);
	#else
		#error "string(uint32) not implemented for compiler."
	#endif

	buffer[buffer_chars] = '\0'; // Add additional \0 for safety.
	set(buffer, size(buffer));
}


string::string(int64 value)
{
	// Maximum length value: 2^63-1 = 9223372036854775807\0,
	// or -2^63 = -9223372036854775808\0, minimum 21 chars.
	// plus some overhead for safety.
	static const int64 buffer_chars = 32;
	char buffer[buffer_chars + 1];

	#if defined(VIRAL__COMPILER_VISUALCPP)

		errno_t errorcode =
			_i64toa_s(value, buffer, buffer_chars, 10);
		if (errorcode != 0)
			throw assert_failed();

	#elif defined(VIRAL__COMPILER_GCC)
		sprintf(buffer, "%ll", value);
	#else
		#error "string(int64) not implemented for compiler."
	#endif

	buffer[buffer_chars] = '\0'; // Add additional \0 for safety.
	set(buffer, size(buffer));
}


string::string(uint64 value)
{
	// Maximum length value: 2^64-1 = 18446744073709551615\0,
	// minimum 21 chars, plus some overhead for safety.
	static const int64 buffer_chars = 32;
	char buffer[buffer_chars + 1];

	#if defined(VIRAL__COMPILER_VISUALCPP)

		errno_t errorcode =
			_ui64toa_s(value, buffer, buffer_chars, 10);
		if (errorcode != 0)
			throw assert_failed();

	#elif defined(VIRAL__COMPILER_GCC)
		sprintf(buffer, "%llu", value);
	#else
		#error "string(uint64) not implemented for compiler."
	#endif

	buffer[buffer_chars] = '\0'; // Add additional \0 for safety.
	set(buffer, size(buffer));
}


string::string(float value, int64 precision)
{
	// Maximum length value: maximum digits precision,
	// sign, period, exponent indicator, exponent sign,
	// exponent, which gives -12345678.12345678e-055\0.
	// Thus minimum 24 chars, lots of overhead for safety,
	// since formatting is less reliable for floats.
	static const int64 buffer_chars = 64;
	char buffer[buffer_chars + 1];

	int clamped_precision = // sprintf_s() requires native int.
		integer_operations::cast_or_throw<int>
			(max(float_to_string_precision_min,
				 min(float_to_string_precision_max, precision)));

	#if defined(VIRAL__COMPILER_VISUALCPP)

		int64 numwritten =
			sprintf_s(buffer, buffer_chars, "%.*g",
					  clamped_precision, value);

		if (numwritten <= - 1 || numwritten > buffer_chars)
			throw assert_failed();

	#else
		#error "string(float, int64) not implemented for compiler."
	#endif

	buffer[buffer_chars] = '\0'; // Add additional \0 for safety.
	set(buffer, size(buffer));
}


string::string(float value)
{
	// Fixed-precision variant of the above.
	static const int64 buffer_chars = 64;
	char buffer[buffer_chars + 1];

	int clamped_precision = // sprintf_s() requires native int.
		integer_operations::cast_or_throw<int>
			(max(float_to_string_precision_min,
				 min(float_to_string_precision_max,
					 float_to_string_precision_default)));

	#if defined(VIRAL__COMPILER_VISUALCPP)

		int64 numwritten =
			sprintf_s(buffer, buffer_chars, "%.*g",
					  clamped_precision, value);

		if (numwritten <= -1 || numwritten > buffer_chars)
			throw assert_failed();

	#else
		#error "string(float) not implemented for compiler."
	#endif

	buffer[buffer_chars] = '\0'; // Add additional \0 for safety.
	set(buffer, size(buffer));
}


auto_pointer<string> string::clone() const
	{ return auto_pointer<string>(new string(*this)); }


void string::reserve(int64 capacity)
	{ buffer_.reserve(capacity); }
void string::resize(int64 size)
	{ buffer_.resize(size); }


int64 string::size() const NOTHROW
	{ return buffer_.size(); }

int64 string::size(const char *nt_string) NOTHROW
{
	if (!nt_string) return 0;

	return integer_operations::cast_or_clamp<int64>
				(strlen(nt_string));
}

int64 string::capacity() const NOTHROW
	{ return buffer_.capacity(); }

bool string::empty() const NOTHROW
	{ return buffer_.size() == 0; }


void string::set(const char *array, int64 count)
{
	if (count < 0) throw invalid_parameter();
	if (count > 0 && !array) throw null_pointer();

	resize(count);
	memory::copy_block_typed(data(), array, count);	
 }


const char* string::data() const NOTHROW
	{ return buffer_.data(); }
char* string::data() NOTHROW
	{ return buffer_.data(); }

	
const char& string::operator [](int64 index) const NOTHROW
	{ return buffer_[index]; }
char& string::operator [](int64 index) NOTHROW
	{ return buffer_[index]; }


const char& string::at(int64 index) const
	{ return buffer_.at(index); }
char& string::at(int64 index)
	{ return buffer_.at(index); }


void string::append(char chr)
{
	 // No overflow, terminating \0 is valid index.
	resize(size() + 1);

	buffer_[size() - 1] = chr;
}


void string::append(char chr, int64 count)
{
	if (count < 0)
		throw invalid_parameter();

	int64 oldlen = size();

	resize
		(integer_operations::add_or_throw
			(size(), count));

	for (int64 i = 0; i < count; ++i)
		buffer_[oldlen + i] = chr;
}


void string::append(const char* str, int64 count)
{
	if (count < 0) throw invalid_parameter();
	if (count > 0 && !str) throw null_pointer();

	int64 oldlen = size();

	resize
		(integer_operations::add_or_throw
			(size(), count));

	memory::copy_block_typed
		(&buffer_[oldlen], str, count);
}


void string::insert(char chr, int64 index)
{
	int64 cursize = size();

	if (index < 0 || index > cursize)
		throw invalid_parameter();

	 // No overflow, terminating \0 is valid index.
	resize(cursize + 1);

	for (int64 i = cursize; i > index; --i)
		buffer_[i] = buffer_[i - 1];

	buffer_[index] = chr;
}


void string::remove(int64 index) NOTHROW
{
	int64 cursize = size();

	if (index < 0 || index >= cursize)
		return; // NOTHROW.

	for (int64 i = index + 1; i < cursize; ++i)
		buffer_[i - 1] = buffer_[i];

	// Resize does not throw:
	// cursize >= 1, i.e. cursize - 1 >= 0, 
	// because index is a valid character.
	resize(cursize - 1);
}


string& string::remove_all(char chr) NOTHROW
{
	int64 cursize = size();
	int64 removed = 0;

	for (int64 i = 0; i < cursize; ++i)
	{
		buffer_[i] = buffer_[i + removed];

		if (buffer_[i] != chr) continue;
		
		--i;

		++removed;
		--cursize;
	}

	resize(cursize);

	return *this;
}


void string::push_front(char chr) { insert(chr, 0); }
void string::push_back(char chr) { insert(chr, size()); }
void string::pop_front() NOTHROW { remove(0); }
void string::pop_back() NOTHROW { remove(size() - 1); }


bool string::contains(const string& other) const NOTHROW
	{ return seek(other) != invalid_index; }
bool string::contains(const char* nt_string) const NOTHROW
	{ return seek(nt_string) != invalid_index; }
bool string::contains_array(const char* array, int64 count) const NOTHROW
	{ return seek_array(array, count) != invalid_index; }


bool string::begins_with(const string& other) const NOTHROW
	{ return begins_with_array(other.data(), other.size()); }
bool string::begins_with(const char* nt_string) const NOTHROW
	{ return begins_with_array(nt_string, size(nt_string)); }

bool string::ends_with(const string& other) const NOTHROW
	{ return ends_with_array(other.data(), other.size()); }
bool string::ends_with(const char* nt_string) const NOTHROW
	{ return ends_with_array(nt_string, size(nt_string)); }


bool string::begins_with_array(const char* array, int64 count) const NOTHROW
{
	if (count <= 0 || count > size())
		return false;

	for (int64 i = 0; i < count; ++i)
	{
		if (buffer_[i] != array[i])
			return false;
	}

	return true;
}

bool string::ends_with_array(const char* array, int64 count) const NOTHROW
{
	if (count <= 0 || count > size())
		return false;

	int64 start_in_this = size() - count;
	for (int64 i = 0; i < count; ++i)
	{
		if (buffer_[start_in_this + i] != array[i])
			return false;
	}

	return true;
}


int64 string::seek(char chr, int64 start_index) const NOTHROW
{
	int64 cursize = size();

	if (start_index >= cursize)
		return invalid_index;

	if (start_index < 0)
		start_index = 0;

	for (int64 i = start_index; i < cursize; ++i)
	{
		if (buffer_[i] == chr)
			return i;
	}

	return invalid_index;
}

int64 string::seek(char chr) const NOTHROW
	{ return seek(chr, 0); }


int64 string::seek_reverse(char chr, int64 start_index) const NOTHROW
{
	if (start_index < 0) return invalid_index;

	int64 cursize = size();

	if (start_index >= cursize)
		start_index = cursize - 1;

	for (int64 i = start_index; i >= 0; --i)
	{
		if (buffer_[i] == chr)
			return i;
	}

	return invalid_index;
}

int64 string::seek_reverse(char chr) const NOTHROW
	{ return seek_reverse(chr, size() - 1); }


int64 string::seek(const string& other) const NOTHROW
	{ return seek(other, 0); }
int64 string::seek(const string& other, int64 start_index) const NOTHROW
	{ return seek_array(other.data(), other.size(), start_index); }

int64 string::seek(const char* nt_string) const NOTHROW
	{ return seek(nt_string, 0); }
int64 string::seek(const char* nt_string, int64 start_index) const NOTHROW
	{ return seek_array(nt_string, size(nt_string), start_index); }


int64 string::seek_array
	(const char* array, int64 count) const NOTHROW
		{ return seek_array(array, count, 0); }

int64 string::seek_array
	(const char* array, int64 count, int64 start_index) const NOTHROW
{
	// Check preconditions.
	if (!array) return invalid_index;
	if (count <= 0) return invalid_index;

	int64 cursize = size();
	if (cursize == 0) return invalid_index;


	// Clamp and check start index.
	// Early-out on past-end starting index.
	start_index =
		max(0, min(cursize, start_index));

	if (start_index == cursize)
		return invalid_index;


	// Search string for matches.
	for (int64 i = start_index; i < cursize; ++i)
	{
		if (!integer_checks::is_add_safe(i, count) ||
			i + count > cursize)
		{
			// String lookup exceeds allowed index range.
			// There can be no further match.
			return invalid_index;
		}

		for (int64 j = 0; j < count; ++j)
		{
			if (buffer_[i + j] != array[j])
				goto no_match;
		}

		// Match found.
		return i;

		no_match:;
	}


	// No match found.
	return invalid_index;
}


string& string::replace_all(char oldchr, char newchr) NOTHROW
{
	int64 cursize = size();

	for (int64 i = 0; i < cursize; ++i)
		if (buffer_[i] == oldchr)
			buffer_[i] = newchr;

	return *this;
}


string string::pad_left(char c, int64 min_size) const
{
	if (min_size < 0)
		throw invalid_parameter();
	if (min_size >= container_limits<char>::max_size) // >=: Terminating \0.
		throw bounds_exceeded();

	string result;
	result.reserve(min_size);

	while (result.size() + size() < min_size)
		result.push_back(c);

	result += *this;
	return result;
}


string string::pad_right(char c, int64 min_size) const
{
	if (min_size < 0)
		throw invalid_parameter();
	if (min_size >= container_limits<char>::max_size) // >=: Terminating \0.
		throw bounds_exceeded();

	string result(*this);
	result.reserve(min_size); // Ignored on min_size < size().

	while (result.size() < min_size)
		result.push_back(c);

	return result;
}


string string::trim() const
{
	int64 cursize = size();

	int64 start = 0;
	while (start < cursize &&
		   (buffer_[start] == ' ' || buffer_[start] == '\t' ||
			buffer_[start] == '\r' || buffer_[start] == '\n'))
		{ ++start; }

	int64 end = cursize - 1;
	while (end >= 0 &&
		   (buffer_[end] == ' ' || buffer_[end] == '\t' ||
			buffer_[end] == '\r' || buffer_[end] == '\n'))
		{ --end; }

	if (end < start) // Empty or only whitespace
		return string();

	return
		substring(start, end - start + 1);
}


string string::compact() const
{
	int64 cursize = size();

	string result;
	result.reserve(cursize);

	int64 i = 0;
	while (i < cursize)
	{
		char curchar = buffer_[i];

		if (curchar == ' ' || curchar == '\t' ||
			curchar == '\r' || curchar == '\n')
		{	
			// Find next non-whitespace character
			int64 next = i + 1;
			char nextchar = buffer_[next];

			while (next < cursize &&
				   (nextchar == ' ' || nextchar == '\t' ||
					nextchar == '\r' || nextchar == '\n'))
			{
				++next;
				nextchar = buffer_[next];
			}

			// Compact only interior whitespaces,
			// discard leading and trailing ones.
			if (i != 0 && next != cursize)
				result.append(' ');

			i = next;
		}
		else
		{
			result.append(curchar);
			++i;
		}
	}

	return result;
}


string string::trim_left(char c) const
{
	int64 cursize = size();

	int64 count_of_c = 0;

	while (count_of_c < cursize)
	{
		if (buffer_[count_of_c] != c) break;
		++count_of_c;
	}
	
	return substring(count_of_c);
}


string string::trim_right(char c) const
{
	int64 cursize = size();

	int64 count_of_c = 0;

	while (count_of_c < cursize)
	{
		if (buffer_[cursize - 1 - count_of_c] != c) break;
		++count_of_c;
	}

	return substring(0, cursize - count_of_c);
}


string string::enquote() const
	{ return string("\"") + (*this) + "\""; }

string string::unenquote() const
{
	if (size() < 2 ||
		buffer_[0] != '\"' ||
		buffer_[size() - 1] != '\"')
	{
		return *this;
	}

	return
		substring(1, size() - 2);
}


string string::substring(int64 offset) const
	{ return substring(offset, size() - offset); }


string string::substring(int64 offset, int64 count) const
{
	if (count < 0)
		throw invalid_parameter();


	// Trivial cases: extraction interval does not
	// intersect with the string. Variable end denotes
	// the first char that is not extracted anymore.
	if (count == 0)
		return string();

	int64 cursize = size();
	if (cursize == 0) return string();

	int64 end = offset + count;

	if (end < 0 || offset >= cursize)
		return string();


	// Now, there is at least a single character within the
	// string and in our interval, and end >= offset + 1.
	// Thus we can just cull to interval borders.
	if (offset < 0) offset = 0;
	if (offset >= cursize) offset = cursize - 1;
	if (end < 1) end = 1;
	if (end > cursize) end = cursize;

	return
		string(data() + offset, end - offset);
}


string string::left(int64 count) const
	{ return substring(0, count); }
string string::right(int64 count) const
	{ return substring(size() - count, count); }


void string::split(char chr, list<string>& result) const
{
	list<string> offhand;

	int64 nextstart = 0;
	int64 cursize = size();
	for (int64 i = 0; i < cursize; ++i)
	{
		if (data()[i] == chr)
		{
			offhand.insert
				(substring(nextstart, i - nextstart));

			nextstart = i + 1;
		}
	}

	offhand.insert
		(substring(nextstart, cursize - nextstart));

	// Exception safety: NOTHROW from here.
	swap(result, offhand);
}


string string::to_upper() const
{
	int64 cursize = size();

	string result;
	result.resize(cursize);

	for (int64 i = 0; i < cursize; ++i)
	{
		char chr = buffer_[i];

		if (chr >= 'a' && chr <= 'z')
			chr = chr - 'a' + 'A';

		result.buffer_[i] = chr;
	}

	return result;
}


string string::to_lower() const
{
	int64 cursize = size();

	string result;
	result.resize(cursize);

	for (int64 i = 0; i < cursize; ++i)
	{
		char chr = buffer_[i];

		if (chr >= 'A' && chr <= 'Z')
			chr = chr - 'A' + 'a';

		result.buffer_[i] = chr;
	}

	return result;
}


bool string::to_bool() const NOTHROW
	{ return contains("true"); }

int32 string::to_int32() const NOTHROW
	{ return strtol(data(), NULL, 10); }
uint32 string::to_uint32() const NOTHROW
	{ return strtoul(data(), NULL, 10); }

int64 string::to_int64() const NOTHROW
	{ return strtoll(data(), NULL, 10); }
uint64 string::to_uint64() const NOTHROW
	{ return strtoull(data(), NULL, 10); }

float string::to_float() const NOTHROW
	{ return (float)atof(data()); }


void string::swap_with(string& other) NOTHROW
	{ swap(buffer_, other.buffer_); }


string string::operator +(const string& other) const
{
	int64 this_size = size();
	int64 other_size = other.size();
	
	string result;
	result.resize
		(integer_operations::add_or_throw
			(this_size, other_size));

	char *resultdata = result.data();

	memory::copy_block_typed
		(resultdata, data(), this_size);
	memory::copy_block_typed
		(resultdata + this_size, other.data(), other_size);

	return result;
}


string& string::operator +=(const string& other)
{
	// Self concattenation works as well
	// since we cache the size values.
	int64 old_size = size();
	int64 other_size = other.size();

	resize
		(integer_operations::add_or_throw
			(old_size, other_size));

	memory::copy_block_typed
		(data() + old_size, other.data(), other_size);

	return *this;
}


string& string::operator =(const string& other)
{
	// Reuse existing memory if possible.
	if (this != &other)
		set(other.data(), other.size());

	return *this;
}


string& string::operator =(string&& other) NOTHROW
{
	if (&other == this) return *this;

	// Exception safety: NOTHROW.
	string offhand_move(std::move(other));
	swap(*this, offhand_move);
	return *this;
}


bool string::operator ==(const string& other) const NOTHROW
{
	if (size() != other.size()) return false;
	return memory::are_blocks_equal_typed
				(data(), other.data(), size());
}

bool string::operator ==(const char* nt_other) const NOTHROW
{
	int64 otherlen = size(nt_other);
	if (size() != otherlen) return false;
	return memory::are_blocks_equal_typed
				(data(), nt_other, size());
}


bool string::operator !=(const string& other) const NOTHROW
	{ return !operator ==(other); }
bool string::operator !=(const char* nt_other) const NOTHROW
	{ return !operator ==(nt_other); }

	
bool string::operator <(const string& other) const NOTHROW
{
	// We cannot use strcmp() here,
	// since there might be intermediate \\0 characters.
	memory::compare_result test =
		memory::compare_blocks_typed
			(data(), other.data(), min(size(), other.size()));
							 
	return (test == memory::compare_result_equal) ?
				(size() < other.size()) :
				(test == memory::compare_result_a_before_b);
}

bool string::operator <=(const string& other) const NOTHROW
{
	memory::compare_result test =
		memory::compare_blocks_typed
			(data(), other.data(), min(size(), other.size()));
							 
	return (test == memory::compare_result_equal) ?
				(size() <= other.size()) :
				(test == memory::compare_result_a_before_b);
}

bool string::operator >(const string& other) const NOTHROW
	{ return !(*this <= other); }
bool string::operator >=(const string& other) const NOTHROW
	{ return !(*this < other); }


uint64 string::hash() const NOTHROW
	{ return hash_array(data(), size()); }


void string::clear() NOTHROW
	{ buffer_.clear(); }
void string::free() NOTHROW
	{ buffer_.free(); }




} /* namespace viral_core */

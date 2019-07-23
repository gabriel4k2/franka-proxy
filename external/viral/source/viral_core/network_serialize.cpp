/**
 *************************************************************************
 *
 * @file network_serialize.cpp
 * 
 * Network serialization of basic types, implementation.
 *
 ************************************************************************/


#include "network_serialize.hpp"


#include "log.hpp"
#include "buffer.hpp"
#include "string.hpp"
#include "network_stream.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// network_serialize
//
//////////////////////////////////////////////////////////////////////////


void network_serialize::send
	(bool value, network_stream& stream)
		{ send_flat_copy(value, stream); }
void network_serialize::send
	(char value, network_stream& stream)
		{ send_flat_copy(value, stream); }
void network_serialize::send
	(int8 value, network_stream& stream)
		{ send_flat_copy(value, stream); }
void network_serialize::send
	(int32 value, network_stream& stream)
		{ send_flat_copy(value, stream); }
void network_serialize::send
	(int64 value, network_stream& stream)
		{ send_flat_copy(value, stream); }
void network_serialize::send
	(float value, network_stream& stream)
		{ send_flat_copy(value, stream); }


void network_serialize::send
	(const dynamic_buffer<unsigned char>& value,
	 network_stream& stream)
{
	network_stream_lock lock(stream);
	send(value.size(), stream);
	stream.send_nonblocking
		(value.data(), value.size());
}


void network_serialize::send
	(const string& value,
	 network_stream& stream)
{
	network_stream_lock lock(stream);
	send(value.size(), stream);
	stream.send_nonblocking
		(reinterpret_cast<const unsigned char*>(value.data()),
		 value.size());
}


bool network_serialize::try_receive
	(bool& value, network_stream& stream, bool peek)
		{ return try_receive_flat_copy(value, stream, peek); }
bool network_serialize::try_receive
	(char& value, network_stream& stream, bool peek)
		{ return try_receive_flat_copy(value, stream, peek); }
bool network_serialize::try_receive
	(int8& value, network_stream& stream, bool peek)
		{ return try_receive_flat_copy(value, stream, peek); }
bool network_serialize::try_receive
	(int32& value, network_stream& stream, bool peek)
		{ return try_receive_flat_copy(value, stream, peek); }
bool network_serialize::try_receive
	(int64& value, network_stream& stream, bool peek)
		{ return try_receive_flat_copy(value, stream, peek); }
bool network_serialize::try_receive
	(float& value, network_stream& stream, bool peek)
		{ return try_receive_flat_copy(value, stream, peek); }


bool network_serialize::try_receive
	(dynamic_buffer<unsigned char>& value,
	 network_stream& stream, bool peek)
{
	return try_receive_byte_array(value, stream, peek);
}


bool network_serialize::try_receive
	(string& value,
	 network_stream& stream, bool peek)
{
	return try_receive_byte_array(value, stream, peek);
}


template <typename T>
	void network_serialize::send_flat_copy
		(const T& value, network_stream& stream)
{
	// No need for a network_stream_lock
	// on a single call.
	stream.send_nonblocking
		(reinterpret_cast<const unsigned char*>(&value),
		 size_of<T>());
}


template <typename T>
	bool network_serialize::try_receive_flat_copy
		(T& value, network_stream& stream, bool peek)
{
	// No need for a network_stream_lock
	// on a single call.
	return
		stream.try_receive_nonblocking
			(reinterpret_cast<unsigned char*>(&value),
			 size_of<T>(), peek);
}


template <typename T>
	bool network_serialize::try_receive_byte_array
		(T& value, network_stream& stream, bool peek)
{
	network_stream_lock lock(stream);


	// Peek for size.
	int64 size = 0;
	if (!try_receive(size, stream, true))
		return false;


	// Explicitly check if all array bytes have already arrived.
	// This enables us to avoid an allocation on too little data.
	int64 total_bytes =
		integer_operations::add_or_throw(size_of<int64>(), size);

	if (stream.buffer_max_size_receive < total_bytes)
	{
		LOG_ERROR("Buffer cannot completely hold requested data.");
		throw network_exception();
	}

	if (stream.pending_receive_bytes() < total_bytes)
		return false;


	// Allocate data and receive byte array.
	// Then, explicitly remove size and data from stream.
	T offhand;
	offhand.resize(size);
	
	if (!stream.try_peek_nonblocking
			(reinterpret_cast<unsigned char*>(offhand.data()),
			 offhand.size(), size_of<int64>()))
	{
		LOG_ERROR("Peeking for pending bytes failed.");
		throw assert_failed();
	}

	if (!peek)
		stream.discard_receive(total_bytes);


	// Strong exception safety.
	// NOTHROW from here.
	swap(offhand, value);
	return true;
}




} /* namespace viral_core */

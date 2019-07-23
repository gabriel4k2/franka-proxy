/**
 *************************************************************************
 *
 * @file network_transfer.cpp
 *
 * One-shot network transfers, implementation.
 *
 ************************************************************************/


#include "network_transfer.hpp"


#include "log.hpp"
#include "buffer.hpp"
#include "timer.hpp"
#include "thread_util.hpp"
#include "network.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// network_buffer_progress
//
//////////////////////////////////////////////////////////////////////////


network_buffer_progress::network_buffer_progress
	(const network_buffer& buffer) NOTHROW
	:
	next_byte_offset_(0),
	bytes_remaining_(buffer.size())
{ }


int64 network_buffer_progress::next_byte_offset() const NOTHROW
	{ return next_byte_offset_; }
int64 network_buffer_progress::bytes_remaining() const NOTHROW
	{ return bytes_remaining_; }


bool network_buffer_progress::finished() const NOTHROW
	{ return bytes_remaining_ == 0; }




//////////////////////////////////////////////////////////////////////////
//
// network_transfer
//
//////////////////////////////////////////////////////////////////////////


void network_transfer::send_blocking
	(network_connection& connection,
	 const network_buffer& buffer)
{
	send_blocking
		(connection, buffer,
		 true, default_timeout_seconds,
		 true, default_sleep_seconds);
}


void network_transfer::receive_blocking
	(network_connection& connection,
	 network_buffer& buffer, int64 size)
{
	receive_blocking
		(connection, buffer, size,
		 true, default_timeout_seconds,
		 true, default_sleep_seconds);
}


void network_transfer::send_blocking
	(network_connection& connection,
	 const network_buffer& buffer,
	 bool enable_timeout, float timeout_seconds,
	 bool enable_sleep, float sleep_seconds)
{
	// Check preconditions.
	if (buffer.size() <= 0)
	{
		LOG_ERROR("Cannot send a buffer with zero or less bytes.");
		throw invalid_parameter();
	}


	// Send data over network.
	// We may need multiple non-blocking send calls.
	const unsigned char* next_data = buffer.data();
	int64 bytes_remaining = buffer.size();

	free_timer timer;

	while (true)
	{
		int64 bytes_sent =
			connection.send_nonblocking
				(next_data, bytes_remaining);

		bytes_remaining -= bytes_sent;
		next_data += bytes_sent;


		// Stop sending on successful transfer.
		// In particular, do not sleep or check timeout.
		if (bytes_remaining == 0)
			break;


		// Handle timeout and sleep in correct order:
		// Sleeping time must not distort the timeout interval.
		if (enable_timeout)
		{
			if (bytes_sent > 0)
			{
				timer.restart();
			}
			else if (timer.seconds_passed() > timeout_seconds)
			{
				LOG_ERROR("Transfer exceeded timeout: " + timeout_seconds);
				throw network_exception();
			}
		}

		if (enable_sleep)
			thread_util::sleep_seconds(timeout_seconds);
	}
}


void network_transfer::receive_blocking
	(network_connection& connection,
	 network_buffer& buffer, int64 size,
	 bool enable_timeout, float timeout_seconds,
	 bool enable_sleep, float sleep_seconds)
{
	// Clear buffer on any exception.
	SCOPE_GUARD
		(buffer_guard, (buffer, &network_buffer::clear));


	// Check preconditions.
	if (size <= 0)
	{
		LOG_ERROR("Cannot fetch a buffer with zero or less bytes.");
		throw invalid_parameter();
	}


	// Make sure that there is enough space for all data.
	buffer.resize(size);


	// Fetch pending data until buffer has been filled.
	// We may need multiple non-blocking receive calls.
	unsigned char* next_data = buffer.data();
	int64 bytes_remaining = size;

	free_timer timer;

	while (true)
	{
		int64 bytes_received =
			connection.receive_nonblocking
				(next_data, bytes_remaining);
		
		bytes_remaining -= bytes_received;
		next_data += bytes_received;


		// Stop receiving on successful transfer.
		// In particular, do not sleep or check timeout.
		if (bytes_remaining == 0)
			break;


		// Handle timeout and sleep in correct order:
		// Sleeping time must not distort the timeout interval.
		if (enable_timeout)
		{
			if (bytes_received > 0)
			{
				timer.restart();
			}
			else if (timer.seconds_passed() > timeout_seconds)
			{
				LOG_ERROR("Transfer exceeded timeout: " + timeout_seconds);
				throw network_exception();
			}
		}

		if (enable_sleep)
			thread_util::sleep_seconds(timeout_seconds);
	}


	// Success.
	buffer_guard.dismiss();
}


void network_transfer::send_partial_nonblocking
	(network_connection& connection,
	 const network_buffer& buffer,
	 network_buffer_progress& progress)
{
	// Check preconditions.
	// This also ensures that the network_buffer
	// has enough bytes to send over the connection.
	if (progress.bytes_remaining_ < 0 ||
		progress.next_byte_offset_ < 0)
	{
		LOG_ERROR("Broken progress counter.");
		throw invalid_parameter();
	}

	if (progress.bytes_remaining_ + progress.next_byte_offset_ != buffer.size())
	{
		LOG_ERROR("Buffer progress does not match buffer size.");
		throw invalid_state();
	}

	if (progress.bytes_remaining_ == 0)
	{
		LOG_ERROR("Buffer transfer has already been completed.");
		throw invalid_state();
	}


	// Send data over connection
	// and update progress.
	const unsigned char* next_data =
		buffer.data() + progress.next_byte_offset_;

	int64 bytes_sent =
		connection.send_nonblocking
			(next_data, progress.bytes_remaining_);

	progress.next_byte_offset_ += bytes_sent;
	progress.bytes_remaining_ -= bytes_sent;
}


void network_transfer::receive_partial_nonblocking
	(network_connection& connection,
	 network_buffer& buffer,
	 network_buffer_progress& progress)
{
	// Check preconditions.
	// This also ensures that the network_buffer
	// has enough storage for any pending data.
	if (progress.bytes_remaining_ < 0 ||
		progress.next_byte_offset_ < 0)
	{
		LOG_ERROR("Broken progress counter.");
		throw invalid_parameter();
	}

	if (progress.bytes_remaining_ + progress.next_byte_offset_ != buffer.size())
	{
		LOG_ERROR("Buffer progress does not match buffer size.");
		throw invalid_state();
	}

	if (progress.bytes_remaining_ == 0)
	{
		LOG_ERROR("Buffer transfer has already been completed.");
		throw invalid_state();
	}


	// Fetch data from connection
	// and update progress.
	unsigned char* next_data =
		buffer.data() + progress.next_byte_offset_;

	int64 bytes_received =
		connection.receive_nonblocking
			(next_data, progress.bytes_remaining_);

	progress.next_byte_offset_ += bytes_received;
	progress.bytes_remaining_ -= bytes_received;
}


const float network_transfer::default_timeout_seconds = 0.5f;
const float network_transfer::default_sleep_seconds = 0.005f;




} /* namespace viral_core */

/**
 *************************************************************************
 *
 * @file network_serialize.hpp
 * 
 * Network serialization of basic types.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__NETWORK_SERIALIZE_HPP)
#define INCLUDED__VIRAL_CORE__NETWORK_SERIALIZE_HPP


#include "build_config.hpp"


#include "container_forward.hpp"
#include "string_forward.hpp"
#include "network_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class network_serialize
 * @ingroup network_module
 *
 * Static utility for serializing and deserializing on network_stream%s.
 *
 * Thread safety:
 * - The network_serialize utility is threadsafe for arbitrary threads.
 *		Notably, both send() and try_receive() calls ensure consistency
 *		with network_stream_lock and are threadsafe on a single stream.
 *
 ************************************************************************/
class network_serialize :
	private non_constructable
{

	public:

		/**
		 * Serialize data into a network @bref{stream}.
		 *
		 * Sending throws on failure (e.g. due to network_stream exceptions).
		 * In this case, the network stream may already hold partial data.
		 */
		//@{
		static void send
			(bool value, network_stream& stream);
		static void send
			(char value, network_stream& stream);
		static void send
			(int8 value, network_stream& stream);
		static void send
			(int32 value, network_stream& stream);
		static void send
			(int64 value, network_stream& stream);
		static void send
			(float value, network_stream& stream);

		static void send
			(const dynamic_buffer<unsigned char>& value,
			 network_stream& stream);
		static void send
			(const string& value,
			 network_stream& stream);
		//@}


		/**
		 * Deserialize data from a network @bref{stream}.
		 *
		 * Returns @true on success and, if @bref{peek} is @false,
		 * removes deserialized data from the network stream buffer.
		 * If not enough data is available, receiving returns @false,
		 * and the network stream remains unchanged.
		 *
		 * Receiving throws on failure (e.g. due to network_stream exceptions).
		 * In this case, output data remains unchanged, but the stream buffer
		 * may already have (partially) been drained.
		 */
		//@{
		static bool try_receive
			(bool& value, network_stream& stream, bool peek);
		static bool try_receive
			(char& value, network_stream& stream, bool peek);
		static bool try_receive
			(int8& value, network_stream& stream, bool peek);
		static bool try_receive
			(int32& value, network_stream& stream, bool peek);
		static bool try_receive
			(int64& value, network_stream& stream, bool peek);
		static bool try_receive
			(float& value, network_stream& stream, bool peek);

		static bool try_receive
			(dynamic_buffer<unsigned char>& value,
			 network_stream& stream, bool peek);
		static bool try_receive
			(string& value,
			 network_stream& stream, bool peek);
		//@}


	private:

		template <typename T>
			static void send_flat_copy
				(const T& value, network_stream& stream);

		template <typename T>
			static bool try_receive_flat_copy
				(T& value, network_stream& stream, bool peek);

		template <typename T>
			static bool try_receive_byte_array
				(T& value, network_stream& stream, bool peek);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__NETWORK_SERIALIZE_HPP) */

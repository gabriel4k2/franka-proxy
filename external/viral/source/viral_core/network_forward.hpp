/**
 *************************************************************************
 *
 * @file network_forward.hpp
 *
 * Forward declarations for the @ref network_module "Network Module".
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__NETWORK_FORWARD_HPP)
#define INCLUDED__VIRAL_CORE__NETWORK_FORWARD_HPP


#include "build_config.hpp"


#include "container_forward.hpp"


namespace viral_core
{


class network_context;
class network_connection;
class network_server;

class network_host;

class null_network_context;

class ms_network_context;
class ms_network_connection;
class ms_network_server;

class posix_network_context;
class posix_network_connection;
class posix_network_server;

typedef dynamic_buffer<unsigned char> network_buffer;
class network_buffer_progress;
class network_transfer;

class network_stream;
class network_task;
class network_serialize;




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__NETWORK_FORWARD_HPP) */

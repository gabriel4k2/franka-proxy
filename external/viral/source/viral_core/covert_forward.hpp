/**
 *************************************************************************
 *
 * @file covert_forward.hpp
 *
 * Forward declarations for the @ref covert_module "Covert Module".
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__COVERT_FORWARD_HPP)
#define INCLUDED__VIRAL_CORE__COVERT_FORWARD_HPP


#include "build_config.hpp"


#include "container_forward.hpp"
#include "string_forward.hpp"


namespace viral_core
{


class covert_object_manager;
class covert_object;
class covert_id;
class covert_data;

class covert_command;
class covert_command_commit;
class covert_command_query;
class covert_command_purge;
class covert_command_queue;

class covert_query_result;

class covert_loader;

typedef owning_cache<covert_data> covert_data_cache;
typedef shared_cache<covert_id> covert_id_cache;


} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__COVERT_FORWARD_HPP) */
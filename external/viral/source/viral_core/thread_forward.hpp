/**
 *************************************************************************
 *
 * @file thread_forward.hpp
 *
 * Forward declarations for the @ref thread_module "Thread Module".
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__THREAD_FORWARD_HPP)
#define INCLUDED__VIRAL_CORE__THREAD_FORWARD_HPP


#include "build_config.hpp"


#include "container_forward.hpp"


namespace viral_core
{


// Generic threading.
class thread_id;
class thread_util;

class mutex;
class signal;

template <typename T> class atomic;
typedef atomic<bool> atomic_bool;
typedef atomic<int32> atomic_int32;
typedef atomic<int64> atomic_int64;
typedef atomic<float> atomic_float;

class threaded_task;


// Multi-locks.
class mulock_id;
class mulock_data;

class mulock_request;
typedef list<mulock_request> mulock_request_list;

class mulock_access_util;
template <typename T> class mulock_access;
template <typename T> class mulock_const_access;

class mulock;
class mulock_context;




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__THREAD_FORWARD_HPP) */
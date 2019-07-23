/**
 *************************************************************************
 *
 * @file physics.hpp
 *
 * Physics simulation header.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__PHYSICS_HPP)
#define INCLUDED__VIRAL_CORE__PHYSICS_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "string.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class physics_context
 * @ingroup physics_module
 *
 * A unique physics simulation.
 *
 ************************************************************************/
class physics_context :
	private non_copyable
{
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__PHYSICS_HPP) */
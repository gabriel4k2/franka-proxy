/**
 *************************************************************************
 *
 * @file cameron_gjk_bind.hpp
 *
 * Binding for Cameron's GJK variant.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__CAMERON_GJK_BIND_HPP)
#define INCLUDED__VIRAL_CORE__CAMERON_GJK_BIND_HPP


#include "build_config.hpp"


#if defined(VIRAL__HAVE_CAMERON_GJK)


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class cameron_gjk_bind
 * @ingroup geo_module
 *
 * Static wrappers around Cameron's GJK variant.
 *
 ************************************************************************/
class cameron_gjk_bind :
	private non_constructable
{
};




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_CAMERON_GJK) */


#endif /* !defined(INCLUDED__VIRAL_CORE__CAMERON_GJK_BIND_HPP) */



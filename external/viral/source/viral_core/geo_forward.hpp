/**
 *************************************************************************
 *
 * @file geo_forward.hpp
 *
 * Forward declarations for the @ref geo_module "Geometry Module".
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GEO_FORWARD_HPP)
#define INCLUDED__VIRAL_CORE__GEO_FORWARD_HPP


#include "build_config.hpp"


namespace viral_core
{


// Fundamentals.
class geo_constants;
class geo_util;
class geo_interpolate;
class grid_math;


// 2D geometry.
class vector2i;
class rect2i;

class vector2f;
class rect2f;


// 3D geometry.
class vector;
class rotation;
class matrix;
class si_transform;

class line;
class segment;
class plane;
class triangle;

class sphere;
class box;

class frustum;
class camera;


// Distance and intersection.
class geo_intersect;
class geo_raycast;

class geo_distance_line;
class geo_distance_segment;
class geo_distance_triangle;
class geo_distance_box_face;
class geo_distance;




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GEO_FORWARD_HPP) */
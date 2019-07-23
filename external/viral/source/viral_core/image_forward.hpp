/**
 *************************************************************************
 *
 * @file image_forward.hpp
 *
 * Forward declarations for the @ref image_module "Image Module".
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__IMAGE_FORWARD_HPP)
#define INCLUDED__VIRAL_CORE__IMAGE_FORWARD_HPP


#include "build_config.hpp"


namespace viral_core
{


// Basic classes.
class color;
class color_util;


// Image semantics.
class image_semantics_binary;
class image_semantics_greyscale;
class image_semantics_rgba;
class image_semantics_hsva;
class image_semantics_depth;


// Standalone images.
template <typename S> class image_raw;
class image_raw_errors;

typedef image_raw<image_semantics_binary> image_binary;
typedef image_raw<image_semantics_greyscale> image_greyscale;
typedef image_raw<image_semantics_rgba> image_rgba;
typedef image_raw<image_semantics_hsva> image_hsva;
typedef image_raw<image_semantics_depth> image_depth;


// Image hierarchy adapters.
class image_any;

template <typename S> class image_any_raw;
typedef image_any_raw<image_semantics_binary> image_any_binary;
typedef image_any_raw<image_semantics_greyscale> image_any_greyscale;
typedef image_any_raw<image_semantics_rgba> image_any_rgba;
typedef image_any_raw<image_semantics_hsva> image_any_hsva;
typedef image_any_raw<image_semantics_depth> image_any_depth;


// Image utilities.
class image_file;

class image_resize;
class image_resize_sampling_window;


// Fonts.
class font_letter;
class font_kerning_pair;
class font;




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__IMAGE_FORWARD_HPP) */
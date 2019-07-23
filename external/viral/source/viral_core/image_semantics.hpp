/**
 *************************************************************************
 *
 * @file image_semantics.hpp
 *
 * Semantics for images.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__IMAGE_SEMANTICS_HPP)
#define INCLUDED__VIRAL_CORE__IMAGE_SEMANTICS_HPP


#include "build_config.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class image_semantics_binary
 * @ingroup image_module
 *
 * Semantics for binary images with one boolean per pixel.
 *
 ************************************************************************/
class image_semantics_binary :
	private non_constructable
{

	public:

		struct pixel_type
		{
			union
			{
				struct { bool b; };
				bool c[1];
			};
		};

		typedef unsigned char component_type;

		static const int64 bytes_per_pixel = sizeof(pixel_type);
		static const int64 bytes_per_component = sizeof(bool);
		static const int64 components_per_pixel = 1;
};




/**
 *************************************************************************
 *
 * @class image_semantics_greyscale
 * @ingroup image_module
 *
 * Semantics for greyscale images with one unsigned char per pixel.
 *
 ************************************************************************/
class image_semantics_greyscale :
	private non_constructable
{

	public:

		struct pixel_type
		{
			union
			{
				struct { unsigned char v; };
				unsigned char c[1];
			};
		};

		typedef unsigned char component_type;

		static const int64 bytes_per_pixel = sizeof(pixel_type);
		static const int64 bytes_per_component = sizeof(unsigned char);
		static const int64 components_per_pixel = 1;
};




/**
 *************************************************************************
 *
 * @class image_semantics_rgba
 * @ingroup image_module
 *
 * Semantics for RGBA images with four unsigned char components per pixel.
 *
 ************************************************************************/
class image_semantics_rgba :
	private non_constructable
{

	public:

		struct pixel_type
		{
			union
			{
				struct
				{
					unsigned char r;
					unsigned char g;
					unsigned char b;
					unsigned char a;
				};
	
				unsigned char c[4];
			};
		};

		typedef unsigned char component_type;

		static const int64 bytes_per_pixel = sizeof(pixel_type);
		static const int64 bytes_per_component = sizeof(unsigned char);
		static const int64 components_per_pixel = 4;
};




/**
 *************************************************************************
 *
 * @class image_semantics_hsva
 * @ingroup image_module
 *
 * Semantics for HSVA images with four unsigned char components per pixel.
 *
 ************************************************************************/
class image_semantics_hsva :
	private non_constructable
{

	public:

		struct pixel_type
		{
			union
			{
				struct
				{
					unsigned char h;
					unsigned char s;
					unsigned char v;
					unsigned char a;
				};
	
				unsigned char c[4];
			};
		};

		typedef unsigned char component_type;

		static const int64 bytes_per_pixel = sizeof(pixel_type);
		static const int64 bytes_per_component = sizeof(unsigned char);
		static const int64 components_per_pixel = 4;
};




/**
 *************************************************************************
 *
 * @class image_semantics_depth
 * @ingroup image_module
 *
 * Semantics for depth images with one float component per pixel.
 *
 ************************************************************************/
class image_semantics_depth :
	private non_constructable
{

	public:

		struct pixel_type
		{
			union
			{
				struct { float d; };
				float c[1];
			};
		};

		typedef float component_type;

		static const int64 bytes_per_pixel = sizeof(pixel_type);
		static const int64 bytes_per_component = sizeof(float);
		static const int64 components_per_pixel = 1;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__IMAGE_SEMANTICS_HPP) */

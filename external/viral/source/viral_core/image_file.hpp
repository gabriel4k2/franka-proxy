/**
 *************************************************************************
 *
 * @file image_file.hpp
 *
 * File operations for images.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__IMAGE_FILE_HPP)
#define INCLUDED__VIRAL_CORE__IMAGE_FILE_HPP


#include "build_config.hpp"


#include "string_forward.hpp"
#include "file_forward.hpp"
#include "image_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class image_file
 * @ingroup image_module
 *
 * Static utilities for saving and loading of images.
 *
 ************************************************************************/
class image_file :
	private non_constructable
{

	public:

		/**
		 * Save uncompressed RGBA image to various sources.
		 *
		 * If no explicit file format is specified, the file format
		 * is determined by the extension on the output file.
		 */
		//@{
		static void save
			(const image_rgba& i,
			 const string& path, file_context& files);

		static void save_tga
			(const image_rgba& i, file& output);
		static void save_jpg
			(const image_rgba& i, file& output, int8 quality);
		static void save_png
			(const image_rgba& i, file& output);
		//@}


		/**
		 * Load uncompressed RGBA image from various sources.
		 *
		 * If no explicit file format is specified, the file format
		 * is determined by the extension on the input file.
		 */
		//@{
		static image_rgba load
			(const string& path, file_context& files);

		static image_rgba load_tga(file& input);
		static image_rgba load_jpg(file& input);
		static image_rgba load_png(file& input);
		//@}


		/** Default quality for JPG export. */
		static const uint8 default_jpg_quality = 100;


	private:

		static const unsigned char tga_header_[12];
		static const int64 tga_max_image_size_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__IMAGE_FILE_HPP) */

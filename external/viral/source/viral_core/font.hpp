/**
 *************************************************************************
 *
 * @file font.hpp
 *
 * Header for font definitions.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__FONT_HPP)
#define INCLUDED__VIRAL_CORE__FONT_HPP


#include "build_config.hpp"


#include "file_forward.hpp"
#include "geo_forward.hpp"
#include "mesh_forward.hpp"


#include "tree_map.hpp"
#include "geo_2d.hpp"
#include "string.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup font_module Font Module
 * @ingroup image_module
 *
 * Contains font and typesetting facilities.
 *
 ************************************************************************/


/**
 *************************************************************************
 *
 * @class font_letter
 * @ingroup font_module
 *
 * Defines a single letter within a bitmap @ref font.
 *
 ************************************************************************/
class font_letter
{

	public:

		font_letter
			(const rect2f& extents,
			 const rect2f& texcoords,
			 float increment) NOTHROW
			:
			extents(extents),
			texcoords(texcoords),
			increment(increment)
		{ }


		/**
		 * Typesetting position and size of the letter,
		 * relative to the current baseline position.
		 */
		const rect2f extents;


		/**
		 * Associated texture coordinates within a texture
		 * of dimensions [(0, 0), (1, 1)].
		 */
		const rect2f texcoords;


		/**
		 * Horizontal distance to the next letter.
		 * This might differ from @bref{extents.size_x},
		 * e.g. for overhang or additional whitespace.
		 */
		const float increment;
};




/**
 *************************************************************************
 *
 * @class font_kerning_pair
 * @ingroup font_module
 *
 * Groups two letters of a @ref font for unique kerning.
 *
 ************************************************************************/
class font_kerning_pair
{
	public:

		font_kerning_pair(int32 left, int32 right) NOTHROW
			: left(left), right(right) { }
	
		bool operator <(const font_kerning_pair& other) const NOTHROW
		{
			return
				left < other.left ||
				(left == other.left && right < other.right);
		}

		const int32 left;
		const int32 right;
};




/**
 *************************************************************************
 *
 * @class font
 * @ingroup font_module
 *
 * Defines a bitmap font for some character set.
 *
 ************************************************************************/
class font
{

	public:

		typedef tree_map<int32, font_letter> letter_map;
		typedef tree_map<font_kerning_pair, float> kerning_pair_map;


		/** Create font from given properties. */
		font(const string& name,
			 const letter_map& letters,
			 const kerning_pair_map& kerning_pairs,
			 float line_height,
			 float base_distance,
			 int32 fallback_letter)
			:
			name(name),
			letters(letters),
			kerning_pairs(kerning_pairs),
			line_height(line_height),
			base_distance(base_distance),
			fallback_letter(fallback_letter)
		{ }


		/**
		 * Fetch XML font data as exported by Angelcode's free BMFont
		 * (http://www.angelcode.com/products/bmfont).
		 */
		//@{
		static font load(const xml_node& root);
		static font load(file& input);
		static font load(const string& path, file_context& files);
		//@}


		/**
		 * Create a @ref mesh with suitable geometry
		 * (vertices, triangles, tex coords, normals)
		 * for rendering some @bref{text} in this font.
		 *
		 * The upper-left origin of the generated text is at (0, 0, 0).
		 * This places the text baseline at (@bref{base_distance}, 0, 0).
		 * The text extends into +x and +y. All text triangles face into
		 * -z direction, and are visible for a default @ref camera.
		 *
		 * Typesetting accepts both Unix (\\n) and Windows (\\r\\n)
		 * line breaks (see @bref{line_height}).
		 *
		 * Clients can provide an optional @bref{clip}ping rectangle
		 * on the XY plane that applies to the resulting geometry.
		 *
		 * @attent The resulting mesh has a tight-fit bounding box
		 * and is not suited for consistent line placement. Instead,
		 * use calculate_line_integral_extents() for layouting.
		 */
		//@{
		mesh typeset_mesh
			(const string& text) const;
		mesh typeset_mesh
			(const string& text, const rect2f& clip) const;
		//@}


		/**
		 * Create a model that contains a typeset_mesh()
		 * in a model_group of given @bref{group_name}.
		 */
		//@{
		model typeset_model
			(const string& text, const string& group_name) const;
		model typeset_model
			(const string& text, const string& group_name,
			 const rect2f& clip) const;
		//@}


		/**
		 * Find line-integral extents for given text.
		 *
		 * Unlike the bounding box of a typeset_mesh() or typeset_model(),
		 * this call considers maximally-sized integral lines for height,
		 * not actual letter extents (e.g. 'g' vs 'C').
		 *
		 * @attent This calculation is expensive, and should be cached
		 * to avoid the overhead of frequent invocation.
		 *
		 * @attent Clients must be careful not to forego @ref NOTHROW
		 * by implicitly creating @bref{text} (e.g. from const char*).
		 */
		vector2f calculate_line_integral_extents
			(const string& text) const NOTHROW;


		/**
		 * Find (potentially clipped) @bref{extents} of the character
		 * at @bref{index} within @bref{text} after typesetting.
		 * Returns @false on entirely clipped chars, invalid indices,
		 * or invalid characters, and then does not set @bref{extents}.
		 *
		 * @attent Clients must be careful not to forego @ref NOTHROW
		 * by implicitly creating @bref{text} (e.g. from const char*).
		 */
		bool find_letter_extents
			(const string& text, int64 index,
			 const rect2f& clip, bool enable_clip,
			 rect2f& extents) const NOTHROW;


		/**
		 * Find next-best letter index within @bref{text}
		 * that matches some typesetting @bref{position}.
		 *
		 * Usage notes:
		 * - Resulting indices are in the range [0, text.size()],
		 *		and may point to the terminating \0, to newlines,
		 *		or to other non-printing characters.
		 * - Seaching for the next-best character first rounds
		 *		to nearest line, then to nearest letter index.
		 * - Clipping text is not supported, but clients should
		 *		clip @bref{position}s instead.
		 *
		 * @attent Clients must be careful not to forego @ref NOTHROW
		 * by implicitly creating @bref{text} (e.g. from const char*).
		 */
		int64 find_letter_index
			(const string& text, const vector2f& position) const NOTHROW;


		/**
		 * Internal (non-unique) name of the font.
		 * Relevant mainly for debugging.
		 */
		const string name;


		/**
		 * Map from letter indices (ASCII chars,
		 * UTF-8 chars) to letter properties.
		 */
		const letter_map letters;


		/**
		 * Map from letter combinations to additional
		 * baseline increments, e.g. for ligatures.
		 */
		const kerning_pair_map kerning_pairs;


		/**
		 * Vertical distance in-between consecutive baselines.
		 */
		const float line_height;


		/**
		 * Vertical distance from the upper bounding
		 * of a line to the letter baseline.
		 */
		const float base_distance; 


		/**
		 * Index of the fallback letter used by typeset_mesh()
		 * in case of unknown letter indices.
		 */
		const int32 fallback_letter;


	private:

		mesh typeset_mesh_helper
			(const string& text,
			 const rect2f& clip, bool enable_clip) const;	

		model typeset_model_helper
			(const string& text, const string& group_name,
			 const rect2f& clip, bool enable_clip) const;


		enum typeset_result
		{
			typeset_ignore, typeset_line_break,
			typeset_clip_entirely, typeset_success
		};			  

		typeset_result typeset_letter
			(const string& text, int64 index, vector2f& base,
			 const rect2f& clip, bool enable_clip,
			 rect2f& extents, rect2f& texcoords) const NOTHROW;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__FONT_HPP) */

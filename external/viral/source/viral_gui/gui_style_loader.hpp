/**
 *************************************************************************
 *
 * @file gui_style_loader.hpp
 *
 * Loading GUI styles.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_STYLE_LOADER_HPP)
#define INCLUDED__VIRAL_GUI__GUI_STYLE_LOADER_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/file_forward.hpp>
#include <viral_core/render_forward.hpp>


#include <viral_core/string.hpp>
#include <viral_core/tree_map.hpp>


#include "gui_forward.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_style_loader
 * @ingroup gui_module
 *
 * Loads assorted gui_style%s from input XML.
 *
 * Basic XML layout:
 * - Example (reduced for simplicity):
 *		@verbatim
 *			<gui_styles>
 *				<large_font type="font_style"
 *							material="large_font_material"
 *							f="fonts/large.bmfont" />
 *				<window_frame_style type="frame_style">
 *					<margin_00 x="16" y="32" />
 *					<margin_22 x="16" y="16" />
 *					<border_00 x="4" y="4" />
 *					<border_22 x="4" y="4" />
 *					<padding_00 x="4" y="4" />
 *					<padding_22 x="4" y="4" />
 *					material="window_active_material" ... />
 *				<default_window_style type="window_style"
 *							  default_style="window_frame_style" ... />
 *				<important_window_style type="window_style"
 *							  inherit="default_window_style"
 *							  caption_label_style="extra_large_label_style" />
 *				<!-- ... -->
 *			</gui_styles>
 *		@endverbatim
 *
 * XML overview:
 * - Each first-level child of the XML root node creates a new gui_style.
 *		The node tag sets a unique name for the style in @bref{style_cache}.
 *		The @bref{type} attribute indicates the type of the style.
 * - Further XML attributes and subordinate XML nodes set style members.
 *		Styles optionally @bref{inherit} unset members from other styles.
 *		Specification of all supported XML exceeds this documentation.
 *		See style loader and xml_converter implementations for details.
 *
 * Usage notes:
 * - Both client-side and style-internal references resolve through
 *		style names. In particular, clients receive results within
 *		the @bref{style_cache} and may prefill the style cache with
 *		custom styles or with several load() calls.
 * - GUI style XML cannot create shaders or materials on its own. 
 *		Instead, clients must provide both of these externally
 *		through @bref{material_cache} and @bref{shader_cache}.
 *		See create_materials() for automating this process.
 * - Loading throws on duplicate names, unresolved references,
 *		or unresolved material IDs. On these and any other errors,
 *		loading may already have modified the @bref{style_cache}.
 *
 ************************************************************************/
class gui_style_loader
{

	public:

		/**
		 * Create a GUI style loader that fills
		 * the specified @bref{style_cache}.
		 *
		 * The loader fetches dependency files through
		 * the @bref{files} context and resolves shaders
		 * and materials against provided caches.
		 *
		 * All bound object references must remain valid
		 * for the entire loader lifetime.
		 */
		gui_style_loader
			(const string& loader_name,
			 file_context& files,		
			 gui_shader_cache& shader_cache,	 
			 gui_material_cache& material_cache,
			 gui_style_cache& style_cache);


		/**
		 * Load named styles from various XML sources
		 * and place results into @bref{style_cache}.
		 * See gui_style_loader class documentation.
		 */
		//@{
		void load(const xml_node& root);
		void load(file& source_file);
		void load(const string& path);
		//@}


		/**
		 * Create textures and materials for all image files
		 * with @bref{extension} in a virtual_path @bref{folder}.
		 * Add results to the bound @bref{material_cache}
		 * and to a rendering command @bref{queue}.
		 *
		 * Both @bref{texture_template} and @bref{material_template}
		 * determine initial data, with the exception of texture
		 * images and per-material texture IDs (which are replaced
		 * by generated content).
		 *
		 * Usage notes:
		 * - File search is not recursive.
		 * - Material cache names are file names without extension
		 *		or folder. The call throws on duplicate names.
		 * - On any exception, the call may have modified
		 *		both the material cache and the command queue.
		 */
		void create_materials
			(const string& folder,
			 const string& extension,
			 const render_texture_2d_data& texture_template,
			 const render_material_data& material_template,
			 render_command_queue& queue);


		const string loader_name;
		file_context& files;
		gui_shader_cache& shader_cache;
		gui_material_cache& material_cache;
		gui_style_cache& style_cache;


	private:

		shared_instance<gui_style> to_cursor_style(const xml_node& node);
		shared_instance<gui_style> to_font_style(const xml_node& node);

		shared_instance<gui_style> to_spacer_style(const xml_node& node);
		shared_instance<gui_style> to_anchor_style(const xml_node& node);
		shared_instance<gui_style> to_grid_cell_style(const xml_node& node);

		shared_instance<gui_style> to_label_style(const xml_node& node);
		shared_instance<gui_style> to_image_style(const xml_node& node);
		shared_instance<gui_style> to_frame_style(const xml_node& node);

		shared_instance<gui_style> to_button_style(const xml_node& node);
		shared_instance<gui_style> to_tristate_style(const xml_node& node);
		shared_instance<gui_style> to_knob_style(const xml_node& node);

		shared_instance<gui_style> to_slider_style(const xml_node& node);
		shared_instance<gui_style> to_value_edit_style(const xml_node& node);
		shared_instance<gui_style> to_progress_bar_style(const xml_node& node);
		shared_instance<gui_style> to_image_map_style(const xml_node& node);

		shared_instance<gui_style> to_split_style(const xml_node& node);
		shared_instance<gui_style> to_scrollbar_style(const xml_node& node);
		shared_instance<gui_style> to_scroll_area_style(const xml_node& node);

		shared_instance<gui_style> to_list_style(const xml_node& node);
		shared_instance<gui_style> to_listbox_style(const xml_node& node);
		shared_instance<gui_style> to_dropdown_style(const xml_node& node);
		shared_instance<gui_style> to_tabview_style(const xml_node& node);

		shared_instance<gui_style> to_edit_style(const xml_node& node);
		shared_instance<gui_style> to_editbox_style(const xml_node& node);

		shared_instance<gui_style> to_window_style(const xml_node& node);

		shared_instance<gui_style> to_tooltip_style(const xml_node& node);
		shared_instance<gui_style> to_tooltip_area_style(const xml_node& node);

		shared_instance<gui_style> to_msgbox_style(const xml_node& node);
		shared_instance<gui_style> to_progress_box_style(const xml_node& node);
		shared_instance<gui_style> to_string_dialog_style(const xml_node& node);
		shared_instance<gui_style> to_file_dialog_style(const xml_node& node);
		shared_instance<gui_style> to_color_dialog_style(const xml_node& node);


		typedef shared_instance<gui_style>
					(gui_style_loader::*style_handler)(const xml_node& node);

		typedef tree_map<string, style_handler> style_handler_map;
		style_handler_map style_handlers_;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_STYLE_LOADER_HPP) */
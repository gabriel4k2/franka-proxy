/**
 *************************************************************************
 *
 * @file gui_element_loader.hpp
 *
 * Loading of GUI elements from XML.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_ELEMENT_LOADER_HPP)
#define INCLUDED__VIRAL_GUI__GUI_ELEMENT_LOADER_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/file_forward.hpp>
#include <viral_core/render_forward.hpp>


#include <viral_core/util.hpp>
#include <viral_core/string.hpp>
#include <viral_core/cache.hpp>


#include "gui_forward.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_element_loader
 * @ingroup gui_module
 *
 * Creates gui_element%s from some XML source.
 *
 * Basic XML layout:
 * - Example (reduced for simplicity):
 *		@verbatim
 *			<gui_elements>
 *				<main_button type="button"
 *					button_style="some_button_style"
 *					behaviour="click_button" />
 *				<main_window type="window"
 *					window_style="some_other_window_style"
 *					caption="Main Tools"
 *					resize_policy="match_content_size"
 *					content="main_button">
 *					<position x="40" y="40" />
 *					<client_size x="100" y="100" />
 *				</main_window>
 *				<window_area type="window_area">
 *					<windows>
 *						<window ref="empty_window" />
 *						<window ref="main_window" />
 *					</windows>
 *				</window_area>
 *				<!-- ... -->
 *			</gui_elements>
 *		@endverbatim
 *
 * XML overview:
 * - Each first-level child of the XML root specifies a gui_element.
 *		The mandatory @bref{type} attribute determines the type
 *		of gui_element. The node tag gives a name for the element
 *		within the @bref{element_cache}. Node contents determine
 *		members of the element.
 * - Full specifications for all available gui_element types and
 *		their XML form exceed the scope of this documentation.
 *		Consult xml_converter and gui_element_loader implementations.
 *
 * Usage notes:
 * - The @bref{element_cache} resolves all references to GUI elements.
 *		Clients use the cache to find elements for GUI logics
 *		and loading uses the cache to link interacting elements.
 * - Clients may prefill the @bref{element_cache} with code-generated
 *		GUI elements in order to link these against file elements.
 * - Loading throws on duplicate names or unresolved references.
 *		On these and any other errors, loading may already have
 *		modified the @bref{element_cache}.
 *
 ************************************************************************/
class gui_element_loader :
	private non_copyable
{

	public:

		/**
		 * Create an element loader that builds gui_element%s
		 * for use with a @bref{gui} and stores loading results
		 * in an @bref{element_cache}.
		 *
		 * The loader fetches dependency files through
		 * the @bref{files} context. Shaders and materials
		 * respectively resolve against @bref{shader_cache}
		 * and @bref{material_cache}. Finally, the loader
		 * queries gui_style%s from @bref{style_cache}.
		 *
		 * All bound object references must remain valid
		 * for the entire loader lifetime.
		 */
		gui_element_loader
			(const string& loader_name,
			 gui_context& gui,
			 file_context& files,		
			 const gui_shader_cache& shader_cache,	 
			 const gui_material_cache& material_cache,
			 const gui_style_cache& style_cache,
			 gui_element_cache& element_cache);


		/**
		 * Load GUI elements from various XML sources
		 * and place results into @bref{element_cache}.
		 * See gui_element_loader class documentation.
		 */
		//@{
		void load_elements(const xml_node& root);
		void load_elements(file& xml_file);
		void load_elements(const string& path);
		//@}


		const string loader_name;

		gui_context& gui;
		file_context& files;

		const gui_shader_cache& shader_cache;
		const gui_material_cache& material_cache;
		const gui_style_cache& style_cache;

		gui_element_cache& element_cache;


	private:		

		void to_slider_helper
			(const xml_node& node, gui_slider& slider);
		void to_list_items_helper
			(const xml_node& node, gui_list& list);

		shared_instance<gui_element> to_spacer(const xml_node& node);
		shared_instance<gui_element> to_anchor(const xml_node& node);
		shared_instance<gui_element> to_stack(const xml_node& node);
		shared_instance<gui_element> to_grid(const xml_node& node);

		shared_instance<gui_element> to_label(const xml_node& node);
		shared_instance<gui_element> to_image(const xml_node& node);
		shared_instance<gui_element> to_frame(const xml_node& node);

		shared_instance<gui_element> to_button(const xml_node& node);
		shared_instance<gui_element> to_tristate(const xml_node& node);
		shared_instance<gui_element> to_knob(const xml_node& node);

		shared_instance<gui_element> to_slider(const xml_node& node);
		shared_instance<gui_element> to_value_edit(const xml_node& node);
		shared_instance<gui_element> to_progress_bar(const xml_node& node);
		shared_instance<gui_element> to_image_map(const xml_node& node);

		shared_instance<gui_element> to_split(const xml_node& node);
		shared_instance<gui_element> to_scrollbar(const xml_node& node);
		shared_instance<gui_element> to_scroll_area(const xml_node& node);

		shared_instance<gui_element> to_list(const xml_node& node);
		shared_instance<gui_element> to_listbox(const xml_node& node);
		shared_instance<gui_element> to_dropdown(const xml_node& node);
		shared_instance<gui_element> to_tabview(const xml_node& node);

		shared_instance<gui_element> to_edit(const xml_node& node);
		shared_instance<gui_element> to_editbox(const xml_node& node);

		shared_instance<gui_element> to_window(const xml_node& node);
		shared_instance<gui_element> to_window_area(const xml_node& node);

		shared_instance<gui_element> to_tooltip(const xml_node& node);
		shared_instance<gui_element> to_tooltip_area(const xml_node& node);

		typedef shared_instance<gui_element>
					(gui_element_loader::*element_handler)(const xml_node& node);

		typedef tree_map<string, element_handler> element_handler_map;
		element_handler_map element_handlers_;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_ELEMENT_LOADER_HPP) */
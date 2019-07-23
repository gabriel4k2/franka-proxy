/**
 *************************************************************************
 *
 * @file collada_parser.hpp
 * 
 * Collada parsing process.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__COLLADA_PARSER_HPP)
#define INCLUDED__VIRAL_CORE__COLLADA_PARSER_HPP


#include "build_config.hpp"


#include "container_forward.hpp"
#include "file_forward.hpp"


#include "util.hpp"
#include "model.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class collada_parser
 * @ingroup mesh_module
 *
 * Stores global parsing state within collada_importer.
 *
 * Usage notes:
 * - See collada_importer for a more convenient client-side interface.
 * - Clients should move @bref{result_model} out of the collada_parser
 *		and into their target model to avoid excessive copying.
 *
 ************************************************************************/
class collada_parser :
	private non_copyable
{
	public:

		/**
		 * Parse Collada from XML root node
		 * into local @bref{result_model}.
		 */
		collada_parser
			(const xml_node& collada_root,
			 model::model_usage_hint usage);


		template <typename T, typename F>
			static void split_to_buffer
				(const string& str, dynamic_buffer<T>& buffer, F func);
		template <typename T, typename F>
			static void split_fixed_size
				(const string& str, T* dest, F func, int64 size);


		const xml_node& fetch_xml_node(const string& id) const;


		const xml_node& collada_root;
		model result_model;
		const model::model_usage_hint usage;


	private:

		void parse();
		void post_process();


		typedef tree_map<string, const xml_node&> id_to_node_map;
		id_to_node_map id_to_node_;

		const auto_pointer<collada_scene_parser> scene_parser_;
};




/**
 *************************************************************************
 *
 * @class collada_parser_limits
 * @ingroup mesh_module
 *
 * Limits for Collada parsing.
 *
 ************************************************************************/
class collada_parser_limits :
	private non_constructable
{

	public:

		/** Maximum supported recursion depth of scene nodes. */
		static const int64 max_node_tree_height = 16;


		/**
		 * Maximum number of model_group as instanced
		 * per combination of geometry ID and material.
		 */
		static const int64 max_instance_count = 128;


		/**
		 * Maximum number of vertex attributes
		 * supported per vertex.
		 */
		static const int64 max_vertex_attribute_count = 64;


		/** Maximum supported source stride. */
		static const int64 max_source_stride = 64;


		/** Maximum allowed number of joints and children per joint. */
		//@{
		static const int64 max_joint_count = 128;
		static const int64 max_children_per_joint = 128;
		//@}
};




/**
 *************************************************************************
 *
 * @class collada_scene_parser
 * @ingroup mesh_module
 *
 * Parses the scene structure of a Collada file,
 * including node transformations and geometry instances.
 *
 * Process overview:
 * - Start at \<library_visual_scenes\>, recurse through all \<node\>s.
 * - Handle all \<instance_geometry\> and \<instance_controller\> nodes.
 * - We query node transforms when actually instantiating geometry
 *		within collada_geometry_parser or collada_skeleton_parser.
 *
 * Example Collada structure:
 * @verbatim
 *	...
 *	<library_visual_scenes>
 *		<visual_scene>
 *			<node>
 *				<matrix>1 0 0 0 0 1 0 ... </matrix>
 *				<translate>1 7 3</translate>
 *				<rotate>1 0 0 90</rotate>
 *				...
 *				<instance_controller url="#ref to controller_id"/>
*				<instance_controller url="#ref to controller_id"/>
 *				...
 *				<instance_geometry url="#ref to geometry_id"/>
 *				<instance_geometry url="#ref to geometry_id"/>
 *				<instance_geometry url="#ref to geometry_id_2"/>
 *				...
 *				<node>
 *					<scale>3 2 1</scale>
 *					...
 *					<instance_controller url="#ref to controller_id"/>
 *					...
 *					<instance_geometry url="#ref to geometry_id"/>
 *					<instance_geometry url="#ref to geometry_id"/>
 *					...
 *				...
 *			<node>
 *			...
 *	...
 * @endverbatim
 *
 ************************************************************************/
class collada_scene_parser :
	private non_copyable
{
	public:

		collada_scene_parser
			(collada_parser& root_parser);

		void parse(const xml_node& scene_node);


		si_transform build_semantic_transform() const;

		si_transform build_absolute_transform
			(const xml_node& scene_node) const;
		si_transform build_local_transform
			(const xml_node& scene_node) const;


	private:

		void parse(const xml_node& scene_node,
				   int64 parent_node_height);


		collada_parser& root_parser_;

		const auto_pointer<collada_geometry_parser> geometry_parser_;
		const auto_pointer<collada_skeleton_parser> skeleton_parser_;

		mutable bool reported_unsupported_transform_;
};




/**
 *************************************************************************
 *
 * @class collada_geometry_parser
 * @ingroup mesh_module
 * 
 * Parses individual geometry instances from a Collada file.
 *
 * Process overview:
 * - All \<float_array\>s in \<source\> XML nodes are preparsed
 *		and stored in a lookup map on parser creation.
 * - For a \<geometry_instance\>, find the referenced \<geometry\>,
 *		then generate a mesh and a model_group for each \<polylist\>
 *		or \<triangles\> block.
 * - Follow \<input\> tags to per-vertex attributes, and extract these
 *		from source lookup maps as indexed by \<p\> and \<vcount\> arrays.
 *
 * Example Collada structure:
 * @verbatim
 *	...
 *	<library_geometries>
 *		<geometry id="geometry_id">
 *			<mesh>
 *				<source id="source_id">
 *					<float_array>1.3 2.123 3.1 44.66 54.37 ...
 *					<technique_common><accessor stride="3"/></technique_common>
 *				...
 *				<source id="source_id_2">
 *				...
 *				<vertices id="vertex_id">
 *					<input semantic="POSITION" source="#ref to source_id"/>
 *				...
 *				<polylist material="material_name">     (or <triangles>)
 *					<input semantic="VERTEX" source="#ref to vertex_id" offset="0"/>
 *					<input semantic="NORMAL" source="#ref to source_id_2" offset="1"/>
 *					<input semantic="TEXCOORD" source="#ref to source_id_2" offset="2"/>
 *					...
 *					<vcount>3 3 3 4 4 ...
 *					<p>1 16 23 123 94 ...
 *				...
 *				<polylist>
 *				...
 *			<mesh>
 *			...
 *		<geometry id="geometry_id_2">
 *		...
 *	...
 *	<instance_geometry url="#ref to geometry_id"/>
 *	...
 * @endverbatim
 *
 ************************************************************************/
class collada_geometry_parser :
	private non_copyable
{
	public:

		collada_geometry_parser
			(collada_parser& root_parser,
			 collada_scene_parser& scene_parser);

		void parse_instance_geometry
			(const xml_node& instance_geometry);

		void parse_skin_geometry
			(const xml_node& geometry,			
			 const array<skeleton_index>& skeleton_indices,
			 const array<skeleton_weight>& skeleton_weights,
			 const si_transform& binding_shape_transform);


	private:

		void preparse_source_values();

		void parse_geometry
			(const xml_node& geometry,
			 const si_transform& transform);

		model_group& create_model_group
			(const xml_node& list_node, const string& geometry_id);
		int64 parse_vertex_attribute_index_count_per_vertex
			(const xml_node& list_node);
		void parse_vertex_attribute_indices
			(dynamic_buffer<int64>& va_indices, const xml_node& list_node,
			 int64 va_count_per_vertex);

		template <typename T>
			void fetch_vertex_attribute
				(mesh_stream_typed<T>& stream,
				 const dynamic_buffer<float>& source, int64 source_stride,
				 const dynamic_buffer<int64>& va_indices,
				 int64 va_index_offset, int64 va_index_count_per_vertex);

		template <typename T>
			void fetch_vertex_attribute_skeleton
				(mesh_stream_typed<T>& stream,
				 const array<T>& source,
				 const dynamic_buffer<int64>& va_indices,
				 int64 va_index_offset, int64 va_index_count_per_vertex);

		void parse_vertices
			(mesh& msh, const xml_node& input,
			 const dynamic_buffer<int64>& va_indices,
			 int64 va_index_offset, int64 va_index_count_per_vertex);

		template <typename T>
			void parse_vertex_stream
				(mesh& msh, const string& stream_name,
				 const xml_node& input,
				 const dynamic_buffer<int64>& va_indices,
				 int64 va_index_offset, int64 va_index_count_per_vertex);

		void parse_vertex_streams(mesh& msh, const xml_node& list_node);
		void parse_triangle_streams(mesh& msh);

		void post_process
			(mesh& msh, const si_transform& transform);


		collada_parser& root_parser_;
		collada_scene_parser& scene_parser_;

		typedef tree_map<string, dynamic_buffer<float>> source_value_map;
		source_value_map source_values_;

		typedef tree_map<string, int64> instance_count_map;
		instance_count_map instance_counts_;

		const array<skeleton_index>* skeleton_indices_;
		const array<skeleton_weight>* skeleton_weights_;
};




/**
 *************************************************************************
 *
 * @class collada_skeleton_parser
 * @ingroup mesh_module
 *
 * Parses a single skeleton controller within a Collada XML file.
 *
 * Process overview:
 * - Follow \<url\> of \<instance_controller\> to skinning data.
 * - Evaluate \<joints\> bound to the \<skin\> data in \<controller\>:
 *		@bref{name_source_id} gives a map of Collada XML indices
 *		to joint IDs and @bref{bind_pose_source_id} holds bind poses.
 * - Follow \<skeleton\> of \<instance_controller\> to root joint.
 *		Parse all children joints and create @ref skeleton rig.
 *		Store as model::skeleton_rig() for first instance controller,
 *		or check against existing skeleton rig for later controllers.
 * - Parse \<controller\> again, but this time read \<vertex_weights\>.
 *		Each position in @bref{geometry_id} (see collada_geometry_parser)
 *		has \<vcount\> entries in \<v\> that specify its joint binding.
 *		Each binding is given as a joint index in @bref{name_source_id}
 *		and a weight index in @bref{weights_source_id}.
 * - Follow the source of \<skin\> to @bref{geometry_id} and instantiate
 *		the referenced geometry with parsed skeleton indices and weights.
 *
 * Joint transformations:
 * - The general skinning transformation specified by Collada is
 *			v' = sum_i(Wi * AJMi * IBPMi * BSM * v),
 *		with BSM as the global binding shape matrix,
 *		with IBPMi as the inverse binding pose matrix,
 *		and with AJMi as the absolute joint matrix,
 *		the latter two for a joint i with weight Wi.
 * - We store vertices pre-transformed to binding shape BSM,
 *		but with a correction matrix for axis semantics SEM,
 *			v' = sum_i(Wi * AJMi * (IBPMi * SEM^-1) * (SEM * BSM) * v).
 *		Thus our global geometry transformation is SEM * BSM,
 *		and our binding_to_joint matrix becomes IBPMi * SEM^-1.
 *		See collada_geometry_parser::post_process().
 *
 * Example Collada structure:
 * @verbatim
 *	...
 *	<library_controllers>
 *		<controller id="controller_id">
 *			<skin source="#ref to geometry_id>
 *				<bind_shape_matrix>1 0 0 0 0 1 0 0 0 0 1 0 ...
 *				<source id="name_source_id">
 *					<Name_array>root_joint_id child_joint_id ...
 *				...
 *				<source id="bind_pose_source_id">
 *					<float_array>1 0 0 1.92 0 0.905 0.423 -4.01 0 ...
 *				...
 *				<source id="weights_source_id">
 *					<float_array>1 1 1 1 1 1 1 1 1 1 1 0.997 0.04 ...
 *				...
 *				<joints>
 *					<input semantic="JOINT" source="#ref to name_source_id"/>
 *					<input semantic="INV_BIND_MATRIX" source="#ref to bind_pose_source_id"/>
 *				</joints>
 *				<vertex_weights count="212">
 *					<input semantic="JOINT" source="#ref to name_source_id" offset="0"/>
 *					<input semantic="WEIGHT" source="#ref to weights_source_id" offset="1"/>
 *					<vcount>1 1 2 1 4 5 ...
 *					<v>2 0 2 1 2 2 2 3 2 4 ...
 *				<vertex_weights>
 *				...
 *			...
 *		<controller id="controller_id_2">
 *		...
 *  ...
 *	<node id="root_joint_id" type="JOINT">
 *		<matrix sid="transform">1 0 0 -0.88 0 0.009 -0.999 ...
 *		<node id="child_joint_id" type="JOINT">
 *			<matrix sid="transform">1 0 -2.38419e-7 0 ...
 *			...
 *	...
 *	<instance_controller url="#ref to controller_id">
 *		<skeleton>#ref to root_joint_id</skeleton>
 *	...
 * @endverbatim
 *
 ************************************************************************/
class collada_skeleton_parser :
	private non_copyable
{
	public:

		collada_skeleton_parser
			(collada_parser& root_parser,
			 collada_scene_parser& scene_parser,
			 collada_geometry_parser& geometry_parser);

		void parse_instance_controller
			(const xml_node& instance_controller);

	
	private:

		void parse_joint_binding
			(const xml_node& skin,
			 array<matrix>& inv_bind_matrices,
			 bimap<string, int64>& joint_name_to_index_xml);
		void parse_inv_bind_matrices
			(const string& inv_bind_matrix_source_id,
			 array<matrix>& inv_bind_matrices);
		void parse_joint_name_to_index_xml
			(const string& joint_source_id,
			 bimap<string, int64>& joint_name_to_index_xml);

		void parse_skeleton_rig
			(const xml_node& instance_controller,
			 const array<matrix>& inv_bind_matrices,
			 const bimap<string, int64>& joint_name_to_index_xml,			 
			 bimap<string, int64>& joint_name_to_index_skeleton);
		void parse_joints
			(const xml_node& root_joint_node,
			 const array<matrix>& inv_bind_matrices,
			 const bimap<string, int64>& joint_name_to_index_xml,
			 bimap<string, int64>& joint_name_to_index_skeleton,
			 skeleton_joint_array& joints);	
		void check_joints_reuse
			(const skeleton_joint_array& joints,
			 const skeleton_joint_array& reuse_joints);
	

		void parse_vertex_binding
			(const xml_node& skin,
			 array<skeleton_index>& skeleton_indices,
			 array<skeleton_weight>& skeleton_weights,
			 const bimap<string, int64>& joint_name_to_index_xml,			 
			 const bimap<string, int64>& joint_name_to_index_skeleton);
		void parse_vertex_weights_and_joint_indices
			(const xml_node& vertex_weights,
			 const xml_node& weights_source,
			 int64 joint_offset, int64 weight_offset, int64 max_offset,
			 array<skeleton_index>& skeleton_indices,
			 array<skeleton_weight>& skeleton_weights);
		void translate_xml_to_skeleton_joint_indices
			(array<skeleton_index>& skeleton_indices,
		 	 const array<skeleton_weight>& skeleton_weights,
			 const bimap<string, int64>& joint_name_to_index_xml,			 
			 const bimap<string, int64>& joint_name_to_index_skeleton);

		void parse_geometry
			(const xml_node& skin,
			 const array<skeleton_index>& skeleton_indices,
			 const array<skeleton_weight>& skeleton_weights);


		static const float skeleton_weight_normalize_epsilon_;

		collada_parser& root_parser_;
		collada_scene_parser& scene_parser_;
		collada_geometry_parser& geometry_parser_;

		string skeleton_root_joint_node_id_;

		mutable bool reported_unsupported_binding_matrix_;
		mutable bool reported_unbound_vertex_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__COLLADA_PARSER_HPP) */
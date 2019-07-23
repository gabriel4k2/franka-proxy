/**
 *************************************************************************
 *
 * @file collada_parser.cpp
 * 
 * Collada parsing process, implementation.
 *
 ************************************************************************/


#include "collada_parser.hpp"


#include "log.hpp"
#include "integer_checks.hpp"
#include "bimap.hpp"
#include "xml.hpp"
#include "color.hpp"
#include "mesh_util.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// collada_parser
//
//////////////////////////////////////////////////////////////////////////


collada_parser::collada_parser
	(const xml_node& collada_root,
	 model::model_usage_hint usage)
	:
	collada_root(collada_root),
	usage(usage),
	scene_parser_(new collada_scene_parser(*this))
{
	parse();
	post_process();
}


template <typename T, typename F>
	void collada_parser::split_to_buffer
		(const string& str, dynamic_buffer<T>& buffer, F func)
{
	/**
	 * Separate input values within @bref{str} at whitespace,
	 * use function @bref{F} to convert into POD type @bref{T},
	 * then store results into dynamic @bref{buffer}.
	 *
	 * Notes:
	 * - The splitting algorithm is optimized for few allocations
	 *		to avoid sluggish Visual C++ debugging.
	 * - Per-iteration \\0 tests are required to avoid infinite
	 *		looping on corrupt files that contain intermediate \\0s.
	 * - static_cast<T> solves a warning with atof returning doubles.
	 */

	string search_me(str.compact());
	search_me.replace_all(' ', '\0');

	const char *chars = search_me.data();
	int64 start = 0;
	
	while (chars[start] != '\0')
	{
		buffer.push_back(static_cast<T>(func(chars + start)));
		start = search_me.seek('\0', start);
		if (start == string::invalid_index) break;
		++start;
	}
}


template <typename T, typename F>
	void collada_parser::split_fixed_size
		(const string& str, T* dest, F func, int64 size)
{
	/**
	 * A variant of split_to_buffer() for fixed-size arrays.
	 * Clients must ensure that dest has enough storage space.
	 * Throws if @bref{str} does not have @bref{size} entries.
	 * In this case, @bref{dest} remains unchanged.
	 */

	dynamic_buffer<T> dest_offhand;
	split_to_buffer(str, dest_offhand, func);

	if (dest_offhand.size() != size)
	{
		LOG_ERROR("Fixed-size container has too many or too few entries.");
		throw xml_exception();
	}

	memory::copy_block_typed
		(dest, dest_offhand.data(), size);
}


const xml_node& collada_parser::fetch_xml_node(const string& id) const
{
	id_to_node_map::const_iterator find(id_to_node_.find(id));	

	if (!find)
	{
		LOG_ERROR("Node with id: " + id + " not found in XML.");
		throw xml_exception();
	}

	return find->value;
}


void collada_parser::parse()
{
	if (collada_root.name() != "COLLADA")
	{
		LOG_ERROR("Name of root node must be COLLADA.");
		throw xml_exception();
	}

	collada_root.create_id_to_node_map(id_to_node_);

	const xml_node& library_visual_scenes =
		collada_root.child("library_visual_scenes");

	if (library_visual_scenes.child_count("visual_scene") != 1)
		LOG_WARN("Ignoring all but the first visual scene in Collada file.");

	const xml_node& visual_scene =
		library_visual_scenes.child("visual_scene");
	scene_parser_->parse(visual_scene);
}


void collada_parser::post_process()
{
	// Post-process entire model.
	// Attention: validate() must be called before optimize()
	// to ensure that optimization has a valid model to work on.
	// Just in case our Collada parser glitches...
	result_model.validate();
	result_model.optimize();
	result_model.rebuild_boundings();
}




//////////////////////////////////////////////////////////////////////////
//
// collada_scene_parser
//
//////////////////////////////////////////////////////////////////////////


collada_scene_parser::collada_scene_parser
	(collada_parser& root_parser)
	:
	root_parser_(root_parser),
	geometry_parser_(new collada_geometry_parser(root_parser_, *this)),
	skeleton_parser_(new collada_skeleton_parser(root_parser_, *this, *geometry_parser_)),
	reported_unsupported_transform_(false)
{ }


void collada_scene_parser::parse(const xml_node& scene_node)
	{ parse(scene_node, 0); }


si_transform collada_scene_parser::build_semantic_transform() const
{
	/**
	 * Build transform from Collada up-axis semantics
	 * to our convention of negative-y as up-axis.
	 */

	if (!root_parser_.collada_root.has_child("asset") ||
		!root_parser_.collada_root.child("asset").has_child("up_axis"))
			return si_transform();

	string up_axis
		(root_parser_.collada_root.
			child("asset").child("up_axis").text());

	if (up_axis == "X_UP")
	{
		return si_transform
			(vector(), rotation(vector(0, 1, 0), 180.f) *
					   rotation(vector(0, 0, 1), 90.f), 1.f);
	}
	else if (up_axis == "Y_UP")
	{
		return si_transform
			(vector(), rotation(vector(1, 0, 0), 180.f), 1.f);
	}
	else if (up_axis == "Z_UP")
	{
		return si_transform
			(vector(), rotation(vector(1, 0, 0), 90.f), 1.f);
	}

	LOG_WARN("Unknown up_axis in Collada file.");
	return si_transform();
}


si_transform collada_scene_parser::build_absolute_transform
	(const xml_node& node) const
{
	/**
	 * Extract all relative transforms stored
	 * within a scene node and its parent nodes
	 * to build the absolute node transformation.
	 */

	si_transform result;
	for (const xml_node* current = &node;
		 current && current->name() == "node";
		 current = current->parent())
	{
		result =
			build_local_transform(*current) * result;
	}

	return result;
}


si_transform collada_scene_parser::build_local_transform
	(const xml_node& node) const
{
	/**
	 * Extract relative transform from a scene node.
	 * Collada supports multiple transformation types,
	 * which accumulate in order of occurrence.
	 */

	matrix result;
	for (xml_node::const_child_iterator it
			(node.first_child()); it; ++it)
	{
		if (it->name() == "matrix")
		{
			matrix transform;
			collada_parser::split_fixed_size
				(it->text(), transform.c, &atof,
				 sizeof(transform.c) / sizeof(transform.c[0]));

			result *= transform;
		}
		else if (it->name() == "translate")
		{
			vector translate;
			collada_parser::split_fixed_size
				(it->text(), translate.c, &atof,
				 sizeof(translate.c) / sizeof(translate.c[0]));

			result *= matrix::translate(translate);
		}
		else if (it->name() == "rotate")
		{
			float values[4];
			collada_parser::split_fixed_size
				(it->text(), values, &atof,
				 sizeof(values) / sizeof(values[0]));
			
			vector axis(values[0], values[1], values[2]);
			rotation rotate(axis, values[3]);
			result *= matrix::rotate(rotate);
		}
		else if (it->name() == "scale")
		{
			vector scale;
			collada_parser::split_fixed_size
				(it->text(), scale.c, &atof,
				 sizeof(scale.c) / sizeof(scale.c[0]));

			result *= matrix::scale(scale);
		}
	}

	if (!reported_unsupported_transform_ &&
		!si_transform::representable(result))
	{
		LOG_WARN("Node transform is not representable by si_transform. "
				 "Model will end up deformed.");
		reported_unsupported_transform_ = true;
	}

	return si_transform::from_matrix(result);
}


void collada_scene_parser::parse
	(const xml_node& scene_node, int64 parent_node_height)
{
	if (parent_node_height + 1 > collada_parser_limits::max_node_tree_height)
	{
		LOG_ERROR("Collada scene tree exceeds maximum height.");
		throw xml_exception();
	}

	for (xml_node::const_child_iterator child_it
			(scene_node.first_child()); child_it; ++child_it)
	{
		if (child_it->name() == "instance_geometry")
			geometry_parser_->parse_instance_geometry(*child_it);

		if (child_it->name() == "instance_controller")
			skeleton_parser_->parse_instance_controller(*child_it);
			
		if (child_it->name() == "node")
			parse(*child_it, parent_node_height + 1);
	}
}




//////////////////////////////////////////////////////////////////////////
//
// collada_geometry_parser
//
//////////////////////////////////////////////////////////////////////////


collada_geometry_parser::collada_geometry_parser
	(collada_parser& root_parser,
	 collada_scene_parser& scene_parser)
	:
	root_parser_(root_parser),
	scene_parser_(scene_parser),
	skeleton_indices_(NULL),
	skeleton_weights_(NULL)
{
	preparse_source_values();
}


void collada_geometry_parser::parse_instance_geometry
	(const xml_node& instance_geometry)
{
	/**
	 * Entry point into geometry parsing
	 * from a \<instance_geometry\> node.
	 */

	string geometry_id
		(instance_geometry.attribute("url").substring(1));
	const xml_node& geometry =
		root_parser_.fetch_xml_node(geometry_id);

	si_transform instance_transform
		(instance_geometry.parent() ?
			 scene_parser_.build_absolute_transform
				(*instance_geometry.parent()) :
			 si_transform());

	skeleton_indices_ = NULL;
	skeleton_weights_ = NULL;

	parse_geometry(geometry, instance_transform);
}


void collada_geometry_parser::parse_skin_geometry
	(const xml_node& geometry,
	 const array<skeleton_index>& skeleton_indices,
	 const array<skeleton_weight>& skeleton_weights,
	 const si_transform& binding_shape_matrix)
{
	/**
	 * Entry point into geometry parsing
	 * from a \<skin\> binding node.
	 */

	skeleton_indices_ = &skeleton_indices;
	skeleton_weights_ = &skeleton_weights;

	parse_geometry(geometry, binding_shape_matrix);
}


void collada_geometry_parser::preparse_source_values()
{
	/**
	 * Prepare a map (node id of source) -> (buffer with source values)
	 * for each \<source\> node within the Collada XML to avoid
	 * reparsing on each reference by an \<input\> node.
	 *
	 * Collada models need not have a library_geometries,
	 * for instance if they contain but a skeleton.
	 */

	if (!root_parser_.collada_root.has_child("library_geometries"))
		return;

	const xml_node& geometries =
		root_parser_.collada_root.child("library_geometries");

	for (xml_node::const_child_iterator geo_it(geometries.first_child());
		 geo_it; ++geo_it)
	{
		if (geo_it->name() != "geometry") continue;

		for (xml_node::const_child_iterator mesh_it(geo_it->first_child());
			 mesh_it; ++mesh_it)
		{
			if (mesh_it->name() != "mesh") continue;

			for (xml_node::const_child_iterator source_it(mesh_it->first_child());
				 source_it; ++source_it)
			{
				if (source_it->name() != "source") continue;

				string source_id
					(source_it->attribute("id"));
				const xml_node& float_array =
					source_it->child("float_array");

				source_value_map::iterator new_buffer
					(source_values_.insert(source_id, dynamic_buffer<float>()));
				collada_parser::split_to_buffer
					(float_array.text(), new_buffer->value, &atof);
			}
		}
	}
}


void collada_geometry_parser::parse_geometry
	(const xml_node& geometry,
	 const si_transform& transform)
{
	/**
	 * Parse a single Collada geometry node and
	 * all its \<polylist\>s and \<triangles\>.
	 * As instancing is not supported within @ref model,
	 * a unique model_group is created for each geometry.
	 */

	string geometry_id
		(geometry.attribute("id"));

	for (xml_node::const_child_iterator mesh_it(geometry.first_child());
		 mesh_it; ++mesh_it)
	{
		if (mesh_it->name() != "mesh") continue;

		for (xml_node::const_child_iterator list_it(mesh_it->first_child());
			 list_it; ++list_it)
		{
			if (list_it->name() != "polylist" &&
				list_it->name() != "triangles") continue;

			model_group& group =
				create_model_group(*list_it, geometry_id);

			parse_vertex_streams(group.mesh, *list_it);
			parse_triangle_streams(group.mesh);
			post_process(group.mesh, transform);
		}
	}
}


model_group& collada_geometry_parser::create_model_group
	(const xml_node& list_node, const string& geometry_id)
{
	/**
	 * Create and return a new model_group in the result_model.
	 * Parser-wide instance counters ensure unique group names.
	 * This is not guaranteed otherwise, as multiple polylists
	 * or triangles may have the same geometry_id and material.
	 */

	string material;
	list_node.try_get_attribute("material", material);

	string groupname(geometry_id + "." + material);

	int64 instance_counter =
		instance_counts_.contains(groupname) ?
			instance_counts_.find(groupname)->value : 0;

	if (instance_counter >= collada_parser_limits::max_instance_count)
	{
		LOG_ERROR("Number of instances for: " + groupname + " exceeds maximum.");
		throw bounds_exceeded();
	}

	++instance_counter;
	instance_counts_.insert(groupname, instance_counter);
	groupname = groupname + "." + instance_counter;

	model::group_iterator groupit
		(root_parser_.result_model.insert_group(groupname));

	return **groupit;
}


int64 collada_geometry_parser::parse_vertex_attribute_index_count_per_vertex
	(const xml_node& list_node)
{
	/**
	 * Determine the number of attribute indices per vertex
	 * within the \<p\> field of \<triangles\> or \<polylist\>.
	 * This information is not explicitly available anywhere else,
	 * but it is required for later index and source parsing.
	 */

	int64 max_offset = -1;		

	for (xml_node::const_child_iterator input_it(list_node.first_child());
		 input_it; ++input_it)
	{
		if (input_it->name() != "input") continue;		

		max_offset =
			max(max_offset,
				input_it->attribute("offset").to_int64());
	}	

	if (max_offset < 0)
	{
		LOG_ERROR("No valid input tag found in triangles or polylist.");
		throw xml_exception();
	}

	if (max_offset + 1 >
		collada_parser_limits::max_vertex_attribute_count)
	{
		LOG_ERROR("Too many vertex attributes per vertex.");
		throw bounds_exceeded();
	}

	return max_offset + 1;
}


void collada_geometry_parser::parse_vertex_attribute_indices
	(dynamic_buffer<int64>& va_indices, const xml_node& list_node,
	 int64 va_index_count_per_vertex)
{
	/**
	 * Extract attribute indices for all vertices and triangles
	 * from Collada XML into an in-memory representation.
	 *
	 * Notes:
	 * - Polygons are triangulated into triangle fans here:
	 *		0 1 2 3 4... becomes 0 1 2 0 2 3 0 3 4 ...
	 * - Indices are cross-checked against boundaries to
	 *		protect from corrupt \<vcount\> entries.
	 * - Once finished, a single triangle is defined by each three
	 *		consecutive vertices. Each vertex in turn corresponds to
	 *		va_indices_per_vertex consecutive entries in va_indices.
	 */

	collada_parser::split_to_buffer
		(list_node.child("p").text(), va_indices, &atoi);

	if (list_node.name() == "polylist")
	{
		dynamic_buffer<int64> vcount;
		collada_parser::split_to_buffer
			(list_node.child("vcount").text(), vcount, &atoi);

		int64 num_tris = 0;
		for (int64 i = 0; i < vcount.size(); ++i)
			num_tris += vcount[i] - 2;

		dynamic_buffer<int64> triangulated;
		triangulated.reserve(num_tris * 3 * va_index_count_per_vertex);

		int64 sourceindex = 0;
		for (int64 i = 0; i < vcount.size(); ++i)
		{
			for (int64 j = 0; j < vcount[i] - 2; ++j)
			{
				for (int64 k = 0; k < va_index_count_per_vertex; ++k)
					triangulated.push_back(va_indices.at(sourceindex + k));
				for (int64 k = 0; k < va_index_count_per_vertex; ++k)
					triangulated.push_back(va_indices.at(sourceindex + (j + 1) * va_index_count_per_vertex + k));			
				for (int64 k = 0; k < va_index_count_per_vertex; ++k)
					triangulated.push_back(va_indices.at(sourceindex + (j + 2) * va_index_count_per_vertex + k));
			}
			sourceindex += vcount[i] * va_index_count_per_vertex;
		}

		swap(va_indices, triangulated);
	}

	if (va_indices.size() % (va_index_count_per_vertex * 3) != 0)
	{
		LOG_ERROR("Some Collada vertex has too few or too many attribute indices.");
		throw xml_exception();
	}
}


template <typename T>
	void collada_geometry_parser::fetch_vertex_attribute
		(mesh_stream_typed<T>& stream,
		 const dynamic_buffer<float>& source, int64 source_stride,
		 const dynamic_buffer<int64>& va_indices,
		 int64 va_index_offset, int64 va_index_count_per_vertex)
{
	/**
	 * Extract a single Collada vertex attribute from
	 * a source array into an appropriate mesh_stream.
	 *
	 * Notes:
	 * - We duplicate the vertex attributes as indexed
	 *		by each triangle corner (see collada_importer).
	 * - Detecting redundant attribute index tuples would
	 *		severely complicate import, as we can't process
	 *		attributes and triangles separately anymore.
	 *		Instead we optimize final meshes to remove
	 *		any redundant vertices.
	 * - We only support at most three components per attribute,
	 *		and initialize missing components to zero.
	 * - Checked access to indices and source values protects
	 *		from invalid strides or offsets.
	 */

	if (source_stride < 1 || source_stride > 3)
	{
		LOG_ERROR("Data source comes with too much or too little data.");
		throw xml_exception();
	}

	int64 strmsize = va_indices.size() / va_index_count_per_vertex;
	stream.resize(strmsize);

	for (int64 i = 0; i < strmsize; ++i)
	{
		float source_a =
			source.at(va_indices.at
				(i * va_index_count_per_vertex + va_index_offset) * source_stride);

		float source_b =
			(source_stride <= 1) ? 0 :
				source.at(va_indices.at
					(i * va_index_count_per_vertex + va_index_offset) * source_stride + 1);

		float source_c =
			(source_stride <= 2) ? 0 :
				source.at(va_indices.at
					(i * va_index_count_per_vertex + va_index_offset) * source_stride + 2);

		stream[i] = T(source_a, source_b, source_c);
	}
}


template <typename T>
	void collada_geometry_parser::fetch_vertex_attribute_skeleton
		(mesh_stream_typed<T>& stream,
		 const array<T>& source,
		 const dynamic_buffer<int64>& va_indices,
		 int64 va_index_offset, int64 va_index_count_per_vertex)
{
	/**
	 * A variant of fetch_vertex_attribute() for the joint weight
	 * and joint index streams retrieved by collada_skeleton_parser.
	 */

	int64 strmsize = va_indices.size() / va_index_count_per_vertex;
	stream.resize(strmsize);

	for (int64 i = 0; i < strmsize; ++i)
		stream[i] = source.at(va_indices.at
						(i * va_index_count_per_vertex + va_index_offset));
}


void collada_geometry_parser::parse_vertices
	(mesh& msh, const xml_node& input,
	 const dynamic_buffer<int64>& va_indices,
	 int64 va_index_offset, int64 va_index_count_per_vertex)
{
	/**
	 * Parse Collada vertex positions into vertices stream.
	 * 
	 * Implementation notes:
	 * - Actual vertex positions have another level of indirection
	 *		through the \<vertices\> node layer.
	 * - Indices into the positions stream match up with the indices
	 *		of joints and weights from collada_skeleton_parser.
	 *		Thus, we also extract skeleton-related streams here.
	 */


	// Fetch source data stream through indirection.
	string source_id
		(input.attribute("source").substring(1));

	string positions_source_id
		(root_parser_.fetch_xml_node(source_id).
			child("input").attribute("source").substring(1));

	source_value_map::const_iterator
		positions_source_it(source_values_.find(positions_source_id));

	if (!positions_source_it)
	{
		LOG_ERROR("Invalid Collada data source: " + positions_source_id);
		throw xml_exception();
	}

	int64 positions_source_stride =
		root_parser_.fetch_xml_node(positions_source_id).
			child("technique_common").child("accessor").
			attribute("stride").to_int64();

	if (positions_source_stride <= 0)
	{
		LOG_ERROR("Invalid input source stride.")
		throw xml_exception();
	}

	if (positions_source_stride > collada_parser_limits::max_source_stride)
	{
		LOG_ERROR("Source stride exceeds maximum.");
		throw bounds_exceeded();
	}


	// Fill actual vertex stream with positions data.
	fetch_vertex_attribute
		(msh.vertices(), positions_source_it->value, positions_source_stride,
		 va_indices, va_index_offset, va_index_count_per_vertex);


	// Fetch skeleton binding data streams.
	if (skeleton_indices_)
	{
		auto_pointer<mesh_stream_skeleton_index> skeleton_indices
			(new mesh_stream_skeleton_index());

		fetch_vertex_attribute_skeleton			
			(*skeleton_indices, *skeleton_indices_,
			 va_indices, va_index_offset, va_index_count_per_vertex);

		msh.insert_vertex_stream
			(mesh_stream_registry::skeleton_index_stream_name,
			 std::move(skeleton_indices));
	}

	if (skeleton_weights_)
	{
		auto_pointer<mesh_stream_skeleton_weight> skeleton_weights
			(new mesh_stream_skeleton_weight());

		fetch_vertex_attribute_skeleton
			(*skeleton_weights, *skeleton_weights_,
			 va_indices, va_index_offset, va_index_count_per_vertex);
			
		msh.insert_vertex_stream
			(mesh_stream_registry::skeleton_weight_stream_name,
			 std::move(skeleton_weights));
	}
}


template <typename T>
	void collada_geometry_parser::parse_vertex_stream
		(mesh& msh, const string& stream_name,
		 const xml_node& input,
		 const dynamic_buffer<int64>& va_indices,
		 int64 va_index_offset, int64 va_index_count_per_vertex)
{
	/**
	 * Parse a generic vertex stream from an \<input\> node,
	 * and add the resulting mesh_stream to the target mesh.
	 */


	// Fetch source data and stride within data.
	string source_id
		(input.attribute("source").substring(1));

	source_value_map::const_iterator
		source_it(source_values_.find(source_id));

	if (!source_it)
	{
		LOG_ERROR("Invalid Collada data source: " + source_id);
		throw xml_exception();
	}

	int64 source_stride =
		root_parser_.fetch_xml_node(source_id).
			child("technique_common").child("accessor").
			attribute("stride").to_int64();

	if (source_stride <= 0)
	{
		LOG_ERROR("Invalid input source stride.")
		throw xml_exception();
	}

	if (source_stride > collada_parser_limits::max_source_stride)
	{
		LOG_ERROR("Source stride exceeds maximum.");
		throw bounds_exceeded();
	}


	// Create vertex stream, fill with data,
	// then attach to mesh.
	auto_pointer<T> stream(new T());
	
	fetch_vertex_attribute
		(*stream, source_it->value, source_stride,
		 va_indices, va_index_offset, va_index_count_per_vertex);

	msh.insert_vertex_stream
		(stream_name, std::move(stream));
}


void collada_geometry_parser::parse_vertex_streams
	(mesh& msh, const xml_node& list_node)
{
	/**
	 * Translate all per-vertex attributes from \<input\> tags
	 * in the Collada XML to appropriate mesh_stream%s.
	 */

	int64 va_index_count_per_vertex =
		parse_vertex_attribute_index_count_per_vertex(list_node);

	dynamic_buffer<int64> va_indices;
	parse_vertex_attribute_indices
		(va_indices, list_node, va_index_count_per_vertex);

	for (xml_node::const_child_iterator input_it(list_node.first_child());
		 input_it; ++input_it)
	{
		if (input_it->name() != "input") continue;

		int64 va_index_offset =
			input_it->attribute("offset").to_int64();	

		if (va_index_offset < 0 || va_index_offset >= va_index_count_per_vertex)
		{
			LOG_ERROR("Invalid vertex attribute index offset in Collada file.");
			throw xml_exception();
		}

		string semantic
			(input_it->attribute("semantic"));

		if (semantic == "VERTEX")
			parse_vertices
				(msh, *input_it,
				 va_indices, va_index_offset, va_index_count_per_vertex);

		if (semantic == "NORMAL")
			parse_vertex_stream<mesh_stream_vector>
				(msh, mesh_stream_registry::normal_stream_name, *input_it, 
				 va_indices, va_index_offset, va_index_count_per_vertex);

		if (root_parser_.usage == model::any_usage ||
			root_parser_.usage == model::rendering_usage)
		{
			if (semantic == "COLOR")
				parse_vertex_stream<mesh_stream_color>
					(msh, mesh_stream_registry::color_stream_name, *input_it,
					 va_indices, va_index_offset, va_index_count_per_vertex);

			if (semantic == "TEXCOORD")
				parse_vertex_stream<mesh_stream_vector>
					(msh, mesh_stream_registry::texcoords_stream_name, *input_it,
					 va_indices, va_index_offset, va_index_count_per_vertex);
		}
	}
}


void collada_geometry_parser::parse_triangle_streams(mesh& msh)
{
	/**
	 * Fill triangle streams as appropriate for desired usage.
	 *
	 * Notes:
	 * - Each triangle has three unique indices due to
	 *		vertex duplication, see fetch_vertex_attribute().
	 *		Original Collada triangle indexing has been discarded.
	 */

	mesh_stream_triangle& triangles = msh.triangles();

	int64 trianglecount = msh.vertex_count() / 3;
	triangles.resize(trianglecount);

	for (int64 i = 0; i < trianglecount; ++i)
		triangles[i] = mesh_triangle(i * 3, i * 3 + 1, i * 3 + 2);
}


void collada_geometry_parser::post_process
	(mesh& msh, const si_transform& transform)
{
	/**
	 * Touch up a mesh after import of all data
	 * from the Collada file.
	 */

	// All mesh_util calls require a valid mesh.
	// Validate here to detect bugs in our parser.
	msh.validate();

	mesh_util::apply_transform
		(msh, transform);
	mesh_util::apply_transform
		(msh, scene_parser_.build_semantic_transform());

	// By specification, Collada has the origin
	// of textures in the lower-left image corner,
	// which conflicts with our own convention.
	mesh_util::flip_texcoord_origin_y(msh);

	if (root_parser_.usage == model::any_usage ||
		root_parser_.usage == model::rendering_usage)
			mesh_util::build_tangential_basis(msh);

	if (root_parser_.usage == model::any_usage ||
		root_parser_.usage == model::physics_usage)
			mesh_util::build_plane_equations(msh);
}




//////////////////////////////////////////////////////////////////////////
//
// collada_skeleton_parser
//
//////////////////////////////////////////////////////////////////////////


collada_skeleton_parser::collada_skeleton_parser
	(collada_parser& root_parser,
	 collada_scene_parser& scene_parser,
	 collada_geometry_parser& geometry_parser)
	:
	root_parser_(root_parser),
	scene_parser_(scene_parser),
	geometry_parser_(geometry_parser),
	reported_unsupported_binding_matrix_(false),
	reported_unbound_vertex_(false)
{ }


void collada_skeleton_parser::parse_instance_controller
	(const xml_node& instance_controller)
{
	string controller_id
		(instance_controller.attribute("url").substring(1));
	const xml_node& controller =
		root_parser_.fetch_xml_node(controller_id);
	const xml_node& skin =
		controller.child("skin");

	array<matrix> inv_bind_matrices;
	bimap<string, int64> joint_name_to_index_xml;
	parse_joint_binding
		(skin, inv_bind_matrices, joint_name_to_index_xml);

	bimap<string, int64> joint_name_to_index_skeleton;
	parse_skeleton_rig
		(instance_controller,
		 inv_bind_matrices,
		 joint_name_to_index_xml,
		 joint_name_to_index_skeleton);

	array<skeleton_index> skeleton_indices;
	array<skeleton_weight> skeleton_weights;
	parse_vertex_binding
		(skin, skeleton_indices, skeleton_weights,
		 joint_name_to_index_xml,
		 joint_name_to_index_skeleton);

	parse_geometry
		(skin, skeleton_indices, skeleton_weights);	
}


void collada_skeleton_parser::parse_joint_binding
	(const xml_node& skin,
	 array<matrix>& inv_bind_matrices,
	 bimap<string, int64>& joint_name_to_index_xml)
{
	/**
	 * Parse all joint binding data within the \<joints\> of a \<skin\>
	 * and store for future use within remaining parsing process.
	 */

	const xml_node& joints =
		skin.child("joints");

	string joint_source_id;
	string inv_bind_matrix_source_id;

	for (xml_node::const_child_iterator it(joints.first_child()); it; ++it)
	{
		if (it->name() != "input") continue;
		if (!it->has_attribute("semantic")) continue;

		if (it->attribute("semantic") == "JOINT")
			joint_source_id = it->attribute("source").substring(1);

		if (it->attribute("semantic") == "INV_BIND_MATRIX")
			inv_bind_matrix_source_id = it->attribute("source").substring(1);
	}

	if (joint_source_id.empty() ||
		inv_bind_matrix_source_id.empty())
	{
		LOG_ERROR("Collada XML does not specify skeleton joints or bind matrices.");
		throw xml_exception();
	}

	parse_inv_bind_matrices
		(inv_bind_matrix_source_id, inv_bind_matrices);
	parse_joint_name_to_index_xml
		(joint_source_id, joint_name_to_index_xml);

	if (inv_bind_matrices.size() != joint_name_to_index_xml.size())
	{
		LOG_ERROR("There is not exactly one bind matrix per joint.");
		throw xml_exception();
	}
}


void collada_skeleton_parser::parse_inv_bind_matrices
	(const string& inv_bind_matrix_source_id,
	 array<matrix>& inv_bind_matrices)
{
	/**
	 * Fetch inverse binding matrices from Collada source
	 * with given ID and store into matrices array.
	 */

	string inv_bind_matrix_string
		(root_parser_.fetch_xml_node(inv_bind_matrix_source_id).
		 child("float_array").text());

	dynamic_buffer<float> inv_bind_matrices_float;
	collada_parser::split_to_buffer
		(inv_bind_matrix_string, inv_bind_matrices_float, &atof);

	if (inv_bind_matrices_float.size() % 16 != 0)
	{
		LOG_ERROR("There are not exactly 16 values per bind matrix.");
		throw xml_exception();
	}

	for (int64 i = 0; i < inv_bind_matrices_float.size(); i += 16)
	{
		float *src =
			inv_bind_matrices_float.data() + i;

		inv_bind_matrices.push_back
			(matrix(src[0], src[1], src[2], src[3], 
					src[4], src[5], src[6], src[7], 
					src[8], src[9], src[10], src[11], 
					src[12], src[13], src[14], src[15]));
	}
}


void collada_skeleton_parser::parse_joint_name_to_index_xml
	(const string& joint_source_id,
	 bimap<string, int64>& joint_name_to_index_xml)
{
	/**
	 * Fetch joint names from Collada skinning data
	 * and store map to XML-internal indices.
	 */

	string joint_names_string
		(root_parser_.fetch_xml_node(joint_source_id).
		 child("Name_array").text());

	list<string> joint_names;
	joint_names_string.compact().split(' ', joint_names);
	
	for (list<string>::const_iterator it(joint_names.first()); it; ++it)
		joint_name_to_index_xml.insert
			(*it, joint_name_to_index_xml.size());
}


void collada_skeleton_parser::parse_skeleton_rig
	(const xml_node& instance_controller,
	 const array<matrix>& inv_bind_matrices,
	 const bimap<string, int64>& joint_name_to_index_xml,			 
	 bimap<string, int64>& joint_name_to_index_skeleton)
{
	/**
	 * On first \<instance_controller\>, parse all referenced joints,
	 * build a skeleton from these, and finally attach the skeleton
	 * to the result model.
	 *
	 * Any later controllers must reuse the existing skeleton.
	 * Checking for reuse is not trivial, as some skeleton state
	 * is not stored within the Collada joint nodes, but instead
	 * depends on the Collada controller (e.g. binding matrices).
	 * Therefore, we reparse skeleton joints on each instance,
	 * then check if we end up with the same joints as before.
	 */

	string root_joint_node_id
		(instance_controller.child("skeleton").text().substring(1));
	const xml_node& root_joint_node =
		root_parser_.fetch_xml_node(root_joint_node_id);

	skeleton_joint_array joints;
	parse_joints
		(root_joint_node, inv_bind_matrices,
		 joint_name_to_index_xml,
		 joint_name_to_index_skeleton,
		 joints);

	const skeleton *reuse_skeleton =
		root_parser_.result_model.skeleton_rig();

	if (reuse_skeleton)
	{
		if (skeleton_root_joint_node_id_ != root_joint_node_id)
		{
			LOG_ERROR("All controllers must use the same skeleton root.");
			throw unsupported_feature();
		}

		check_joints_reuse(joints, reuse_skeleton->joints());
		return;
	}

	skeleton_root_joint_node_id_ = root_joint_node_id;

	auto_pointer<skeleton> skeleton_rig(new skeleton(joints));
	root_parser_.result_model.set_skeleton_rig
		(std::move(skeleton_rig));
}


void collada_skeleton_parser::parse_joints
	(const xml_node& root_joint_node,
	 const array<matrix>& inv_bind_matrices,
	 const bimap<string, int64>& joint_name_to_index_xml,
	 bimap<string, int64>& joint_name_to_index_skeleton,
	 skeleton_joint_array& joints)
{
	/**
	 * Implementation notes:
	 * - Sequential children indices within a skeleton_joint_array
	 *		require width-first traversal of skeleton nodes.
	 * - We use an open list to store nodes pending for expansion,
	 *		and a parent list to remember original parent indices.
	 * - Since skeleton_joint constructors require valid children,
	 *		we must track where children will later be placed
	 *		within our joint array.
	 */

	list<const xml_node*> open_nodes;
	open_nodes.insert(&root_joint_node);

	list<int64> parent_joints;
	parent_joints.insert(skeleton::invalid_index);

	int64 next_free_child = 1;

	while (!open_nodes.empty())
	{
		// Extract pending node from open list.
		const xml_node& node = **open_nodes.first();
		open_nodes.pop_front();

		int64 parent_joint = *parent_joints.first();
		parent_joints.pop_front();


		// Directly map Collada node IDs to joint names.
		// These IDs must be unique, and if they are not,
		// then the Collada file is corrupt. In this case,
		// our index maps will fill with inconsistent data,
		// and later skeleton construction will throw.
		const string& joint_name = node.attribute("id");


		// Push children to node list for later expansion.
		int64 child_count = 0;
		for (xml_node::const_child_iterator child_node_it
				(node.first_child()); child_node_it; ++child_node_it)
		{
			if (child_node_it->name() == "node" &&
				child_node_it->has_attribute("type") &&
				child_node_it->attribute("type") == "JOINT")
			{
				open_nodes.push_back(&(*child_node_it));
				parent_joints.push_back(joints.size());

				if (child_count + 1 > collada_parser_limits::max_children_per_joint)
				{
					LOG_ERROR("Joint has too many children in Collada file.");
					throw bounds_exceeded();
				}

				++child_count;
			}
		}


		// Reserve indices of later children, if any.
		// Abort if these exceed allowed joint counts.
		int64 first_child =
			skeleton::invalid_index;

		if (child_count != 0)
		{
			if (!integer_checks::is_add_safe(next_free_child, child_count) ||
				next_free_child + child_count > collada_parser_limits::max_joint_count)
			{
				LOG_ERROR("Joint count exceeds maximum in Collada file.");
				throw bounds_exceeded();
			}

			first_child = next_free_child;
			next_free_child += child_count;
		}


		// Build transform from joint to parent joint.
		// On root joint, also consider skeleton position
		// relative to origin of model and axis semantic.
		si_transform joint_to_parent
			(scene_parser_.build_local_transform(node));

		if (parent_joint == skeleton::invalid_index)
		{
			if (node.parent())
			{
				joint_to_parent =
					scene_parser_.build_absolute_transform(*node.parent()) *
					joint_to_parent;
			}			

			joint_to_parent =
				scene_parser_.build_semantic_transform() *
				joint_to_parent;
		}


		// Fetch transformation from vertices
		// to joint-space within binding pose.
		// See collada_skeleton_parser.
		bimap<string, int64>::const_r_iterator find_xml_joint_index
			(joint_name_to_index_xml.find_r_for_l(joint_name));

		if (!find_xml_joint_index)
		{
			LOG_ERROR("Joint node does not have an associated binding matrix.");
			throw xml_exception();
		}

		matrix binding_to_joint_matrix
			(inv_bind_matrices.at(*find_xml_joint_index));

		if (!reported_unsupported_binding_matrix_ &&
			!si_transform::representable(binding_to_joint_matrix))
		{
			LOG_WARN("Joint binding matrix is not a si_transform. "
					 "Animation will be distorted.");
			reported_unsupported_binding_matrix_ = true;
		}
		
		si_transform binding_to_joint
			(si_transform::from_matrix(binding_to_joint_matrix) *
			 scene_parser_.build_semantic_transform().invert());
	

		// Create joint from accumulated data
		// and store skeleton-internal joint index.
		joints.push_back
			(skeleton_joint
				(joint_name,
				 parent_joint, first_child, child_count,
				 joint_to_parent, binding_to_joint));

		joint_name_to_index_skeleton.insert
			(joint_name, joints.size() - 1);
	}
}


void collada_skeleton_parser::check_joints_reuse
	(const skeleton_joint_array& joints,
	 const skeleton_joint_array& reuse_joints)
{
	/**
	 * Check if two joint arrays match
	 * for reusing an existing skeleton.
	 */

	if (reuse_joints.size() != joints.size())
	{
		LOG_ERROR("Skeleton joint count not constant over controllers.");
		throw unsupported_feature();
	}

	for (int64 i = 0; i < joints.size(); ++i)
	{
		if (reuse_joints[i].name != joints[i].name ||
			reuse_joints[i].parent != joints[i].parent ||
			reuse_joints[i].first_child != joints[i].first_child ||
			reuse_joints[i].child_count != joints[i].child_count)
		{
			LOG_ERROR("Skeleton joint hierarchy does not match over controllers.");
			throw unsupported_feature();
		}

		if (!reuse_joints[i].joint_to_parent_matrix.nearly_equal
				(joints[i].joint_to_parent_matrix) ||
			!reuse_joints[i].binding_to_joint_matrix.nearly_equal
				(joints[i].binding_to_joint_matrix))
		{
			LOG_ERROR("Skeleton joint transforms do not match over controllers.");
			throw unsupported_feature();
		}
	}
}


void collada_skeleton_parser::parse_vertex_binding
	(const xml_node& skin,
	 array<skeleton_index>& skeleton_indices,
	 array<skeleton_weight>& skeleton_weights,
	 const bimap<string, int64>& joint_name_to_index_xml,			 
	 const bimap<string, int64>& joint_name_to_index_skeleton)
{
	/**
	 * Parse all vertex binding data within the \<vertex_weights\> of a \<skin\>
	 * and store for future use within remaining parsing process.
	 */

	const xml_node& vertex_weights =
		skin.child("vertex_weights");


	// Fetch source IDs for joints and weights
	// as well as offsets of joint and weight indices
	// within the overall \<v\> index stream.
	bool found_joint_offset = false;
	int64 joint_offset = -1;
	bool found_weight_offset = false;
	int64 weight_offset = -1;
	int64 max_offset = -1;

	string weight_source_id;

	for (xml_node::const_child_iterator it(vertex_weights.first_child()); it; ++it)
	{
		if (it->name() != "input") continue;

		int64 offset = it->attribute("offset").to_int64();
		max_offset = max(max_offset, offset);

		if (!it->has_attribute("semantic")) continue;

		if (it->attribute("semantic") == "JOINT")
		{
			if (found_joint_offset)
			{
				LOG_ERROR("Duplicate joint semantics in vertex attributes.");
				throw xml_exception();
			}

			found_joint_offset = true;
			joint_offset = offset;
		}
		else if (it->attribute("semantic") == "WEIGHT")
		{
			if (found_weight_offset)
			{
				LOG_ERROR("Duplicate joint weight semantics in vertex attributes.");
				throw xml_exception();
			}

			found_weight_offset = true;
			weight_offset = offset;
			weight_source_id = it->attribute("source").substring(1);
		}
	}


	// Check resulting offsets for consistency.
	// Some checks (e.g. max_offset) have already been performed
	// by other parsing components (e.g. collada_geometry_parser),
	// but we include redundant checks for reusability.
	if (!found_joint_offset ||
		!found_weight_offset)
	{
		LOG_ERROR("Collada XML vertex weights do not have joint or weight indices.");
		throw xml_exception();
	}

	if (max_offset < 0)
	{
		LOG_ERROR("No valid input tag found in triangles or polylist.");
		throw xml_exception();
	}

	if (max_offset + 1 >
		collada_parser_limits::max_vertex_attribute_count)
	{
		LOG_ERROR("Too many vertex attributes per vertex.");
		throw bounds_exceeded();
	}

	if (joint_offset < 0 || joint_offset > max_offset ||
		weight_offset < 0 || weight_offset > max_offset)
	{
		LOG_ERROR("Offsets to joints or weights exceed allowed range.");
		throw xml_exception();
	}

	if (weight_source_id.empty())
	{
		LOG_ERROR("Collada XML does not reference vertex weight stream.");
		throw xml_exception();
	}

	const xml_node& weights_source =
		root_parser_.fetch_xml_node(weight_source_id);


	// Parse vertex weights and joint indices.
	// Finally, postprocess joint indices to use
	// skeleton indices instead of XML indices.
	parse_vertex_weights_and_joint_indices
		(vertex_weights, weights_source,
		 joint_offset, weight_offset, max_offset,
		 skeleton_indices, skeleton_weights);

	translate_xml_to_skeleton_joint_indices
		(skeleton_indices, skeleton_weights,
		 joint_name_to_index_xml,
		 joint_name_to_index_skeleton);
}


void collada_skeleton_parser::parse_vertex_weights_and_joint_indices
	(const xml_node& vertex_weights,
	 const xml_node& weights_source,
	 int64 joint_offset, int64 weight_offset, int64 max_offset,
	 array<skeleton_index>& skeleton_indices,
	 array<skeleton_weight>& skeleton_weights)
{
	/**
	 * Store vertex weights and XML indices of referenced joints
	 * within provided mesh_stream%s.
	 */

	dynamic_buffer<int64> vcount;
	collada_parser::split_to_buffer
		(vertex_weights.child("vcount").text(), vcount, &atoi);

	dynamic_buffer<int64> v;
	collada_parser::split_to_buffer
		(vertex_weights.child("v").text(), v, &atoi);

	dynamic_buffer<float> weights;
	collada_parser::split_to_buffer
		(weights_source.child("float_array").text(), weights, &atof);
	
	int64 current_v_index = 0;
	for (int64 i = 0; i < vcount.size(); ++i)
	{
		skeleton_index index;
		skeleton_weight weight; 


		// Fetch individual index-weight pairs for a single vertex. 
		// We potentially have to replace the lowest-weight joint
		// if there are more than max_joints_per_vertex.
		for (int64 j = 0; j < vcount[i]; ++j)
		{
			int64 target_index = j;
			if (target_index >= skeleton_index::max_joints_per_vertex)
			{
				float min_weight = weight.C[0];
				target_index = 0;
				for (int64 k = 1; k < skeleton_index::max_joints_per_vertex; ++k)
				{
					if (weight.C[k] > min_weight) continue;
					min_weight = weight.C[k];
					target_index = k;
				}
			}

			index.C[target_index] =
				v.at(current_v_index + joint_offset);
			weight.C[target_index] =
				weights.at(v.at(current_v_index + weight_offset));

			current_v_index += (max_offset + 1);
		}


		// Renormalize in case we had more joints
		// than max_joints_per_vertex allowed.
		float weight_sum = 0;
		for (int64 k = 0; k < skeleton_index::max_joints_per_vertex; ++k)
			weight_sum += weight.C[k];

		if (weight_sum > skeleton_weight_normalize_epsilon_)
		{
			for (int64 k = 0; k < skeleton_index::max_joints_per_vertex; ++k)
				weight.C[k] /= weight_sum;
		}


		// Warn on bindless vertices.
		// Typical rendering back-ends will not separately test
		// for vertices with only invalid joint index bindings.
		// Those vertices then always display at the model origin.
		if (vcount[i] == 0 ||
			weight_sum <= skeleton_weight_normalize_epsilon_)
		{
			if (!reported_unbound_vertex_)
			{
				LOG_WARN("Skin geometry contains at least one vertex without joint binding. "
						 "Bindless vertices should be in a skeleton-less geometry instance, "
						 "otherwise such vertices may transform incorrectly.");
				reported_unbound_vertex_ = true;
			}
		}


		// Store final per-vertex index-weight pairs.
		skeleton_indices.push_back(index);
		skeleton_weights.push_back(weight);
	}
}


void collada_skeleton_parser::translate_xml_to_skeleton_joint_indices
	(array<skeleton_index>& skeleton_indices,
	 const array<skeleton_weight>& skeleton_weights,
	 const bimap<string, int64>& joint_name_to_index_xml,			 
	 const bimap<string, int64>& joint_name_to_index_skeleton)
{
	/**
	 * Map @bref{skeleton_indices} from XML-internal indexing
	 * to the joint indexing used in model::skeleton_rig().
	 */


	// For efficiency reasons, create a fast lookup structure
	// that maps from XML indices to skeleton-internal indices.
	dynamic_buffer<int64> index_xml_to_index_skeleton;
	index_xml_to_index_skeleton.resize(joint_name_to_index_xml.size());

	for (int64 i = 0; i < index_xml_to_index_skeleton.size(); ++i)
	{
		bimap<string, int64>::const_l_iterator find_joint_name
			(joint_name_to_index_xml.find_l_for_r(i));

		if (!find_joint_name)
		{
			LOG_ERROR("XML joint indices are not linear.");
			throw assert_failed();
		}

		bimap<string, int64>::const_r_iterator find_index_skeleton
			(joint_name_to_index_skeleton.find_r_for_l(*find_joint_name));

		if (!find_index_skeleton)
		{
			LOG_ERROR("Mismatch between joint names in skeleton and XML.");
			throw assert_failed();
		}

		index_xml_to_index_skeleton[i] = *find_index_skeleton;
	}


	// Apply lookup structure to all joint indices.
	// Do not translate invalid_index bindings, which
	// differ from root joint bindings by a zero weight.
	for (int64 i = 0; i < skeleton_indices.size(); ++i)
	for (int64 j = 0; j < skeleton_index::max_joints_per_vertex; ++j)
	{
		int64 index_xml = skeleton_indices[i].C[j];

		if (index_xml < 0 ||
			index_xml >= index_xml_to_index_skeleton.size())
		{
			LOG_ERROR("Collada skeleton binding references invalid joint index.");
			throw xml_exception();
		}
	
		if (index_xml == skeleton_index::unused_index &&
			skeleton_weights[i].C[j] == 0.f)
				continue;
	
		skeleton_indices[i].C[j] =
			index_xml_to_index_skeleton[index_xml];
	}
}


void collada_skeleton_parser::parse_geometry
	(const xml_node& skin,
	 const array<skeleton_index>& skeleton_indices,
	 const array<skeleton_weight>& skeleton_weights)
{
	/**
	 * Instantiate the geometry source of the skeleton
	 * and attach to skin via joint weights and joint indices.
	 */

	string geometry_id
		(skin.attribute("source").substring(1));
	const xml_node& geometry =
		root_parser_.fetch_xml_node(geometry_id);

	matrix binding_shape_matrix;
	collada_parser::split_fixed_size
		(skin.child("bind_shape_matrix").text(),
		 binding_shape_matrix.c, &atof,
		 sizeof(binding_shape_matrix.c) /
			sizeof(binding_shape_matrix.c[0]));

	if (!si_transform::representable(binding_shape_matrix))
		LOG_WARN("Binding shape matrix is not representable by si_transform. "
				 "Model will end up deformed.");

	geometry_parser_.parse_skin_geometry
		(geometry, skeleton_indices, skeleton_weights,
		 si_transform::from_matrix(binding_shape_matrix));
}


const float collada_skeleton_parser::skeleton_weight_normalize_epsilon_ = 1e-5f;




} /* namespace viral_core */

/**
 *************************************************************************
 *
 * @file mesh.cpp
 *
 * Implementation for the general mesh class.
 *
 ************************************************************************/


#include "mesh.hpp"


#include <math.h>


#include "log.hpp"
#include "color.hpp"
#include "mesh_optimize.hpp"
#include "skeleton.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//	mesh_stream_registry 
//
//////////////////////////////////////////////////////////////////////////


const char* const mesh_stream_registry::vertex_stream_name = "vertices";
const char* const mesh_stream_registry::normal_stream_name = "normals";
const char* const mesh_stream_registry::color_stream_name = "colors";
const char* const mesh_stream_registry::texcoords_stream_name = "texcoords";
const char* const mesh_stream_registry::tangent_stream_name = "tangents";
const char* const mesh_stream_registry::bitangent_stream_name = "bitangents";
const char* const mesh_stream_registry::skeleton_index_stream_name = "skeleton_indices";
const char* const mesh_stream_registry::skeleton_weight_stream_name = "skeleton_weights";
const char* const mesh_stream_registry::plane_stream_name = "planes";




//////////////////////////////////////////////////////////////////////////
//
//	mesh
//
//////////////////////////////////////////////////////////////////////////


mesh::mesh()
	: bounding_radius_(0) { }


mesh::mesh(const mesh& other)
	:
	vertices_(other.vertices_),
	points_(other.points_),
	lines_(other.lines_),
	triangles_(other.triangles_),
	bounding_box_(other.bounding_box_),
	bounding_radius_(other.bounding_radius_)
{
	for (const_stream_iterator it(other.vertex_streams_.first()); it; ++it)
		vertex_streams_.insert(it->key, it->value->clone());

	for (const_stream_iterator it(other.triangle_streams_.first()); it; ++it)
		triangle_streams_.insert(it->key, it->value->clone());
}


mesh::mesh(mesh&& other) NOTHROW
	:
	bounding_radius_(0)
{
	// Swap ensures defaulted other.
	swap(*this, other);
}


auto_pointer<mesh> mesh::clone() const
	{ return auto_pointer<mesh>(new mesh(*this)); }


mesh& mesh::operator =(const mesh& other)
{
	if (&other == this) return *this;

	// Strong exception safety.
	mesh offhand_copy(other);
	swap(*this, offhand_copy);
	return *this;
}


mesh& mesh::operator =(mesh&& other) NOTHROW
{
	if (&other == this) return *this;

	// Exception safety: NOTHROW.
	mesh offhand_move(std::move(other));
	swap(*this, offhand_move);
	return *this;
}


void mesh::swap_with(mesh& other) NOTHROW
{
	swap(vertices_, other.vertices_);
	swap(points_, other.points_);
	swap(lines_, other.lines_);
	swap(triangles_, other.triangles_);

	swap(vertex_streams_, other.vertex_streams_);
	swap(triangle_streams_, other.triangle_streams_);

	swap(bounding_box_, other.bounding_box_);
	swap(bounding_radius_, other.bounding_radius_);
}


mesh_stream_vector& mesh::allocate_vertices(int64 vertex_count)
{
	vertices_.resize(vertex_count);
	return vertices_;
}

void mesh::set_vertices(const mesh_stream_vector& new_vertices)
	{ vertices_ = new_vertices; }
void mesh::set_vertices(mesh_stream_vector&& new_vertices) NOTHROW
	{ vertices_ = std::move(new_vertices); }

mesh_stream_vector& mesh::vertices() NOTHROW
	{ return vertices_; }
const mesh_stream_vector& mesh::vertices() const NOTHROW
	{ return vertices_; }
int64 mesh::vertex_count() const NOTHROW
	{ return vertices_.size(); }


mesh_stream_point& mesh::allocate_points(int64 point_count)
{
	points_.resize(point_count);
	return points_;
}

void mesh::set_points(const mesh_stream_point& new_points)
	{ points_ = new_points; }
void mesh::set_points(mesh_stream_point&& new_points) NOTHROW
	{ points_ = std::move(new_points); }

mesh_stream_point& mesh::points() NOTHROW
	{ return points_; }
const mesh_stream_point& mesh::points() const NOTHROW
	{ return points_; }
int64 mesh::point_count() const NOTHROW
	{ return points_.size(); }


mesh_stream_line& mesh::allocate_lines(int64 line_count)
{
	lines_.resize(line_count);
	return lines_;
}

void mesh::set_lines(const mesh_stream_line& new_lines)
	{ lines_ = new_lines; }
void mesh::set_lines(mesh_stream_line&& new_lines) NOTHROW
	{ lines_ = std::move(new_lines); }

mesh_stream_line& mesh::lines() NOTHROW
	{ return lines_; }
const mesh_stream_line& mesh::lines() const NOTHROW
	{ return lines_; }
int64 mesh::line_count() const NOTHROW
	{ return lines_.size(); }


mesh_stream_triangle& mesh::allocate_triangles(int64 triangle_count)
{
	triangles_.resize(triangle_count);
	return triangles_;
}

void mesh::set_triangles(const mesh_stream_triangle& new_triangles)
	{ triangles_ = new_triangles; }
void mesh::set_triangles(mesh_stream_triangle&& new_triangles) NOTHROW
	{ triangles_ = std::move(new_triangles); }

mesh_stream_triangle& mesh::triangles() NOTHROW
	{ return triangles_; }
const mesh_stream_triangle& mesh::triangles() const NOTHROW
	{ return triangles_; }
int64 mesh::triangle_count() const NOTHROW
	{ return triangles_.size(); }


mesh::stream_iterator mesh::insert_vertex_stream
	(const string& name, auto_pointer<mesh_stream> stream)
{
	if (!stream)
	{
		LOG_ERROR("Can't insert NULL vertex stream into mesh.");
		throw null_pointer();
	}

	return vertex_streams_.insert(name, std::move(stream));
}


mesh::stream_iterator mesh::first_vertex_stream()
	{ return vertex_streams_.first(); }
mesh::const_stream_iterator mesh::first_vertex_stream() const
	{ return vertex_streams_.first(); }
mesh::stream_iterator mesh::find_vertex_stream(const string& name)
	{ return vertex_streams_.find(name); }
mesh::const_stream_iterator mesh::find_vertex_stream(const string& name) const
	{ return vertex_streams_.find(name); }
void mesh::remove_vertex_stream(const string& name) NOTHROW
	{ vertex_streams_.remove(name); }
int64 mesh::vertex_stream_count() const
	{ return vertex_streams_.size(); }


mesh::stream_iterator mesh::insert_triangle_stream
	(const string& name, auto_pointer<mesh_stream> stream)
{
	if (!stream)
	{
		LOG_ERROR("Can't insert NULL triangle stream into mesh.");
		throw null_pointer();
	}

	return triangle_streams_.insert(name, std::move(stream));
}


mesh::stream_iterator mesh::first_triangle_stream()
	{ return triangle_streams_.first(); }
mesh::const_stream_iterator mesh::first_triangle_stream() const
	{ return triangle_streams_.first(); }
mesh::stream_iterator mesh::find_triangle_stream(const string& name)
	{ return triangle_streams_.find(name); }
mesh::const_stream_iterator mesh::find_triangle_stream(const string& name) const
	{ return triangle_streams_.find(name); }
void mesh::remove_triangle_stream(const string& name) NOTHROW
	{ triangle_streams_.remove(name); }
int64 mesh::triangle_stream_count() const
	{ return triangle_streams_.size(); }


mesh_stream_vector& mesh::allocate_normals()
	{ return allocate_vertex_stream<mesh_stream_vector>
				(mesh_stream_registry::normal_stream_name); }
mesh_stream_color& mesh::allocate_colors()
	{ return allocate_vertex_stream<mesh_stream_color>
				(mesh_stream_registry::color_stream_name); }
mesh_stream_vector& mesh::allocate_texcoords()
	{ return allocate_vertex_stream<mesh_stream_vector>
				(mesh_stream_registry::texcoords_stream_name); }
mesh_stream_vector& mesh::allocate_tangents()
	{ return allocate_vertex_stream<mesh_stream_vector>
				(mesh_stream_registry::tangent_stream_name); }
mesh_stream_vector& mesh::allocate_bitangents()
	{ return allocate_vertex_stream<mesh_stream_vector>
				(mesh_stream_registry::bitangent_stream_name); }

mesh_stream_skeleton_index& mesh::allocate_skeleton_indices()
	{ return allocate_vertex_stream<mesh_stream_skeleton_index>
				(mesh_stream_registry::skeleton_index_stream_name); }
mesh_stream_skeleton_weight& mesh::allocate_skeleton_weights()
	{ return allocate_vertex_stream<mesh_stream_skeleton_weight>
				(mesh_stream_registry::skeleton_weight_stream_name); }

mesh_stream_plane& mesh::allocate_planes()
	{ return allocate_triangle_stream<mesh_stream_plane>
				(mesh_stream_registry::plane_stream_name); }


bool mesh::has_normals() const
	{ return find_vertex_stream(mesh_stream_registry::normal_stream_name); }
bool mesh::has_colors() const
	{ return find_vertex_stream(mesh_stream_registry::color_stream_name); }
bool mesh::has_texcoords() const
	{ return find_vertex_stream(mesh_stream_registry::texcoords_stream_name); }
bool mesh::has_tangents() const
	{ return find_vertex_stream(mesh_stream_registry::tangent_stream_name); }
bool mesh::has_bitangents() const
	{ return find_vertex_stream(mesh_stream_registry::bitangent_stream_name); }	
bool mesh::has_skeleton_indices() const
	{ return find_vertex_stream(mesh_stream_registry::skeleton_index_stream_name); }
bool mesh::has_skeleton_weights() const
	{ return find_vertex_stream(mesh_stream_registry::skeleton_weight_stream_name); }
bool mesh::has_planes() const
	{ return find_triangle_stream(mesh_stream_registry::plane_stream_name); }


mesh_stream_vector& mesh::normals()
	{ return vertex_stream<mesh_stream_vector>(mesh_stream_registry::normal_stream_name); }
const mesh_stream_vector& mesh::normals() const
	{ return vertex_stream<mesh_stream_vector>(mesh_stream_registry::normal_stream_name); }
mesh_stream_color& mesh::colors()
	{ return vertex_stream<mesh_stream_color>(mesh_stream_registry::color_stream_name); }
const mesh_stream_color& mesh::colors() const
	{ return vertex_stream<mesh_stream_color>(mesh_stream_registry::color_stream_name); }
mesh_stream_vector& mesh::texcoords()
	{ return vertex_stream<mesh_stream_vector>(mesh_stream_registry::texcoords_stream_name); }
const mesh_stream_vector& mesh::texcoords() const
	{ return vertex_stream<mesh_stream_vector>(mesh_stream_registry::texcoords_stream_name); }
mesh_stream_vector& mesh::tangents()
	{ return vertex_stream<mesh_stream_vector>(mesh_stream_registry::tangent_stream_name); }
const mesh_stream_vector& mesh::tangents() const
	{ return vertex_stream<mesh_stream_vector>(mesh_stream_registry::tangent_stream_name); }
mesh_stream_vector& mesh::bitangents()
	{ return vertex_stream<mesh_stream_vector>(mesh_stream_registry::bitangent_stream_name); }
const mesh_stream_vector& mesh::bitangents() const
	{ return vertex_stream<mesh_stream_vector>(mesh_stream_registry::bitangent_stream_name); }


mesh_stream_skeleton_index& mesh::skeleton_indices()
{
	return vertex_stream<mesh_stream_skeleton_index>
		(mesh_stream_registry::skeleton_index_stream_name);
}

const mesh_stream_skeleton_index& mesh::skeleton_indices() const
{
	return vertex_stream<mesh_stream_skeleton_index>
		(mesh_stream_registry::skeleton_index_stream_name);
}

mesh_stream_skeleton_weight& mesh::skeleton_weights()
{
	return vertex_stream<mesh_stream_skeleton_weight>
		(mesh_stream_registry::skeleton_weight_stream_name);
}

const mesh_stream_skeleton_weight& mesh::skeleton_weights() const
{
	return vertex_stream<mesh_stream_skeleton_weight>
		(mesh_stream_registry::skeleton_weight_stream_name);
}


mesh_stream_plane& mesh::planes()
	{ return triangle_stream<mesh_stream_plane>(mesh_stream_registry::plane_stream_name); }
const mesh_stream_plane& mesh::planes() const
	{ return triangle_stream<mesh_stream_plane>(mesh_stream_registry::plane_stream_name); }


const box& mesh::bounding_box() const NOTHROW
	{ return bounding_box_; }
float mesh::bounding_radius() const NOTHROW
	{ return bounding_radius_; }


void mesh::rebuild_boundings()
{
	if (vertices_.size() < 1)
	{
		bounding_box_ = box();
		bounding_radius_ = 0;
		return;
	}

	int64 vcount = vertices_.size();

	bounding_box_ = box(vertices_[0], vertices_[0]);
	bounding_radius_ = vertices_[0].length(); 

	for (int64 i = 1; i < vcount; ++i)
	{
		bounding_box_.grow_to(vertices_[i]);

		float len =	vertices_[i].length();
		if (len > bounding_radius_) bounding_radius_ = len;
	}
}


void mesh::optimize()
{
	mesh_optimize::remove_unused_vertices(*this);
	mesh_optimize::merge_duplicate_vertices(*this);
	mesh_optimize::remove_degenerate_lines(*this);
	mesh_optimize::remove_degenerate_triangles(*this);
}


void mesh::validate() const
{
	int64 vcount = vertices_.size();
	int64 pcount = points_.size();
	int64 lcount = lines_.size();
	int64 tcount = triangles_.size();


	// Vertex indices in points.
	{
		for (int64 i = 0; i < pcount; ++i)
		{
			if (points_[i].i < 0 || points_[i].i >= vcount)
			{
				LOG_ERROR("Mesh point references invalid vertex index.");
				throw validation_failed();
			}	
		}
	}


	// Vertex indices in lines.
	{
		for (int64 i = 0; i < lcount; ++i)
		for (int64 j = 0; j < 2; ++j)
		{
			if (lines_[i].C[j] < 0 || lines_[i].C[j] >= vcount)
			{
				LOG_ERROR("Mesh line references invalid vertex index.");
				throw validation_failed();
			}
		}
	}


	// Vertex indices in triangles.
	{
		for (int64 i = 0; i < tcount; ++i)
		for (int64 j = 0; j < 3; ++j)
		{
			if (triangles_[i].C[j] < 0 || triangles_[i].C[j] >= vcount)
			{
				LOG_ERROR("Mesh triangle references invalid vertex index.");
				throw validation_failed();
			}
		}
	}


	// Length of optional streams.	
	{
		for (const_stream_iterator it(vertex_streams_.first()); it; ++it)
		{
			if (it->value->size() != vcount)
			{
				LOG_ERROR("Size mismatch in-between vertices and vertex streams.");
				throw validation_failed();
			}
		}

		for (const_stream_iterator it(triangle_streams_.first()); it; ++it)
		{
			if (it->value->size() != tcount)
			{
				LOG_ERROR("Size mismatch in-between triangles and triangle streams.");
				throw validation_failed();
			}
		}
	}
}


void mesh::validate(const skeleton* skeleton_rig) const
{
	validate();

	if (skeleton_rig &&
		has_skeleton_indices())
	{
		const mesh_stream_skeleton_index& indices =
			skeleton_indices();

		for (int64 i = 0; i < indices.size(); ++i)
		for (int64 j = 0; j < skeleton_index::max_joints_per_vertex; ++j)
		{
			// Note it is not possible to create a skeleton
			// without any joints, hence we need not check
			// for invalid_index entries explicitly.
			if (indices[i].C[j] < 0 ||
				indices[i].C[j] >= skeleton_rig->joints().size())
			{
				LOG_ERROR("Mesh vertex references invalid joint index.");
				throw validation_failed();
			}
		}
	}
}


void mesh::clear() NOTHROW
{
	vertices_.clear();
	points_.clear();
	lines_.clear();
	triangles_.clear();
	vertex_streams_.clear();
	triangle_streams_.clear();
	bounding_box_ = box();
	bounding_radius_ = 0;
}


void mesh::throw_not_found
	(const string& stream_type, const string& name)
{
	LOG_ERROR("Mesh does not contain a " + stream_type + " with name: " + name);
	throw not_found();
}


void mesh::throw_type_mismatch
	(const string& stream_type, const string& name)
{
	LOG_ERROR("A " + stream_type + " with name: " + name + " exists, but has mismatching type.");
	throw type_mismatch();
}




} /* namespace viral_core */

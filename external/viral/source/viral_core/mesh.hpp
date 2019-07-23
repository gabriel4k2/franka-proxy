/**
 *************************************************************************
 *
 * @file mesh.hpp
 *
 * Main header for the general mesh class.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MESH_HPP)
#define INCLUDED__VIRAL_CORE__MESH_HPP


#include "build_config.hpp"


#include "string_forward.hpp"
#include "image_forward.hpp"
#include "mesh_forward.hpp"


#include "util.hpp"
#include "auto_pointer.hpp"
#include "buffer.hpp"
#include "string.hpp"
#include "geo_vector.hpp"
#include "geo_primitive.hpp"
#include "geo_volume.hpp"
#include "owning_map.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup mesh_module Mesh Module
 * @ingroup geo_module
 *
 * Manages polygonal 3D geometry within memory, in particular
 * for later use in rendering, animation and collision detection.
 *
 ************************************************************************/


/**
 *************************************************************************
 *
 * @class mesh_point
 * @ingroup mesh_module
 *
 * An explicit point as part of a @ref mesh,
 * defined by an index into the vertex stream.
 *
 * Implementation notes:
 * - We need a separate mesh_point class (instead of a simple int64)
 *		to distinguish between mesh_stream_point and mesh_stream_int.
 *
 ************************************************************************/
class mesh_point
{
	public:

		mesh_point() NOTHROW
			: i(0) { }

		mesh_point(int64 i) NOTHROW
			: i(i) { }

		union
		{
			int64 C[1];
			struct { int64 i; };
		};
};




/**
 *************************************************************************
 *
 * @class mesh_line
 * @ingroup mesh_module
 *
 * Defines a line within a @ref mesh by two indices
 * into the vertex stream.
 *
 ************************************************************************/
class mesh_line
{
	public:

		mesh_line() NOTHROW 
			: i(0), j(0) { }

		mesh_line(int64 i, int64 j) NOTHROW 
			: i(i), j(j) { }

		union
		{
			int64 C[2];
			struct { int64 i, j; };
		};
};




/**
 *************************************************************************
 *
 * @class mesh_triangle
 * @ingroup mesh_module
 *
 * Defines a triangle within a @ref mesh by three indices
 * into the vertex stream.
 *
 * Usage notes:
 * - See @ref triangle for geometric conventions (e.g. winding order).
 *
 ************************************************************************/
class mesh_triangle
{
	public:

		mesh_triangle() NOTHROW 
			: i(0), j(0), k(0) { }

		mesh_triangle(int64 i, int64 j, int64 k) NOTHROW 
			: i(i), j(j), k(k) { }

		union
		{
			int64 C[3];
			struct { int64 i, j, k; };
		};
};




/**
 *************************************************************************
 *
 * @class mesh_stream
 * @ingroup mesh_module
 *
 * Defines an arbitrary data stream within a @ref mesh.
 *
 ************************************************************************/
class mesh_stream
{
	public:

		mesh_stream() { }
		virtual ~mesh_stream() NOTHROW { }

		/** Retrieve number of entries in this stream. */
		virtual int64 size() const NOTHROW = 0;

		/** Retrieve the size of a single entry in the stream. */
		virtual int64 bytes_per_entry() const NOTHROW = 0;

		/** Retrieve untyped pointer to internal stream data. */
		virtual const void* data_untyped() const NOTHROW = 0;

		/** Create deep copy of stream instance. */
		virtual auto_pointer<mesh_stream> clone() const = 0;
};




/**
 *************************************************************************
 *
 * @class mesh_stream_typed
 * @ingroup mesh_module
 *
 * A mesh_stream that stores data of type @bref{T}.
 *
 * Usage notes:
 * - Mimics behaviour of encapsulated dynamic_buffer.
 *
 ************************************************************************/
template <typename T>
	class mesh_stream_typed : public mesh_stream
{
	public:

		mesh_stream_typed() NOTHROW { }


		/**
		 * Copy or move construct from @bref{other} mesh stream.
		 * Move construction leaves a valid, empty source.
		 */
		//@{		
		mesh_stream_typed(const mesh_stream_typed& other)
			:
			buffer_(other.buffer_)
		{ }

		mesh_stream_typed(mesh_stream_typed&& other) NOTHROW
		{
			// Swap ensures defaulted other.
			swap(*this, other);
		}
		//@}


		auto_pointer<mesh_stream> clone() const override
			{ return auto_pointer<mesh_stream>(new mesh_stream_typed(*this)); }


		/**
		 * Copy or move assign of @bref{other} mesh stream.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty source.
		 */
		//@{
		mesh_stream_typed& operator =
			(const mesh_stream_typed& other)
		{
			if (&other == this) return *this;

			// Strong exception safety.
			mesh_stream_typed offhand_copy(other);
			swap(*this, offhand_copy);
			return *this;
		}

		mesh_stream_typed& operator =(mesh_stream_typed&& other) NOTHROW
		{
			if (&other == this) return *this;

			// Exception safety: NOTHROW.
			mesh_stream_typed offhand_move(std::move(other));
			swap(*this, offhand_move);
			return *this;
		}
		//@}


		void swap_with(mesh_stream_typed& other) NOTHROW
			{ swap(buffer_, other.buffer_); }


		int64 size() const NOTHROW override
			{ return buffer_.size(); }
		int64 bytes_per_entry() const NOTHROW override
			{ return sizeof(T); }
		const void* data_untyped() const NOTHROW override
			{ return buffer_.data(); }

		void reserve(int64 new_capacity)
			{ buffer_.reserve(new_capacity); }
		void resize(int64 new_size)
			{ buffer_.resize(new_size); }

		void push_back(const T& value)
			{ buffer_.push_back(value); }
		void pop_back() NOTHROW
			{ buffer_.pop_back(); }

		T* data() NOTHROW
			{ return buffer_.data(); }
		const T* data() const NOTHROW
			{ return buffer_.data(); }

		const T& at(int64 index) const
			{ return buffer_.at(index); }
		T& at(int64 index)
			{ return buffer_.at(index); }

		const T& operator [](int64 index) const NOTHROW
			{ return buffer_[index]; }
		T& operator [](int64 index) NOTHROW
			{ return buffer_[index]; }

		void clear() NOTHROW
			{ buffer_.clear(); }
		void free() NOTHROW
			{ buffer_.free(); }


	private:

		typedef dynamic_buffer<T> data_buffer;
		data_buffer buffer_;
};




/**
 * Various predefined data stream classes.
 * @ingroup mesh_module
 */
//@{
typedef mesh_stream_typed<int32> mesh_stream_int32;
typedef mesh_stream_typed<uint32> mesh_stream_uint32;
typedef mesh_stream_typed<float> mesh_stream_float;

typedef mesh_stream_typed<color> mesh_stream_color;
typedef mesh_stream_typed<vector> mesh_stream_vector;
typedef mesh_stream_typed<plane> mesh_stream_plane;

typedef mesh_stream_typed<mesh_point> mesh_stream_point;
typedef mesh_stream_typed<mesh_line> mesh_stream_line;
typedef mesh_stream_typed<mesh_triangle> mesh_stream_triangle;

typedef mesh_stream_typed<skeleton_index> mesh_stream_skeleton_index;
typedef mesh_stream_typed<skeleton_weight> mesh_stream_skeleton_weight;
//@}




/**
 *************************************************************************
 *
 * @class mesh_stream_registry
 * @ingroup mesh_module
 *
 * Holds name and class suggestions for common mesh_stream%s.
 *
 ************************************************************************/
class mesh_stream_registry
{

	public:

		/**
		 * Reserved name for mesh::vertices().
		 * Note vertices are not part of the per-vertex stream list,
		 * but instead are always present as a direct member.
		 */
		static const char* const vertex_stream_name;


		/** A mesh_stream_vector with per-vertex normals. */
		static const char* const normal_stream_name;

		/** A mesh_stream_color with per-vertex material colors. */
		static const char* const color_stream_name;


		/**
		 * A mesh_stream_vector with per-vertex texture coordinates.
		 * The upper-left corner of any 2D uv texture is at (0, 0, 0),
		 * the lower-right corner is at (1, 1, 0). For 3D uvw textures,
		 * w maps into z, (0, 0, 1), or surface normal direction.
		 * This complies to image_rgba and @ref camera conventions.
		 */
		static const char* const texcoords_stream_name;


		/**
		 * The mesh_stream_vector%s with the tangential basis
		 * for the base texture (i.e. texcoords_stream_name).
		 */
		//@{
		static const char* const tangent_stream_name;
		static const char* const bitangent_stream_name;
		//@}


		/**
		 * Mesh streams that hold skeleton_joint indices and weights.
		 * See mesh_stream_skeleton_index and mesh_stream_skeleton_weight.
		 */
		//@{
		static const char* const skeleton_index_stream_name;
		static const char* const skeleton_weight_stream_name;
		//@}


		/** A mesh_stream_plane with per-triangle plane equations. */
		static const char* const plane_stream_name;
};




/**
 *************************************************************************
 *
 * @class mesh
 * @ingroup mesh_module
 *
 * Contains 3D geometry and associated data streams.
 *
 * Data structure:
 * - Each @ref mesh stores geometry in the form of mesh_stream%s.
 * - Mandatory mesh_streams hold vertex coordinates and indices
 *		for geometric primitives (e.g. lines or triangles).
 * - Optional, uniquely named mesh_streams with additional per-vertex
 *		or per-triangle data (e.g. vertex normals or triangle planes)
 *		can be attached to each mesh. See the mesh_stream_registry
 *		for suggested stream names and stream classes.
 *
 ************************************************************************/
class mesh
{

	public:

		typedef owning_map<string, mesh_stream>::iterator stream_iterator;
		typedef owning_map<string, mesh_stream>::const_iterator const_stream_iterator;


		/** Create mesh without any geometry. */
		mesh();


		/**
		 * Copy or move construct from @bref{other} mesh.
		 * Move construction leaves a valid, empty source.
		 */
		//@{
		mesh(const mesh& other);
		mesh(mesh&& other) NOTHROW;
		//@}


		/** Create not-@NULL heap copy of mesh and all its streams. */
		auto_pointer<mesh> clone() const;


		/**
		 * Copy or move assign of @bref{other} mesh.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty source.
		 */
		//@{
		mesh& operator =(const mesh& other);
		mesh& operator =(mesh&& other) NOTHROW;
		//@}


		void swap_with(mesh& other) NOTHROW;


		mesh_stream_vector& allocate_vertices(int64 vertex_count);
		void set_vertices(const mesh_stream_vector& new_vertices);
		void set_vertices(mesh_stream_vector&& new_vertices) NOTHROW;
		mesh_stream_vector& vertices() NOTHROW;
		const mesh_stream_vector& vertices() const NOTHROW;
		int64 vertex_count() const NOTHROW;

		mesh_stream_point& allocate_points(int64 point_count);
		void set_points(const mesh_stream_point& new_points);
		void set_points(mesh_stream_point&& new_points) NOTHROW;
		mesh_stream_point& points() NOTHROW;
		const mesh_stream_point& points() const NOTHROW;
		int64 point_count() const NOTHROW;

		mesh_stream_line& allocate_lines(int64 line_count);
		void set_lines(const mesh_stream_line& new_lines);
		void set_lines(mesh_stream_line&& new_lines) NOTHROW;
		mesh_stream_line& lines() NOTHROW;
		const mesh_stream_line& lines() const NOTHROW;
		int64 line_count() const NOTHROW;
		
		mesh_stream_triangle& allocate_triangles(int64 triangle_count);
		void set_triangles(const mesh_stream_triangle& new_triangles);
		void set_triangles(mesh_stream_triangle&& new_triangles) NOTHROW;
		mesh_stream_triangle& triangles() NOTHROW;
		const mesh_stream_triangle& triangles() const NOTHROW;
		int64 triangle_count() const NOTHROW;
		

		/**
		 * Take ownership over not-@NULL @bref{name}d @bref{stream}
		 * with additional per-vertex data. The stream must contain
		 * one entry per vertex in the mesh. Replaces any existing
		 * vertex stream of same name.
		 */
		stream_iterator insert_vertex_stream
			(const string& name, auto_pointer<mesh_stream> stream);


		/**
		 * Allocate per-vertex mesh_stream of type @bref{T}
		 * with vertex_count() uninitialized entries and
		 * insert_vertex_stream() under given @bref{name}.
		 */
		template <typename T>
			T& allocate_vertex_stream(const string& name)
		{
			auto_pointer<T> stream(new T());
			stream->resize(vertex_count());

			T& result = *stream;
			vertex_streams_.insert
				(name, std::move(stream));

			return result;
		}


		stream_iterator first_vertex_stream();					   
		const_stream_iterator first_vertex_stream() const;
		stream_iterator find_vertex_stream(const string& name);
		const_stream_iterator find_vertex_stream(const string& name) const;
		void remove_vertex_stream(const string& name) NOTHROW;
		int64 vertex_stream_count() const;


		/**
		 * Fetch per-vertex stream with @bref{name} and exact @bref{T}ype,
		 * or throw an exception if no such stream exists.
		 */
		//@{
		template <typename T> T& vertex_stream(const string& name)
		{
			stream_iterator it(find_vertex_stream(name));
			if (!it || !it->value)
				throw_not_found("vertex stream", name); 
			if (typeid(*it->value) != typeid(T))
				throw_type_mismatch("vertex stream", name);
			return static_cast<T&>(*it->value);
		}

		template <typename T> const T& vertex_stream(const string& name) const
		{
			const_stream_iterator it(find_vertex_stream(name));
			if (!it || !it->value)
				throw_not_found("vertex stream", name); 
			if (typeid(*it->value) != typeid(T))
				throw_type_mismatch("vertex stream", name);
			return static_cast<const T&>(*it->value);
		}
		//@}


		/**
		 * Take ownership over not-@NULL @bref{name}d @bref{stream}
		 * with additional per-triangle data. The stream must contain
		 * one entry per triangle in the mesh. Replaces any existing
		 * triangle stream of same name.
		 */
		stream_iterator insert_triangle_stream
			(const string& name, auto_pointer<mesh_stream> stream);


		/**
		 * Allocate per-triangle mesh_stream of type @bref{T}
		 * with triangle_count() uninitialized entries and
		 * insert_triangle_stream() under given @bref{name}.
		 */
		template <typename T>
			T& allocate_triangle_stream(const string& name)
		{
			auto_pointer<T> stream(new T());
			stream->resize(triangle_count());

			T& result = *stream;
			triangle_streams_.insert
				(name, std::move(stream));

			return result;
		}


		stream_iterator first_triangle_stream();
		const_stream_iterator first_triangle_stream() const;
		stream_iterator find_triangle_stream(const string& name);
		const_stream_iterator find_triangle_stream(const string& name) const;
		void remove_triangle_stream(const string& name) NOTHROW;
		int64 triangle_stream_count() const;


		/**
		 * Fetch per-triangle stream with @bref{name} and exact @bref{T}ype,
		 * or throw an exception if no such stream exists.
		 */
		//@{
		template <typename T> T& triangle_stream(const string& name)
		{
			stream_iterator it(find_triangle_stream(name));
			if (!it || !it->value)
				throw_not_found("triangle stream", name); 
			if (typeid(*it->value) != typeid(T))
				throw_type_mismatch("triangle stream", name);
			return static_cast<T&>(*it->value);
		}

		template <typename T> const T& triangle_stream(const string& name) const
		{
			const_stream_iterator it(find_triangle_stream(name));
			if (!it || !it->value)
				throw_not_found("triangle stream", name); 
			if (typeid(*it->value) != typeid(T))
				throw_type_mismatch("triangle stream", name);
			return static_cast<const T&>(*it->value);
		}
		//@}


		/**
		 * Allocate mesh_stream%s with names from mesh_stream_registry.
		 * Prefer mesh_util to add streams with reasonable initial values.
		 */
		//@{
		mesh_stream_vector& allocate_normals();
		mesh_stream_color& allocate_colors();
		mesh_stream_vector& allocate_texcoords();
		mesh_stream_vector& allocate_tangents();
		mesh_stream_vector& allocate_bitangents();

		mesh_stream_skeleton_index& allocate_skeleton_indices();
		mesh_stream_skeleton_weight& allocate_skeleton_weights();

		mesh_stream_plane& allocate_planes();
		//@}
		

		/**
		 * Check if the mesh contains some mesh_stream
		 * with a name from the mesh_stream_registry.
		 */
		//@{
		bool has_normals() const;
		bool has_colors() const;
		bool has_texcoords() const;
		bool has_tangents() const;
		bool has_bitangents() const;
			
		bool has_skeleton_indices() const;
		bool has_skeleton_weights() const;

		bool has_planes() const;
		//@}


		/**
		 * Fetch mesh_stream%s for names from the mesh_stream_registry,
		 * or throw an exception if the requested streams do not exist.
		 */
		//@{
		mesh_stream_vector& normals();
		const mesh_stream_vector& normals() const;
		mesh_stream_color& colors();
		const mesh_stream_color& colors() const;
		mesh_stream_vector& texcoords();
		const mesh_stream_vector& texcoords() const;
		mesh_stream_vector& tangents();
		const mesh_stream_vector& tangents() const;
		mesh_stream_vector& bitangents();
		const mesh_stream_vector& bitangents() const;

		mesh_stream_skeleton_index& skeleton_indices();
		const mesh_stream_skeleton_index& skeleton_indices() const;
		mesh_stream_skeleton_weight& skeleton_weights();
		const mesh_stream_skeleton_weight& skeleton_weights() const;

		mesh_stream_plane& planes();
		const mesh_stream_plane& planes() const;
		//@}


		/**
		 * Epsilon-less boundings over all mesh geometry.
		 * After any mesh updates, call rebuild_boundings()
		 * to recalculate bounding_box() and bounding_radius().
		 */
		//@{
		const box& bounding_box() const NOTHROW;
		float bounding_radius() const NOTHROW;
		void rebuild_boundings();
		//@}


		/**
		 * Optimize all geometry within this mesh,
		 * for instance by merging duplicate vertices
		 * and by removing degenerate triangles.
		 */
		void optimize();


		/**
		 * Validate all geometry within this mesh,
		 * including stream sizes and primitive indices.
		 * Throws if the mesh holds invalid geometry.
		 *
		 * Optionally, check skeleton streams against
		 * a @bref{skeleton_rig}.
		 */
		//@{
		void validate() const;
		void validate(const skeleton* skeleton_rig) const;
		//@}


		void clear() NOTHROW;


	private:

		static void throw_not_found
			(const string& stream_type, const string& name);
		static void throw_type_mismatch
			(const string& stream_type, const string& name);


		mesh_stream_vector vertices_;
		mesh_stream_point points_;
		mesh_stream_line lines_;
		mesh_stream_triangle triangles_;

		typedef owning_map<string, mesh_stream> stream_map;
		stream_map vertex_streams_; 
		stream_map triangle_streams_; 

		box bounding_box_;
		float bounding_radius_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MESH_HPP) */

/**
 *************************************************************************
 *
 * @file gl3_uniform.hpp
 *
 * OpenGL3 uniform variables, header.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_UNIFORM_HPP)
#define INCLUDED__VIRAL_CORE__GL3_UNIFORM_HPP


#include "build_config.hpp"
#include "gl3_config.hpp"


#include "gl3_forward.hpp"


#include "auto_pointer.hpp"
#include "owning_map.hpp"
#include "string.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_uniform
 * @ingroup gl3_render_module
 *
 * Represents a uniform variable as declared
 * in the source code of a gl3_program_object.
 *
 ************************************************************************/
class gl3_uniform
{
	friend class gl3_uniform_value;

	public:

		gl3_uniform
			(const gl3_api& gl3,
			 const string& internal_name,
			 GLsizei size, GLenum type, GLint location);

		const gl3_api& gl3;
		const string internal_name;
		const GLsizei size;
		const GLenum type;
		const GLint location;

		const GLenum apply_type;


	private:

		/**
		 * Apply a low-level @bref{value} or @bref{array}
		 * to this uniform after double-dispatch through
		 * gl3_uniform_value::apply_to().
		 *
		 * All variants throw if incoming types or sizes
		 * do not match with those of the target uniform.
		 * This avoids overflow for int64 to GLint sizes
		 * and circumvents automatic value conversions
		 * for improved debugging capabilities.
		 */
		//@{
		void apply(bool value) const;
		void apply(int32 value) const;
		void apply(float value) const;

		void apply(const color& value) const;
		void apply(const vector& value) const;
		void apply(const matrix& value) const;

		void apply(const int32* array, int64 count) const;
		void apply(const float* array, int64 count) const;
		void apply(const color* array, int64 count) const;
		void apply(const vector* array, int64 count) const;
		void apply(const matrix* array, int64 count) const;
		//@}


		static GLenum to_apply_type(GLenum type);
		
		void throw_apply_error_type
			(const string& incoming_type) const;
		void throw_apply_error_count
			(int64 incoming_count) const;
};




/**
 *************************************************************************
 *
 * @class gl3_uniform_value
 * @ingroup gl3_render_module
 *
 * A value for some uniform variable within an OpenGL 3 shader.
 *
 * Usage notes:
 * - @attent Arbitrary threads may use uniform values within
 *		their client-side data, e.g. in gl3_puppet_data.
 *		While covert_command%s copy or move values to enforce
 *		thread-safe data access, values still must not expose
 *		OpenGL functionality to clients.
 *
 ************************************************************************/
class gl3_uniform_value
{
	friend class gl3_program_object;
 
	public:

		virtual ~gl3_uniform_value() NOTHROW { }

		virtual auto_pointer<gl3_uniform_value> clone() const = 0;

	protected:

		template <typename T>
			void apply_value_to
				(const gl3_uniform& uniform, const T& value) const
					{ uniform.apply(value); }

		template <typename T>
			void apply_values_to
				(const gl3_uniform& uniform, const T* values, int64 count) const
					{ uniform.apply(values, count); }

	private:

		virtual void apply_to
			(const gl3_uniform& uniform) const = 0;
};




/**
 *************************************************************************
 *
 * @class gl3_uniform_value_typed
 * @ingroup gl3_render_module
 *
 * A gl3_uniform_value that holds a single value of some type @bref{T}.
 *
 ************************************************************************/
template <typename T>
	class gl3_uniform_value_typed : public gl3_uniform_value
{
	public:

		gl3_uniform_value_typed() { }
		gl3_uniform_value_typed(const T& value) : value(value) { }
		
		auto_pointer<gl3_uniform_value> clone() const override
		{
			return auto_pointer<gl3_uniform_value>
				(new gl3_uniform_value_typed(*this));
		}

		T value;

	private:

		void apply_to
			(const gl3_uniform& uniform) const override
				{ apply_value_to(uniform, value); }
};


/**
 * Various basic uniform types.
 * @ingroup gl3_render_module
 */
//@{
typedef gl3_uniform_value_typed<bool> gl3_uniform_value_bool;
typedef gl3_uniform_value_typed<int32> gl3_uniform_value_int32;
typedef gl3_uniform_value_typed<float> gl3_uniform_value_float;
typedef gl3_uniform_value_typed<color> gl3_uniform_value_color;
typedef gl3_uniform_value_typed<vector> gl3_uniform_value_vector;
typedef gl3_uniform_value_typed<matrix> gl3_uniform_value_matrix;
//@}




/**
 *************************************************************************
 *
 * @class gl3_uniform_value_array_typed
 * @ingroup gl3_render_module
 *
 * A gl3_uniform_value that holds @bref{count} values of type @bref{T}.
 *
 ************************************************************************/
template <typename T, int64 count>
	class gl3_uniform_value_array_typed : public gl3_uniform_value
{
	public:

		typedef T initializer_type;

		gl3_uniform_value_array_typed() { }
		
		auto_pointer<gl3_uniform_value> clone() const override
		{
			return auto_pointer<gl3_uniform_value>
				(new gl3_uniform_value_array_typed(*this));
		}

		T values[count];

	private:

		void apply_to
			(const gl3_uniform& uniform) const override
				{ apply_values_to(uniform, values, count); }
};




/**
 *************************************************************************
 *
 * @class gl3_uniform_value_map
 * @ingroup gl3_render_module
 *
 * Maps gl3_uniform_value%s to gl3_uniform::internal_name%s.
 *
 * Usage notes:
 * - See gl3_uniform_value for vital thread-safety details.
 *
 ************************************************************************/
class gl3_uniform_value_map
{
	public:

		typedef owning_map<string, gl3_uniform_value> value_map;
		typedef value_map::iterator iterator;
		typedef value_map::const_iterator const_iterator;


		gl3_uniform_value_map() NOTHROW;
		gl3_uniform_value_map(const gl3_uniform_value_map& other);

		gl3_uniform_value_map& operator =
			(const gl3_uniform_value_map& other);


		void swap_with(gl3_uniform_value_map& other) NOTHROW;


		/**
		 * Find uniform value of given type @bref{T} in map.
		 * If no such uniform value exists yet, create it.
		 * Throws if the map contains a value of matching
		 * @bref{name} but mismatching type.
		 */
		template <typename T>
			T& request(const string& name)
		{
			iterator it(values_.find(name));
	
			if (it)
			{
				if (!it->value) throw_null_pointer(name);
				if (typeid(*it->value) != typeid(T)) throw_type_mismatch(name);
				return static_cast<T&>(*it->value);
			}

			auto_pointer<T> new_value(new T());
			T& result = *new_value; // auto_pointer becomes NULL on insert

			values_.insert
				(name, std::move(new_value));

			return result;
		}


		/**
		 * Store a @bref{name}d, not-@NULL uniform value.
		 * Replaces any existing value of same @bref{name},
		 * regardless of any gl3_uniform_value types.
		 */
		void insert(const string& name,
					auto_pointer<gl3_uniform_value> value);


		int64 size() const NOTHROW;
		bool empty() const NOTHROW;

		const_iterator first() const NOTHROW;
		const_iterator last() const NOTHROW;

		const_iterator find(const string& name) const NOTHROW;

		void clear() NOTHROW;


	private:

		static void throw_null_pointer(const string& name);
		static void throw_type_mismatch(const string& name);

		value_map values_;
};




/**
 * A LIFO stack of gl3_uniform_value_map%s.
 * @ingroup gl3_render_module
 *
 * Usage notes:
 * - Uniform values with higher indices have precedence
 *		over uniform values of same name with lower indices.
 *		This allows to override uniform values, such as
 *		when a client sets uniforms through gl3_puppet_data.
 * - For performance reasons, the gl3_uniform_value_stack
 *		does not copy contained gl3_uniform_value_map%s.
 *		Clients are responsible for lifetime management.
 */
typedef dynamic_buffer<const gl3_uniform_value_map*> gl3_uniform_value_stack;




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_UNIFORM_HPP) */


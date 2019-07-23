/**
 *************************************************************************
 *
 * @file gl3_uniform.cpp
 *
 * OpenGL3 uniform variables, implementation.
 *
 ************************************************************************/


#include "gl3_uniform.hpp"


#include "log.hpp"
#include "array.hpp"
#include "color.hpp"
#include "geo_vector.hpp"
#include "geo_transform.hpp"
#include "gl3_api.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// gl3_uniform
//
//////////////////////////////////////////////////////////////////////////


gl3_uniform::gl3_uniform
	(const gl3_api& gl3,
	 const string& internal_name,
	 GLint size, GLenum type, GLint location)
	:
	gl3(gl3),
	internal_name(internal_name),
	size(size), type(type), location(location),
	apply_type(to_apply_type(type))
{ }


void gl3_uniform::apply(bool value) const
{
	if (apply_type != GL_BOOL) throw_apply_error_type("bool");
	if (size != 1) throw_apply_error_count(1);

	gl3.glUniform1i(location, (value == true) ? 1 : 0);
}

void gl3_uniform::apply(int32 value) const
{
	if (apply_type != GL_INT) throw_apply_error_type("int32");
	if (size != 1) throw_apply_error_count(1);

	gl3.glUniform1i(location, value);
}

void gl3_uniform::apply(float value) const
{
	if (apply_type != GL_FLOAT) throw_apply_error_type("float");
	if (size != 1) throw_apply_error_count(1);

	gl3.glUniform1f(location, value);
}


void gl3_uniform::apply(const color& value) const
{
	if (apply_type != GL_FLOAT_VEC4) throw_apply_error_type("vec4");
	if (size != 1) throw_apply_error_count(1);

	gl3.glUniform4fv(location, 1, value.c);
}

void gl3_uniform::apply(const vector& value) const
{
	if (apply_type != GL_FLOAT_VEC3) throw_apply_error_type("vec3");
	if (size != 1) throw_apply_error_count(1);

	gl3.glUniform3fv(location, 1, value.c);
}

void gl3_uniform::apply(const matrix& value) const
{
	if (apply_type != GL_FLOAT_MAT4) throw_apply_error_type("mat4");
	if (size != 1) throw_apply_error_count(1);

	gl3.glUniformMatrix4fv(location, 1, true, value.c);
}


void gl3_uniform::apply(const int32* array, int64 count) const
{
	if (apply_type != GL_INT) throw_apply_error_type("int32");
	if (size != count) throw_apply_error_count(count);

	gl3.glUniform1iv(location, size, array);
}

void gl3_uniform::apply(const float* array, int64 count) const
{
	if (apply_type != GL_FLOAT) throw_apply_error_type("float");
	if (size != count) throw_apply_error_count(count);

	gl3.glUniform1fv(location, size, array);
}


void gl3_uniform::apply(const color* array, int64 count) const
{
	if (apply_type != GL_FLOAT_VEC4) throw_apply_error_type("vec4");
	if (size != count) throw_apply_error_count(count);

	gl3.glUniform4fv
		(location, size, reinterpret_cast<const float*>(array));
}

void gl3_uniform::apply(const vector* array, int64 count) const
{
	if (apply_type != GL_FLOAT_VEC3) throw_apply_error_type("vec3");
	if (size != count) throw_apply_error_count(count);

	gl3.glUniform3fv
		(location, size, reinterpret_cast<const float*>(array));
}

void gl3_uniform::apply(const matrix* array, int64 count) const
{
	if (apply_type != GL_FLOAT_MAT4) throw_apply_error_type("mat4");
	if (size != count) throw_apply_error_count(count);

	gl3.glUniformMatrix4fv
		(location, size, true, reinterpret_cast<const float*>(array));
}


void gl3_uniform::throw_apply_error_type
	(const string& incoming_type) const
{
	LOG_ERROR("Type of uniform value: " + incoming_type +
				" does not match type of uniform: " + internal_name);
	throw invalid_parameter();
}


void gl3_uniform::throw_apply_error_count
	(int64 incoming_count) const
{
	LOG_ERROR("Size of uniform value: " + incoming_count +
				" does not match size of uniform: " + internal_name);
	throw invalid_parameter();
}


GLenum gl3_uniform::to_apply_type(GLenum type)
{
	/**
	 * Determine native, low-level C++ type
	 * that clients must use for some GLSL type.
	 * Relevant in particular for GL_SAMPLERs
	 * that actually require integer values.
	 */

	if (type == GL_BOOL ||
		type == GL_INT ||
		type == GL_FLOAT ||
		type == GL_FLOAT_VEC3 ||
		type == GL_FLOAT_VEC4 ||
		type == GL_FLOAT_MAT4)
			return type;

	if (type == GL_SAMPLER_1D ||
		type == GL_SAMPLER_1D_ARRAY ||
		type == GL_SAMPLER_2D ||
		type == GL_SAMPLER_2D_ARRAY ||
		type == GL_SAMPLER_3D ||
		type == GL_SAMPLER_CUBE)
			return GL_INT;

	LOG_ERROR("Unsupported uniform type: " + type);
	throw unsupported_feature();
}




//////////////////////////////////////////////////////////////////////////
//
// gl3_uniform_value_map
//
//////////////////////////////////////////////////////////////////////////


gl3_uniform_value_map::gl3_uniform_value_map() NOTHROW { }


gl3_uniform_value_map::gl3_uniform_value_map
	(const gl3_uniform_value_map& other)
{
	for (value_map::const_iterator it(other.values_.first()); it; ++it)
	{
		if (!it->value)
		{
			LOG_ERROR("Source map contains NULL uniform value: " + it->key);
			throw null_pointer();
		}

		insert(it->key, it->value->clone());
	}
}


gl3_uniform_value_map& gl3_uniform_value_map::operator =
	(const gl3_uniform_value_map& other)
{
	gl3_uniform_value_map offhand_copy(other);
	swap(*this, offhand_copy); // Strong exception safety.
	return *this;
}


void gl3_uniform_value_map::swap_with
	(gl3_uniform_value_map& other) NOTHROW
		{ swap(values_, other.values_); }


void gl3_uniform_value_map::insert
	(const string& name, auto_pointer<gl3_uniform_value> value)
{
	if (!value)
	{
		LOG_ERROR("Cannot insert NULL uniform value: " + name);
		throw invalid_parameter();
	}

	values_.insert
		(name, std::move(value));
}


int64 gl3_uniform_value_map::size() const NOTHROW
	{ return values_.size(); }
bool gl3_uniform_value_map::empty() const NOTHROW
	{ return values_.empty(); }


gl3_uniform_value_map::const_iterator
	gl3_uniform_value_map::first() const NOTHROW
		{ return values_.first(); }

gl3_uniform_value_map::const_iterator
	gl3_uniform_value_map::last() const NOTHROW
		{ return values_.last(); }

		
gl3_uniform_value_map::const_iterator
	gl3_uniform_value_map::find(const string& name) const NOTHROW
		{ return values_.find(name); }


void gl3_uniform_value_map::clear() NOTHROW
	{ values_.clear(); }


void gl3_uniform_value_map::throw_null_pointer(const string& name)
{
	LOG_ERROR("Uniform value is NULL pointer: " + name);
	throw null_pointer();
}


void gl3_uniform_value_map::throw_type_mismatch(const string& name)
{
	LOG_ERROR("Type mismatch on uniform value: " + name);
	throw type_mismatch();
}




} /* namespace viral_core */
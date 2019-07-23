/**
 *************************************************************************
 *
 * @file util.cpp
 *
 * Utility implementation.
 *
 ************************************************************************/


#include "util.hpp"


#include <math.h>
#include <typeinfo>


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// Utility functions
//
//////////////////////////////////////////////////////////////////////////


void swap(char& a, char& b) NOTHROW
	{ char temp = a; a = b; b = temp; }
void swap(int8& a, int8& b) NOTHROW
	{ int8 temp = a; a = b; b = temp; }
void swap(uint8& a, uint8& b) NOTHROW
	{ uint8 temp = a; a = b; b = temp; }
void swap(int16& a, int16& b) NOTHROW
	{ int16 temp = a; a = b; b = temp; }
void swap(uint16& a, uint16& b) NOTHROW
	{ uint16 temp = a; a = b; b = temp; }
void swap(int32& a, int32& b) NOTHROW
	{ int32 temp = a; a = b; b = temp; }
void swap(uint32& a, uint32& b) NOTHROW
	{ uint32 temp = a; a = b; b = temp; }
void swap(int64& a, int64& b) NOTHROW
	{ int64 temp = a; a = b; b = temp; }
void swap(uint64& a, uint64& b) NOTHROW
	{ uint64 temp = a; a = b; b = temp; }
void swap(float& a, float& b) NOTHROW
	{ float temp = a; a = b; b = temp; }
void swap(double& a, double& b) NOTHROW
	{ double temp = a; a = b; b = temp; }


bool nearly_equal(float value, float target) NOTHROW
{
	return nearly_equal(value, target,
						nearly_equal_epsilon_constant,
						nearly_equal_epsilon_linear);
}

bool nearly_equal
	(float value, float target,
	 float epsilon_constant,
	 float epsilon_linear) NOTHROW
{
	float maxvalue = max(fabsf(value), fabsf(target));
	float epsilon = max(maxvalue * epsilon_linear, epsilon_constant);
	return fabsf(value - target) < epsilon;
}




//////////////////////////////////////////////////////////////////////////
//
// type_info_ref
//
//////////////////////////////////////////////////////////////////////////


type_info_ref::type_info_ref(const std::type_info& ref) NOTHROW
	: ptr_(&ref) { }
type_info_ref::type_info_ref(const type_info_ref& other) NOTHROW
	: ptr_(other.ptr_) { }

const std::type_info& type_info_ref::get() const NOTHROW
	{ return *ptr_; }

type_info_ref& type_info_ref::operator =(const type_info_ref& other) NOTHROW
	{ ptr_ = other.ptr_; return *this; }
		
bool type_info_ref::operator ==(const type_info_ref& other) const NOTHROW
	{ return *ptr_ == *other.ptr_; }
bool type_info_ref::operator !=(const type_info_ref& other) const NOTHROW
	{ return *ptr_ != *other.ptr_; }	

bool type_info_ref::operator <(const type_info_ref& other) const NOTHROW
	{ return ptr_->before(*other.ptr_); }
bool type_info_ref::operator >(const type_info_ref& other) const NOTHROW
	{ return other.ptr_->before(*ptr_); }
bool type_info_ref::operator <=(const type_info_ref& other) const NOTHROW
	{ return !other.ptr_->before(*ptr_); }
bool type_info_ref::operator >=(const type_info_ref& other) const NOTHROW
	{ return !ptr_->before(*other.ptr_); }




} /* namespace viral_core */

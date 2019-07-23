/**
 *************************************************************************
 *
 * @file geo_2d.cpp
 *
 * Implementation for geometry framework, 2D primitives.
 *
 ************************************************************************/


#include "geo_2d.hpp"


#include <math.h>


#include "util.hpp"
#include "integer_operations.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// vector2i
//
//////////////////////////////////////////////////////////////////////////


vector2i::vector2i() NOTHROW
	: x(0), y(0) { }
vector2i::vector2i(int64 x, int64 y) NOTHROW
	: x(x), y(y) { }


const int64& vector2i::operator [](int64 index) const NOTHROW
	{ return c[index]; }
int64& vector2i::operator [](int64 index) NOTHROW
	{ return c[index]; }


vector2i vector2i::operator -() const NOTHROW
	{ return vector2i(-x, -y); }

vector2i vector2i::operator +(const vector2i &v2) const NOTHROW
	{ return vector2i(x + v2.x, y + v2.y); }
vector2i vector2i::operator -(const vector2i &v2) const NOTHROW
	{ return vector2i(x - v2.x, y - v2.y); }
vector2i vector2i::operator *(const vector2i &v2) const NOTHROW
	{ return vector2i(x * v2.x, y * v2.y); }
vector2i vector2i::operator /(const vector2i &v2) const NOTHROW
	{ return vector2i(x / v2.x, y / v2.y); }

vector2i& vector2i::operator +=(const vector2i &v2) NOTHROW
	{ x += v2.x; y += v2.y; return *this; }
vector2i& vector2i::operator -=(const vector2i &v2) NOTHROW
	{ x -= v2.x; y -= v2.y; return *this; }
vector2i& vector2i::operator *=(const vector2i &v2) NOTHROW
	{ x *= v2.x; y *= v2.y; return *this; }
vector2i& vector2i::operator /=(const vector2i &v2) NOTHROW
	{ x /= v2.x; y /= v2.y; return *this; }

vector2i vector2i::operator *(int64 val) const NOTHROW
	{ return vector2i(x * val, y * val); }
vector2i vector2i::operator /(int64 val) const NOTHROW
	{ return vector2i(x / val, y / val); }
vector2i& vector2i::operator *=(int64 val) NOTHROW
	{ x *= val; y *= val; return *this; }
vector2i& vector2i::operator /=(int64 val) NOTHROW
	{ x /= val; y /= val; return *this; }

bool vector2i::operator ==(const vector2i &v2) const NOTHROW
	{ return x == v2.x && y == v2.y; }
bool vector2i::operator !=(const vector2i &v2) const NOTHROW
	{ return x != v2.x || y != v2.y; }


void vector2i::swap_with(vector2i& other) NOTHROW
	{ swap(x, other.x); swap(y, other.y); }


int64 vector2i::min() const NOTHROW
	{ return viral_core::min(x, y); }
int64 vector2i::max() const NOTHROW
	{ return viral_core::max(x, y); }

vector2i vector2i::min(const vector2i& v2) const NOTHROW
	{ return vector2i(viral_core::min(x, v2.x), viral_core::min(y, v2.y)); }
vector2i vector2i::max(const vector2i& v2) const NOTHROW
	{ return vector2i(viral_core::max(x, v2.x), viral_core::max(y, v2.y)); }

vector2i vector2i::abs() const NOTHROW
	{ return vector2i(::abs(x), ::abs(y)); }


vector2f vector2i::to_vector2f() const NOTHROW
	{ return vector2f(static_cast<float>(x), static_cast<float>(y)); }




//////////////////////////////////////////////////////////////////////////
//
// rect2i
//
//////////////////////////////////////////////////////////////////////////


rect2i::rect2i() NOTHROW { }

rect2i::rect2i
	(int64 pos_x, int64 pos_y, int64 size_x, int64 size_y) NOTHROW
		: pos(pos_x, pos_y), size(size_x, size_y) { }

rect2i::rect2i
	(const vector2i& pos, const vector2i& size) NOTHROW
		: pos(pos), size(size) { }


void rect2i::set
	(int64 pos_x, int64 pos_y, int64 size_x, int64 size_y) NOTHROW
{
	pos.x = pos_x;
	pos.y = pos_y;
	size.x = size_x;
	size.y = size_y;
}

void rect2i::set
	(const vector2i& new_pos, const vector2i& new_size) NOTHROW
{
	pos = new_pos;
	size = new_size;
}


vector2i rect2i::end() const NOTHROW { return pos + size; }
int64 rect2i::end_x() const NOTHROW { return pos.x + size.x; }
int64 rect2i::end_y() const NOTHROW { return pos.y + size.y; }


bool rect2i::operator ==(const rect2i& other) const NOTHROW
{
	return
		pos.x == other.pos.x && pos.y == other.pos.y &&
		size.x == other.size.x && size.y == other.size.y;
}


bool rect2i::operator !=(const rect2i& other) const NOTHROW
{
	return
		pos.x != other.pos.x || pos.y != other.pos.y ||
		size.x != other.size.x || size.y != other.size.y;
}


void rect2i::swap_with(rect2i& other) NOTHROW
	{ swap(pos, other.pos); swap(size, other.size); }


bool rect2i::contains(int64 x, int64 y) const NOTHROW
{
	int64 endx = pos.x + size.x;
	int64 endy = pos.y + size.y;

	// Point needs to be on left and on right side
	// of one of the boundaries for both axi.
	return
		(x >= pos.x || x >= endx) && (x <= pos.x || x <= endx) &&
		(y >= pos.y || y >= endy) && (y <= pos.y || y <= endy);
}

bool rect2i::contains(const vector2i& point) const NOTHROW
	{ return contains(point.x, point.y); }


rect2i rect2i::flipped_rect() const NOTHROW
{
	return
		rect2i(pos.x + size.x, pos.y + size.y, -size.x, -size.y);
}


void rect2i::grow_to(int64 x, int64 y) NOTHROW
{
	// Growing needs to consider current rectangle
	// size direction, so we keep that one up.
	if (size.x >= 0)
	{
		if (x < pos.x)
			size.x += (pos.x - x), pos.x = x;
		else if (x > pos.x + size.x)
			size.x = x - pos.x;
	}
	else
	{
		if (x < pos.x + size.x)
			size.x = x - pos.x;
		else if (x > pos.x)
			size.x += (pos.x - x), pos.x = x;
	}

	if (size.y >= 0)
	{
		if (y < pos.y)
			size.y += (pos.y - y), pos.y = y;
		else if (y > pos.y + size.y)
			size.y = y - pos.y;
	}
	else
	{
		if (y < pos.y + size.y)
			size.y = y - pos.y;
		else if (y > pos.y)
			size.y += (pos.y - y), pos.y = y;
	}
}


void rect2i::grow_to(const rect2i& other) NOTHROW
{
	grow_to(other.pos.x, other.pos.y);
	grow_to(other.pos.x + other.size.x, other.pos.y + other.size.y);
}


rect2f rect2i::to_rect2f() const NOTHROW
	{ return rect2f(pos.to_vector2f(), size.to_vector2f()); }




//////////////////////////////////////////////////////////////////////////
//
// vector2f
//
//////////////////////////////////////////////////////////////////////////


vector2f::vector2f() NOTHROW : x(0), y(0) { }
vector2f::vector2f(float x, float y) NOTHROW : x(x), y(y) { }


const float& vector2f::operator [](int64 index) const NOTHROW
	{ return c[index]; }
float& vector2f::operator [](int64 index) NOTHROW
	{ return c[index]; }


vector2f vector2f::operator -() const NOTHROW
	{ return vector2f(-x, -y); }

vector2f vector2f::operator +(const vector2f &v2) const NOTHROW
	{ return vector2f(x + v2.x, y + v2.y); }
vector2f vector2f::operator -(const vector2f &v2) const NOTHROW
	{ return vector2f(x - v2.x, y - v2.y); }
vector2f vector2f::operator *(const vector2f &v2) const NOTHROW
	{ return vector2f(x * v2.x, y * v2.y); }
vector2f vector2f::operator /(const vector2f &v2) const NOTHROW
	{ return vector2f(x / v2.x, y / v2.y); }

vector2f& vector2f::operator +=(const vector2f &v2) NOTHROW
	{ x += v2.x; y += v2.y; return *this; }
vector2f& vector2f::operator -=(const vector2f &v2) NOTHROW
	{ x -= v2.x; y -= v2.y; return *this; }
vector2f& vector2f::operator *=(const vector2f &v2) NOTHROW
	{ x *= v2.x; y *= v2.y; return *this; }
vector2f& vector2f::operator /=(const vector2f &v2) NOTHROW
	{ x /= v2.x; y /= v2.y; return *this; }

vector2f vector2f::operator *(float val) const NOTHROW
	{ return vector2f(x * val, y * val); }
vector2f vector2f::operator /(float val) const NOTHROW
	{ return vector2f(x / val, y / val); }
vector2f& vector2f::operator *=(float val) NOTHROW
	{ x *= val; y *= val; return *this; }
vector2f& vector2f::operator /=(float val) NOTHROW
	{ x /= val; y /= val; return *this; }

bool vector2f::operator ==(const vector2f &v2) const NOTHROW
	{ return x == v2.x && y == v2.y; }
bool vector2f::operator !=(const vector2f &v2) const NOTHROW
	{ return x != v2.x || y != v2.y; }


void vector2f::swap_with(vector2f& other) NOTHROW
	{ swap(x, other.x); swap(y, other.y); }


float vector2f::dot(const vector2f &v2) const NOTHROW
	{ return x * v2.x + y * v2.y; }
float vector2f::length() const NOTHROW
	{ return sqrtf(x * x + y * y); }


vector2f vector2f::normalized() const NOTHROW
{
	float invlen = 1 / length();
	return vector2f(x * invlen, y * invlen);
}

void vector2f::normalize() NOTHROW
{
	float invlen = 1 / length();
	x *= invlen; y *= invlen;
}


float vector2f::min() const NOTHROW
	{ return viral_core::min(x, y); }
float vector2f::max() const NOTHROW
	{ return viral_core::max(x, y); }

vector2f vector2f::min(const vector2f& v2) const NOTHROW
	{ return vector2f(viral_core::min(x, v2.x), viral_core::min(y, v2.y)); }
vector2f vector2f::max(const vector2f& v2) const NOTHROW
	{ return vector2f(viral_core::max(x, v2.x), viral_core::max(y, v2.y)); }

vector2f vector2f::abs() const NOTHROW
	{ return vector2f(fabsf(x), fabsf(y)); }

vector2f vector2f::floor() const NOTHROW
	{ return vector2f(::floorf(x), ::floorf(y)); }
vector2f vector2f::ceil() const NOTHROW
	{ return vector2f(::ceilf(x), ::ceilf(y)); }


vector2i vector2f::to_vector2i() const NOTHROW
{
	return vector2i
		(integer_operations::cast_or_clamp<int64>(x),
		 integer_operations::cast_or_clamp<int64>(y));
}




//////////////////////////////////////////////////////////////////////////
//
// rect2f
//
//////////////////////////////////////////////////////////////////////////


rect2f::rect2f() NOTHROW { }

rect2f::rect2f
	(float pos_x, float pos_y, float size_x, float size_y) NOTHROW
		: pos(pos_x, pos_y), size(size_x, size_y) { }

rect2f::rect2f
	(const vector2f& pos, const vector2f& size) NOTHROW
		: pos(pos), size(size) { }


void rect2f::set
	(float pos_x, float pos_y, float size_x, float size_y) NOTHROW
{
	pos.x = pos_x;
	pos.y = pos_y;
	size.x = size_x;
	size.y = size_y;
}

void rect2f::set
	(const vector2f& new_pos, const vector2f& new_size) NOTHROW
{
	pos = new_pos;
	size = new_size;
}


vector2f rect2f::end() const NOTHROW { return pos + size; }
float rect2f::end_x() const NOTHROW { return pos.x + size.x; }
float rect2f::end_y() const NOTHROW { return pos.y + size.y; }


bool rect2f::operator ==(const rect2f& other) const NOTHROW
{
	return
		pos.x == other.pos.x && pos.y == other.pos.y &&
		size.x == other.size.x && size.y == other.size.y;
}


bool rect2f::operator !=(const rect2f& other) const NOTHROW
{
	return
		pos.x != other.pos.x || pos.y != other.pos.y ||
		size.x != other.size.x || size.y != other.size.y;
}


void rect2f::swap_with(rect2f& other) NOTHROW
	{ swap(pos, other.pos); swap(size, other.size); }


bool rect2f::contains(float x, float y) const NOTHROW
{
	float endx = pos.x + size.x;
	float endy = pos.y + size.y;

	// Point needs to be on left and on right side
	// of one of the boundaries for both axi.
	return
		(x >= pos.x || x >= endx) && (x <= pos.x || x <= endx) &&
		(y >= pos.y || y >= endy) && (y <= pos.y || y <= endy);
}

bool rect2f::contains(const vector2f& point) const NOTHROW
	{ return contains(point.x, point.y); }


rect2f rect2f::floor_rect() const NOTHROW
{
	return
		rect2f(floorf(pos.x), floorf(pos.y),
			   floorf(size.x), floorf(size.y));
}


rect2f rect2f::round_rect() const NOTHROW
{
	return
		rect2f(floorf(pos.x + 0.5f), floorf(pos.y + 0.5f),
		   	   floorf(size.x + 0.5f), floorf(size.y + 0.5f));
}


rect2f rect2f::flipped_rect() const NOTHROW
{
	return
		rect2f(pos.x + size.x, pos.y + size.y, -size.x, -size.y);
}


rect2f rect2f::with_positive_size() const NOTHROW
{
	return
		rect2f(size.x >= 0 ? pos.x : (pos.x + size.x),
			   size.y >= 0 ? pos.y : (pos.y + size.y),
			   size.x >= 0 ? size.x : -size.x,
			   size.y >= 0 ? size.y : -size.y);
}


bool rect2f::intersecting(const rect2f& other) const NOTHROW
{
	float end_x = pos.x + size.x;
	float end_y = pos.y + size.y;

	float other_end_x = other.pos.x + other.size.x;
	float other_end_y = other.pos.y + other.size.y;

	// No intersection if one rectangle is completely
	// on one side of the other.
	return !((pos.x < other.pos.x && pos.x < other_end_x &&
				end_x < other.pos.x && end_x < other_end_x) ||
			 (other.pos.x < pos.x && other.pos.x < end_x &&
				other_end_x < pos.x && other_end_x < end_x) ||
			 (pos.y < other.pos.y && pos.y < other_end_y &&
				end_y < other.pos.y && end_y < other_end_y) ||
			 (other.pos.y < pos.y && other.pos.y < end_y &&
				other_end_y < pos.y && other_end_y < end_y));
}


bool rect2f::try_intersect
	(const rect2f& other, rect2f& result) const NOTHROW
{
	if (!intersecting(other))
		return false;


	// We know the rectangles intersect.
	// The intersection area then is defined by the middle
	// two points on each axis. Find these by sorting.
	float sort_x[4] =
		{ pos.x, pos.x + size.x,
		  other.pos.x, other.pos.x + other.size.x };

	float sort_y[4] =
		{ pos.y, pos.y + size.y,
		  other.pos.y, other.pos.y + other.size.y };

	for (int64 i = 0; i < 4; ++i)
	for (int64 j = i + 1; j < 4; ++j)
	{
		if (sort_x[i] > sort_x[j]) swap(sort_x[i], sort_x[j]);
		if (sort_y[i] > sort_y[j]) swap(sort_y[i], sort_y[j]);
	}


	// Fix up sorted coordinates to match current
	// size variable sign.
	if (size.x < 0) swap(sort_x[1], sort_x[2]);
	if (size.y < 0) swap(sort_y[1], sort_y[2]);


	result.pos.x = sort_x[1];
	result.size.x = sort_x[2] - sort_x[1];
	result.pos.y = sort_y[1];
	result.size.y = sort_y[2] - sort_y[1];

	return true;
}


void rect2f::grow_to(float x, float y) NOTHROW
{
	// Growing needs to consider current rectangle
	// size direction, so we keep that one up.
	if (size.x >= 0)
	{
		if (x < pos.x)
			size.x += (pos.x - x), pos.x = x;
		else if (x > pos.x + size.x)
			size.x = x - pos.x;
	}
	else
	{
		if (x < pos.x + size.x)
			size.x = x - pos.x;
		else if (x > pos.x)
			size.x += (pos.x - x), pos.x = x;
	}

	if (size.y >= 0)
	{
		if (y < pos.y)
			size.y += (pos.y - y), pos.y = y;
		else if (y > pos.y + size.y)
			size.y = y - pos.y;
	}
	else
	{
		if (y < pos.y + size.y)
			size.y = y - pos.y;
		else if (y > pos.y)
			size.y += (pos.y - y), pos.y = y;
	}
}


void rect2f::grow_to(const rect2f& other) NOTHROW
{
	grow_to(other.pos.x, other.pos.y);
	grow_to(other.pos.x + other.size.x, other.pos.y + other.size.y);
}


rect2f rect2f::apply_linear_map
	(const rect2f& preimage, const rect2f& image) const NOTHROW
{
	rect2f result;


	// Find the scale factor and offset of the linear
	// mapping, then apply to current rectangle.
	if (preimage.size.x != 0)
	{
		float scalex = image.size.x / preimage.size.x;
		float offsetx = image.pos.x - preimage.pos.x * scalex;

		result.pos.x = pos.x * scalex + offsetx;
		result.size.x = size.x * scalex;
	}

	if (preimage.size.y != 0)
	{
		float scaley = image.size.y / preimage.size.y;
		float offsety = image.pos.y - preimage.pos.y * scaley;

		result.pos.y = pos.y * scaley + offsety;
		result.size.y = size.y * scaley;
	}


	return result;
}


rect2i rect2f::to_rect2i() const NOTHROW
	{ return rect2i(pos.to_vector2i(), size.to_vector2i()); }




} /* namespace viral_core */

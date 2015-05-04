#pragma once
#include <math.h>
#include "fixed.h"

template <typename T, int p>
class vec3_fixed {
public:
	vec3_fixed();
	explicit vec3_fixed(float a, float b, float c);
	explicit vec3_fixed(float *a);
	fixed<T, p>				length();
	fixed<T, p>				normalize();
	fixed<T, p>				operator[](const int index) const;
	fixed<T, p> &			operator[](const int index);
	vec3_fixed &			operator+=(const vec3_fixed &a);
	vec3_fixed &			operator-=(const vec3_fixed &a);
	fixed<T, p>				operator*(const vec3_fixed &a) const;
	vec3_fixed &			operator=(const float* a);
	vec3_fixed &			operator=(const vec3_fixed<short,3> &);
	vec3_fixed				operator*(const fixed<T, p> &a);
	vec3_fixed				operator*(const float a);
	vec3_fixed				operator+(const vec3_fixed &a);
	vec3_fixed				operator-(const vec3_fixed &a);
	vec3_fixed				operator-() const;
	
	void					copy_to(float *a);
	vec3_fixed<int, 16>		multiply(vec3_fixed<short, 3> &a);
	vec3_fixed<int, 16>		multiply(vec3_fixed<short, 12> &a);
	vec3_fixed<int, 16>		scale(fixed<int, 16> &a);
	fixed<int, 16>			dot(const vec3_fixed<short, 12> &a) const;
	fixed<int, 16>			dot(const vec3_fixed<short, 3> &a) const;
private:
	fixed<T,p> x, y, z;
};

template <typename T, int p>
inline vec3_fixed<T, p>::vec3_fixed() {
	x = y = z = 0;
}

template <typename T, int p>
inline vec3_fixed<T, p>::vec3_fixed(float a, float b, float c) {
	x = a;
	y = b;
	z = c;
}

template <typename T, int p>
inline vec3_fixed<T, p>::vec3_fixed(float *a) {
	x = a[0];
	y = a[1];
	z = a[2];
}

template <typename T, int p>
inline fixed<T, p> vec3_fixed<T, p>::length() {
	float _x = x.c_float();
	float _y = y.c_float();
	float _z = z.c_float();
	float len = (float)::sqrt((double)(_x*_x + _y*_y + _z*_z));
	return  fixed<T, p>(len);
}

template <typename T, int p>
inline fixed<T, p> vec3_fixed<T, p>::normalize() {
	float _x = x.c_float();
	float _y = y.c_float();
	float _z = z.c_float();
	float len = sqrt(_x*_x + _y*_y + _z*_z);

	if (len != 0.0f) {
		x /= len;
		y /= len;
		z /= len;
	}

	return  fixed<T, p>(len);
}

template <typename T, int p>
inline void vec3_fixed<T, p>::copy_to(float *a) {
	a[0] = x.c_float();
	a[1] = y.c_float();
	a[2] = z.c_float();
}

template <typename T, int p>
inline fixed<T, p> vec3_fixed<T, p>::operator[](const int index) const {
	return (&x)[index];
}

template <typename T, int p>
inline fixed<T, p> &vec3_fixed<T, p>::operator[](const int index) {
	return (&x)[index];
}

template <typename T, int p>
inline vec3_fixed<T, p> &vec3_fixed<T, p>::operator+=(const vec3_fixed &a) {
	x += a.x;
	y += a.y;
	z += a.z;
	return *this;
}

template <typename T, int p>
inline vec3_fixed<T, p> &vec3_fixed<T, p>::operator-=(const vec3_fixed &a) {
	x -= a.x;
	y -= a.y;
	z -= a.z;
	return *this;
}

template <typename T, int p>
inline fixed<T, p> vec3_fixed<T, p>::operator*(const vec3_fixed &a) const {
	return x * a.x + y * a.y + z * a.z;
}

template <typename T, int p>
inline vec3_fixed<T, p> & vec3_fixed<T, p>::operator=(const float* a) {
	x = a[0];
	y = a[1];
	z = a[2];
	return *this;
}

template<>
inline vec3_fixed<int, 16> & vec3_fixed<int, 16>::operator=(const vec3_fixed<short, 3> &a) {
	x.x = a[0].x;
	y.x = a[1].x;
	z.x = a[2].x;
	x.x <<= 13;
	y.x <<= 13;
	z.x <<= 13;
	return *this;
}

template<>
inline vec3_fixed<short, 3> & vec3_fixed<short, 3>::operator=(const vec3_fixed<short, 3> &a) {
	x = a.x;
	y = a.y;
	z = a.z;
	return *this;
}

template <typename T, int p>
inline vec3_fixed<T, p> vec3_fixed<T, p>::operator-(const vec3_fixed &a) {
	vec3_fixed b;
	b[0] = x - a[0];
	b[1] = y - a[1];
	b[2] = z - a[2];
	return b;
}
template <typename T, int p>
inline vec3_fixed<T, p> vec3_fixed<T, p>::operator-() const {
	vec3_fixed b;
	b[0] = -x;
	b[1] = -y;
	b[2] = -z;
	return b;
}
template <typename T, int p>
inline vec3_fixed<T, p> vec3_fixed<T, p>::operator+(const vec3_fixed &a) {
	vec3_fixed b;
	b[0] = x + a[0];
	b[1] = y + a[1];
	b[2] = z + a[2];
	return b;
}
template <typename T, int p>
inline vec3_fixed<T, p> vec3_fixed<T, p>::operator*(const fixed<T, p> &a) {
	vec3_fixed b;
	b[0] = x * a;
	b[1] = y * a;
	b[2] = z * a;
	return b;
}

template <typename T, int p>
inline vec3_fixed<T, p> vec3_fixed<T, p>::operator*(const float a) {
	vec3_fixed b;
	b[0] = x * a;
	b[1] = y * a;
	b[2] = z * a;
	return b;
}

template<>
inline vec3_fixed<int, 16> vec3_fixed<int, 24>::multiply(vec3_fixed<short, 3> &a) {
	vec3_fixed<int, 16> b;
	b[0].x = (int)((((__int64_t)x.x * a[0].x)) >> 11);
	b[1].x = (int)((((__int64_t)y.x * a[1].x)) >> 11);
	b[2].x = (int)((((__int64_t)z.x * a[2].x)) >> 11);
	return b;
}

template<>
inline vec3_fixed<int, 16> vec3_fixed<int, 16>::multiply(vec3_fixed<short, 12> &a) {
	vec3_fixed<int, 16> b;
	b[0].x = (int)((((__int64_t)x.x * a[0].x)) >> 12);
	b[1].x = (int)((((__int64_t)y.x * a[1].x)) >> 12);
	b[2].x = (int)((((__int64_t)z.x * a[2].x)) >> 12);
	return b;
}

template<>
inline vec3_fixed<int, 16> vec3_fixed<int, 24>::scale(fixed<int, 16> &a) {
	vec3_fixed<int, 16> b;
	b[0].x = (int)((((__int64_t)x.x * (__int64_t)a.x)) >> 24);
	b[1].x = (int)((((__int64_t)y.x * (__int64_t)a.x)) >> 24);
	b[2].x = (int)((((__int64_t)z.x * (__int64_t)a.x)) >> 24);
	return b;
}

template<>
inline vec3_fixed<int, 16> vec3_fixed<int, 16>::scale(fixed<int, 16> &a) {
	vec3_fixed<int, 16> b;
	b[0].x = (int)((((__int64_t)x.x * (__int64_t)a.x)) >> 16);
	b[1].x = (int)((((__int64_t)y.x * (__int64_t)a.x)) >> 16);
	b[2].x = (int)((((__int64_t)z.x * (__int64_t)a.x)) >> 16);
	return b;
}

template<>
inline fixed<int, 16> vec3_fixed<int, 24>::dot(const vec3_fixed<short, 3> &a) const {
	__int64_t val;

	val = ((((__int64_t)x.x * (__int64_t)a[0].x)) >> 11) +
		((((__int64_t)y.x * (__int64_t)a[1].x)) >> 11) +
		((((__int64_t)z.x * (__int64_t)a[2].x)) >> 11);
	return fixed<int, 16>((int)val);
}

template<>
inline fixed<int, 16> vec3_fixed<int, 16>::dot(const vec3_fixed<short, 12> &a) const {
	__int64_t val;

	val = ((((__int64_t)x.x * (__int64_t)a[0].x)) >> 12) +
		((((__int64_t)y.x * (__int64_t)a[1].x)) >> 12) +
		((((__int64_t)z.x * (__int64_t)a[2].x)) >> 12);
	return fixed<int, 16>((int)val);
}

template<>
inline fixed<int, 16> vec3_fixed<short, 3>::dot(const vec3_fixed<short, 12> &a) const {
	int val;

	val = ((((int)x.x * (int)a[0].x)) << 1) +
		((((int)y.x * (int)a[1].x)) << 1) +
		((((int)z.x * (int)a[2].x)) << 1);
	return fixed<int, 16>((int)val);
}

typedef vec3_fixed<unsigned char, 0> vec3_fixed8;
typedef vec3_fixed<short, 3> vec3_fixed16;
typedef vec3_fixed<int, 16> vec3_fixed32;
typedef vec3_fixed<short, 12> tex3_fixed16;
typedef vec3_fixed<int, 24> norm3_fixed32;


#define MAX_WORLD_SIZE 8192.0f

class bbox {
public:
	bbox(void);

	vec3_fixed32  operator[](const int index) const;
	vec3_fixed32 &operator[](const int index);
	bbox & operator+=(const vec3_fixed32 &v);

	bool Intersects(const bbox &b) const;
	void grow(float d);
	void grow(vec3_fixed32 &v);
	bbox & operator+=(const bbox &a);
	fixed32p16 radius();
private:
	vec3_fixed32	b[2];
public:
	vec3_fixed32 bmin() { return b[0]; }
	vec3_fixed32 bmax() { return b[1]; }
	void	init();
};

inline bbox::bbox(void) {
	init();
}
inline void bbox::init(void) {
	b[0] = vec3_fixed32(MAX_WORLD_SIZE, MAX_WORLD_SIZE, MAX_WORLD_SIZE);
	b[1] = vec3_fixed32(-MAX_WORLD_SIZE, -MAX_WORLD_SIZE, -MAX_WORLD_SIZE);
}

inline void bbox::grow(float d) {
	b[0] -= vec3_fixed32(d, d, d);
	b[1] += vec3_fixed32(d, d, d);
}

inline void bbox::grow(vec3_fixed32 &v) {
	b[0] -= v;
	b[1] += v;
}

inline vec3_fixed32 bbox::operator[](const int index) const {
	return b[index];
}

inline vec3_fixed32 &bbox::operator[](const int index) {
	return b[index];
}

inline bbox &bbox::operator+=(const bbox &a) {
	if (a.b[0][0] < b[0][0]) {
		b[0][0] = a.b[0][0];
	}
	if (a.b[1][0] > b[1][0]) {
		b[1][0] = a.b[1][0];
	}
	if (a.b[0][1] < b[0][1]) {
		b[0][1] = a.b[0][1];
	}
	if (a.b[1][1] > b[1][1]) {
		b[1][1] = a.b[1][1];
	}
	if (a.b[0][2] < b[0][2]) {
		b[0][2] = a.b[0][2];
	}
	if (a.b[1][2] > b[1][2]) {
		b[1][2] = a.b[1][2];
	}
	return *this;
}

inline bbox &bbox::operator+=(const vec3_fixed32 &v) {
	if (v[0] < b[0][0]) {
		b[0][0] = v[0];
	}
	if (v[0] > b[1][0]) {
		b[1][0] = v[0];
	}
	if (v[1] < b[0][1]) {
		b[0][1] = v[1];
	}
	if (v[1] > b[1][1]) {
		b[1][1] = v[1];
	}
	if (v[2] < b[0][2]) {
		b[0][2] = v[2];
	}
	if (v[2] > b[1][2]) {
		b[1][2] = v[2];
	}
	return *this;
}

inline bool bbox::Intersects(const bbox &a) const {
	if (a.b[1][0] < b[0][0] || a.b[1][1] < b[0][1] || a.b[1][2] < b[0][2]
		|| a.b[0][0] > b[1][0] || a.b[0][1] > b[1][1] || a.b[0][2] > b[1][2]) {
		return false;
	}
	return true;
}

inline fixed32p16 bbox::radius()
{
	int i;
	vec3_fixed32 corner;

	for (i = 0; i<3; i++)
	{
		corner[i] = (b[0][i].abs()) > (b[1][i].abs()) ? (b[0][i].abs()) : (b[1][i].abs());
	}

	return corner.length();
}



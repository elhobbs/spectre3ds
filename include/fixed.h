#pragma once

template <typename T,int p>
class fixed {
public:
				fixed();
				explicit fixed(const short a);
				explicit fixed(const int a);
				explicit fixed(float a);
				fixed(const fixed<short,3> &a);
				fixed			operator-() const;
	fixed &		operator=( const fixed &a );
	fixed &		operator=( const float a );
	fixed &		operator=( const int a );
	fixed			operator*(const fixed<short, 3> &a) const;
	fixed			operator*(const fixed<int, 16> &a) const;
	fixed			operator*(const fixed<int, 24> &a) const;
	fixed			operator*(const float a) const;
	fixed			operator/( const fixed &a ) const;
	fixed			operator/(const float a) const;
	fixed			operator/(const int a) const;
	fixed			operator+(const fixed &a) const;
	fixed			operator-( const fixed &a ) const;
	fixed &		operator+=(const fixed &a);
	fixed &		operator-=(const fixed &a);
	fixed &		operator+=(const float a);
	fixed &		operator-=(const float a);
	fixed &		operator/=(const fixed &a);
	fixed &		operator/=(const float a);
	fixed &		operator/=(const int a);
	fixed &		operator*=(const float a);
	fixed &		operator*=( const fixed &a );

	fixed<int, 16> multiply16(fixed<short, 3> &a);
	fixed<int, 16> multiply3(fixed<short, 3> &a);


	bool			operator==(const fixed &a) const;						// exact compare, no epsilon
	bool			operator!=(const fixed &a) const;						// exact compare, no epsilon
	bool			operator<(const fixed &a) const;
	bool			operator<=(const fixed &a) const;
	bool			operator<(const int a) const;
	bool			operator<=(const int a) const;
	bool			operator<=(const float a) const;
	bool			operator>(const fixed &a) const;
	bool			operator>=(const fixed &a) const;
	bool			operator>(const int a) const;
	bool			operator>=(const int a) const;
	bool			operator>=(const float a) const;
	bool			operator==(const int a) const;
	float			c_float() const;
	short			c_short() const;
	int				c_int() const;
	fixed			abs() const;

	T x;
};

template <typename T, int p>
inline fixed<T, p>::fixed() {
	x = 0;
}

template <typename T, int p>
inline fixed<T,p>::fixed(const short a) {
	x = a;
}

template <typename T,int p>
inline fixed<T,p>::fixed(const int a) {
	x = a;
}

template <typename T, int p>
inline fixed<T, p>::fixed(const float a) {
	x = (T)(a*(float)(1 << p));
}

template <typename T, int p>
inline fixed<T, p>::fixed(const fixed<short,3> &a) {
	x = (T)(a.x<<(p-3));
}

template <typename T, int p>
inline fixed<T,p> fixed<T,p>::operator-() const {
	return fixed(-x);
}

template <typename T,int p>
inline fixed<T,p> &fixed<T,p>::operator=(const fixed &a) {
	x = a.x;
	return *this;
}

template <typename T,int p>
inline fixed<T,p> &fixed<T,p>::operator=(const float a) {
	x = (T)(a*(float)(1<<p));
	return *this;
}

template <typename T,int p>
inline fixed<T,p> &fixed<T,p>::operator=(const int a) {
	x = (T)(a << p);
	return *this;
}

/*
template <typename T, int p>
inline fixed<T, p> fixed<T, p>::operator*(const fixed &a) const {
	return fixed((T)((x * a.x) >> p));
}*/

template<>
inline fixed<short, 3> fixed<short, 3>::operator*(const fixed<short, 3> &a) const {
	return fixed((short)(((int)x * a.x) >> 3));
}

template<>
inline fixed<int, 16> fixed<int, 16>::operator*(const fixed<int, 16> &a) const {
	return fixed((int)(((__int64_t)x * a.x) >> 16));
}

template<>
inline fixed<int, 24> fixed<int, 24>::operator*(const fixed<int, 24> &a) const {
	return fixed((int)(((__int64_t)x * a.x) >> 24));
}

template<>
inline fixed<int, 24> fixed<int, 24>::operator*(const fixed<short, 3> &a) const {
	return fixed((int)(((__int64_t)x * a.x) >> 3));
}

template <>
inline fixed<int, 16> fixed<int, 24>::multiply16(fixed<short, 3> &a) {
	return fixed<int, 16>((int)(((__int64_t)x * a.x) >> 11));
}

template <>
inline fixed<int, 16> fixed<int, 24>::multiply3(fixed<short, 3> &a) {
	return fixed<int, 16>((int)(((__int64_t)x * a.x) >> 11));
}


template <typename T, int p>
inline fixed<T,p> fixed<T,p>::operator*(const float a) const {
	return fixed((T)(x * a));
}

template <typename T,int p>
inline fixed<T, p> fixed<T, p>::operator/(const fixed &a) const {
	return fixed((T)((x << p) / a.x));
}

template<>
inline fixed<int, 16> fixed<int, 16>::operator/(const fixed &a) const {
	return fixed<int, 16>((int)((((__int64_t)x) << 16) / a.x));
}

template <typename T, int p>
inline fixed<T, p> fixed<T, p>::operator/(const float a) const {
	return fixed((T)((x << p) / (T)(a*(float)(1 << p))));
}

template <typename T, int p>
inline fixed<T, p> fixed<T, p>::operator/(const int a) const {
	return fixed((T)((x) / (T)(a)));
}

template <typename T, int p>
inline fixed<T, p> fixed<T, p>::operator+(const fixed<T, p> &a) const {
	return fixed((T)(x + a.x));
}

template <typename T, int p>
inline fixed<T, p> fixed<T, p>::operator-(const fixed<T, p> &a) const {
	return fixed((T)(x - a.x));
}

template <typename T, int p>
inline fixed<T, p> &fixed<T, p>::operator+=(const fixed<T, p> &a) {
	x += a.x;
	return *this;
}

template <typename T, int p>
inline fixed<T, p> &fixed<T, p>::operator-=(const fixed<T, p> &a) {
	x -= a.x;
	return *this;
}

template <typename T, int p>
inline fixed<T, p> &fixed<T, p>::operator+=(const float a) {
	x += (T)(a*(float)(1 << p));
	return *this;
}

template <typename T, int p>
inline fixed<T, p> &fixed<T, p>::operator-=(const float a) {
	x -= (T)(a*(float)(1 << p));
	return *this;
}

template <typename T, int p>
inline fixed<T, p> &fixed<T, p>::operator*=(const fixed<T, p> &a) {
	x *= a.x;
	x >>= p;
	return *this;
}

template <typename T, int p>
inline fixed<T, p> &fixed<T, p>::operator/=(const float a) {
	x /= a;
	return *this;
}

template <typename T, int p>
inline bool fixed<T, p>::operator<(const fixed<T, p> &a) const {
	return x < a.x;
}

template <typename T, int p>
inline bool fixed<T, p>::operator<=(const fixed<T, p> &a) const {
	return x <= a.x;
}

template <typename T, int p>
inline bool fixed<T, p>::operator<(const int a) const {
	return x < (a << p);
}

template <typename T, int p>
inline bool fixed<T, p>::operator<=(const int a) const {
	return x <= (a << p);
}

template <typename T, int p>
inline bool fixed<T, p>::operator<=(const float a) const {
	return x <= (T)(a*(float)(1 << p));
}

template <typename T, int p>
inline bool fixed<T, p>::operator>(const fixed<T, p> &a) const {
	return x > a.x;
}

template <typename T, int p>
inline bool fixed<T, p>::operator>=(const fixed<T, p> &a) const {
	return x >= a.x;
}

template <typename T, int p>
inline bool fixed<T, p>::operator>(const int a) const {
	return x > (a << p);
}

template <typename T, int p>
inline bool fixed<T, p>::operator>=(const int a) const {
	return x >= (a << p);
}

template <typename T, int p>
inline bool fixed<T, p>::operator>=(const float a) const {
	return x >= (T)(a*(float)(1 << p));
}

template <typename T, int p>
inline bool fixed<T, p>::operator==(const int a) const {
	return x == (a << p);
}

/*
template <typename T, int p>
inline fixed<T, p>::operator float()  {
	return (float)x/(1<<p);
}
*/

template <typename T, int p>
inline float fixed<T, p>::c_float() const  {
	return (float)x / (1 << p);
}

template <typename T, int p>
inline short fixed<T, p>::c_short() const  {
	return (short)(x>>p);
}

template <typename T, int p>
inline int fixed<T, p>::c_int() const  {
	return (int)(x >> p);
}

template <typename T, int p>
inline fixed<T, p> fixed<T, p>::abs() const  {
	return fixed<T,p>(x>0 ? x : -x);
}

template <typename T, int p>
inline fixed<T, p> &fixed<T, p>::operator/=(const int a) {
	x /= a;
	return *this;
}


typedef fixed<unsigned char, 0> fixed8p0;
typedef fixed<short, 3> fixed16p3;
typedef fixed<short, 12> fixed16p12;
typedef fixed<int, 16> fixed32p16;
typedef fixed<int, 24> fixed32p24;

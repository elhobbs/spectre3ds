#pragma once
#include "sys.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.141592653589793f
#endif // !M_PI


// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2

inline void AngleVectors(vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float		angle;
	float		sr, sp, sy, cr, cp, cy;

	angle = angles[YAW] * (M_PI * 2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI * 2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI * 2 / 360);
	sr = sin(angle);
	cr = cos(angle);

	forward[0] = cp*cy;
	forward[1] = cp*sy;
	forward[2] = -sp;
	right[0] = (-1 * sr*sp*cy + -1 * cr*-sy);
	right[1] = (-1 * sr*sp*sy + -1 * cr*cy);
	right[2] = -1 * sr*cp;
	up[0] = (cr*sp*cy + -sr*-sy);
	up[1] = (cr*sp*sy + -sr*cy);
	up[2] = cr*cp;
}

inline void VectorCopy(vec3_t a, vec3_t b)
{
	b[0] = a[0];
	b[1] = a[1];
	b[2] = a[2];
}

inline void VectorSubtract(vec3_t a, vec3_t b, vec3_t c)
{
	c[0] = a[0] - b[0];
	c[1] = a[1] - b[1];
	c[2] = a[2] - b[2];
}

#define Length(v) (sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]))
#define DotProduct(x,y) (x[0]*y[0]+x[1]*y[1]+x[2]*y[2])

inline float anglemod(float a)
{
	a = (float)((360.0 / 65536) * ((int)(a*(65536 / 360.0)) & 65535));
	return a;
}

inline void VectorMA(vec3_t a, float s, vec3_t b, vec3_t c)
{
	c[0] = a[0] + (s * b[0]);
	c[1] = a[1] + (s * b[1]);
	c[2] = a[2] + (s * b[2]);
}

inline void VectorAdd(vec3_t a, vec3_t b, vec3_t c)
{
	c[0] = a[0] + b[0];
	c[1] = a[1] + b[1];
	c[2] = a[2] + b[2];
}

inline float VectorNormalize(vec3_t v)
{
	float	length, ilength;

	length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	length = sqrt(length);		// FIXME

	if (length)
	{
		ilength = 1.0f / length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}

	return length;

}

inline void VectorScale(vec3_t a, float b, vec3_t c)
{
	c[0] = a[0] * b;
	c[1] = a[1] * b;
	c[2] = a[2] * b;
}

inline void CrossProduct(vec3_t a, vec3_t b, vec3_t c)
{
	c[0] = a[1] * b[2] - a[2] * b[1];
	c[1] = a[2] * b[0] - a[0] * b[2];
	c[2] = a[0] * b[1] - a[1] * b[0];
}


extern vec3_t vec3_origin;
extern	int nanmask;
#define	IS_NAN(x) (((*(int *)&x)&nanmask)==nanmask)

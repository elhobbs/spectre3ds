#include "Frustum.h"

#define DEG2RAD(a) ((a) * (3.14159265358979323846f / 180.0f))

void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal)
{
	float d;
	vec3_t n;
	float inv_denom;

	inv_denom = 1.0F / DotProduct(normal, normal);

	d = DotProduct(normal, p) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector(vec3_t dst, const vec3_t src)
{
	int	pos;
	int i;
	float minelem = 1.0F;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for (pos = 0, i = 0; i < 3; i++)
	{
		if (fabs(src[i]) < minelem)
		{
			pos = i;
			minelem = fabs(src[i]);
		}
	}
	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane(dst, tempvec, src);

	/*
	** normalize the result
	*/
	VectorNormalize(dst);
}

#ifdef _WIN32
#pragma optimize( "", off )
#endif

/*
================
R_ConcatRotations
================
*/
void R_ConcatRotations(float in1[3][3], float in2[3][3], float out[3][3])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
		in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
		in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
		in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
		in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
		in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
		in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
		in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
		in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
		in1[2][2] * in2[2][2];
}

void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees)
{
	float	m[3][3];
	float	im[3][3];
	float	zrot[3][3];
	float	tmpmat[3][3];
	float	rot[3][3];
	int	i;
	vec3_t vr, vup, vf;

	vf[0] = dir[0];
	vf[1] = dir[1];
	vf[2] = dir[2];

	PerpendicularVector(vr, dir);
	CrossProduct(vr, vf, vup);

	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	memcpy(im, m, sizeof(im));

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	memset(zrot, 0, sizeof(zrot));
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

	zrot[0][0] = cos(DEG2RAD(degrees));
	zrot[0][1] = sin(DEG2RAD(degrees));
	zrot[1][0] = -sin(DEG2RAD(degrees));
	zrot[1][1] = cos(DEG2RAD(degrees));

	R_ConcatRotations(m, zrot, tmpmat);
	R_ConcatRotations(tmpmat, im, rot);

	for (i = 0; i < 3; i++)
	{
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
	}
}

#ifdef _WIN32
#pragma optimize( "", on )
#endif

Frustum::Frustum() {
}

void Frustum::transform(vec3_t origin, vec3_t vpn, vec3_t vright, vec3_t vup, float fov_x, float fov_y) {
	int		i;
	vec3_t temp;
	vec3_t normal[4];


	if (fov_x == 90)
	{
		// front side is visible
		VectorAdd(vpn, vright, normal[0]);
		VectorSubtract(vpn, vright, normal[1]);

		VectorAdd(vpn, vup, normal[2]);
		VectorSubtract(vpn, vup, normal[3]);
	}
	else
	{
		// rotate VPN right by FOV_X/2 degrees
		RotatePointAroundVector(normal[0], vup, vpn, -(90 - fov_x / 2));
		// rotate VPN left by FOV_X/2 degrees
		RotatePointAroundVector(normal[1], vup, vpn, 90 - fov_x / 2);
		// rotate VPN up by FOV_X/2 degrees
		RotatePointAroundVector(normal[2], vright, vpn, 90 - fov_y / 2);
		// rotate VPN down by FOV_X/2 degrees
		RotatePointAroundVector(normal[3], vright, vpn, -(90 - fov_y / 2));
	}

	for (i = 0; i<4; i++)
	{
		VectorNormalize(normal[i]);

		float dist = DotProduct(origin, normal[i]);

		//m_planes[i].m_type = PLANE_ANYZ;
		m_planes[i].m_normal = normal[i];
		m_planes[i].m_dist = dist;
		m_planes[i].calc_signbits();
	}
}

unsigned int Frustum::cull(bbox &box, unsigned int planebits) {
	int r, bits = planebits;
#if 1
	if ((planebits & 1) != 0) {
		r = m_planes[0].on_side(box);
		if (r == 2) {
			return 0;
		}
		else if (r == 1) {
			bits &= 14;
		}
	}
#endif
#if 1
	if ((planebits & 2) != 0) {
		r = m_planes[1].on_side(box);
		if (r == 2) {
			return 0;
		}
		else if (r == 1) {
			bits &= 13;
		}
	}
#endif
#if 1
	if ((planebits & 4) != 0) {
		r = m_planes[2].on_side(box);
		if (r == 2) {
			return 0;
		}
		else if (r == 1) {
			bits &= 11;
		}
	}
#endif
#if 1
	if ((planebits & 8) != 0) {
		r = m_planes[3].on_side(box);
		if (r == 2) {
			return 0;
		}
		else if (r == 1) {
			bits &= 7;
		}
	}
#endif
	return planebits;
}
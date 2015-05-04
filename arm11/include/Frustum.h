#pragma once
#include "sys.h"
#include "vec3_fixed.h"
#include "Model.h"

class Frustum {
public:
	Frustum();
	void transform(vec3_t origin, vec3_t vpn, vec3_t right, vec3_t up, float fov_x, float fov_y);
	unsigned int cull(bbox & box, unsigned int planebits);
private:
	q1_plane	m_planes[4];
};
#include "sys.h"
#include "Host.h"
#include "DLight.h"

void DLights::alloc(int key, float *origin, int radius, float die, float decay, float minlight) {

	DLight *dl,*use=0;
	double cl_time = host.cl_time();

	// first look for an exact key match
	if (key)
	{
		dl = m_list;
		for (int i = 0; i<MAX_DLIGHTS; i++, dl++)
		{
			if (dl->m_key == key)
			{
				use = dl;
				break;
			}
		}
	}

	if (use == 0) {
		// then look for anything else
		dl = m_list;
		for (int i = 0; i < MAX_DLIGHTS; i++, dl++)
		{
			if (dl->m_die < cl_time)
			{
				use = dl;
				break;
			}
		}
		if (use == 0) {
			return;
		}
	}


	dl->m_key = key;
	dl->m_origin = origin;
	dl->m_radius = radius;
	dl->m_die = (float)cl_time + die;
	dl->m_decay = decay;
	dl->m_minlight = minlight;
}

void DLights::clear() {
	DLight *dl = m_list;
	for (int i = 0; i<MAX_DLIGHTS; i++)
	{
		dl->m_key = 0;
		dl->m_radius = 0;
		dl->m_die = 0;
	}
}

void DLights::decay_lights() {
	DLight *dl = m_list;
	double cl_time = host.cl_time();
	double time = host.frame_time();
	for (int i = 0; i<MAX_DLIGHTS; i++, dl++) {
		if (dl->m_die < cl_time || dl->m_radius == 0) {
			continue;
		}
		dl->m_radius -= (float)(time*dl->m_decay);
		if (dl->m_radius < 0) {
			dl->m_radius = 0;
		}
	}
}

bool DLights::light_face(q1_face *face) {
	if (face->m_dlightframe != m_framecount) {
		return false;
	}
	return true;
}

void DLights::light_face(q1_face *face, unsigned int *bl) {
	if (face->m_dlightframe != m_framecount) {
		return;
	}
	//if (strstr(face->m_texinfo->m_miptex->m_name, "quake")) {
	//	int i = 0;
	//}
	fixed32p16 dist;
	fixed32p16 radius;
	fixed32p16 minlight;
	vec3_fixed32 impact;
	vec3_fixed32 origin;
	tex3_fixed16 &u_vec = face->m_texinfo->m_vecs[0];
	tex3_fixed16 &v_vec = face->m_texinfo->m_vecs[1];
	fixed32p16 u_ofs(face->m_texinfo->m_ofs[0]);
	fixed32p16 v_ofs(face->m_texinfo->m_ofs[1]);
	int sext = (face->m_extents[0] >> 4) + 1;
	int text = (face->m_extents[1] >> 4) + 1;
	int smin = face->m_texturemins[0];
	int tmin = face->m_texturemins[1];
	int dynamic_num = 0;
	int dynamic[64][4];
	q1_plane &plane = *(face->m_plane);
	DLight *dl = m_list;
	for (int i = 0; i < MAX_DLIGHTS; i++, dl++) {
		if (!(face->m_dlightbits & (1 << i)))
			continue;		// not lit by this light
		origin = dl->m_origin;
		radius = dl->m_radius;
		dist = *(face->m_plane) * origin;
		radius -= dist.abs();

		minlight = dl->m_minlight;
		if (radius <= minlight) {
			continue;
		}

		minlight = radius - minlight;

		impact = origin - plane.m_normal.scale(dist);

		//render_impact(origin, impact, plane, radius);


		int uu = (impact.dot(u_vec) + u_ofs).c_int();
		int vv = (impact.dot(v_vec) + v_ofs).c_int();

		int x = (uu - smin);
		int y = (vv - tmin);
		int r = radius.c_int();
		int ml = minlight.c_int();
		int sd, td;
		int s, t;
		int d;
		for (t = 0; t<text; t++)
		{
			td = y - t * 16;
			if (td < 0)
				td = -td;
			for (s = 0; s<sext; s++)
			{
				sd = x - s * 16;
				if (sd < 0)
					sd = -sd;
				if (sd > td)
					d = sd + (td >> 1);
				else
					d = td + (sd >> 1);
				if (d < ml)
					bl[t*sext + s] += (r - d) * 256;
			}
		}
	}
}

void DLights::light_face(q1_face *face, int *color) {
	if (face->m_dlightframe != m_framecount) {
		return;
	}
	//if (strstr(face->m_texinfo->m_miptex->m_name, "quake")) {
	//	int i = 0;
	//}
	fixed32p16 dist;
	fixed32p16 radius;
	fixed32p16 minlight;
	vec3_fixed32 impact;
	vec3_fixed32 origin;
	tex3_fixed16 &u_vec = face->m_texinfo->m_vecs[0];
	tex3_fixed16 &v_vec = face->m_texinfo->m_vecs[1];
	fixed32p16 u_ofs(face->m_texinfo->m_ofs[0]);
	fixed32p16 v_ofs(face->m_texinfo->m_ofs[1]);
	int sext = (face->m_extents[0] >> 4) + 1;
	int text = (face->m_extents[1] >> 4) + 1;
	int smin = face->m_texturemins[0];
	int tmin = face->m_texturemins[1];
	int dynamic_num = 0;
	int dynamic[64][4];
	q1_plane &plane = *(face->m_plane);
	DLight *dl = m_list;
	for (int i = 0; i<MAX_DLIGHTS; i++,dl++) {
		if (!(face->m_dlightbits & (1 << i)))
			continue;		// not lit by this light
		origin = dl->m_origin;
		radius = dl->m_radius;
		dist = *(face->m_plane) * origin;
		radius -= dist.abs();

		minlight = dl->m_minlight;
		if (radius <= minlight) {
			continue;
		}

		minlight = radius - minlight;

		impact = origin - plane.m_normal.scale(dist);

		//render_impact(origin, impact, plane, radius);


		int uu = (impact.dot(u_vec) + u_ofs).c_int();
		int vv = (impact.dot(v_vec) + v_ofs).c_int();

		int x = (uu - smin) / 16;
		int y = (vv - tmin) / 16;
		if (minlight.c_int() <= 0) {
			x = x;
		}
		dynamic[dynamic_num][0] = x;
		dynamic[dynamic_num][1] = y;
		dynamic[dynamic_num][2] = radius.c_int();
		dynamic[dynamic_num][3] = minlight.c_int();
		dynamic_num++;
	}

	//no lights contribute
	if (dynamic_num == 0) {
		return;
	}

	int num_points = face->m_num_points;

	for (int i = 0; i < num_points; i++) {
		int uu = (face->m_points[i].dot(u_vec) + u_ofs).c_int();
		int vv = (face->m_points[i].dot(v_vec) + v_ofs).c_int();

		int x = (uu-smin) / 16;
		int y = (vv-tmin) / 16;

		int colr = 0;
		for (int j = 0; j < dynamic_num; j++) {

			int sd = x - dynamic[j][0];
			int td = y - dynamic[j][1];

			if (sd < 0) {
				sd = -sd;
			}
			if (td < 0) {
				td = -td;
			}
			int dst;
			sd *= 16;
			td *= 16;
			if (sd > td) {
				dst = sd + (td >> 1);
			}
			else {
				dst = td + (sd >> 1);
			}
			if (dst < dynamic[j][3]) {
				dst = (dynamic[j][2] - dst)*256;
				colr += dst;
			}
		}
		color[i] += colr;
	}
}

int DLights::light_point(vec3_fixed32 p) {
	double cl_time = host.cl_time();
	
	vec3_fixed32 pp(p);
	vec3_fixed32 origin;
	fixed32p16 dist;
	int colr = 0;
	DLight *dl = m_list;
	for (int i = 0; i < MAX_DLIGHTS; i++, dl++) {
		if (dl->m_die < cl_time) {
			continue;
		}
		origin = dl->m_origin;
		dist = (pp - origin).length();
		int add = (dl->m_radius - dist).c_int();
		if (add > 0) {
			colr += add;
		}
	}
	return colr;
}

void DLights::mark_light(DLight *dl, int bit, q1_bsp_node *node) {
	fixed32p16 dist;
	q1_face *face;
	int i, num;

	do {
		if (node == 0 || node->m_contents < 0) {
			return;
		}

		dist = *(node->m_plane) * dl->m_origin;
		if (dist >  dl->m_radius) {
			node = node->m_children[0];
			continue;
		}
		if (dist <  -dl->m_radius) {
			node = node->m_children[1];
			continue;
		}
		
		//mark all faces on node
		num = node->m_num_faces;
		face = node->m_first_face;
		for (i = 0; i < num; i++,face++) {
			if (face->m_dlightframe != m_framecount)
			{
				face->m_dlightbits = 0;
				face->m_dlightframe = m_framecount;
			}
			face->m_dlightbits |= bit;
		}

		mark_light(dl, bit, node->m_children[0]);

		node = node->m_children[1];
	} while (1);
}

void DLights::mark_lights(q1_bsp_node *node) {
	if (!m_dynamic) {
		return;
	}
	DLight *dl = m_list;
	double cl_time = host.cl_time();

	m_framecount++;

	for (int i = 0; i < MAX_DLIGHTS; i++, dl++) {
		if (dl->m_die < cl_time || dl->m_radius == 0) {
			continue;
		}
		mark_light(dl, 1 << i, node);
	}
}
#pragma once

#include "FileSystem.h"
#include "vec3_fixed.h"

#include "q1MdlFile.h"

#include "memPool.h"
#include "Model.h"

#include "gs.h"

#define Q1_MDL_FRAME_NAMES 0

typedef struct vboList_s {
	u32	prim;
	u32 length;
	u32 count;
	struct vboList_s *next;
} vboList_t;

typedef struct {
	int		onseam;
	fixed16p12	s;
	fixed16p12	t;
} mstvert_t;

class q1MdlFrame {
public:
	q1MdlFrame();
	int				build(int num, dtriangle_t *tris, trivertx_t *verts, mstvert_t *st, int skinwidth, int skinheight);
#if Q1_MDL_FRAME_NAMES
	char			m_name[16];	// frame name from grabbing
#endif
	//int				m_num_verts;
	//vec3_fixed8		*m_verts;
	trivertx_t		*m_triverts;
	vboList_t		m_vboList;
	trivertx_t		*m_verts;
};

inline q1MdlFrame::q1MdlFrame() {
#if Q1_MDL_FRAME_NAMES
	m_name[0] = 0;
#endif
	//m_num_verts = 0;
	//m_verts = 0;
}

class q1MdlFrameGroup {
public:
	q1MdlFrameGroup();
	int render(int frame);
	int allocate_frames(int numframes);
	unsigned char	m_min[3];
	unsigned char	m_max[3];
	int				m_num_frames;
	q1MdlFrame		*m_frames;
	float			*m_intervals;
};

inline q1MdlFrameGroup::q1MdlFrameGroup() {
	m_frames = 0;
	m_num_frames = 0;
	m_intervals = 0;
}

inline int q1MdlFrameGroup::allocate_frames(int numframes) {
	m_num_frames = numframes;
	m_frames = new(pool) q1MdlFrame[numframes];
	return 0;
}

class q1Mdl:public Model {
public:
	q1Mdl(char *name,sysFile* file);
	int trace(traceResults &trace, int hull, vec3_fixed16 &p0, vec3_fixed16 &p1);

	q1MdlFrame* get_frame(int framenum);
	tex3_fixed16& scale();
	vec3_fixed16& scale_origin();
	int num_tris();
	int get_skin();
	int get_skin(int skin);
	int get_triangle(q1MdlFrame *frame, int index, float *tri, float *norm);
	friend class ViewState;
	friend class q1MdlFrame;
	static vec3_t	m_normals[162];
	static void set_render_mode();

	u32				*m_control;
private:
	//int				build_uv(int num, dtriangle_t *tris, stvert_t *in_stverts);

	int				m_skin_width;
	int				m_skin_height;
	int				m_num_skins;
	unsigned int	*m_skin_texture_ids;
	//fixed16p12		*m_u;
	//fixed16p12		*m_v;
	tex3_fixed16	m_scale;
	vec3_fixed16	m_scale_origin;
	int				m_num_groups;
	q1MdlFrameGroup	*m_groups;
	int rframe;

	mstvert_t		*m_st;
	int				m_num_tris;
	dtriangle_t		*m_tris;
	bool			m_fullbright;
};

inline int q1Mdl::num_tris() {
	return m_num_tris;
}

inline int q1Mdl::get_skin() {
	return m_skin_texture_ids[0];
}

inline int q1Mdl::get_skin(int skin) {
	if (skin >= m_num_skins || skin < 0) {
		skin = 0;
	}
	return m_skin_texture_ids[skin];
}

inline tex3_fixed16& q1Mdl::scale() {
	return m_scale;
}

inline vec3_fixed16& q1Mdl::scale_origin() {
	return m_scale_origin;
}

inline int q1Mdl::get_triangle(q1MdlFrame *frame, int index, float *tri, float *norm) {
	for (int j = 0; j < 3; j++) {
		float x = frame->m_triverts[m_tris[index].vertindex[j]].v[0];
		float y = frame->m_triverts[m_tris[index].vertindex[j]].v[1];
		float z = frame->m_triverts[m_tris[index].vertindex[j]].v[2];
		float s = m_st[m_tris[index].vertindex[j]].s.c_float();
		float t = m_st[m_tris[index].vertindex[j]].t.c_float();
		if (!m_tris[index].facesfront && m_st[m_tris[index].vertindex[j]].onseam) {
			s += 0.5;
		}
		*tri++ = x;
		*tri++ = y;
		*tri++ = z;
		*tri++ = s;
		*tri++ = t;

		*norm++ = m_normals[frame->m_triverts[m_tris[index].vertindex[j]].lightnormalindex][0];
		*norm++ = m_normals[frame->m_triverts[m_tris[index].vertindex[j]].lightnormalindex][1];
		*norm++ = m_normals[frame->m_triverts[m_tris[index].vertindex[j]].lightnormalindex][2];
	}
	return 0;
}
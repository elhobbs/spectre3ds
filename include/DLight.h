#pragma once
#include "vec3_fixed.h"

#define	MAX_DLIGHTS		32

class DLight {
private:
	vec3_fixed16	m_origin;
	fixed32p16		m_radius;
	float			m_die;				// stop lighting after this time
	float			m_decay;			// drop this each second
	float			m_minlight;			// don't add when contributing less
	int				m_key;

	friend class DLights;
};

class DLights {
public:
	DLights();
	void clear();

	void setup_lights();
	void decay_lights();
	void frame();
	void dynamic(bool on);
	bool dynamic();

	void alloc(int key, float *origin,int radius, float die, float decay, float minlight);

	void render_impact(vec3_fixed32 origin, vec3_fixed32 impact, q1_plane &plane, fixed32p16 radius);
	void light_face(q1_face *face, int *color);
	void light_face(q1_face *face, unsigned int *lightmap);
	bool light_face(q1_face *face);
	int light_point(vec3_fixed32 p);
	void mark_light(DLight *dl, int bit, q1_bsp_node *node);
	void mark_lights(q1_bsp_node *node);
private:
	int			m_framecount;
	bool		m_dynamic;
	DLight		m_list[MAX_DLIGHTS];
};

inline DLights::DLights() {
	m_framecount = 1;
	m_dynamic = true;
}
inline void DLights::frame() {
	m_framecount++;
	dynamic(true);
}
inline void DLights::dynamic(bool on) {
	m_dynamic = on;
}
inline bool DLights::dynamic() {
	return m_dynamic;
}

#pragma once
#include "vec3_fixed.h"
#include "ctr_vbo.h"

typedef struct {
	float pos[3];
	float color[4];
} vbo_particle_t;

typedef enum {
	pt_static, pt_grav, pt_slowgrav, pt_fire, pt_explode, pt_explode2, pt_blob, pt_blob2, pt_max = 0xffffffff
} ptype_t;

class Particle {
public:
	// driver-usable fields
	vec3_fixed32	org;
	int				color;

	// drivers never touch the following fields
	Particle		*next;
	vec3_fixed32	vel;
	float			ramp;
	float			die;
	ptype_t			type;
};

#define MAX_PARTICLES 8192

class Particles {
public:
	Particles();
	void		init();
	void		clear();

	void rocket_trail(vec3_t startf, vec3_t endf, int type);
	void teleport_splash(vec3_t orgf);
	void explosion(vec3_t orgf);
	void explosion2(vec3_t org, int colorStart, int colorLength);
	void effect(vec3_t org, vec3_t dir, int color, int count);
	void lava_splash(vec3_t orgf);

	void bind_texture();

	void render(vbo_particle_t& p);

	friend class ViewState;
protected:
	ctrVbo_t	m_vbo_particles;
	ctrVbo_t	m_vbo_index;
private:
	int			m_texture_id;
	int			m_num_particles;
	Particle	*m_particles;
	Particle	*m_active_particles;
	Particle	*m_free_particles;
	static const int		ramp1[8];
	static const int		ramp2[8];
	static const int		ramp3[8];
};

inline Particles::Particles() {
	m_num_particles = MAX_PARTICLES;
	m_particles = new Particle[m_num_particles];
	clear();
}

inline void Particles::clear() {
	m_active_particles = 0;
	m_free_particles = &m_particles[0];
	for (int i = 0; i < m_num_particles-1; i++) {
		m_particles[i].next = &m_particles[i + 1];
	}
	m_particles[m_num_particles-1].next = 0;
}


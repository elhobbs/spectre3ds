#pragma once

#include "mathlib.h"
#include "CmdBuffer.h"
#include "Particle.h"
#include "q1Bsp.h"
#include "cvar.h"
#include "Dlight.h"

extern cvar_t	r_ambient;
extern cvar_t	r_fullbright;


typedef struct entity_s
{
	bool					forcelink;		// model changed

	int						update_type;

	entity_state_t			baseline;		// to fill in defaults in updates

	float					msgtime;		// time of last update
	vec3_t					msg_origins[2];	// last two updates (0 is newest)	
	vec3_t					origin;
	vec3_t					msg_angles[2];	// last two updates (0 is newest)
	vec3_t					angles;
	Model					*model;			// NULL = no model
	struct efrag_s			*m_efrag;			// linked list of efrags
	int						frame;
	float					syncbase;		// for client-side animations
	byte					*colormap;
	int						effects;		// light, particals, etc
	int						skinnum;		// for Alias models
	int						visframe;		// last frame this entity was
	//  found in an active leaf

	struct entity_s			*next;
	int						dlightframe;	// dynamic lighting
	int						dlightbits;

	// FIXME: could turn these into a union
	int						trivial_accept;
	q1_bsp_node				*m_topnode;		// for bmodels, first world node
	//  that splits bmodel, or NULL if
	//  not split
} entity_t;

typedef struct
{
	int		entity;
	Model	*model;
	float	endtime;
	vec3_t	start, end;
} beam_t;

#define MAX_VISEDICTS	256
#define	MAX_EFRAGS		640

class ViewState {
public:
	ViewState();
	void init();
	void clear();
	void reset();
	void render(float *origin,float *angles);
	void load_dirty_lightmaps(q1Bsp *bsp);
	void set_world(q1Bsp *world);
	void set_viewent(entity_t *ent);
	void add(entity_t *ent);
	void add_efrags(entity_t *ent);
	void remove_efrags(entity_t *ent);
	void store_efrags(efrag_t *ppefrag);
	void render(entity_t *ent);
	void render_viewent();
	void animate_lights(struct lightstyle_s *lights);
	int light_point(vec3_t p);
	q1_leaf_node *view_leaf();
	DLights						m_dlights;
	Particles					m_particles;
private:
	void split_entity_on_node(q1_bsp_node *node, entity_t *ent);
	int light_point_r(q1_bsp_node *node, fixed32p16 f0, fixed32p16 f1, vec3_fixed32 &p0, vec3_fixed32 &p1);

	void render_world(float *origin, float *angles);
	void render_sprite(entity_t *ent);
	void render_mdl(entity_t *ent);
	void render_bsp(entity_t *ent);
	int	render_bsp_r(q1_bsp_node *node);
	void render_leaf(q1_leaf_node *leaf);
	void render_face(q1_face *face,int framenum=0);
	void render_particles();

	bool			m_draw_sky;

	int				m_numvisedicts;
	entity_t		*m_visedicts[MAX_VISEDICTS];

	int				m_framecount;
	vec3_t			m_emins, m_emaxs;
	efrag_t			**m_lastlink;
	q1_bsp_node		*m_pefragtopnode;
	efrag_s			*m_free_efrags;
	efrag_t			m_efrags[MAX_EFRAGS];
	q1Bsp			*m_worldmodel;
	q1_leaf_node	*m_viewleaf;
	entity_t		*m_viewent;

	vec3_fixed32	m_camera;
	vec3_fixed32	m_forward, m_right, m_up;

	int				m_ambient;
	int				m_fullbright;
	int				m_lightstylevalue[256];	// 8.8 fraction of base light value

	gsVbo_s			m_vbo;
	int				m_no_light_id;
};

inline ViewState::ViewState() {
	Cvar_RegisterVariable(&r_ambient);
	Cvar_RegisterVariable(&r_fullbright);
	m_framecount = 1;
	m_fullbright = 0;
}

inline q1_leaf_node* ViewState::view_leaf() {
	return m_viewleaf;
}

inline void ViewState::set_world(q1Bsp *world) {
	m_worldmodel = world;
}

inline void ViewState::set_viewent(entity_t *ent) {
	m_viewent = ent;
}

inline void ViewState::clear() {
	m_numvisedicts = 0;
	memset(m_visedicts, 0, sizeof(m_visedicts));
	//m_framecount++;
}

inline void ViewState::reset() {
	int i;
	clear();
	//
	// allocate the efrags and chain together into a free list
	//
	m_free_efrags = m_efrags;
	for (i = 0; i<MAX_EFRAGS - 1; i++)
		m_free_efrags[i].entnext = &m_free_efrags[i + 1];
	m_free_efrags[i].entnext = 0;
	m_dlights.clear();
	m_particles.clear();
}

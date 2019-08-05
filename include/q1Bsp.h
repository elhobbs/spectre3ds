#pragma once

#include "FileSystem.h"
#include "vec3_fixed.h"

#include "q1BspFile.h"
#include "memPool.h"
#include "Model.h"

#define FACE_SKY 4

class q1_edge {
public:
	unsigned short	v[2];		// vertex numbers
};

class q1_texture {
public:
	q1_texture();
	q1_texture *animate(int frame);

	void bind();

	char m_name[16];
	int m_id, m_width, m_height;
	int m_id2; //this will be used for red full bright alternate texture
	int			anim_total;				// total tenths in sequence ( 0 = no)
	int			anim_min, anim_max;		// time for this frame min <=time< max
	q1_texture *anim_next;		// in the animation sequence
	q1_texture *alternate_anims;	// bmodels in frmae 1 use these

	byte		*m_data; //original 8 bit source
	byte		*m_cache; //original 8 bit source
	bool		m_cachable;
};

inline q1_texture::q1_texture() {
}

class q1_texinfo {
public:
	q1_texinfo();
	tex3_fixed16		m_vecs[2];
	fixed16p3			m_ofs[2];
	q1_texture			*m_miptex;
	int					m_flags;
};

inline q1_texinfo::q1_texinfo() {
}

#define LIGHTMAP_BIN_WIDTH 64
#define LIGHTMAP_BIN_HEIGHT 64

typedef struct q1_lightmap_s {
	int		m_id;
	int		m_width, m_height;
	int		m_dirty;
	byte	m_dirty_map[(LIGHTMAP_BIN_WIDTH / 8)*(LIGHTMAP_BIN_HEIGHT / 8)];
	byte	*m_data;

	void	mark_dirty(short *rect);
	void	update_block(unsigned int *blocklight, short *rect, bool rotated);

	struct q1_lightmap_s *m_next;
} q1_lightmap;

class q1_face {
public:
	
	q1_face();

	int				render(int frame);
	void			build_vertex_array();
	void			build_lightmap(unsigned int *blocklights, int *lightstyles);
	void			bind();

	q1_plane		*m_plane;
	int				side;

	int				m_num_points;
	vec3_fixed16	*m_points;
	
	fixed32p16		*m_u;
	fixed32p16		*m_v;
	short			*m_lightmap_ofs;

	short			m_extents[2];
	short			m_texturemins[2];

	q1_texinfo		*m_texinfo;

	int				m_visframe;
	int				m_dlightframe;
	int				m_dlightbits;
	bool			m_cached_dlight;
	//gsVbo_s			m_vbo;
	//byte			*m_cache;

	void			*m_vertex_array;

	// lighting info
	unsigned char	m_styles[MAXLIGHTMAPS];
	int				m_cached_light[MAXLIGHTMAPS];	// values currently used in lightmap
	unsigned char	*m_lightmaps;		// start of [numstyles*surfsize] samples
	unsigned char	*m_vertlights;

	q1_lightmap		*m_lightmap_id;
	short			m_lightmap_rect[4];
	bool			m_lightmap_rotated;
};

inline q1_face::q1_face() {
	//m_cache = 0;
}

#define	CONTENTS_EMPTY		-1
#define	CONTENTS_SOLID		-2
#define	CONTENTS_WATER		-3
#define	CONTENTS_SLIME		-4
#define	CONTENTS_LAVA		-5
#define	CONTENTS_SKY		-6

#define	CONTENTS_CURRENT_0		-9
#define	CONTENTS_CURRENT_90		-10
#define	CONTENTS_CURRENT_180	-11
#define	CONTENTS_CURRENT_270	-12
#define	CONTENTS_CURRENT_UP		-13
#define	CONTENTS_CURRENT_DOWN	-14

class q1_clipnode {
public:
	q1_clipnode();
	q1_plane	*m_plane;
	short		m_children[2];
};

inline q1_clipnode::q1_clipnode() {
}

class q1_node {
public:
	q1_node();
	int				m_contents;
	int				m_visframe;
	bbox			m_bounds;
	q1_node			*m_parent;
};

inline q1_node::q1_node() {
}

class q1_bsp_node : public q1_node {
public:
	q1_bsp_node();
	q1_face			*m_first_face;
	int				m_num_faces;
	q1_plane		*m_plane;
	q1_bsp_node		*m_children[2];

	void			set_parent_r(q1_node *parent);
};

inline q1_bsp_node::q1_bsp_node() : q1_node(){
	m_children[0] = m_children[1] = 0;
}


class q1_leaf_node : public q1_node {
public:
	q1_leaf_node();
	int				render(int frame);
	int				m_num_faces;
	q1_face			**m_first_face;
	unsigned char	*m_compressed_vis;
	struct efrag_s	*m_efrags;
	byte			m_ambient_sound_level[NUM_AMBIENTS];
};

typedef struct efrag_s
{
	q1_leaf_node		*leaf;
	struct efrag_s		*leafnext;
	struct entity_s		*entity;
	struct efrag_s		*entnext;
} efrag_t;

inline q1_leaf_node::q1_leaf_node() : q1_node(){
}


#define	MAX_MAP_HULLS		4

typedef struct
{
	float		mins[3], maxs[3];
	float		origin[3];
	int			headnode[MAX_MAP_HULLS];
	int			visleafs;		// not including the solid leaf 0
	int			firstface, numfaces;
} dmodel_t;

class q1Model {
public:
	bbox			m_bounds;
	vec3_fixed16	m_origin;
	int				m_headnode[MAX_MAP_HULLS];
	int				m_visleafs;
	int				m_firstface, m_numfaces;
};

typedef struct
{
	q1_clipnode	*clipnodes;
	q1_plane	*planes;
	int			firstclipnode;
	int			lastclipnode;
	vec3_t		clip_mins;
	vec3_t		clip_maxs;
} hull_t;

class q1Bsp : public Model {
public:
	q1Bsp(char *name, sysFile* file);
	q1Bsp(int submodel, q1Bsp *bsp);

	q1_leaf_node*	in_leaf();
	q1_leaf_node*	in_leaf(vec3_fixed16 &pos);
	q1_leaf_node*	in_leaf(vec3_t pos);
	int				leaf_num(q1_leaf_node *leaf);
	int render(int frame);
	int trace(traceResults &trace, int hull, vec3_fixed16 &p0, vec3_fixed16 &p1);

	char* load_entities();

	int num_submodels();

	int				m_visframe;

	dheader_t		m_header;

	q1_face*		get_face_list();
	int				get_face_count();

	int render_bsp(q1_leaf_node &leaf);
	int render();

	int trace_r(q1_bsp_node *parent, q1_bsp_node *node, traceResults &trace, fixed32p16 f0, fixed32p16 f1, vec3_fixed32 &p0, vec3_fixed32 &p1);
	int trace_clip_r(short parent, short node, traceResults &trace, fixed32p16 f0, fixed32p16 f1, vec3_fixed32 &p0, vec3_fixed32 &p1);

	unsigned char*	decompress_vis(unsigned char *);
	int				mark_visible_leafs(q1_leaf_node &leaf);
	int				mark_visible_leafs(q1_leaf_node &leaf,int frame);
	int				render_bsp_r(q1_bsp_node *node);

	int				load_planes();
	int				load_texinfos();
	int				load_textures();
	int				load_faces();
	int				load_nodes();
	int				load_clipnodes();
	int				load_markfaces();
	int				load_visibility();
	int				load_lighting();
	int				load_leafs();
	int				load_submodels();
	void			make_hulls();
	void			make_texture_animations();

	void			build_lightmaps();

	int				m_num_planes;
	q1_plane		*m_planes;

	int				m_num_texinfos;
	q1_texinfo		*m_texinfos;

	int				m_num_textures;
	q1_texture		*m_textures;

	int				m_num_faces;
	q1_face			*m_faces;

	int				m_num_nodes;
	q1_bsp_node		*m_nodes;

	int				m_num_clipnodes;
	q1_clipnode		*m_clipnodes;

	int				m_num_markfaces;
	q1_face			**m_markfaces;

	int				m_len_visibility;
	unsigned char	*m_visibility;

	int				m_len_lighting;
	unsigned char	*m_lighting;

	int				m_num_leafs;
	q1_leaf_node	*m_leafs;

	int				m_num_submodels;
	q1Model			*m_submodels;

	hull_t			m_hulls[MAX_MAP_HULLS];

	int				m_firstmodelsurface, m_nummodelsurfaces;

	int				m_frame;

	int				m_skytexture;
	int				m_skytexture_id;

	q1_lightmap		*m_lightmaps;

	static void		set_render_mode();
};

inline q1_face* q1Bsp::get_face_list() {
	return &m_faces[m_firstmodelsurface];
}

inline int q1Bsp::get_face_count() {
	return m_nummodelsurfaces;
}

inline int q1Bsp::num_submodels() {
	return m_num_submodels;
}

#define BOX_ON_PLANE_SIDE(emins, emaxs, p)	\
	(((p)->m_type < 3) ? \
	(\
	((p)->m_dist <= (emins)[(p)->m_type]) ? \
	1								\
	:									\
	(\
	((p)->m_dist >= (emaxs)[(p)->m_type]) ? \
	2							\
	:								\
	3							\
	)									\
	)										\
	:										\
	BoxOnPlaneSide((emins), (emaxs), (p)))

int BoxOnPlaneSide(vec3_t femins, vec3_t femaxs, q1_plane *p);

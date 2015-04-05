#pragma once

#include "FileSystem.h"
#include "vec3_fixed.h"
#include "q1SpriteFile.h"

#include "memPool.h"
#include "Model.h"

#include "gs.h"

class q1SpriteFrame {
public:
	q1SpriteFrame();
	int render();
	//int				build(int num, dtriangle_t *tris,trivertx_t *verts);
#if Q1_MDL_FRAME_NAMES
	char			m_name[16];	// frame name from grabbing
#endif
	short	m_width;
	short	m_height;
	int		m_id;

	int up, down, left, right;
};

inline q1SpriteFrame::q1SpriteFrame() {
#if Q1_MDL_FRAME_NAMES
	m_name[0] = 0;
#endif
}

class q1SpriteFrameGroup {
public:
	q1SpriteFrameGroup();
	int render(int frame);
	int allocate_frames(int numframes);
	unsigned char	m_min[3];
	unsigned char	m_max[3];
	int				m_num_frames;
	q1SpriteFrame	*m_frames;
	float			*m_intervals;
};

inline q1SpriteFrameGroup::q1SpriteFrameGroup() {
	m_frames = 0;
	m_num_frames = 0;
	m_intervals = 0;
}

inline int q1SpriteFrameGroup::allocate_frames(int numframes) {
	m_num_frames = numframes;
	m_frames = new(pool)q1SpriteFrame[numframes];
	return 0;
}


class q1Sprite :public Model {
public:
	q1Sprite(char *name, sysFile* file);
	int trace(traceResults &trace, int hull, vec3_fixed16 &p0, vec3_fixed16 &p1);
	q1SpriteFrame* get_frame(int framenum);
private:

	int load_frame(q1SpriteFrame *frame);
	int load_group(q1SpriteFrameGroup *group);


	int				m_width;
	int				m_height;
	unsigned int	*m_texture_ids;


	int					m_num_groups;
	q1SpriteFrameGroup	*m_groups;
};
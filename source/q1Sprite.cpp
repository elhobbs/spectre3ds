#include "sys.h"
#include "q1Sprite.h"
#include "Host.h"

extern unsigned char *q1_palette;

int q1Sprite::load_frame(q1SpriteFrame *frame) {
	dspriteframe_t header;
	if (m_file->read(reinterpret_cast<char *>(&header), -1, sizeof(header)) == 0) {
		return -1;
	}

	frame->m_width = header.width;
	frame->m_height = header.height;

	frame->up = header.origin[1];
	frame->down = header.origin[1] - header.height;
	frame->left = header.origin[0];
	frame->right = header.origin[0] + header.width;

	//load the sprite pixels
	int size = header.width * header.height;
	char *sprite = new char[size];
	if (m_file->read(sprite, -1, size) == 0) {
		return -1;
	}
	//create the texture and release the temp buffer
	int id = frame->m_id = sys.gen_texture_id();
	sys.load_texture256(id, header.width, header.height, (unsigned char *)sprite, q1_palette,1);
	delete[] sprite;

	return 0;
}

int q1Sprite::load_group(q1SpriteFrameGroup *group) {
	dspritegroup_t header;
	if (m_file->read(reinterpret_cast<char *>(&header), -1, sizeof(header)) == 0) {
		return -1;
	}
	//load intervals
	dspriteinterval_t *intervals = new dspriteinterval_t[header.numframes];
	if (m_file->read(reinterpret_cast<char *>(intervals), -1, sizeof(dspriteinterval_t)*header.numframes) == 0) {
		return -1;
	}
	group->m_intervals = new(pool) float[header.numframes];
	for (int j = 0; j < header.numframes; j++) {
		group->m_intervals[j] = intervals[j].interval;
	}
	delete[] intervals;

	//load frames
	group->allocate_frames(header.numframes);
	for (int i = 0; i < header.numframes; i++) {
		load_frame(&group->m_frames[i]);
	}

	return 0;
}

q1Sprite::q1Sprite(char *name, sysFile *file) :Model(name) {

	m_type = mod_sprite;

	m_file = file;

	dsprite_t spr;
	if (m_file->read(reinterpret_cast<char *>(&spr), 0, sizeof(spr)) == 0) {
		return;
	}
	m_num_groups = spr.numframes;
	m_groups = new(pool) q1SpriteFrameGroup[spr.numframes];

	for (int i = 0; i < m_num_groups; i++) {
		dspriteframetype_t frame_type;
		if (m_file->read(reinterpret_cast<char *>(&frame_type), -1, sizeof(frame_type)) == 0) {
			return;
		}
		if (frame_type.type == SPR_SINGLE) {
			//allocate a single frame
			m_groups[i].allocate_frames(1);
			load_frame(&m_groups[i].m_frames[0]);
		}
		else {
			load_group(&m_groups[i]);
		}
	}
	m_bounds[0][0] = (float)-spr.width / 2.0f;
	m_bounds[0][1] = (float)-spr.width / 2.0f;
	m_bounds[0][2] = (float)-spr.height / 2.0f;
	m_bounds[1][0] = (float)spr.width / 2.0f;
	m_bounds[1][1] = (float)spr.width / 2.0f;
	m_bounds[1][2] = (float)spr.height / 2.0f;
	m_radius = 0;

}

q1SpriteFrame* q1Sprite::get_frame(int framenum) {
	if (framenum >= m_num_groups) {
		framenum = 0;
	}
	q1SpriteFrameGroup *group = &m_groups[framenum];
	q1SpriteFrame *frame = 0;

	if (group->m_intervals) {
		int i;
		int numframes = m_groups[framenum].m_num_frames;
		float *intervals = m_groups[framenum].m_intervals;
		float fullinterval = intervals[numframes - 1];
		float time = host.cl_time();

		float targettime = time - ((int)(time / fullinterval)) * fullinterval;
		for (i = 0; i<(numframes - 1); i++)
		{
			if (intervals[i] > targettime)
				break;
		}
		frame = &m_groups[framenum].m_frames[i];
	}
	else {
		frame = &group->m_frames[0];
	}

	return frame;
}

int q1Sprite::trace(traceResults &trace, int hull, vec3_fixed16 &p0, vec3_fixed16 &p1) {
	return 0;
}

#pragma once

#include "sys.h"
#include "memPool.h"
#include "cvar.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define SIN_BUFFER_SIZE 1280
#define CYCLE 64
#define	AMP		3*0x10000
#define	AMP2	3
#define	SPEED	20

extern int	sintable[SIN_BUFFER_SIZE];
extern unsigned char *q1_palette;

extern cvar_t	proctex;

class procTexture {
public:
	procTexture();
	virtual int update() = 0;
};

inline procTexture::procTexture() {
}

class procTextureList {
public:
	procTextureList(int count);
	int add(procTexture *tx);
	void update();
	static void reset();
private:
	static int			m_num;
	static int			m_max;
	static procTexture	**m_list;
};

inline procTextureList::procTextureList(int count) {
	m_list = new procTexture*[count];
	m_num = 0;
	m_max = count;
	for (int i = 0; i < SIN_BUFFER_SIZE; i++) {
		//sintable[i] = AMP + sin(i*3.14159 * 2 / CYCLE)*AMP;
		sintable[i] = (3.0f + sin(i*M_PI * 2.0f / 64.0f)*3.0f) * 65536.0f;
	}
	Cvar_RegisterVariable(&proctex);
}

inline void procTextureList::reset() {
	m_num = 0;
}

inline int procTextureList::add(procTexture *tx) {
	if (m_num >= m_max) {
		return -1;
	}
	m_list[m_num++] = tx;
	return 0;
}

inline void procTextureList::update() {
	if (proctex.value == 0) {
		return;
	}
	for (int i = 0; i < m_num; i++) {
		m_list[i]->update();
	}
}

class warpTexture : public procTexture {
public:
	explicit warpTexture(int id, int w, int h, unsigned char *data);
	int update();
private:
	int m_id;
	int m_width,m_height;
	byte *m_data;
	byte *m_pal;
	float m_time;
};

inline warpTexture::warpTexture(int id, int w, int h, unsigned char *data) {
	m_id = id;
	m_width = w;
	m_height = h;
	m_data = new(pool) unsigned char[m_width*m_height];
	m_pal = new(pool)byte[16*3];
	memcpy(m_data, data, m_width*m_height);
	//sys.convert_texture256to16(w, h, data, q1_palette, m_data, m_pal);
	//sys.load_texture_byte(m_id, m_width, m_height, m_data, m_pal);
	sys.load_texture256(m_id, m_width, m_height, m_data, q1_palette);
	m_time = sys.seconds() + rand() * 0.1 / 32767.0;
}

class skyTexture : public procTexture {
public:
	explicit skyTexture(int id, int w, int h, unsigned char *data);
	int update();
private:
	int m_id;
	int m_width, m_height;
	unsigned char *m_data_top;
	unsigned char *m_data_bottom;
};

inline skyTexture::skyTexture(int id, int w, int h, unsigned char *data) {
	m_id = id;
	m_width = w / 2;
	m_height = h;
	m_data_top = new(pool) unsigned char[w*h];
	m_data_bottom = m_data_top + w*h / 2;
	for (int i = 0; i < h; i++) {
		memcpy(&m_data_top[i*m_width], &data[i*w], m_width);
		memcpy(&m_data_bottom[i*m_width], &data[i*w + m_width], m_width);
	}
	//sys.load_texture_byte(m_id, m_width, m_height, m_data_bottom, q1_palette);
	sys.load_texture256(m_id, m_width, m_height, m_data_bottom, q1_palette);
}

extern procTextureList proc_textures;

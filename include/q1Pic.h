#pragma once

#include "sys.h"
#include "LoadList.h"
#include "memPool.h"

class q1Pic {
public:
	q1Pic(char *name);
	char *name();
	byte *data();
	void parse(void *p);
	void render(int x, int y);
private:
	char	*m_name;
	int		m_width, m_height;
	int		m_id;
	byte	*m_data;
};

inline q1Pic::q1Pic(char *name) {
	int len = strlen(name);
	m_name = new(pool) char[len + 1];
	if (m_name == 0)
		return;
	memcpy(m_name, name, len);
	m_name[len] = 0;
	m_data = 0;
	m_width = 0;
	m_height = 0;
}

inline char *q1Pic::name() {
	return m_name;
}

extern LoadList<q1Pic, 256> Pics;

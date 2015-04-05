#include "q1Pic.h"
#ifdef WIN32
#include <gl\gl.h>
#include <gl\glu.h>
#endif

LoadList<q1Pic, 256> Pics;

template<>
void LoadList<class q1Pic, 256>::reset() {
	m_num_known = 0;
}

template<>
q1Pic* LoadList<class q1Pic, 256>::load(char *name) {
	q1Pic *pic = new(pool) q1Pic(name);
	if (pic == 0 || pic->name() == 0) {
		return 0;
	}
	return pic;
}

byte *q1Pic::data() {
	if (m_data) {
		return m_data;
	}
	sysFile *file = sys.fileSystem.open(m_name);
	if (file == 0) {
		return 0;
	}
	return m_data;
}

void q1Pic::parse(void *p) {
	int *data = (int *)p;
	m_width = *data++;
	m_height = *data++;
	m_data = (byte *)data;

	m_id = sys.gen_texture_id();
	extern unsigned char *q1_palette;
	sys.load_texture256(m_id, m_width, m_height, m_data, q1_palette, 0);
}

void q1Pic::render(int x, int y) {
#ifdef WIN32
	glColor3f(1, 1, 1);
	glBindTexture(GL_TEXTURE_2D, m_id);
	glBegin(GL_QUADS);

	glTexCoord2f(0, 0);
	glVertex2f(x + 0, y + 0);

	glTexCoord2f(1, 0);
	glVertex2f(x + m_width, y + 0);

	glTexCoord2f(1, 1);
	glVertex2f(x + m_width, y - m_height);

	glTexCoord2f(0, 1);
	glVertex2f(x + 0, y - m_height);

	glEnd();
#endif
}

void render_pic(int id, int x, int y,int width, int height) {
#ifdef WIN32

	glBindTexture(GL_TEXTURE_2D, id);
	glBegin(GL_QUADS);

	glTexCoord2f(0, 0);
	glVertex2f(x + 0, y + 0);

	glTexCoord2f(1, 0);
	glVertex2f(x + width, y + 0);

	glTexCoord2f(1, 1);
	glVertex2f(x + width, y - height);

	glTexCoord2f(0, 1);
	glVertex2f(x + 0, y - height);

	glEnd();
#endif
}
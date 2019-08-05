#include "sys.h"
#include <3ds/gfx.h>
#include <citro3d.h>

#if 0
extern "C" {
extern unsigned char font8x8_basic[128][8];
};
#else
#include "font8x8_basic.h"
#endif

#ifdef WIN32
#include <gl\gl.h>
#include <gl\glu.h>
#endif
#include <stdio.h>
#include <cstdarg>
#include "ctr_vbo.h"

ctrVbo_t con_tris;


int SYS::build_font() {
	unsigned char *buffer = new unsigned char[128 * 8 * 8];
	if (buffer == 0) {
		return -1;
	}

	for (int ord = 0; ord<128; ord++) {

		unsigned char *bitmap = font8x8_basic[ord];
		unsigned char *row = &buffer[(((ord>>4)*16*8*8) + ((ord&15)*8))];

		for (int x = 0; x < 8; x++) {
			for (int y = 0; y < 8; y++) {
				int set = bitmap[x] & 1 << y;
				row[y] = set ? 1 : 0;
			}
			row += (16 * 8);
		}
	}
	m_font_texture_id = gen_texture_id(GPU_NEAREST,GPU_NEAREST);
	byte pal[6] = { 0, 0, 0, 0xff, 0xff, 0xff };
	sys.load_texture256(m_font_texture_id, 16 * 8, 8 * 8, buffer, pal,1);

	delete buffer;
	
	ctrVboInit(&con_tris);
	ctrVboCreate(&con_tris, sizeof(faceVertex_s)*(10000) * 3);

	return 0;
}

void SYS::printf(float x, float y, float scale, char* format, ...) {
	va_list args;
	char buffer[255];

	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);

	print(x, y, scale, buffer);

}
#ifdef WIN32
static float sys_colors[10][4] = {
		{ 0.78125f, 0.59765625f, 0.2578125f, 1.0f }, // 0 not used
		{ 1.00000f, 0.00000000f, 0.0000000f, 1.0f }, //1 red
		{ 0.78125f, 0.59765625f, 0.2578125f, 1.0f }, //2 yellow
		{ 0.00000f, 1.00000000f, 0.0000000f, 1.0f }, //3 green
		{ 0.00000f, 0.00000000f, 1.0000000f, 1.0f }, //4 blue
		{ 1.00000f, 1.00000000f, 1.0000000f, 1.0f }, //5 white
		{ 0.78125f, 0.78125000f, 0.7812500f, 1.0f }, //6 gray
		{ 0.00000f, 0.00000000f, 0.0000000f, 1.0f }, //7 black
		{ 0.78125f, 0.59765625f, 0.2578125f, 0.0f }, //8 clear
		{ 0.78125f, 0.59765625f, 0.2578125f, 1.0f }
};

void SYS::print(float x, float y, float scale, char* buffer, int len) {
	char *p;
	GLfloat s1, t1, s2, t2, x_pos, y_pos;
	short s, t, num_ret;


	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

	glBindTexture(GL_TEXTURE_2D, m_font_texture_id);
	num_ret = 1;

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBegin(GL_QUADS);

	x_pos = x;
	if (y < 0) {
		y_pos = 8 - y;
	}
	else {
		y_pos = m_height - y - 8;
	}

	for (p = buffer, x_pos = x; *p && len > 0; p++, x_pos += 8)
	{
		if (*p > 0 && *p < 10) {
			//glColor4f(0.78125f, 0.59765625f, 0.2578125f, 1.0f);
			glColor4fv(sys_colors[*p]);
			x_pos -= 8;
			continue;
		}
		if (*p == '\n')
		{
			x_pos = x - 8;
			y_pos -= 8;
			num_ret++;
			continue;
		}
		s = (*p);
		t = 0;
		s1 = (float)s / 128.0f;
		t1 = 0;
		s2 = s1 + (1.0f / 128.0f);
		t2 = t1 + (1.0f);

		glTexCoord2f(s1, t2);
		glVertex2f(x_pos, y_pos);

		glTexCoord2f(s1, t1);
		glVertex2f(x_pos, y_pos + 8);

		glTexCoord2f(s2, t1);
		glVertex2f(x_pos + 8, y_pos + 8);

		glTexCoord2f(s2, t2);
		glVertex2f(x_pos + 8, y_pos);

		len--;

	}
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glEnd();
	glPopAttrib();

}
#endif

void SYS::print(float x, float y, float scale, char* buffer, int len) {
	char *p;
	float s1, t1, s2, t2, x_pos, y_pos;
	short num_ret;
	int ord;

	//return;

	bind_texture(m_font_texture_id);
	num_ret = 1;

	x_pos = x;
	if (y < 0) {
		y_pos = 8 - y;
	}
	else {
		y_pos = m_height - y - 8;
	}

	for (p = buffer, x_pos = x; *p && len > 0; p++, x_pos += 8)
	{
		ord = (*p);
		if (ord > 0 && ord < 10) {
			//glColor4f(0.78125f, 0.59765625f, 0.2578125f, 1.0f);
			//glColor4fv(sys_colors[*p]);
			x_pos -= 8;
			continue;
		}
		if (ord == '\n')
		{
			x_pos = x - 8;
			y_pos -= 8;
			num_ret++;
			continue;
		}
		s1 = (float)(ord & 15) / 16.0f;
		s2 = s1 + (1.0f / 16.0f);
		t1 = (float)(ord >> 4) / 8.0f;
		t2 = t1 + (1.0f / 8.0f);

		//::printf("%d %d (%3.1f %3.1f) %1.4f %1.4f\n", m_height, ord, x_pos, y_pos, s1, t1);

#if 1
#ifdef MDL_NORMALS
		faceVertex_s tri0[3] = {
			{ { 0.0f, 0.0f, -0.01f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, +1.0f } },
			{ { 399.0f, 0.0f, -0.01f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, +1.0f } },
			{ { 399.0f, 64.0f, -0.01f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, +1.0f } },
		};
		faceVertex_s tri1[3] = {
			{ { 399.0f, 64.0f, -0.01f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, +1.0f } },
			{ { 0.0f, 64.0f, -0.01f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, +1.0f } },
			{ { 0.0f, 0.0f, -0.01f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, +1.0f } }
		};
#else
		faceVertex_s tri0[3] = {
			{ { 0.0f, 0.0f, -0.01f }, { 0.0f, 0.0f } },
			{ { 399.0f, 0.0f, -0.01f }, { 1.0f, 0.0f } },
			{ { 399.0f, 64.0f, -0.01f }, { 1.0f, 1.0f } },
		};
		faceVertex_s tri1[3] = {
			{ { 399.0f, 64.0f, -0.01f }, { 1.0f, 1.0f } },
			{ { 0.0f, 64.0f, -0.01f }, { 0.0f, 1.0f } },
			{ { 0.0f, 0.0f, -0.01f }, { 0.0f, 0.0f } }
		};
#endif
		//first tri
		tri0[0].position.x = x_pos;
		tri0[0].position.y = y_pos;
		tri0[0].texcoord[0] = s1;
		tri0[0].texcoord[1] = t2;

		tri0[1].position.x = x_pos + 8;
		tri0[1].position.y = y_pos;
		tri0[1].texcoord[0] = s2;
		tri0[1].texcoord[1] = t2;

		tri0[2].position.x = x_pos + 8;
		tri0[2].position.y = y_pos + 8;
		tri0[2].texcoord[0] = s2;
		tri0[2].texcoord[1] = t1;

		//second tri
		tri1[0].position.x = x_pos + 8;
		tri1[0].position.y = y_pos + 8;
		tri1[0].texcoord[0] = s2;
		tri1[0].texcoord[1] = t1;

		tri1[1].position.x = x_pos;
		tri1[1].position.y = y_pos + 8;
		tri1[1].texcoord[0] = s1;
		tri1[1].texcoord[1] = t1;

		tri1[2].position.x = x_pos;
		tri1[2].position.y = y_pos;
		tri1[2].texcoord[0] = s1;
		tri1[2].texcoord[1] = t2;

		ctrVboAddData(&con_tris, tri0, sizeof(tri0), 3);
		ctrVboAddData(&con_tris, tri1, sizeof(tri1), 3);
#endif
		
		len--;

	}

}
/*
static faceVertex_s test[] = {
		{ { 0.0f, 0.0f, -0.01f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, +1.0f } },
		{ { 128.0f, 0.0f, -0.01f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, +1.0f } },
		{ { 128.0f, 64.0f, -0.01f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, +1.0f } },
		//ngle
		{ { 128.0f, 64.0f, -0.01f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, +1.0f } },
		{ { 0.0f, 64.0f, -0.01f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, +1.0f } },
		{ { 0.0f, 0.0f, -0.01f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, +1.0f } }
};
*/
void draw_stuff2d() {
	//gsVboAddData(&con_tris, test, sizeof(test),6);
}
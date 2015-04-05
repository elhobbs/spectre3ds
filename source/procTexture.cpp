#include "procTexture.h"
#include <malloc.h>
#include "Host.h"

int procTextureList::m_num;
int procTextureList::m_max;
procTexture	**procTextureList::m_list;
int	sintable[SIN_BUFFER_SIZE];


extern unsigned char *q1_palette;
cvar_t	proctex = { "proctex", "1" };						// 0 - 3

int warpTexture::update() {
	unsigned char *buffer = (unsigned char *)alloca(m_width*m_height);
	int	*turb;
	int	i, j, s, t;

	//LONGLONG t0 = sys.microseconds();

	turb = sintable + ((int)(host.cl_time()*SPEED)&(CYCLE - 1));
	unsigned char *pd = buffer;

	for (i = 0; i<m_height; i++)
	{
		for (j = 0; j<m_width; j++)
		{
			s = (((j << 16) + turb[i & (CYCLE - 1)] + 32768) >> 16) & 63;
			t = (((i << 16) + turb[j & (CYCLE - 1)] + 32768) >> 16) & 63;
			*pd++ = *(m_data + (t << 6) + s);
		}
	}

	//LONGLONG t1 = sys.microseconds();
	//sys.update_texture_byte(m_id, m_width, m_height, buffer, m_pal);
	sys.update_texture_byte(m_id, m_width, m_height, buffer, q1_palette);
	//LONGLONG t2 = sys.microseconds();

	//host.printf("warp: %6d %6d\n", (int)(t1 - t0), (int)(t2 - t1));

	return 0;
}

int skyTexture::update() {
	unsigned char *buffer = (unsigned char *)alloca(m_width*m_height);
	byte *pd = buffer;

	float speedscale = host.real_time() * 8;
	//speedscale -= (int)speedscale & ~127;
	float speedscale2 = host.real_time() * 16;
	//speedscale2 -= (int)speedscale2 & ~127;

	for (int i = 0; i < m_height; i++) {
		byte *top = &m_data_top[(int(i+speedscale)&127)*m_width];
		byte *bot = &m_data_bottom[(int(i + speedscale2) & 127)*m_width];
		for (int j = 0; j < m_width; j++) {
			if (*top==0) {
				*pd++ =*bot++;
				top++;
			}
			else {
				*pd++ = *top++;
				bot++;
			}
		}
	}

	sys.update_texture_byte(m_id, m_width, m_height, buffer, q1_palette);

	return 0;
}

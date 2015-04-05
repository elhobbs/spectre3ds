#include "sys.h"
#include "Host.h"
#include "Particle.h"

#ifdef WIN32
#include <gl\gl.h>
#include <gl\glu.h>
#endif

byte	dottexture[8][8] =
{
	{ 0x0, 0xf, 0xf, 0xf, 0, 0, 0, 0 },
	{ 0xf, 0xf, 0xf, 0xf, 0xf, 0, 0, 0 },
	{ 0xf, 0xf, 0xf, 0xf, 0xf, 0, 0, 0 },
	{ 0xf, 0xf, 0xf, 0xf, 0xf, 0, 0, 0 },
	{ 0x0, 0xf, 0xf, 0xf, 0, 0, 0, 0 },
	{ 0x0, 0x0, 0x0, 0x0, 0, 0, 0, 0 },
	{ 0x0, 0x0, 0x0, 0x0, 0, 0, 0, 0 },
	{ 0x0, 0x0, 0x0, 0x0, 0, 0, 0, 0 },
};

const int		Particles::ramp1[8] = { 0x6f, 0x6d, 0x6b, 0x69, 0x67, 0x65, 0x63, 0x61 };
const int		Particles::ramp2[8] = { 0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x68, 0x66 };
const int		Particles::ramp3[8] = { 0x6d, 0x6b, 6, 5, 4, 3 };

void Particles::init() {
	extern unsigned char *q1_palette;
	m_texture_id = sys.gen_texture_id();
	sys.load_texture256(m_texture_id, 8, 8, &dottexture[0][0], q1_palette,1);
}

void Particles::teleport_splash(vec3_t orgf) {
	int				i, j, k;
	Particle		*p;
	fixed32p16		vel;
	vec3_fixed32	dir;
	vec3_fixed32	org;
	float cl_time = host.cl_time();

	org = orgf;

	for (i = -16; i < 16; i += 4) {
		for (j = -16; j < 16; j += 4) {
			for (k = -24; k < 32; k += 4) {
				if (!m_free_particles)
					return;
				p = m_free_particles;
				m_free_particles = p->next;
				p->next = m_active_particles;
				m_active_particles = p;

				p->die = cl_time + 0.2f + (rand() & 7) * 0.02f;
				p->color = 7 + (rand() & 7);
				p->type = pt_slowgrav;

				dir[0] = j * 8;
				dir[1] = i * 8;
				dir[2] = k * 8;

				p->org[0] = org[0] + fixed32p16((i + (rand() & 3)) << 16);
				p->org[1] = org[1] + fixed32p16((j + (rand() & 3)) << 16);
				p->org[2] = org[2] + fixed32p16((k + (rand() & 3)) << 16);

				dir.normalize();
				vel = 50 + (rand() & 63);
				p->vel = dir*vel;
			}
		}
	}
}

void Particles::explosion(vec3_t orgf) {
	int			i, j;
	Particle	*p;
	float cl_time = host.cl_time();
	float frame_time = host.frame_time();
	vec3_fixed32	org;

	org = orgf;

	for (i = 0; i < 1024; i++)
	{
		if (!m_free_particles)
			return;
		p = m_free_particles;
		m_free_particles = p->next;
		p->next = m_active_particles;
		m_active_particles = p;

		p->die = cl_time + 5.0f;
		p->color = ramp1[0];
		p->ramp = rand() & 3;
		if (i & 1)
		{
			p->type = pt_explode;
			for (j = 0; j < 3; j++)
			{
				p->org[j] = org[j] + fixed32p16(((rand() % 32) - 16) << 16);
				p->vel[j] = fixed32p16(((rand() % 512) - 256) << 16);
			}
		}
		else
		{
			p->type = pt_explode2;
			for (j = 0; j < 3; j++)
			{
				p->org[j] = org[j] + fixed32p16(((rand() % 32) - 16)<<16);
				p->vel[j] = fixed32p16(((rand() % 512) - 256)<<16);
			}
		}
	}
}

void Particles::explosion2(vec3_t org, int colorStart, int colorLength)
{
	int			i, j;
	Particle	*p;
	int			colorMod = 0;
	float		cl_time = host.cl_time();

	for (i = 0; i<512; i++)
	{
		if (!m_free_particles)
			return;
		p = m_free_particles;
		m_free_particles = p->next;
		p->next = m_active_particles;
		m_active_particles = p;

		p->die = cl_time + 0.3;
		p->color = colorStart + (colorMod % colorLength);
		colorMod++;

		p->type = pt_blob;
		for (j = 0; j<3; j++)
		{
			p->org[j] = org[j] + ((rand() % 32) - 16);
			p->vel[j] = (rand() % 512) - 256;
		}
	}
}

void Particles::rocket_trail(vec3_t startf, vec3_t endf, int type)
{
	int				j;
	Particle		*p;
	int				dec;
	static int		tracercount;
	vec3_fixed32	start;
	vec3_fixed32	end;
	vec3_fixed32	vec;
	fixed32p16		len;
	float cl_time = host.cl_time();
	float frame_time = host.frame_time();

	start = startf;
	end = endf;

	vec = end - start;
	len = vec.normalize();

	if (type < 128)
		dec = 3;
	else
	{
		dec = 1;
		type -= 128;
	}

	while (len > 0)
	{
		len -= dec;

		if (!m_free_particles)
			return;
		p = m_free_particles;
		m_free_particles = p->next;
		p->next = m_active_particles;
		m_active_particles = p;

		p->vel = vec3_origin;
		p->die = cl_time + 2;

		switch (type)
		{
		case 0:	// rocket trail
			p->ramp = (rand() & 3);
			p->color = ramp3[(int)p->ramp];
			p->type = pt_fire;
			for (j = 0; j<3; j++)
				p->org[j] = start[j] + fixed32p16(((rand() % 6) - 3)<<16);
			break;

		case 1:	// smoke smoke
			p->ramp = (rand() & 3) + 2;
			p->color = ramp3[(int)p->ramp];
			p->type = pt_fire;
			for (j = 0; j<3; j++)
				p->org[j] = start[j] + fixed32p16(((rand() % 6) - 3) << 16);
			break;

		case 2:	// blood
			p->type = pt_grav;
			p->color = 67 + (rand() & 3);
			for (j = 0; j<3; j++)
				p->org[j] = start[j] + fixed32p16(((rand() % 6) - 3) << 16);
			break;

		case 3:
		case 5:	// tracer
			p->die = cl_time + 0.5f;
			p->type = pt_static;
			if (type == 3)
				p->color = 52 + ((tracercount & 4) << 1);
			else
				p->color = 230 + ((tracercount & 4) << 1);

			tracercount++;

			p->org = start;
			if (tracercount & 1)
			{
				p->vel[0] = vec[1] * 30;
				p->vel[1] = vec[0] * -30;
			}
			else
			{
				p->vel[0] = vec[1] * -30;
				p->vel[1] = vec[0] * 30;
			}
			break;

		case 4:	// slight blood
			p->type = pt_grav;
			p->color = 67 + (rand() & 3);
			for (j = 0; j<3; j++)
				p->org[j] = start[j] + fixed32p16(((rand() % 6) - 3) << 16);
			len -= 3;
			break;

		case 6:	// voor trail
			p->color = 9 * 16 + 8 + (rand() & 3);
			p->type = pt_static;
			p->die = cl_time + 0.3f;
			for (j = 0; j<3; j++)
				p->org[j] = start[j] + fixed32p16(((rand() % 15) - 8) << 16);
			break;
		}

		start += vec;
	}
}

void Particles::effect(vec3_t org, vec3_t dir, int color, int count)
{
	int			i, j;
	Particle	*p;
	float cl_time = host.cl_time();

	for (i = 0; i<count; i++)
	{
		if (!m_free_particles)
			return;
		p = m_free_particles;
		m_free_particles = p->next;
		p->next = m_active_particles;
		m_active_particles = p;

		if (count == 1024)
		{	// rocket explosion
			p->die = cl_time + 5;
			p->color = ramp1[0];
			p->ramp = rand() & 3;
			if (i & 1)
			{
				p->type = pt_explode;
				for (j = 0; j<3; j++)
				{
					p->org[j] = org[j] + ((rand() % 32) - 16);
					p->vel[j] = (rand() % 512) - 256;
				}
			}
			else
			{
				p->type = pt_explode2;
				for (j = 0; j<3; j++)
				{
					p->org[j] = org[j] + ((rand() % 32) - 16);
					p->vel[j] = (rand() % 512) - 256;
				}
			}
		}
		else
		{
			p->die = cl_time + 0.1*(rand() % 5);
			p->color = (color&~7) + (rand() & 7);
			p->type = pt_slowgrav;
			for (j = 0; j<3; j++)
			{
				p->org[j] = org[j] + ((rand() & 15) - 8);
				p->vel[j] = dir[j] * 15;// + (rand()%300)-150;
			}
		}
	}
}

void Particles::lava_splash(vec3_t orgf) {
	int				i, j, k;
	Particle		*p;
	fixed32p16		vel;
	vec3_fixed32	dir;
	vec3_fixed32	org;
	float cl_time = host.cl_time();

	org = orgf;

	for (i = -16; i < 16; i++) {
		for (j = -16; j < 16; j++) {
			for (k = 0; k < 1; k++) {
				if (!m_free_particles)
					return;
				p = m_free_particles;
				m_free_particles = p->next;
				p->next = m_active_particles;
				m_active_particles = p;

				p->die = cl_time + 2.0f + (rand() & 31) * 0.02f;
				p->color = 224 + (rand() & 7);
				p->type = pt_slowgrav;

				dir[0] = j * 8 + (rand() & 7);
				dir[1] = i * 8 + (rand() & 7);
				dir[2] = 256;

				p->org[0] = org[0] + dir[0];
				p->org[1] = org[1] + dir[1];
				p->org[2] = org[2] + fixed32p16((rand() & 63) << 16);

				dir.normalize();
				vel = 50 + (rand() & 63);
				p->vel = dir*vel;
			}
		}
	}
}


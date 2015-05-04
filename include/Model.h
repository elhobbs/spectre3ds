#pragma once

#include "FileSystem.h"
#include "vec3_fixed.h"
#include "pr_comp.h"

#include "LoadList.h"

class q1_plane{
public:
	q1_plane();
	q1_plane &		operator=(const float *a);
	q1_plane		operator-() const;

	void			calc_signbits();
	int				on_side(bbox &box);

	fixed32p16		operator*(const vec3_t p);
	fixed32p16		operator*(const vec3_fixed16 &p);
	fixed32p16		operator*(const vec3_fixed32 &p);
	norm3_fixed32	m_normal;
	fixed32p16		m_dist;
	byte			m_type;
	byte			m_signbits;
};

class traceResults {
public:
	bool		allsolid;	// if true, plane is not valid
	bool		startsolid;	// if true, the initial point was in a solid area
	bool		inopen, inwater;
	edict_t		*ent;			// entity the surface is on
	fixed32p16		fraction;
	q1_plane		plane;
	vec3_fixed32	start;
	vec3_fixed32	end;
};

inline q1_plane::q1_plane() {
	m_signbits = 0;
}

inline q1_plane q1_plane::operator-() const{
	q1_plane pl;
	pl.m_dist = -m_dist;
	pl.m_normal = -m_normal;

	return pl;
}

inline void q1_plane::calc_signbits() {
	int bits = 0;
	for (int i = 0; i < 3; i++) {
		if (m_normal[i] < 0) {
			bits |= 1 << i;
		}
	}
	m_signbits = bits;
}

inline q1_plane & q1_plane::operator=(const float *plane) {
	m_normal[0] = plane[0];
	m_normal[1] = plane[1];
	m_normal[2] = plane[2];
	m_dist = plane[3];

	calc_signbits();

	return *this;
}

inline fixed32p16 q1_plane::operator*(const vec3_t p) {
	float val = 
		(m_normal[0].c_float() * p[0]) +
		(m_normal[1].c_float() * p[1]) +
		(m_normal[2].c_float() * p[2]) - 
		m_dist.c_float();

	return fixed32p16(val);
}

inline fixed32p16 q1_plane::operator*(const vec3_fixed16 &p) {
	__int64_t val = ((__int64_t)m_normal[0].x * p[0].x) +
		((__int64_t)m_normal[1].x * p[1].x) +
		((__int64_t)m_normal[2].x * p[2].x);

	return fixed32p16((int)(val >> 11)) - m_dist;
}

inline fixed32p16 q1_plane::operator*(const vec3_fixed32 &p) {
	__int64_t val = ((__int64_t)m_normal[0].x * p[0].x) +
		((__int64_t)m_normal[1].x * p[1].x) +
		((__int64_t)m_normal[2].x * p[2].x);

	return fixed32p16((int)(val >> 24)) - m_dist;
}

typedef enum { mod_brush, mod_sprite, mod_alias, mod_max = 0xffffffff } modtype_t;

#define	MAX_MOD_KNOWN	256

#define	EF_ROCKET	1			// leave a trail
#define	EF_GRENADE	2			// leave a trail
#define	EF_GIB		4			// leave a trail
#define	EF_ROTATE	8			// rotate (bonus items)
#define	EF_TRACER	16			// green split trail
#define	EF_ZOMGIB	32			// small blood trail
#define	EF_TRACER2	64			// orange split trail + rotate
#define	EF_TRACER3	128			// purple trail

class Model
{
public:
	Model(char *name);
	Model(int num);
	~Model();

	void set_pos(vec3_fixed16 &pos);
	void set_pos(float *pos);
	char *name();
	modtype_t type();

	//virtual int render(int frame) = 0;
	//virtual int trace(traceResults &trace, int hull, vec3_fixed16 &p0, vec3_fixed16 &p1) = 0;

	bbox			m_bounds;
	fixed16p3		m_radius;
	int				m_flags;

	struct entity_s		*m_ents;

protected:
	modtype_t		m_type;
	char			*m_name;
	sysFile*		m_file;
	vec3_fixed16	m_pos;
};

inline Model::Model(int num)
{
	char name[32];

	sprintf(name, "*%i", num);
	int len = strlen(name) + 1;
	m_name = new(pool) char[len];
	memcpy(m_name, name, len);
	m_flags = 0;
	m_ents = 0;
}

inline Model::Model(char *name)
{
	int len = strlen(name) + 1;
	m_name = new(pool) char[len];
	memcpy(m_name, name, len);
	m_flags = 0;
}

inline Model::~Model()
{
}

inline char *Model::name()
{
	return m_name;
}

inline modtype_t Model::type()
{
	return m_type;
}

inline void Model::set_pos(vec3_fixed16 &pos)
{
	m_pos = pos;
}

inline void Model::set_pos(float *pos)
{
	m_pos = pos;
}

extern LoadList<Model, 256> Models;


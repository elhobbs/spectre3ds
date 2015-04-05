#include "q1Progs.h"
#include "FileSystem.h"
#include "memPool.h"
#include "Host.h"
#include "protocol.h"
#include <stddef.h>
#include <stdlib.h>

#include "cvar.h"
#include "cmd.h"

extern SYS sys;
extern unsigned char *q1_palette;

vec3_t vec3_origin = { 0, 0, 0 };
int nanmask = 255 << 23;



/*
===============================================================================

BUILT-IN FUNCTIONS

===============================================================================
*/

edict_t *q1Progs::EDICT_NUM(int n)
{
	if (n < 0 || n >= m_num_edicts)
		sys.error("EDICT_NUM: bad number %i", n);
	return m_edicts[n];
}

edict_t *q1Progs::EDICT_NUM2(int n)
{
	if (n < 0 || n >= m_max_edicts)
		sys.error("EDICT_NUM: bad number %i", n);
	while (m_num_edicts <= n && m_num_edicts < m_max_edicts)
	{
		ed_create();
	}
	return m_edicts[n];
}

void q1Progs::ED_ClearEdict(edict_t *e)
{
	memset(&e->v, 0, m_programs.entityfields * 4);
	e->free = false;
	e->area.prev = 0;
	e->area.next = 0;
	e->num_leafs = 0;
	//*EDICT_NUM_PTR(e) = -1;
}

void q1Progs::ED_Free(edict_t *ed)
{
	m_sv.unlink_edict(ed);		// unlink from world bsp

	ed->free = true;
	ed->v.model = 0;
	ed->v.takedamage = 0;
	ed->v.modelindex = 0;
	ed->v.colormap = 0;
	ed->v.skin = 0;
	ed->v.frame = 0;
	VectorCopy(vec3_origin, ed->v.origin);
	VectorCopy(vec3_origin, ed->v.angles);
	ed->v.nextthink = -1;
	ed->v.solid = 0;

	ed->freetime = m_sv.time();
	//*EDICT_NUM_PTR(ed) = -1;
}

static byte *edict_pool = 0;
static int edict_pool_count = -1;
static int edict_pool_chunk_size = 32;

void ED_ResetPool()
{
	edict_pool_count = -1;
}

edict_t *q1Progs::ed_create()
{
	edict_t *e;
	if (m_num_edicts == MAX_EDICTS)
		sys.error("ed_create: no free edicts");
	if (edict_pool_count < 0)
	{
		edict_pool_count = edict_pool_chunk_size - 1;
		edict_pool = new(pool) byte[(m_edict_size + 4) * edict_pool_chunk_size];
		//memset(edict_pool,0xff,pr_edict_size * edict_pool_chunk_size);
	}

	e = (edict_t *)(edict_pool + ((m_edict_size + 4) * edict_pool_count));
	ED_ClearEdict(e);
	edict_pool_count--;

	m_edicts[m_num_edicts] = e;
	*EDICT_NUM_PTR(e) = m_num_edicts++;
	return e;
}


edict_t *q1Progs::ED_Alloc(void)
{
	int			i;
	edict_t		*e;

	for (i = m_sv.maxclients() + 1; i<m_num_edicts; i++)
	{
		e = EDICT_NUM(i);
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (e->free && (e->freetime < 2 || m_sv.time() - e->freetime > 0.5))
		{
			ED_ClearEdict(e);
			*EDICT_NUM_PTR(e) = i;
			//e->num = i;
			return e;
		}
	}

	if (i == MAX_EDICTS)
		sys.error("ED_Alloc: no free edicts");

	e = ed_create();
	return e;
}


edict_t * q1Progs::NEXT_EDICT(edict_t *e)
{
	int n = *EDICT_NUM_PTR(e) + 1;
	//if (n < 0 || n >= m_num_edicts)
	//	sys.error ("NEXT_EDICT: bad pointer");

	return m_edicts[n];
}

edict_t* q1Progs::G_EDICT(int o)
{
	int prog = m_globals[o]._int;

	return prog_to_edict(prog);
}

char *q1Progs::PF_VarString(int	first)
{
	int		i;
	static char out[256];

	out[0] = 0;
	for (i = first; i<m_pr_argc; i++)
	{
		strcat(out, G_STRING((OFS_PARM0 + i * 3)));
	}
	return out;
}


/*
=================
PF_errror

This is a TERMINAL error, which will kill off the entire server.
Dumps self.

error(value)
=================
*/
void q1Progs::PF_error(void)
{
	char	*s;
	edict_t	*ed;

	s = PF_VarString(0);
	printf("======SERVER ERROR in %s:\n%s\n"
		, m_strings + m_pr_xfunction->s_name, s);
	ed = prog_to_edict(m_global_struct->self);
	//ED_Print(ed);

	run_error("Program error");
}

/*
=================
PF_objerror

Dumps out self, then an error message.  The program is aborted and self is
removed, but the level can continue.

objerror(value)
=================
*/
void q1Progs::PF_objerror(void)
{
	char	*s;
	edict_t	*ed;

	s = PF_VarString(0);
	printf("======OBJECT ERROR in %s:\n%s\n"
		, m_strings + m_pr_xfunction->s_name, s);
	ed = prog_to_edict(m_global_struct->self);
	//ED_Print(ed);
	//ED_Free(ed);

	run_error("Program error");
}



/*
==============
PF_makevectors

Writes new values for v_forward, v_up, and v_right based on angles
makevectors(vector)
==============
*/
void q1Progs::PF_makevectors(void)
{
	AngleVectors(G_VECTOR(OFS_PARM0), m_global_struct->v_forward, m_global_struct->v_right, m_global_struct->v_up);
}

/*
=================
PF_setorigin

This is the only valid way to move an object without using the physics of the world (setting velocity and waiting).  Directly changing origin will not set internal links correctly, so clipping would be messed up.  This should be called when an object is spawned, and then only if it is teleported.

setorigin (entity, origin)
=================
*/
void q1Progs::PF_setorigin(void)
{
	edict_t	*e;
	float	*org;

	e = G_EDICT(OFS_PARM0);
	org = G_VECTOR(OFS_PARM1);
	VectorCopy(org, e->v.origin);
	m_sv.link_edict(e, false);
}

int set_minmax_error = 0;


void q1Progs::SetMinMaxSize(edict_t *e, float *min, float *max, bool rotate)
{
	float	*angles;
	vec3_t	rmin, rmax;
	float	bounds[2][3];
	float	xvector[2], yvector[2];
	float	a;
	vec3_t	base, transformed;
	int		i, j, k, l;

	set_minmax_error = 0;

	for (i = 0; i<3; i++) {
		if (min[i] > max[i]) {
			set_minmax_error = 1;
			run_error("SetMinMaxSize float backwards mins/maxs\n%4.6f %4.6f", min[i], max[i]);
		}
	}

	rotate = false;		// FIXME: implement rotation properly again

	if (!rotate)
	{
		VectorCopy(min, rmin);
		VectorCopy(max, rmax);
	}
	else
	{
		// find min / max for rotations
		angles = e->v.angles;

		a = angles[1] / 180 * M_PI;

		xvector[0] = cos(a);
		xvector[1] = sin(a);
		yvector[0] = -sin(a);
		yvector[1] = cos(a);

		VectorCopy(min, bounds[0]);
		VectorCopy(max, bounds[1]);

		rmin[0] = rmin[1] = rmin[2] = 9999;
		rmax[0] = rmax[1] = rmax[2] = -9999;

		for (i = 0; i <= 1; i++)
		{
			base[0] = bounds[i][0];
			for (j = 0; j <= 1; j++)
			{
				base[1] = bounds[j][1];
				for (k = 0; k <= 1; k++)
				{
					base[2] = bounds[k][2];

					// transform the point
					transformed[0] = xvector[0] * base[0] + yvector[0] * base[1];
					transformed[1] = xvector[1] * base[0] + yvector[1] * base[1];
					transformed[2] = base[2];

					for (l = 0; l<3; l++)
					{
						if (transformed[l] < rmin[l])
							rmin[l] = transformed[l];
						if (transformed[l] > rmax[l])
							rmax[l] = transformed[l];
					}
				}
			}
		}
	}

	// set derived values
	VectorCopy(rmin, e->v.mins);
	VectorCopy(rmax, e->v.maxs);
	VectorSubtract(max, min, e->v.size);

	m_sv.link_edict(e, false);
}

void q1Progs::SetMinMaxSize(edict_t *e, vec3_fixed16 &min, vec3_fixed16 &max, bool rotate)
{
	float	*angles;
	vec3_t	rmin, rmax;
	float	bounds[2][3];
	float	xvector[2], yvector[2];
	float	a;
	vec3_t	base, transformed;
	int		i, j, k, l;

	set_minmax_error = 0;

	for (i = 0; i<3; i++) {
		if (min[i] > max[i]) {
			set_minmax_error = 1;
			run_error("SetMinMaxSize backwards mins/maxs\n%4.6f %4.6f", min[i].c_float(), max[i].c_float());
		}
	}

	rotate = false;		// FIXME: implement rotation properly again

	if (!rotate)
	{
		min.copy_to(rmin);
		max.copy_to(rmax);
	}
	else
	{
		// find min / max for rotations
		angles = e->v.angles;

		a = angles[1] / 180 * M_PI;

		xvector[0] = cos(a);
		xvector[1] = sin(a);
		yvector[0] = -sin(a);
		yvector[1] = cos(a);

		min.copy_to(bounds[0]);
		max.copy_to(bounds[1]);

		rmin[0] = rmin[1] = rmin[2] = 9999;
		rmax[0] = rmax[1] = rmax[2] = -9999;

		for (i = 0; i <= 1; i++)
		{
			base[0] = bounds[i][0];
			for (j = 0; j <= 1; j++)
			{
				base[1] = bounds[j][1];
				for (k = 0; k <= 1; k++)
				{
					base[2] = bounds[k][2];

					// transform the point
					transformed[0] = xvector[0] * base[0] + yvector[0] * base[1];
					transformed[1] = xvector[1] * base[0] + yvector[1] * base[1];
					transformed[2] = base[2];

					for (l = 0; l<3; l++)
					{
						if (transformed[l] < rmin[l])
							rmin[l] = transformed[l];
						if (transformed[l] > rmax[l])
							rmax[l] = transformed[l];
					}
				}
			}
		}
	}

	// set derived values
	VectorCopy(rmin, e->v.mins);
	VectorCopy(rmax, e->v.maxs);
	(max - min).copy_to(e->v.size);

	m_sv.link_edict(e, false);
}

/*
=================
PF_setsize

the size box is rotated by the current angle

setsize (entity, minvector, maxvector)
=================
*/
void q1Progs::PF_setsize(void)
{
	edict_t	*e;
	float	*min, *max;

	e = G_EDICT(OFS_PARM0);
	min = G_VECTOR(OFS_PARM1);
	max = G_VECTOR(OFS_PARM2);
	SetMinMaxSize(e, min, max, false);
}

/*
=================
PF_setmodel

setmodel(entity, model)
=================
*/
void q1Progs::PF_setmodel(void)
{
	edict_t	*e;
	char	*m, **check;
	Model	*mod;
	int		i;

	e = G_EDICT(OFS_PARM0);
	m = G_STRING(OFS_PARM1);

	// check to see if model was properly precached
	for (i = 0, check = m_sv.m_model_precache; *check; i++, check++) {
		if (!strcmp(*check, m)) {
			break;
		}
	}

	if (!*check)
		run_error("no precache: %s\n", m);


	e->v.model = m - m_strings;
	e->v.modelindex = i; //SV_ModelIndex (m);

	mod = m_sv.m_models[(int)e->v.modelindex];  // Mod_ForName (m, true);

	if (mod)
		SetMinMaxSize(e, mod->m_bounds[0], mod->m_bounds[1], true);
	else
		SetMinMaxSize(e, vec3_origin, vec3_origin, true);

	if (set_minmax_error) {
		printf("PF_setmodel minmax error: %s\n", m); 
		set_minmax_error = 0;
	}
}

/*
=================
PF_bprint

broadcast print to everyone on server

bprint(value)
=================
*/
void q1Progs::PF_bprint(void)
{
	char		*s;

	s = PF_VarString(0);
	//SV_BroadcastPrintf("%s", s);
}

/*
=================
PF_sprint

single print to a specific client

sprint(clientent, value)
=================
*/
void q1Progs::PF_sprint(void)
{
	char		*s;
	Client	*client;
	int			entnum;

	entnum = G_EDICTNUM(OFS_PARM0);
	s = PF_VarString(1);


	client = m_sv.client(entnum - 1);
	if (client) {
		client->m_msg.write_char(svc_print);
		client->m_msg.write_string(s);
	}

}


/*
=================
PF_centerprint

single print to a specific client

centerprint(clientent, value)
=================
*/
void q1Progs::PF_centerprint(void)
{
	char		*s;
	Client	*client;
	int			entnum;

	entnum = G_EDICTNUM(OFS_PARM0);
	s = PF_VarString(1);

	client = m_sv.client(entnum - 1);
	if (client) {
		client->m_msg.write_char(svc_centerprint);
		client->m_msg.write_string(s);
	}

}


/*
=================
PF_normalize

vector normalize(vector)
=================
*/
void q1Progs::PF_normalize(void)
{
	float	*value1;
	vec3_t	newvalue;
	float	nw;

	value1 = G_VECTOR(OFS_PARM0);

	nw = value1[0] * value1[0] + value1[1] * value1[1] + value1[2] * value1[2];
	nw = sqrt(nw);
	//nw = InvSqrt(nw);

	if (nw == 0)
		newvalue[0] = newvalue[1] = newvalue[2] = 0;
	else
	{
		nw = 1.0f / nw;
		newvalue[0] = value1[0] * nw;
		newvalue[1] = value1[1] * nw;
		newvalue[2] = value1[2] * nw;
	}

	VectorCopy(newvalue, G_VECTOR(OFS_RETURN));
}

/*
=================
PF_vlen

scalar vlen(vector)
=================
*/
void q1Progs::PF_vlen(void)
{
	float	*value1;
	float	nw;

	value1 = G_VECTOR(OFS_PARM0);

	nw = value1[0] * value1[0] + value1[1] * value1[1] + value1[2] * value1[2];
	nw = sqrt(nw);

	G_FLOAT(OFS_RETURN) = nw;
}

/*
=================
PF_vectoyaw

float vectoyaw(vector)
=================
*/
void q1Progs::PF_vectoyaw(void)
{
	float	*value1;
	float	yaw;

	value1 = G_VECTOR(OFS_PARM0);

	if (value1[1] == 0 && value1[0] == 0)
		yaw = 0;
	else
	{
		yaw = (int)(atan2(value1[1], value1[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;
	}

	G_FLOAT(OFS_RETURN) = yaw;
}


/*
=================
PF_vectoangles

vector vectoangles(vector)
=================
*/
void q1Progs::PF_vectoangles(void)
{
	float	*value1;
	float	forward;
	float	yaw, pitch;

	value1 = G_VECTOR(OFS_PARM0);

	if (value1[1] == 0 && value1[0] == 0)
	{
		yaw = 0;
		if (value1[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		yaw = (int)(atan2(value1[1], value1[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;

		forward = sqrt(value1[0] * value1[0] + value1[1] * value1[1]);
		pitch = (int)(atan2(value1[2], forward) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}

	G_FLOAT(OFS_RETURN + 0) = pitch;
	G_FLOAT(OFS_RETURN + 1) = yaw;
	G_FLOAT(OFS_RETURN + 2) = 0;
}

/*
=================
PF_Random

Returns a number from 0<= num < 1

random()
=================
*/
void q1Progs::PF_random(void)
{
	float		num;

	num = (rand() & 0x7fff) / ((float)0x7fff);

	G_FLOAT(OFS_RETURN) = num;
}

/*
=================
PF_particle

particle(origin, color, count)
=================
*/
void q1Progs::PF_particle(void)
{
	float		*org, *dir;
	float		color;
	float		count;

	org = G_VECTOR(OFS_PARM0);
	dir = G_VECTOR(OFS_PARM1);
	color = G_FLOAT(OFS_PARM2);
	count = G_FLOAT(OFS_PARM3);
	m_sv.start_particle(org, dir, color, count);
}


/*
=================
PF_ambientsound

=================
*/
void q1Progs::PF_ambientsound(void)
{
	char		*samp;
	float		*pos;
	float 		vol, attenuation;

	pos = G_VECTOR(OFS_PARM0);
	samp = G_STRING(OFS_PARM1);
	vol = G_FLOAT(OFS_PARM2);
	attenuation = G_FLOAT(OFS_PARM3);


	m_sv.start_sound_static(pos, samp, vol*255, attenuation);
}

/*
=================
PF_sound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything
allready running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.

=================
*/
void q1Progs::PF_sound(void)
{
	char		*sample;
	int			channel;
	edict_t		*entity;
	int 		volume;
	float attenuation;

	entity = G_EDICT(OFS_PARM0);
	channel = G_FLOAT(OFS_PARM1);
	sample = G_STRING(OFS_PARM2);
	volume = G_FLOAT(OFS_PARM3) * 255;
	attenuation = G_FLOAT(OFS_PARM4);

	if (volume < 0 || volume > 255)
		sys.error("SV_StartSound: volume = %i", volume);

	if (attenuation < 0 || attenuation > 4)
		sys.error("SV_StartSound: attenuation = %f", attenuation);

	if (channel < 0 || channel > 7)
		sys.error("SV_StartSound: channel = %i", channel);

	m_sv.start_sound(entity, channel, sample, volume, attenuation);
}

/*
=================
PF_break

break()
=================
*/
void q1Progs::PF_break(void)
{
	printf("break statement\n");
	*(int *)-4 = 0;	// dump to debugger
	//	PR_RunError ("break statement");
}

/*
=================
PF_traceline

Used for use tracing and shot targeting
Traces are blocked by bbox and exact bsp entityes, and also slide box entities
if the tryents flag is set.

traceline (vector1, vector2, tryents)
=================
*/
void q1Progs::PF_traceline(void)
{
#if 1
	float	*v1, *v2;
	trace_t	trace;
	int		nomonsters;
	edict_t	*ent;

	v1 = G_VECTOR(OFS_PARM0);
	v2 = G_VECTOR(OFS_PARM1);
	nomonsters = G_FLOAT(OFS_PARM2);
	ent = G_EDICT(OFS_PARM3);

	trace = m_sv.SV_Move(v1, vec3_origin, vec3_origin, v2, nomonsters, ent);

	m_global_struct->trace_allsolid = trace.allsolid;
	m_global_struct->trace_startsolid = trace.startsolid;
	m_global_struct->trace_fraction = trace.fraction;
	m_global_struct->trace_inwater = trace.inwater;
	m_global_struct->trace_inopen = trace.inopen;
	VectorCopy(trace.endpos, m_global_struct->trace_endpos);
	for (int i = 0; i < 3; i++) {
		m_global_struct->trace_plane_normal[i] = trace.plane.m_normal[0].c_float();
	}
	m_global_struct->trace_plane_dist = trace.plane.m_dist.c_float();
	if (trace.ent)
		m_global_struct->trace_ent = edict_to_prog(trace.ent);
	else
		m_global_struct->trace_ent = edict_to_prog(m_edicts[0]);
#endif
}

//============================================================================
#define	MAX_MAP_LEAFS		8192
byte	checkpvs[MAX_MAP_LEAFS / 8];

int q1Progs::PF_newcheckclient(int check)
{
	int		i = 1;
	edict_t	*ent;
	vec3_t	org;

	// cycle to the next one

	if (check < 1)
		check = 1;
	if (check > m_sv.maxclients())
		check = m_sv.maxclients();

	if (check == m_sv.maxclients())
		i = 1;
	else
		i = check + 1;

	for (;; i++)
	{
		if (i == m_sv.maxclients() + 1)
			i = 1;

		ent = EDICT_NUM(i);

		if (i == check)
			break;	// didn't find anything else

		if (ent->free)
			continue;
		if (ent->v.health <= 0)
			continue;
		if ((int)ent->v.flags & FL_NOTARGET)
			continue;

		// anything that is a client, or has a client as an enemy
		break;
	}

	// get the PVS for the entity
	VectorAdd(ent->v.origin, ent->v.view_ofs, org);
	m_sv.SV_PVS(org, checkpvs);
	return i;
}

/*
=================
PF_checkclient

Returns a client (or object that has a client enemy) that would be a
valid target.

If there are more than one valid options, they are cycled each frame

If (self.origin + self.viewofs) is not in the PVS of the current target,
it is not returned at all.

name checkclient ()
=================
*/
#define	MAX_CHECK	16
int c_invis, c_notvis;
void q1Progs::PF_checkclient(void)
{
#if 0
	RETURN_EDICT(m_edicts[0]);
#else
	edict_t	*ent, *self;
	q1_leaf_node	*leaf;
	int		l;
	vec3_t	view;

	// find a new check if on a new frame
	if (m_sv.time() - m_sv.lastchecktime >= 0.1)
	{
		m_sv.lastcheck = PF_newcheckclient(m_sv.lastcheck);
		m_sv.lastchecktime = m_sv.time();
	}

	// return check if it might be visible	
	ent = EDICT_NUM(m_sv.lastcheck);
	if (ent->free || ent->v.health <= 0)
	{
		RETURN_EDICT(m_edicts[0]);
		return;
	}

	// if current entity can't possibly see the check entity, return 0
	self = prog_to_edict(m_global_struct->self);
	VectorAdd(self->v.origin, self->v.view_ofs, view);
	leaf = m_sv.m_worldmodel->in_leaf(view);
	l = m_sv.m_worldmodel->leaf_num(leaf) - 1;
	if ((l<0) || !(checkpvs[l >> 3] & (1 << (l & 7))))
	{
		c_notvis++;
		RETURN_EDICT(m_edicts[0]);
		return;
	}

	// might be able to see it
	c_invis++;
	RETURN_EDICT(ent);
#endif
}

//============================================================================


/*
=================
PF_stuffcmd

Sends text over to the client's execution buffer

stuffcmd (clientent, value)
=================
*/
void q1Progs::PF_stuffcmd(void)
{
#if 0
	int		entnum;
	char	*str;
	client_t	*old;

	entnum = G_EDICTNUM(OFS_PARM0);
	if (entnum < 1 || entnum > svs.maxclients)
		PR_RunError("Parm 0 not a client");
	str = G_STRING(OFS_PARM1);

	old = host_client;
	host_client = &svs.clients[entnum - 1];
	Host_ClientCommands("%s", str);
	host_client = old;
#endif
}

/*
=================
PF_localcmd

Sends text over to the client's execution buffer

localcmd (string)
=================
*/
void q1Progs::PF_localcmd(void)
{
	char	*str;

	str = G_STRING(OFS_PARM0);
	host.add_text(str);
	//Cbuf_AddText(str);
}

/*
=================
PF_cvar

float cvar (string)
=================
*/
void q1Progs::PF_cvar(void)
{
	char	*str;

	str = G_STRING(OFS_PARM0);

	G_FLOAT(OFS_RETURN) = Cvar_VariableValue(str);
}

/*
=================
PF_cvar_set

float cvar (string)
=================
*/
void q1Progs::PF_cvar_set(void)
{
	char	*var, *val;

	var = G_STRING(OFS_PARM0);
	val = G_STRING(OFS_PARM1);

	Cvar_Set(var, val);
}

/*
=================
PF_findradius

Returns a chain of entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
void q1Progs::PF_findradius(void)
{
	edict_t	*ent, *chain;
	float	rad;
	float	*org;
	vec3_t	eorg;
	int		i, j;

	chain = m_edicts[0];

	org = G_VECTOR(OFS_PARM0);
	rad = G_FLOAT(OFS_PARM1);

	ent = NEXT_EDICT(m_edicts[0]);
	for (i = 1; i<m_num_edicts; i++, ent = NEXT_EDICT(ent))
	{
		if (ent->free)
			continue;
		if (ent->v.solid == SOLID_NOT)
			continue;
		for (j = 0; j<3; j++)
			eorg[j] = org[j] - (ent->v.origin[j] + (ent->v.mins[j] + ent->v.maxs[j])*0.5);
		if (Length(eorg) > rad)
			continue;

		ent->v.chain = edict_to_prog(chain);
		chain = ent;
	}

	RETURN_EDICT(chain);
}


/*
=========
PF_dprint
=========
*/
void q1Progs::PF_dprint(void)
{
	printf("%s", PF_VarString(0));
}

char	pr_string_temp[128];

void q1Progs::PF_ftos(void)
{
	float	v;
	v = G_FLOAT(OFS_PARM0);

	if (v == (int)v)
		sprintf(pr_string_temp, "%d", (int)v);
	else
		sprintf(pr_string_temp, "%5.1f", v);
	G_INT(OFS_RETURN) = pr_string_temp - m_strings;
}

void q1Progs::PF_fabs(void)
{
	float	v;
	v = G_FLOAT(OFS_PARM0);
	G_FLOAT(OFS_RETURN) = fabs(v);
}

void q1Progs::PF_vtos(void)
{
	sprintf(pr_string_temp, "'%5.1f %5.1f %5.1f'", G_VECTOR(OFS_PARM0)[0], G_VECTOR(OFS_PARM0)[1], G_VECTOR(OFS_PARM0)[2]);
	G_INT(OFS_RETURN) = pr_string_temp - m_strings;
}

void q1Progs::PF_Spawn(void)
{
	edict_t	*ed;
	ed = ED_Alloc();
	RETURN_EDICT(ed);
}

void q1Progs::PF_Remove(void)
{
	edict_t	*ed;

	ed = G_EDICT(OFS_PARM0);
	ED_Free(ed);
}

edict_t* q1Progs::find(char *classname) {
	for (int e = 1; e < m_num_edicts; e++)
	{
		edict_t *ed = EDICT_NUM(e);
		if (ed->free)
			continue;
		if (!strcmp(m_strings + ed->v.classname, classname))
		{
			return ed;
		}
	}
	return 0;
}

// entity (entity start, .string field, string match) find = #5;
void q1Progs::PF_Find(void)
{
	int		e;
	int		f;
	char	*s, *t;
	edict_t	*ed;

	e = G_EDICTNUM(OFS_PARM0);
	f = G_INT(OFS_PARM1);
	s = G_STRING(OFS_PARM2);
	if (!s)
		run_error("PF_Find: bad search string");

	for (e++; e < m_num_edicts; e++)
	{
		ed = EDICT_NUM(e);
		if (ed->free)
			continue;
		t = E_STRING(ed, f);
		if (!t)
			continue;
		if (!strcmp(t, s))
		{
			RETURN_EDICT(ed);
			return;
		}
	}

	RETURN_EDICT(m_edicts[0]);
}

void q1Progs::PR_CheckEmptyString(char *s)
{
	if (s[0] <= ' ')
		run_error("Bad string");
}

void q1Progs::PF_precache_file(void)
{	// precache_file is only used to copy files with qcc, it does nothing
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
}

void q1Progs::PF_precache_sound(void)
{
	char	*s;
	int		i;

	if (m_sv.state() != ss_loading)
		run_error("PF_Precache_*: Precache can only be done in spawn functions");

	s = G_STRING(OFS_PARM0);
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
	PR_CheckEmptyString(s);
	for (i = 0; i<MAX_SOUNDS; i++)
	{
		if (!m_sv.m_sound_precache[i])
		{
			m_sv.m_sound_precache[i] = s;
			return;
		}
		if (!strcmp(m_sv.m_sound_precache[i], s))
			return;
	}
	run_error("PF_precache_sound: overflow");
}

void q1Progs::PF_precache_model(void)
{
	char	*s;
	int		i;

	if (m_sv.state() != ss_loading) {
		run_error("PF_Precache_*: Precache can only be done in spawn functions");
	}

	s = G_STRING(OFS_PARM0);
	G_INT(OFS_RETURN) = G_INT(OFS_PARM0);
	PR_CheckEmptyString(s);
#if 1
	for (i = 0; i<MAX_MODELS; i++)
	{
		if (!m_sv.m_model_precache[i])
		{
			m_sv.m_model_precache[i] = s;
			m_sv.m_models[i] = Models.find(s);// , true);
			return;
		}
		if (!strcmp(m_sv.m_model_precache[i], s))
			return;
	}
	run_error("PF_precache_model: overflow");
#endif
}


void q1Progs::PF_coredump(void)
{
	//ED_PrintEdicts();
}

void q1Progs::PF_traceon(void)
{
	//pr_trace = true;
}

void q1Progs::PF_traceoff(void)
{
	//pr_trace = false;
}

void q1Progs::PF_eprint(void)
{
	//ED_PrintNum(G_EDICTNUM(OFS_PARM0));
}

/*
===============
PF_walkmove

float(float yaw, float dist) walkmove
===============
*/
void q1Progs::PF_walkmove(void)
{
	edict_t	*ent;
	float	yaw, dist;
	vec3_t	move;
	dfunction_t	*oldf;
	int 	oldself;

	ent = prog_to_edict(m_global_struct->self);
	yaw = G_FLOAT(OFS_PARM0);
	dist = G_FLOAT(OFS_PARM1);

	if (!((int)ent->v.flags & (FL_ONGROUND | FL_FLY | FL_SWIM)))
	{
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

	yaw = yaw*M_PI * 2 / 360;

	move[0] = cos(yaw)*dist;
	move[1] = sin(yaw)*dist;
	move[2] = 0;

	// save program state, because SV_movestep may call other progs
	oldf = m_pr_xfunction;
	oldself = m_global_struct->self;

	G_FLOAT(OFS_RETURN) = m_sv.move_step(ent, move, true);


	// restore program state
	m_pr_xfunction = oldf;
	m_global_struct->self = oldself;
}

/*
===============
PF_droptofloor

void() droptofloor
===============
*/
void q1Progs::PF_droptofloor(void)
{
	edict_t		*ent;
	vec3_t		end;
	trace_t		trace;

	ent = prog_to_edict(m_global_struct->self);

	VectorCopy(ent->v.origin, end);
	end[2] -= 256;

	trace = m_sv.SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, end, false, ent);

	if (trace.fraction == 1 || trace.allsolid)
		G_FLOAT(OFS_RETURN) = 0;
	else
	{
		VectorCopy(trace.endpos, ent->v.origin);
		m_sv.link_edict(ent, false);
		ent->v.flags = (int)ent->v.flags | FL_ONGROUND;
		ent->v.groundentity = edict_to_prog(trace.ent);
		G_FLOAT(OFS_RETURN) = 1;
	}
}

/*
===============
PF_lightstyle

void(float style, string value) lightstyle
===============
*/
void q1Progs::PF_lightstyle(void)
{
	int		style;
	char	*val;
	Client	*client;
	int			j;

	style = G_FLOAT(OFS_PARM0);
	val = G_STRING(OFS_PARM1);

	// change the string in sv
	m_lightstyles[style] = val;

	// send message to all clients on this server
	if (m_sv.state() != ss_active)
		return;

	for (j = 0; j < m_sv.maxclients(); j++) {
		client = m_sv.client(j);
		if (client->m_active || client->m_spawned) {
			client->m_msg.write_char(svc_lightstyle);
			client->m_msg.write_char(style);
			client->m_msg.write_string(val);
		}
	}
}

void q1Progs::PF_rint(void)
{
	float	f;
	f = G_FLOAT(OFS_PARM0);
	if (f > 0)
		G_FLOAT(OFS_RETURN) = (int)(f + 0.5);
	else
		G_FLOAT(OFS_RETURN) = (int)(f - 0.5);
}
void q1Progs::PF_floor(void)
{
	G_FLOAT(OFS_RETURN) = floor(G_FLOAT(OFS_PARM0));
}
void q1Progs::PF_ceil(void)
{
	G_FLOAT(OFS_RETURN) = ceil(G_FLOAT(OFS_PARM0));
}


/*
=============
PF_checkbottom
=============
*/
void q1Progs::PF_checkbottom(void)
{
	edict_t	*ent;

	ent = G_EDICT(OFS_PARM0);

	G_FLOAT(OFS_RETURN) = m_sv.check_bottom(ent);
}

/*
=============
PF_pointcontents
=============
*/
void q1Progs::PF_pointcontents(void)
{
	float	*v;

	v = G_VECTOR(OFS_PARM0);

	G_FLOAT(OFS_RETURN) = m_sv.point_contents(v);
}

/*
=============
PF_nextent

entity nextent(entity)
=============
*/
void q1Progs::PF_nextent(void)
{
	int		i;
	edict_t	*ent;

	i = G_EDICTNUM(OFS_PARM0);
	while (1)
	{
		i++;
		if (i == m_num_edicts)
		{
			RETURN_EDICT(m_edicts[0]);
			return;
		}
		ent = EDICT_NUM(i);
		if (!ent->free)
		{
			RETURN_EDICT(ent);
			return;
		}
	}
}

/*
=============
PF_aim

Pick a vector for the player to shoot along
vector aim(entity, missilespeed)
=============
*/
void q1Progs::PF_aim(void)
{
	VectorCopy(m_global_struct->v_forward, G_VECTOR(OFS_RETURN));
}

/*
==============
PF_changeyaw

This was a major timewaster in progs, so it was converted to C
==============
*/
void q1Progs::PF_changeyaw(void)
{
	edict_t		*ent;
	float		ideal, current, move, speed;

	ent = prog_to_edict(m_global_struct->self);
	current = anglemod(ent->v.angles[1]);
	ideal = ent->v.ideal_yaw;
	speed = ent->v.yaw_speed;

	if (current == ideal)
		return;
	move = ideal - current;
	if (ideal > current)
	{
		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}
	if (move > 0)
	{
		if (move > speed)
			move = speed;
	}
	else
	{
		if (move < -speed)
			move = -speed;
	}

	ent->v.angles[1] = anglemod(current + move);
}

/*
===============================================================================

MESSAGE WRITING

===============================================================================
*/

#define	MSG_BROADCAST	0		// unreliable to all
#define	MSG_ONE			1		// reliable to one (msg_entity)
#define	MSG_ALL			2		// reliable to all
#define	MSG_INIT		3		// write to the init string

NetBuffer * q1Progs::WriteDest()
{
	int		entnum;
	int		dest;
	edict_t	*ent;
	Client *client = 0;

	dest = G_FLOAT(OFS_PARM0);
	switch (dest)
	{
	case MSG_BROADCAST:
		return m_sv.datagram();

	case MSG_ONE:
		ent = prog_to_edict(m_global_struct->msg_entity);
		entnum = NUM_FOR_EDICT(ent);
		client = m_sv.client(entnum - 1);
		return &client->m_msg;

	case MSG_ALL:
		return m_sv.reliable_datagram();

	case MSG_INIT:
		return m_sv.signon();

	default:
		run_error("WriteDest: bad destination");
		break;
	}

	return 0;
}

void q1Progs::PF_WriteByte(void)
{
	NetBuffer *msg = WriteDest();
	if (msg) {
		msg->write_byte(G_FLOAT(OFS_PARM1));
	}
}

void q1Progs::PF_WriteChar(void)
{
	NetBuffer *msg = WriteDest();
	if (msg) {
		msg->write_char(G_FLOAT(OFS_PARM1));
	}
}

void q1Progs::PF_WriteShort(void)
{
	NetBuffer *msg = WriteDest();
	if (msg) {
		msg->write_short(G_FLOAT(OFS_PARM1));
	}
}

void q1Progs::PF_WriteLong(void)
{
	NetBuffer *msg = WriteDest();
	if (msg) {
		msg->write_long(G_FLOAT(OFS_PARM1));
	}
}

void q1Progs::PF_WriteAngle(void)
{
	NetBuffer *msg = WriteDest();
	if (msg) {
		msg->write_angle(G_FLOAT(OFS_PARM1));
	}
}

void q1Progs::PF_WriteCoord(void)
{
	NetBuffer *msg = WriteDest();
	if (msg) {
		msg->write_coord(G_FLOAT(OFS_PARM1));
	}
}

void q1Progs::PF_WriteString(void)
{
	NetBuffer *msg = WriteDest();
	if (msg) {
		msg->write_string(G_STRING(OFS_PARM1));
	}
}


void q1Progs::PF_WriteEntity(void)
{
	NetBuffer *msg = WriteDest();
	if (msg) {
		msg->write_short(G_EDICTNUM(OFS_PARM1));
	}
}

//=============================================================================

int SV_ModelIndex(char *name);

void q1Progs::PF_makestatic(void)
{
	edict_t	*ent;
	int		i;
	NetBuffer *signon = m_sv.signon();

	ent = G_EDICT(OFS_PARM0);

	signon->write_byte(svc_spawnstatic);

	signon->write_byte(m_sv.model_index(m_strings + ent->v.model));

	signon->write_byte(ent->v.frame);
	signon->write_byte(ent->v.colormap);
	signon->write_byte(ent->v.skin);
	for (i = 0; i<3; i++)
	{
		signon->write_coord(ent->v.origin[i]);
		signon->write_angle(ent->v.angles[i]);
	}

	// throw the entity away now
	ED_Free(ent);
}

//=============================================================================

/*
==============
PF_setspawnparms
==============
*/
void q1Progs::PF_setspawnparms(void)
{
	edict_t	*ent;
	int		i;
	Client	*client;

	ent = G_EDICT(OFS_PARM0);
	i = NUM_FOR_EDICT(ent);

	client = m_sv.client(i - 1);
	if (client) {
		for (i = 0; i < NUM_SPAWN_PARMS; i++) {
			(&m_global_struct->parm1)[i] = client->m_spawn_parms[i];
		}

	}
}

/*
==============
PF_changelevel
==============
*/
void q1Progs::PF_changelevel(void)
{
	char	*s;

	// make sure we don't issue two changelevels
	if (m_sv.changelevel_issued)
		return;
	m_sv.changelevel_issued = true;

	s = G_STRING(OFS_PARM0);
	host.add_text("changelevel %s", s);
	//Cbuf_AddText(va("changelevel %s\n", s));
}

void q1Progs::PF_Fixme(void)
{
	run_error("unimplemented bulitin");
}

void q1Progs::PF_MoveToGoal(void)
{
	edict_t		*ent, *goal;
	float		dist;

	ent = prog_to_edict(m_global_struct->self);
	goal = prog_to_edict(ent->v.goalentity);
	dist = G_FLOAT(OFS_PARM0);

	if (!((int)ent->v.flags & (FL_ONGROUND | FL_FLY | FL_SWIM)))
	{
		G_FLOAT(OFS_RETURN) = 0;
		return;
	}

	m_sv.move_to_goal(ent, goal, dist);

}

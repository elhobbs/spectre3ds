#include "Host.h"
#include "cmd.h"
#include "cvar.h"
#include "q1Bsp.h"

cvar_t	sv_friction = { "sv_friction", "4", false, true };
cvar_t	sv_stopspeed = { "sv_stopspeed", "100" };
cvar_t	sv_gravity = { "sv_gravity", "800", false, true };
cvar_t	sv_maxvelocity = { "sv_maxvelocity", "2000" };
cvar_t	sv_nostep = { "sv_nostep", "0" };
cvar_t	sv_maxspeed = { "sv_maxspeed", "320", false, true };
cvar_t	sv_accelerate = { "sv_accelerate", "10" };
cvar_t	sv_edgefriction = { "edgefriction", "2" };

/*
=============
SV_RunThink

Runs thinking code if time.  There is some play in the exact time the think
function will be called, because it is called before any movement is done
in a frame.  Not used for pushmove objects, because they must be exact.
Returns false if the entity removed itself.
=============
*/
bool Server::run_think(edict_t *ent)
{
	float	thinktime = ent->v.nextthink;

	if (thinktime <= 0 || thinktime > m_time + m_frametime)
		return true;

	if (thinktime < m_time)
		thinktime = m_time;	// don't let things stay in the past.
	// it is possible to start that way
	// by a trigger with a local time.
	ent->v.nextthink = 0;
	m_progs->m_global_struct->time = thinktime;
	m_progs->m_global_struct->self = m_progs->edict_to_prog(ent);
	m_progs->m_global_struct->other = m_progs->edict_to_prog(m_progs->m_edicts[0]);
	m_progs->program_execute(ent->v.think);

	return !ent->free;
}

/*
=============
SV_Physics_None

Non moving objects can only think
=============
*/
void Server::Physics_None(edict_t *ent)
{
	// regular thinking
	run_think(ent);
}

/*
=============
SV_Physics_Noclip

A moving object that doesn't obey physics
=============
*/
void Server::Physics_Noclip(edict_t *ent)
{
	// regular thinking
	if (!run_think(ent))
		return;

	VectorMA(ent->v.angles, m_frametime, ent->v.avelocity, ent->v.angles);
	VectorMA(ent->v.origin, m_frametime, ent->v.velocity, ent->v.origin);

	link_edict(ent, false);
}

void Server::check_velocity(edict_t *ent)
{
	int		i;

	//
	// bound velocity
	//
	for (i = 0; i<3; i++)
	{
		if (IS_NAN(ent->v.velocity[i]))
		{
			printf("Got a NaN velocity on %s\n", m_progs->m_strings + ent->v.classname);
			ent->v.velocity[i] = 0;
		}
		if (IS_NAN(ent->v.origin[i]))
		{
			printf("Got a NaN origin on %s\n", m_progs->m_strings + ent->v.classname);
			ent->v.origin[i] = 0;
		}
		if (ent->v.velocity[i] > sv_maxvelocity.value)
			ent->v.velocity[i] = sv_maxvelocity.value;
		else if (ent->v.velocity[i] < -sv_maxvelocity.value)
			ent->v.velocity[i] = -sv_maxvelocity.value;
	}
}


/*
=============
SV_CheckWater
=============
*/
bool Server::check_water(edict_t *ent)
{
	vec3_t	point;
	int		cont;

	point[0] = ent->v.origin[0];
	point[1] = ent->v.origin[1];
	point[2] = ent->v.origin[2] + ent->v.mins[2] + 1;

	ent->v.waterlevel = 0;
	ent->v.watertype = CONTENTS_EMPTY;
	cont = point_contents(point);
	if (cont <= CONTENTS_WATER)
	{
		ent->v.watertype = cont;
		ent->v.waterlevel = 1;
		point[2] = ent->v.origin[2] + (ent->v.mins[2] + ent->v.maxs[2])*0.5;
		cont = point_contents(point);
		if (cont <= CONTENTS_WATER)
		{
			ent->v.waterlevel = 2;
			point[2] = ent->v.origin[2] + ent->v.view_ofs[2];
			cont = point_contents(point);
			if (cont <= CONTENTS_WATER)
				ent->v.waterlevel = 3;
		}
	}

	return ent->v.waterlevel > 1;
}

/*
============
SV_AddGravity

============
*/
void Server::add_gravity(edict_t *ent)
{
	float	ent_gravity;
	eval_t	*val;

	val = m_progs->GetEdictFieldValue(ent, "gravity");
	if (val && val->_float)
		ent_gravity = val->_float;
	else
		ent_gravity = 1.0;

	ent->v.velocity[2] -= ent_gravity * sv_gravity.value * m_frametime;
}

edict_t	*Server::test_entity_position(edict_t *ent)
{
#if 0
	if (point_contents(ent) == CONTENTS_SOLID) {
		return m_progs->m_edicts[0];
	}
#else
	trace_t	trace;

	trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, ent->v.origin, 0, ent);

	if (trace.startsolid)
	{
		return m_progs->m_edicts[0];
	}
#endif
	return 0;
}

void Server::check_stuck(edict_t *ent)
{
	int		i, j;
	int		z;
	vec3_t	org;

	if (!test_entity_position(ent))
	{
		VectorCopy(ent->v.origin, ent->v.oldorigin);
		return;
	}

	VectorCopy(ent->v.origin, org);
	VectorCopy(ent->v.oldorigin, ent->v.origin);
	if (!test_entity_position(ent))
	{
		printf("un 1 ");
		link_edict(ent, true);
		return;
	}

	for (z = 0; z< 18; z++)
	for (i = -1; i <= 1; i++)
	for (j = -1; j <= 1; j++)
	{
		ent->v.origin[0] = org[0] + i;
		ent->v.origin[1] = org[1] + j;
		ent->v.origin[2] = org[2] + z;
		if (!test_entity_position(ent))
		{
			printf("Unstuck 2\n");
			link_edict(ent, true);
			return;
		}
	}

	VectorCopy(org, ent->v.origin);
	printf("player is stuck.\n");
}

/*
==================
SV_Impact

Two entities have touched, so run their touch functions
==================
*/
void Server::impact(edict_t *e1, edict_t *e2)
{
	int		old_self, old_other;

	old_self = m_progs->m_global_struct->self;
	old_other = m_progs->m_global_struct->other;

	m_progs->m_global_struct->time = m_time;
	if (e1->v.touch && e1->v.solid != SOLID_NOT)
	{
		m_progs->m_global_struct->self = m_progs->edict_to_prog(e1);
		m_progs->m_global_struct->other = m_progs->edict_to_prog(e2);
		m_progs->program_execute(e1->v.touch);
	}

	if (e2->v.touch && e2->v.solid != SOLID_NOT)
	{
		m_progs->m_global_struct->self = m_progs->edict_to_prog(e2);
		m_progs->m_global_struct->other = m_progs->edict_to_prog(e1);
		m_progs->program_execute(e2->v.touch);
	}

	m_progs->m_global_struct->self = old_self;
	m_progs->m_global_struct->other = old_other;
}

/*
==================
ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
#define	STOP_EPSILON	0.1

int Server::clip_velocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float	backoff;
	float	change;
	int		i, blocked;

	blocked = 0;
	if (normal[2] > 0)
		blocked |= 1;		// floor
	if (!normal[2])
		blocked |= 2;		// step

	backoff = DotProduct(in, normal) * overbounce;

	for (i = 0; i<3; i++)
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}

	return blocked;
}

/*
============
SV_FlyMove

The basic solid body movement clip that slides along multiple planes
Returns the clipflags if the velocity was modified (hit something solid)
1 = floor
2 = wall / step
4 = dead stop
If steptrace is not NULL, the trace of any vertical wall hit will be stored
============
*/
#define	MAX_CLIP_PLANES	5
int Server::fly_move(edict_t *ent, float time, trace_t *steptrace)
{
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity, original_velocity, new_velocity;
	int			i, j;
	trace_t		trace;
	vec3_t		end;
	float		time_left;
	int			blocked;

	numbumps = 4;

	blocked = 0;
	VectorCopy(ent->v.velocity, original_velocity);
	VectorCopy(ent->v.velocity, primal_velocity);
	numplanes = 0;

	time_left = time;

	for (bumpcount = 0; bumpcount<numbumps; bumpcount++)
	{
		if (!ent->v.velocity[0] && !ent->v.velocity[1] && !ent->v.velocity[2])
			break;

		for (i = 0; i<3; i++)
			end[i] = ent->v.origin[i] + time_left * ent->v.velocity[i];

		trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, end, false, ent);

		if (trace.allsolid)
		{	// entity is trapped in another solid
			VectorCopy(vec3_origin, ent->v.velocity);
			return 3;
		}

		if (trace.fraction >= 0 && !trace.startsolid)
		{
			// actually covered some distance
			VectorCopy(trace.endpos, ent->v.origin);
			VectorCopy(ent->v.velocity, original_velocity);
			numplanes = 0;
		}

		if (trace.fraction == 1)
			break;		// moved the entire distance

		if (!trace.ent)
			sys.error("SV_FlyMove: !trace.ent");

		if (trace.plane.m_normal[2].c_float() > 0.7f)
		{
			//printf("floor\n");
			blocked |= 1;		// floor
			if (trace.ent->v.solid == SOLID_BSP)
			{
				ent->v.flags = (int)ent->v.flags | FL_ONGROUND;
				ent->v.groundentity = m_progs->edict_to_prog(trace.ent);
			}
		}
		if (!trace.plane.m_normal[2].x)
		{
			blocked |= 2;		// step
			if (steptrace)
				*steptrace = trace;	// save for player extrafriction
		}

		//
		// run the impact function
		//
		impact(ent, trace.ent);
		if (ent->free)
			break;		// removed by the impact function


		time_left -= time_left * trace.fraction;

		// cliped to another plane
		if (numplanes >= MAX_CLIP_PLANES)
		{	// this shouldn't really happen
			VectorCopy(vec3_origin, ent->v.velocity);
			return 3;
		}

		for (int ii = 0; ii < 3; ii++ ) {
			planes[numplanes][ii] = trace.plane.m_normal[ii].c_float();
		}
		numplanes++;

		//
		// modify original_velocity so it parallels all of the clip planes
		//
		for (i = 0; i<numplanes; i++)
		{
			clip_velocity(original_velocity, planes[i], new_velocity, 1);
			for (j = 0; j < numplanes; j++) {
				if (j != i) {
					if (DotProduct(new_velocity, planes[j]) < 0)
						break;	// not ok
				}
			}
			if (j == numplanes) {
				break;
			}
		}

		if (i != numplanes)
		{	// go along this plane
			VectorCopy(new_velocity, ent->v.velocity);
		}
		else
		{	// go along the crease
			if (numplanes != 2)
			{
				printf ("clip velocity, numplanes == %i\n",numplanes);
				VectorCopy(vec3_origin, ent->v.velocity);
				return 7;
			}
			CrossProduct(planes[0], planes[1], dir);
			d = DotProduct(dir, ent->v.velocity);
			VectorScale(dir, d, ent->v.velocity);
		}

		//
		// if original velocity is against the original velocity, stop dead
		// to avoid tiny occilations in sloping corners
		//
		if (DotProduct(ent->v.velocity, primal_velocity) <= 0)
		{
			VectorCopy(vec3_origin, ent->v.velocity);
			return blocked;
		}
	}

	return blocked;
}

void Server::wall_friction(edict_t *ent, trace_t *trace)
{
	vec3_t		forward, right, up;
	float		d, i;
	vec3_t		into, side;
	vec3_t		normal;

	for (int ii=0; ii < 3; ii++) {
		normal[ii] = trace->plane.m_normal[ii].c_float();
	}

	AngleVectors(ent->v.v_angle, forward, right, up);
	d = DotProduct(normal, forward);

	//printf("\nwall: %1.4f %1.4f %1.4f\n", normal[0], normal[1], normal[2]);
	//printf("norm: %1.4f %1.4f %1.4f\n", forward[0], forward[1], forward[2]);

	d += 0.5f;
	if (d >= 0)
		return;

	// cut the tangential velocity
	i = DotProduct(normal, ent->v.velocity);
	VectorScale(normal, i, into);
	VectorSubtract(ent->v.velocity, into, side);

	ent->v.velocity[0] = side[0] * (1 + d);
	ent->v.velocity[1] = side[1] * (1 + d);
}

int Server::try_unstick(edict_t *ent, vec3_t oldvel)
{
	int		i;
	vec3_t	oldorg;
	vec3_t	dir;
	int		clip;
	trace_t	steptrace;

	VectorCopy(ent->v.origin, oldorg);
	VectorCopy(vec3_origin, dir);

	for (i = 0; i<8; i++)
	{
		// try pushing a little in an axial direction
		switch (i)
		{
		case 0:	dir[0] = 2; dir[1] = 0; break;
		case 1:	dir[0] = 0; dir[1] = 2; break;
		case 2:	dir[0] = -2; dir[1] = 0; break;
		case 3:	dir[0] = 0; dir[1] = -2; break;
		case 4:	dir[0] = 2; dir[1] = 2; break;
		case 5:	dir[0] = -2; dir[1] = 2; break;
		case 6:	dir[0] = 2; dir[1] = -2; break;
		case 7:	dir[0] = -2; dir[1] = -2; break;
		}

		push_entity(ent, dir);

		// retry the original move
		ent->v.velocity[0] = oldvel[0];
		ent->v.velocity[1] = oldvel[1];
		ent->v.velocity[2] = 0;
		clip = fly_move(ent, 0.1f, &steptrace);

		if (fabs(oldorg[1] - ent->v.origin[1]) > 4
			|| fabs(oldorg[0] - ent->v.origin[0]) > 4)
		{
			printf ("unstuck!\n");
			return clip;
		}

		// go back to the original pos and try again
		VectorCopy(oldorg, ent->v.origin);
	}

	VectorCopy(vec3_origin, ent->v.velocity);
	return 7;		// still not moving
}

trace_t Server::push_entity(edict_t *ent, vec3_t push)
{
	trace_t	trace;
	vec3_t	end;

	VectorAdd(ent->v.origin, push, end);

	if (ent->v.movetype == MOVETYPE_FLYMISSILE)
	{
		trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, end, MOVE_MISSILE, ent);
	}
	else if (ent->v.solid == SOLID_TRIGGER || ent->v.solid == SOLID_NOT)
		// only clip against bmodels
		trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, end, MOVE_NOMONSTERS, ent);
	else
		trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, end, MOVE_NORMAL, ent);

	VectorCopy(trace.endpos, ent->v.origin);
	link_edict(ent, true);

	if (trace.ent)
	{
		impact(ent, trace.ent);
	}

	return trace;
}

/*
=====================
SV_WalkMove

Only used by players
======================
*/
void Server::walk_move(edict_t *ent)
{
	vec3_t		upmove, downmove;
	vec3_t		oldorg, oldvel;
	vec3_t		nosteporg, nostepvel;
	int			clip;
	int			oldonground;
	trace_t		steptrace, downtrace;

	//return;
	//
	// do a regular slide move unless it looks like you ran into a step
	//
	oldonground = (int)ent->v.flags & FL_ONGROUND;
	ent->v.flags = (int)ent->v.flags & ~FL_ONGROUND;

	VectorCopy(ent->v.origin, oldorg);
	VectorCopy(ent->v.velocity, oldvel);

	clip = fly_move(ent, m_frametime, &steptrace);

	if (!(clip & 2))
		return;		// move didn't block on a step

	//printf("a");
	if (!oldonground && ent->v.waterlevel == 0)
		return;		// don't stair up while jumping

	//printf("b");
	if (ent->v.movetype != MOVETYPE_WALK)
		return;		// gibbed by a trigger

	//printf("c");
	if (sv_nostep.value)
		return;

	if ((int)ent->v.flags & FL_WATERJUMP)
		return;

	//printf("d");
	VectorCopy(ent->v.origin, nosteporg);
	VectorCopy(ent->v.velocity, nostepvel);
	oldonground = (int)ent->v.flags & FL_ONGROUND;
	ent->v.flags = (int)ent->v.flags & ~FL_ONGROUND;

	//
	// try moving up and forward to go up a step
	//
	VectorCopy(oldorg, ent->v.origin);	// back to start pos

	VectorCopy(vec3_origin, upmove);
	VectorCopy(vec3_origin, downmove);
	upmove[2] = STEPSIZE;
	downmove[2] = -STEPSIZE + oldvel[2] * m_frametime;

	// move up
	push_entity(ent, upmove);	// FIXME: don't link?

	//printf("e");

	// move forward
	ent->v.velocity[0] = oldvel[0];
	ent->v.velocity[1] = oldvel[1];
	ent->v.velocity[2] = 0;
	clip = fly_move(ent, m_frametime, &steptrace);

	// check for stuckness, possibly due to the limited precision of floats
	// in the clipping hulls
	if (clip)
	{
		if (fabs(oldorg[1] - ent->v.origin[1]) < 0.03125
			&& fabs(oldorg[0] - ent->v.origin[0]) < 0.03125)
		{	// stepping up didn't make any progress
			clip = try_unstick(ent, oldvel);
		}
	}

	// extra friction based on view angle
	if (clip & 2)
		wall_friction(ent, &steptrace);

	// move down
	downtrace = push_entity(ent, downmove);	// FIXME: don't link?

	if (downtrace.plane.m_normal[2].c_float() > 0.7f)
	{
		if (ent->v.solid == SOLID_BSP)
		{
			//printf("f");
			ent->v.flags = (int)ent->v.flags | FL_ONGROUND;
			ent->v.groundentity = m_progs->edict_to_prog(downtrace.ent);
		}
	}
	else
	{
		//printf("g");
		// if the push down didn't end up on good ground, use the move without
		// the step up.  This happens near wall / slope combinations, and can
		// cause the player to hop up higher on a slope too steep to climb	
		VectorCopy(nosteporg, ent->v.origin);
		VectorCopy(nostepvel, ent->v.velocity);
		if (oldonground)
		{
			ent->v.flags = (int)ent->v.flags | FL_ONGROUND;
		}
		else
		{
			ent->v.flags = (int)ent->v.flags & ~FL_ONGROUND;
		}
	}
}

void Server::check_water_transition(edict_t *ent)
{
	int		cont;
	cont = point_contents(ent->v.origin);

	if (!ent->v.watertype)
	{	// just spawned here
		ent->v.watertype = cont;
		ent->v.waterlevel = 1;
		return;
	}

	if (cont <= CONTENTS_WATER)
	{
		if (ent->v.watertype == CONTENTS_EMPTY)
		{	// just crossed into water
			//SV_StartSound(ent, 0, "misc/h2ohit1.wav", 255, 1);
		}
		ent->v.watertype = cont;
		ent->v.waterlevel = 1;
	}
	else
	{
		if (ent->v.watertype != CONTENTS_EMPTY)
		{	// just crossed into water
			//SV_StartSound(ent, 0, "misc/h2ohit1.wav", 255, 1);
		}
		ent->v.watertype = CONTENTS_EMPTY;
		ent->v.waterlevel = cont;
	}
}

/*
=============
SV_Physics_Toss

Toss, bounce, and fly movement.  When onground, do nothing.
=============
*/
void Server::Physics_Toss(edict_t *ent)
{
	trace_t	trace;
	vec3_t	move;
	float	backoff;
	vec3_t	normal;

	// regular thinking
	if (!run_think(ent))
		return;

	// if onground, return without moving
	if (((int)ent->v.flags & FL_ONGROUND))
		return;

	check_velocity(ent);

	// add gravity
	if (ent->v.movetype != MOVETYPE_FLY
		&& ent->v.movetype != MOVETYPE_FLYMISSILE)
		add_gravity(ent);

	// move angles
	VectorMA(ent->v.angles, m_frametime, ent->v.avelocity, ent->v.angles);

	// move origin
	VectorScale(ent->v.velocity, m_frametime, move);

	trace = push_entity(ent, move);

	if (trace.fraction == 1)
		return;
	if (ent->free)
		return;

	if (ent->v.movetype == MOVETYPE_BOUNCE)
		backoff = 1.5;
	else
		backoff = 1;

	trace.plane.m_normal.copy_to(normal);

	clip_velocity(ent->v.velocity, normal, ent->v.velocity, backoff);

	// stop if on ground
	if (normal[2] > 0.7)
	{
		if (ent->v.velocity[2] < 60 || ent->v.movetype != MOVETYPE_BOUNCE)
		{
			ent->v.flags = (int)ent->v.flags | FL_ONGROUND;
			ent->v.groundentity = m_progs->edict_to_prog(trace.ent);
			VectorCopy(vec3_origin, ent->v.velocity);
			VectorCopy(vec3_origin, ent->v.avelocity);
		}
	}

	// check for in water
	check_water_transition(ent);
}

void Server::Physics_Client(edict_t	*ent, int num)
{
	if (!m_clients[num - 1].m_active)
		return;		// unconnected slot

	//
	// call standard client pre-think
	//	
	m_progs->m_global_struct->time = m_time;
	m_progs->m_global_struct->self = m_progs->edict_to_prog(ent);
	m_progs->program_execute(m_progs->m_global_struct->PlayerPreThink);

	//
	// do a move
	//
	check_velocity(ent);

	//
	// decide which move function to call
	//
	//ent->v.movetype = MOVETYPE_NOCLIP;
	switch ((int)ent->v.movetype)
	{
	case MOVETYPE_NONE:
		if (!run_think(ent)) {
			return;
		}
		break;

	case MOVETYPE_WALK:
		if (!run_think(ent)) {
			return;
		}

		if (!check_water(ent) && !((int)ent->v.flags & FL_WATERJUMP)) {
			add_gravity(ent);
		}

		check_stuck(ent);

		if (sv_nostep.value) {
			sv_nostep.value = 0;
		}

		walk_move(ent);

		break;

	case MOVETYPE_TOSS:
	case MOVETYPE_BOUNCE:
		Physics_Toss(ent);
		break;

	case MOVETYPE_FLY:
		if (!run_think(ent))
			return;
		fly_move(ent, m_frametime, NULL);
		break;
	case MOVETYPE_NOCLIP:
		if (!run_think(ent))
			return;
		VectorMA(ent->v.origin, m_frametime, ent->v.velocity, ent->v.origin);
		break;

	default:
		sys.error("SV_Physics_client: bad movetype %i", (int)ent->v.movetype);
	}

	//
	// call standard player post-think
	//		
	link_edict(ent, true);

	m_progs->m_global_struct->time = m_time;
	m_progs->m_global_struct->self = m_progs->edict_to_prog(ent);
	m_progs->program_execute(m_progs->m_global_struct->PlayerPostThink);
}

/*
============
SV_PushMove

============
*/
void Server::push_move(edict_t *pusher, float movetime)
{
	int			i, e;
	edict_t		*check, *block;
	vec3_t		mins, maxs, move;
	vec3_t		entorig, pushorig;
	int			num_moved;
	edict_t		*moved_edict[MAX_EDICTS];
	vec3_t		moved_from[MAX_EDICTS];

	if (!pusher->v.velocity[0] && !pusher->v.velocity[1] && !pusher->v.velocity[2])
	{
		pusher->v.ltime += movetime;
		return;
	}

	for (i = 0; i<3; i++)
	{
		move[i] = pusher->v.velocity[i] * movetime;
		mins[i] = pusher->v.absmin[i] + move[i];
		maxs[i] = pusher->v.absmax[i] + move[i];
	}

	VectorCopy(pusher->v.origin, pushorig);

	// move the pusher to it's final position

	VectorAdd(pusher->v.origin, move, pusher->v.origin);
	pusher->v.ltime += movetime;
	link_edict(pusher, false);


	// see if any solid entities are inside the final position
	num_moved = 0;
	check = m_progs->NEXT_EDICT(m_progs->m_edicts[0]);
	for (e = 1; e<m_progs->m_num_edicts; e++, check = m_progs->NEXT_EDICT(check))
	{
		if (check->free)
			continue;
		if (check->v.movetype == MOVETYPE_PUSH ||
			check->v.movetype == MOVETYPE_NONE ||
			check->v.movetype == MOVETYPE_NOCLIP)
			continue;

		// if the entity is standing on the pusher, it will definately be moved
		if (!(((int)check->v.flags & FL_ONGROUND)
			&& m_progs->prog_to_edict(check->v.groundentity) == pusher))
		{
			if (check->v.absmin[0] >= maxs[0] ||
				check->v.absmin[1] >= maxs[1] ||
				check->v.absmin[2] >= maxs[2] ||
				check->v.absmax[0] <= mins[0] ||
				check->v.absmax[1] <= mins[1] ||
				check->v.absmax[2] <= mins[2])
				continue;

			// see if the ent's bbox is inside the pusher's final position
			if (!test_entity_position(check))
				continue;
		}

		// remove the onground flag for non-players
		if (check->v.movetype != MOVETYPE_WALK)
			check->v.flags = (int)check->v.flags & ~FL_ONGROUND;

		VectorCopy(check->v.origin, entorig);
		VectorCopy(check->v.origin, moved_from[num_moved]);
		moved_edict[num_moved] = check;
		num_moved++;

		// try moving the contacted entity 
		pusher->v.solid = SOLID_NOT;
		push_entity(check, move);
		pusher->v.solid = SOLID_BSP;

		// if it is still inside the pusher, block
		block = test_entity_position(check);
		if (block)
		{	// fail the move
			if (check->v.mins[0] == check->v.maxs[0])
				continue;
			if (check->v.solid == SOLID_NOT || check->v.solid == SOLID_TRIGGER)
			{	// corpse
				check->v.mins[0] = check->v.mins[1] = 0;
				VectorCopy(check->v.mins, check->v.maxs);
				continue;
			}

			VectorCopy(entorig, check->v.origin);
			link_edict(check, true);

			VectorCopy(pushorig, pusher->v.origin);
			link_edict(pusher, false);
			pusher->v.ltime -= movetime;

			// if the pusher has a "blocked" function, call it
			// otherwise, just stay in place until the obstacle is gone
			if (pusher->v.blocked)
			{
				m_progs->m_global_struct->self = m_progs->edict_to_prog(pusher);
				m_progs->m_global_struct->other = m_progs->edict_to_prog(check);
				m_progs->program_execute(pusher->v.blocked);
			}

			// move back any entities we already moved
			for (i = 0; i<num_moved; i++)
			{
				VectorCopy(moved_from[i], moved_edict[i]->v.origin);
				link_edict(moved_edict[i], false);
			}
			return;
		}
	}


}

/*
================
SV_Physics_Pusher

================
*/
void Server::Physics_Pusher(edict_t *ent)
{
	float	thinktime;
	float	oldltime;
	float	movetime;

	oldltime = ent->v.ltime;

	thinktime = ent->v.nextthink;
	if (thinktime < ent->v.ltime + m_frametime)
	{
		movetime = thinktime - ent->v.ltime;
		if (movetime < 0)
			movetime = 0;
	}
	else
		movetime = m_frametime;

	if (movetime)
	{
		push_move(ent, movetime);	// advances ent->v.ltime if not blocked
	}

	if (thinktime > oldltime && thinktime <= ent->v.ltime)
	{
		ent->v.nextthink = 0;
		m_progs->m_global_struct->time = m_time;
		m_progs->m_global_struct->self = m_progs->edict_to_prog(ent);
		m_progs->m_global_struct->other = m_progs->edict_to_prog(m_progs->m_edicts[0]);
		m_progs->program_execute(ent->v.think);
		if (ent->free)
			return;
	}

}

/*
=============
SV_Physics_Step

Monsters freefall when they don't have a ground entity, otherwise
all movement is done with discrete steps.

This is also used for objects that have become still on the ground, but
will fall if the floor is pulled out from under them.
=============
*/
void Server::Physics_Step(edict_t *ent)
{
	bool	hitsound;

	// freefall if not onground
	if (!((int)ent->v.flags & (FL_ONGROUND | FL_FLY | FL_SWIM)))
	{
		if (ent->v.velocity[2] < sv_gravity.value*-0.1)
			hitsound = true;
		else
			hitsound = false;

		add_gravity(ent);
		check_velocity(ent);
		fly_move(ent, m_frametime, NULL);
		link_edict(ent, true);

		if ((int)ent->v.flags & FL_ONGROUND)	// just hit ground
		{
			//if (hitsound)
				//SV_StartSound(ent, 0, "demon/dland2.wav", 255, 1);
		}
	}

	// regular thinking
	run_think(ent);

	check_water_transition(ent);
}

void Server::physics(float frametime) {
	int		i;
	edict_t	*ent;

	if (m_paused) {
		return;
	}

	m_frametime = frametime;

	// let the progs know that a new frame has started
	m_progs->m_global_struct->self = m_progs->edict_to_prog(m_progs->m_edicts[0]);
	m_progs->m_global_struct->other = m_progs->edict_to_prog(m_progs->m_edicts[0]);
	m_progs->m_global_struct->time = m_time;
	m_progs->program_execute(m_progs->m_global_struct->StartFrame);

	//SV_CheckAllEnts ();

	//
	// treat each object in turn
	//
	ent = m_progs->m_edicts[0];
	for (i = 0; i<m_progs->m_num_edicts; i++, ent = m_progs->NEXT_EDICT(ent))
	{
		if (ent->free)
			continue;

		if (m_progs->m_global_struct->force_retouch)
		{
			link_edict(ent, true);	// force retouch even for stationary
		}

		if (i > 0 && i <= m_maxclients) {
			Physics_Client(ent, i);
		}
		else if (ent->v.movetype == MOVETYPE_PUSH) {
			Physics_Pusher(ent);
		}
		else if (ent->v.movetype == MOVETYPE_NONE) {
			Physics_None(ent);
		}
		else if (ent->v.movetype == MOVETYPE_NOCLIP) {
			Physics_Noclip(ent);
		}
		else if (ent->v.movetype == MOVETYPE_STEP) {
			Physics_Step(ent);
		}
		else if (ent->v.movetype == MOVETYPE_TOSS ||
			ent->v.movetype == MOVETYPE_BOUNCE ||
			ent->v.movetype == MOVETYPE_FLY ||
			ent->v.movetype == MOVETYPE_FLYMISSILE) {
			Physics_Toss(ent);
		}
		else {
			sys.error("SV_Physics: bad movetype %i", (int)ent->v.movetype);
		}
	}

	if (m_progs->m_global_struct->force_retouch) {
		m_progs->m_global_struct->force_retouch--;
	}

	m_time += frametime;
}

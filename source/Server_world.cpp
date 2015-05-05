#include "Host.h"
#include "Server.h"
#include "stdio.h"
#include "protocol.h"
#include "cmd.h"
#include "cvar.h"
#include "Model.h"
#include "q1Bsp.h"

void ClearLink(link_t *l)
{
	l->prev = l->next = l;
}

void RemoveLink(link_t *l)
{
	l->next->prev = l->prev;
	l->prev->next = l->next;
}

void InsertLinkBefore(link_t *l, link_t *before)
{
	link_t *next;
	for (link_t *ll = before->next; ll != before; ll = next)
	{
		if (ll == l) {
			int i = 0;
		}
		next = ll->next;
	}
	l->next = before;
	l->prev = before->prev;
	l->prev->next = l;
	l->next->prev = l;
}

void InsertLinkAfter(link_t *l, link_t *after)
{
	l->next = after->next;
	l->prev = after;
	l->prev->next = l;
	l->next->prev = l;
}

/*
===============
SV_CreateAreaNode

===============
*/
areanode_t *Server::CreateAreaNode(int depth, vec3_fixed32 &mins, vec3_fixed32 &maxs)
{
	areanode_t	*anode;
	vec3_fixed32		size;
	vec3_fixed32		mins1, maxs1, mins2, maxs2;

	anode = &sv_areanodes[sv_numareanodes];
	sv_numareanodes++;

	ClearLink(&anode->trigger_edicts);
	ClearLink(&anode->solid_edicts);

	if (depth == AREA_DEPTH)
	{
		anode->axis = -1;
		anode->children[0] = anode->children[1] = NULL;
		return anode;
	}

	//VectorSubtract(maxs, mins, size);
	size = maxs - mins;
	if (size[0] > size[1])
		anode->axis = 0;
	else
		anode->axis = 1;

	anode->dist = (maxs[anode->axis] + mins[anode->axis])/2;
	mins1 = mins;
	mins2 = mins;
	maxs1 = maxs;
	maxs2 = maxs;

	maxs1[anode->axis] = mins2[anode->axis] = anode->dist;

	anode->children[0] = CreateAreaNode(depth + 1, mins2, maxs2);
	anode->children[1] = CreateAreaNode(depth + 1, mins1, maxs1);

	return anode;
}

/*
===================
SV_HullForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
hull_t	*Server::hull_for_box(vec3_t mins, vec3_t maxs)
{
	box_planes[0].m_dist = maxs[0];
	box_planes[1].m_dist = mins[0];
	box_planes[2].m_dist = maxs[1];
	box_planes[3].m_dist = mins[1];
	box_planes[4].m_dist = maxs[2];
	box_planes[5].m_dist = mins[2];

	return &box_hull;
}

/*
================
SV_HullForEntity

Returns a hull that can be used for testing or clipping an object of mins/maxs
size.
Offset is filled in to contain the adjustment that must be added to the
testing object's origin to get a point to use with the returned hull.
================
*/
hull_t *Server::hull_for_entity(edict_t *ent, vec3_t mins, vec3_t maxs, vec3_t offset)
{
	Model	*model;
	vec3_t		size;
	vec3_t		hullmins, hullmaxs;
	hull_t		*hull;

	// decide which clipping hull to use, based on the size
	if (ent->v.solid == SOLID_BSP)
	{	// explicit hulls in the BSP model
		if (ent->v.movetype != MOVETYPE_PUSH)
			sys.error("SOLID_BSP without MOVETYPE_PUSH");

		model = m_models[(int)ent->v.modelindex];

		if (!model || m_models[(int)ent->v.modelindex]->type() != mod_brush)
			sys.error("MOVETYPE_PUSH with a non bsp model");

		q1Bsp *bsp = reinterpret_cast<q1Bsp *>(model);

		VectorSubtract(maxs, mins, size);
		if (size[0] < 3)
			hull = &bsp->m_hulls[0];
		else if (size[0] <= 32)
			hull = &bsp->m_hulls[1];
		else
			hull = &bsp->m_hulls[2];

		// calculate an offset value to center the origin
		VectorSubtract(hull->clip_mins, mins, offset);
		VectorAdd(offset, ent->v.origin, offset);
	}
	else
	{	// create a temp hull from bounding box sizes

		VectorSubtract(ent->v.mins, maxs, hullmins);
		VectorSubtract(ent->v.maxs, mins, hullmaxs);
		hull = hull_for_box(hullmins, hullmaxs);

		VectorCopy(ent->v.origin, offset);
	}


	return hull;
}

/*
===================
SV_InitBoxHull

Set up the planes and clipnodes so that the six floats of a bounding box
can just be stored out and get a proper hull_t structure.
===================
*/
void Server::init_box_hull(void)
{
	int		i;
	int		side;

	box_hull.clipnodes = box_clipnodes;
	box_hull.planes = box_planes;
	box_hull.firstclipnode = 0;
	box_hull.lastclipnode = 5;

	for (i = 0; i<6; i++)
	{
		//box_clipnodes[i].planenum = i;
		box_clipnodes[i].m_plane = &box_planes[i];

		side = i & 1;

		box_clipnodes[i].m_children[side] = CONTENTS_EMPTY;
		if (i != 5)
			box_clipnodes[i].m_children[side ^ 1] = i + 1;
		else
			box_clipnodes[i].m_children[side ^ 1] = CONTENTS_SOLID;
		float pl[4] = { 0, 0, 0, 0 };
		pl[i >> 1] = 1;
		box_planes[i] = pl;
	}

}

/*
===============
SV_ClearWorld

===============
*/
void Server::ClearWorld(void)
{
	init_box_hull();

	memset(sv_areanodes, 0, sizeof(sv_areanodes));
	sv_numareanodes = 0;
	CreateAreaNode(0, m_worldmodel->m_bounds[0], m_worldmodel->m_bounds[1]);
}

#if 1
/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
int BoxOnPlaneSide(vec3_t femins, vec3_t femaxs, q1_plane *p)
{
	__int64_t	dist1, dist2;
	int		sides;
	vec3_fixed32 emins;
	vec3_fixed32 emaxs;

	emins = femins;
	emaxs = femaxs;


	// general case
	switch (p->m_signbits)
	{
	case 0:
		dist1 = (__int64_t)p->m_normal[0].x * emaxs[0].x + (__int64_t)p->m_normal[1].x * emaxs[1].x + (__int64_t)p->m_normal[2].x * emaxs[2].x;
		dist2 = (__int64_t)p->m_normal[0].x * emins[0].x + (__int64_t)p->m_normal[1].x * emins[1].x + (__int64_t)p->m_normal[2].x * emins[2].x;
		break;
	case 1:
		dist1 = (__int64_t)p->m_normal[0].x * emins[0].x + (__int64_t)p->m_normal[1].x * emaxs[1].x + (__int64_t)p->m_normal[2].x * emaxs[2].x;
		dist2 = (__int64_t)p->m_normal[0].x * emaxs[0].x + (__int64_t)p->m_normal[1].x * emins[1].x + (__int64_t)p->m_normal[2].x * emins[2].x;
		break;
	case 2:
		dist1 = (__int64_t)p->m_normal[0].x * emaxs[0].x + (__int64_t)p->m_normal[1].x * emins[1].x + (__int64_t)p->m_normal[2].x * emaxs[2].x;
		dist2 = (__int64_t)p->m_normal[0].x * emins[0].x + (__int64_t)p->m_normal[1].x * emaxs[1].x + (__int64_t)p->m_normal[2].x * emins[2].x;
		break;
	case 3:
		dist1 = (__int64_t)p->m_normal[0].x * emins[0].x + (__int64_t)p->m_normal[1].x * emins[1].x + (__int64_t)p->m_normal[2].x * emaxs[2].x;
		dist2 = (__int64_t)p->m_normal[0].x * emaxs[0].x + (__int64_t)p->m_normal[1].x * emaxs[1].x + (__int64_t)p->m_normal[2].x * emins[2].x;
		break;
	case 4:
		dist1 = (__int64_t)p->m_normal[0].x * emaxs[0].x + (__int64_t)p->m_normal[1].x * emaxs[1].x + (__int64_t)p->m_normal[2].x * emins[2].x;
		dist2 = (__int64_t)p->m_normal[0].x * emins[0].x + (__int64_t)p->m_normal[1].x * emins[1].x + (__int64_t)p->m_normal[2].x * emaxs[2].x;
		break;
	case 5:
		dist1 = (__int64_t)p->m_normal[0].x * emins[0].x + (__int64_t)p->m_normal[1].x * emaxs[1].x + (__int64_t)p->m_normal[2].x * emins[2].x;
		dist2 = (__int64_t)p->m_normal[0].x * emaxs[0].x + (__int64_t)p->m_normal[1].x * emins[1].x + (__int64_t)p->m_normal[2].x * emaxs[2].x;
		break;
	case 6:
		dist1 = (__int64_t)p->m_normal[0].x * emaxs[0].x + (__int64_t)p->m_normal[1].x * emins[1].x + (__int64_t)p->m_normal[2].x * emins[2].x;
		dist2 = (__int64_t)p->m_normal[0].x * emins[0].x + (__int64_t)p->m_normal[1].x * emaxs[1].x + (__int64_t)p->m_normal[2].x * emaxs[2].x;
		break;
	case 7:
		dist1 = (__int64_t)p->m_normal[0].x * emins[0].x + (__int64_t)p->m_normal[1].x * emins[1].x + (__int64_t)p->m_normal[2].x * emins[2].x;
		dist2 = (__int64_t)p->m_normal[0].x * emaxs[0].x + (__int64_t)p->m_normal[1].x * emaxs[1].x + (__int64_t)p->m_normal[2].x * emaxs[2].x;
		break;
	default:
		dist1 = dist2 = 0;		// shut up compiler
		//BOPS_Error();
		break;
	}

	dist1 >>= 24;
	dist2 >>= 24;

	sides = 0;
	if (dist1 >= p->m_dist.x)
		sides = 1;
	if (dist2 < p->m_dist.x)
		sides |= 2;

	return sides;
}
#else
int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, q1_plane *p)
{
	fixed32p16	dist1, dist2;
	int		sides;

	dist1 = (*p) * emins;
	dist2 = (*p) * emaxs;

	sides = 0;
	if (dist1 >= 0)
		sides = 1;
	if (dist2 < 0)
		sides |= 2;

	return sides;
}
#endif

void Server::find_touched_leafs(edict_t *ent, q1_bsp_node *node)
{
	q1_plane		*splitplane;
	q1_leaf_node	*leaf;
	int				sides;
	int				leafnum;

	if (node->m_contents == CONTENTS_SOLID)
		return;

	// add an efrag if the node is a leaf

	if (node->m_contents < 0)
	{
		if (ent->num_leafs == MAX_ENT_LEAFS)
			return;

		leaf = (q1_leaf_node *)node;
		leafnum = leaf - m_worldmodel->m_leafs - 1;

		ent->leafnums[ent->num_leafs] = leafnum;
		ent->num_leafs++;
		return;
	}

	// NODE_MIXED

	splitplane = node->m_plane;
	sides = BOX_ON_PLANE_SIDE(ent->v.absmin, ent->v.absmax, splitplane);

	// recurse down the contacted sides
	if (sides & 1) {
		find_touched_leafs(ent, node->m_children[0]);
	}

	if (sides & 2) {
		find_touched_leafs(ent, node->m_children[1]);
	}
}

void Server::touch_links(edict_t *ent, areanode_t *node)
{
	link_t		*l, *next;
	edict_t		*touch;
	int			old_self, old_other;

	// touch linked edicts
	for (l = node->trigger_edicts.next; l != &node->trigger_edicts; l = next)
	{
		next = l->next;
		touch = EDICT_FROM_AREA(l);
		if (touch == ent)
			continue;
		if (!touch->v.touch || touch->v.solid != SOLID_TRIGGER)
			continue;
		if (ent->v.absmin[0] > touch->v.absmax[0]
			|| ent->v.absmin[1] > touch->v.absmax[1]
			|| ent->v.absmin[2] > touch->v.absmax[2]
			|| ent->v.absmax[0] < touch->v.absmin[0]
			|| ent->v.absmax[1] < touch->v.absmin[1]
			|| ent->v.absmax[2] < touch->v.absmin[2])
			continue;
		old_self = m_progs->m_global_struct->self;
		old_other = m_progs->m_global_struct->other;

		m_progs->m_global_struct->self = m_progs->edict_to_prog(touch);
		m_progs->m_global_struct->other = m_progs->edict_to_prog(ent);
		m_progs->m_global_struct->time = m_time;
		m_progs->program_execute(touch->v.touch);

		m_progs->m_global_struct->self = old_self;
		m_progs->m_global_struct->other = old_other;
	}

	// recurse down both sides
	if (node->axis == -1)
		return;

	if (ent->v.absmax[node->axis] > node->dist.c_float())
		touch_links(ent, node->children[0]);
	if (ent->v.absmin[node->axis] < node->dist.c_float())
		touch_links(ent, node->children[1]);
}

void Server::link_edict(edict_t *ent, bool touch_triggers)
{
	areanode_t	*node;

	if (ent->area.prev)
		unlink_edict(ent);	// unlink from old position

	if (ent == m_progs->m_edicts[0])
		return;		// don't add the world

	if (ent->free)
		return;

	// set the abs box


	VectorAdd(ent->v.origin, ent->v.mins, ent->v.absmin);
	VectorAdd(ent->v.origin, ent->v.maxs, ent->v.absmax);

	//
	// to make items easier to pick up and allow them to be grabbed off
	// of shelves, the abs sizes are expanded
	//
	if ((int)ent->v.flags & FL_ITEM)
	{
		ent->v.absmin[0] -= 15;
		ent->v.absmin[1] -= 15;
		ent->v.absmax[0] += 15;
		ent->v.absmax[1] += 15;
	}
	else
	{	// because movement is clipped an epsilon away from an actual edge,
		// we must fully check even when bounding boxes don't quite touch
		ent->v.absmin[0] -= 1;
		ent->v.absmin[1] -= 1;
		ent->v.absmin[2] -= 1;
		ent->v.absmax[0] += 1;
		ent->v.absmax[1] += 1;
		ent->v.absmax[2] += 1;
	}

	// link to PVS leafs
	ent->num_leafs = 0;
	if (ent->v.modelindex)
	{
		find_touched_leafs(ent, m_worldmodel->m_nodes);
	}

	if (ent->v.solid == SOLID_NOT)
		return;

	// find the first node that the ent's box crosses
	node = sv_areanodes;
	while (1)
	{
		if (node->axis == -1)
			break;
		if (ent->v.absmin[node->axis] > node->dist.c_float())
			node = node->children[0];
		else if (ent->v.absmax[node->axis] < node->dist.c_float())
			node = node->children[1];
		else
			break;		// crosses the node
	}

	// link it in	

	if (ent->v.solid == SOLID_TRIGGER)
		InsertLinkBefore(&ent->area, &node->trigger_edicts);
	else
		InsertLinkBefore(&ent->area, &node->solid_edicts);

	// if touch_triggers, touch all entities at this node and decend for more
	if (touch_triggers)
		touch_links(ent, sv_areanodes);
}

void Server::unlink_edict(edict_t *ent)
{
	if (!ent->area.prev)
		return;		// not linked in anywhere
	RemoveLink(&ent->area);
	ent->area.prev = ent->area.next = 0;
}


int Server::trace_clip_r(hull_t *hull, int parentnum, int nodenum, traceResults &trace, fixed32p16 f0, fixed32p16 f1, vec3_fixed32 &p0, vec3_fixed32 &p1) {

	static const fixed32p16 offset(0.001f);
	static const fixed32p16 one(1.0f);
	fixed32p16 d0;
	fixed32p16 d1;
	q1_clipnode *node;
	int side = 0;

	do {
		//alread at end
		if (trace.fraction == 0) {
			return 0;
		}

		//in a leaf
		if (nodenum < 0) {
			if (nodenum == CONTENTS_SOLID) {
				if (trace.inopen == false)
					trace.startsolid = true;
				return 1;
			}
			trace.allsolid = false;
			if (nodenum == CONTENTS_EMPTY) {
				trace.inopen = true;
			}
			else {
				trace.inwater = true;
			}
			return 0;
		}

		node = &hull->clipnodes[nodenum];

		d0 = *(node->m_plane) * p0;
		d1 = *(node->m_plane) * p1;

		//both in front
		if (d0 >= 0 && d1 >= 0) {
			side = 0;
			parentnum = nodenum;
			nodenum = node->m_children[0];
			continue;
		}

		//both in back
		if (d0 < 0 && d1 < 0) {
			side = 1;
			parentnum = nodenum;
			nodenum = node->m_children[1];
			continue;
		}

		//found a split point
		break;
	} while (1);

	int ret;
	fixed32p16 frac0;
	fixed32p16 adjf;
	static const fixed32p16 eps(0.03125f);

	frac0 = d0 / (d0 - d1);
	if (d0 < d1) {
		side = 1;
		adjf = eps / (d1 - d0);
	}
	else if (d0 > d1) {
		side = 0;
		adjf = eps / (d0 - d1);
	}
	else {
		side = 0;
		frac0 = 1.0f;
		adjf = 0;
	}

	frac0 -= adjf;

	//make sure 0 to 1
	if (frac0 < 0) {
		frac0 = 0;
	}
	else if (frac0 > one) {
		frac0 = one;
	}

	//generate a split
	vec3_fixed32 midp0;
	fixed32p16 midf0;

	midp0 = p0 + (p1 - p0) * frac0;
	midf0 = f0 + (f1 - f0) * frac0;

	ret = trace_clip_r(hull, nodenum, node->m_children[side], trace, f0, midf0, p0, midp0);
	if (ret != 0) {
		return ret;
	}

	ret = trace_clip_r(hull, nodenum, node->m_children[side ^ 1], trace, midf0, f1, midp0, p1);
	if (ret != 1) {
		return ret;
	}

	trace.fraction = midf0;
	trace.end = midp0;
	trace.plane = side ? -(*node->m_plane) : *node->m_plane;

	return 2;
}


int Server::trace_clip(hull_t *hull, trace_t &trace, vec3_t p0, vec3_t p1) {
	vec3_fixed32 pp0;
	vec3_fixed32 pp1;
	fixed32p16 f0(0.0f);
	fixed32p16 f1(1.0f);
	traceResults tr;

	pp0 = p0;
	pp1 = p1;

	memset(&tr, 0, sizeof(tr));
	tr.allsolid = true;
	tr.fraction = trace.fraction;
	tr.end = trace.endpos;
	int ret = trace_clip_r(hull, hull->firstclipnode, hull->firstclipnode, tr, f0, f1, pp0, pp1);

	trace.allsolid = tr.allsolid;
	trace.startsolid = tr.startsolid;
	trace.inopen = tr.inopen;
	trace.inwater = tr.inwater;
	trace.ent = tr.ent;
	trace.fraction = tr.fraction.c_float();
	tr.end.copy_to(trace.endpos);
	trace.plane = tr.plane;

	return ret;
}

/*
==================
SV_ClipMoveToEntity

Handles selection or creation of a clipping hull, and offseting (and
eventually rotation) of the end points
==================
*/
trace_t Server::SV_ClipMoveToEntity(edict_t *ent, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	trace_t		trace;
	vec3_t		offset;
	vec3_t		start_l, end_l;
	hull_t		*hull;

	// fill in a default trace
	memset(&trace, 0, sizeof(trace_t));
	trace.fraction = 1;
	trace.allsolid = true;
	VectorCopy(end, trace.endpos);

	// get the clipping hull
	hull = hull_for_entity(ent, mins, maxs, offset);

	VectorSubtract(start, offset, start_l);
	VectorSubtract(end, offset, end_l);

	// trace a line through the apropriate clipping hull
	trace_clip(hull, trace, start_l, end_l);

	// fix trace up by the offset
	if (trace.fraction != 1)
		VectorAdd(trace.endpos, offset, trace.endpos);

	// did we clip the move?
	if (trace.fraction < 1 || trace.startsolid)
		trace.ent = ent;

	return trace;
}

void Server::SV_MoveBounds(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, vec3_t boxmins, vec3_t boxmaxs)
{
	int		i;

	for (i = 0; i<3; i++)
	{
		if (end[i] > start[i])
		{
			boxmins[i] = start[i] + mins[i] - 1;
			boxmaxs[i] = end[i] + maxs[i] + 1;
		}
		else
		{
			boxmins[i] = end[i] + mins[i] - 1;
			boxmaxs[i] = start[i] + maxs[i] + 1;
		}
	}
}

void Server::SV_ClipToLinks(areanode_t *node, moveclip_t *clip)
{
	link_t		*l, *next;
	edict_t		*touch;
	trace_t		trace;

	// touch linked edicts
	for (l = node->solid_edicts.next; l != &node->solid_edicts; l = next)
	{
		next = l->next;
		touch = EDICT_FROM_AREA(l);
		if (touch->v.solid == SOLID_NOT)
			continue;
		if (touch == clip->passedict)
			continue;
		if (touch->v.solid == SOLID_TRIGGER)
			sys.error("Trigger in clipping list");

		if (clip->type == MOVE_NOMONSTERS && touch->v.solid != SOLID_BSP)
			continue;

		if (clip->boxmins[0] > touch->v.absmax[0]
			|| clip->boxmins[1] > touch->v.absmax[1]
			|| clip->boxmins[2] > touch->v.absmax[2]
			|| clip->boxmaxs[0] < touch->v.absmin[0]
			|| clip->boxmaxs[1] < touch->v.absmin[1]
			|| clip->boxmaxs[2] < touch->v.absmin[2])
			continue;

		if (clip->passedict && clip->passedict->v.size[0] && !touch->v.size[0])
			continue;	// points never interact

		// might intersect, so do an exact clip
		if (clip->trace.allsolid)
			return;
		if (clip->passedict)
		{
			if (m_progs->prog_to_edict(touch->v.owner) == clip->passedict)
				continue;	// don't clip against own missiles
			if (m_progs->prog_to_edict(clip->passedict->v.owner) == touch)
				continue;	// don't clip against owner
		}

		if ((int)touch->v.flags & FL_MONSTER)
			trace = SV_ClipMoveToEntity(touch, clip->start, clip->mins2, clip->maxs2, clip->end);
		else
			trace = SV_ClipMoveToEntity(touch, clip->start, clip->mins, clip->maxs, clip->end);
		if (trace.allsolid || trace.startsolid ||
			trace.fraction < clip->trace.fraction)
		{
			trace.ent = touch;
			if (clip->trace.startsolid)
			{
				clip->trace = trace;
				clip->trace.startsolid = true;
			}
			else
				clip->trace = trace;
		}
		else if (trace.startsolid)
			clip->trace.startsolid = true;
	}

	// recurse down both sides
	if (node->axis == -1)
		return;

	if (clip->boxmaxs[node->axis] > node->dist.c_float())
		SV_ClipToLinks(node->children[0], clip);
	if (clip->boxmins[node->axis] < node->dist.c_float())
		SV_ClipToLinks(node->children[1], clip);
}
/*
==================
SV_Move
==================
*/
trace_t Server::SV_Move(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int type, edict_t *passedict)
{
	moveclip_t	clip;
	int			i;

	memset(&clip, 0, sizeof (moveclip_t));

	// clip to world
	clip.trace = SV_ClipMoveToEntity(m_progs->m_edicts[0], start, mins, maxs, end);

	clip.start = start;
	clip.end = end;
	clip.mins = mins;
	clip.maxs = maxs;
	clip.type = type;
	clip.passedict = passedict;

	if (type == MOVE_MISSILE)
	{
		for (i = 0; i<3; i++)
		{
			clip.mins2[i] = -15;
			clip.maxs2[i] = 15;
		}
	}
	else
	{
		VectorCopy(mins, clip.mins2);
		VectorCopy(maxs, clip.maxs2);
	}

	// create the bounding box of the entire move
	SV_MoveBounds(start, clip.mins2, clip.maxs2, end, clip.boxmins, clip.boxmaxs);

	// clip to entities
	SV_ClipToLinks(sv_areanodes, &clip);

	return clip.trace;
}

int Server::hull_point_contents(hull_t *hull, int num, vec3_t p)
{
	fixed32p16	d;
	q1_clipnode	*node;
	q1_plane	*plane;
	vec3_fixed16 pp;

	pp = p;

	while (num >= 0)
	{
		if (num < hull->firstclipnode || num > hull->lastclipnode)
			sys.error("SV_HullPointContents: bad node number");

		node = hull->clipnodes + num;
		//plane = hull->planes + node->planenum;
		plane = node->m_plane;

		d = *plane * p;

		if (d < 0)
			num = node->m_children[1];
		else
			num = node->m_children[0];
	}

	return num;
}

int Server::hull_point_contents(hull_t *hull, int num, vec3_fixed32 p)
{
	fixed32p16	d;
	q1_clipnode	*node;
	q1_plane	*plane;

	while (num >= 0)
	{
		if (num < hull->firstclipnode || num > hull->lastclipnode)
			sys.error("SV_HullPointContents: bad node number");

		node = hull->clipnodes + num;
		//plane = hull->planes + node->planenum;
		plane = node->m_plane;

		d = *plane * p;

		if (d < 0)
			num = node->m_children[1];
		else
			num = node->m_children[0];
	}

	return num;
}

int Server::point_contents(vec3_t p)
{
	int		cont;

	cont = hull_point_contents(&m_worldmodel->m_hulls[0], 0, p);
	if (cont <= CONTENTS_CURRENT_0 && cont >= CONTENTS_CURRENT_DOWN)
		cont = CONTENTS_WATER;
	return cont;
}

int Server::point_contents(edict_t *ent)
{
	int		cont;

	vec3_t size;
	hull_t *hull;

	VectorSubtract(ent->v.maxs, ent->v.mins, size);
	if (size[0] < 3)
		hull = &m_worldmodel->m_hulls[0];
	else if (size[0] <= 32)
		hull = &m_worldmodel->m_hulls[1];
	else
		hull = &m_worldmodel->m_hulls[2];

	cont = hull_point_contents(hull, hull->firstclipnode, ent->v.origin);

	return cont;
}


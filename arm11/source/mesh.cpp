/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// gl_mesh.c: triangle model functions

#include "sys.h"
#include "q1Mdl.h"
#include "Host.h"

#ifdef WIN32
#include <gl\gl.h>
#include <gl\glu.h>

typedef struct vboList_s {
	UINT32	prim;
	UINT32 length;
	UINT32 count;
	struct vboList_s *next;
} vboList_t;

typedef struct
{
	float x, y, z;
}vect3Df_s;

typedef struct
{
	vect3Df_s position;
	float texcoord[2];
	vect3Df_s normal;
}faceVertex_s;

void *memalign(int size, int align) {
	byte *data = new byte[size + align - 1];
	UINT32 adata = ((UINT32)data) + (align - 1);
	adata &= (~(align - 1));

	return (void *)adata;
}
#else
#include "ctr_render.h"
#endif

/*
=================================================================

ALIAS MODEL DISPLAY LIST GENERATION

=================================================================
*/

//model_t		*aliasmodel;
//aliashdr_t	*paliashdr;

int	used[8192];

// the command list holds counts and s/t values that are valid for
// every frame
int		commands[8192];
int		numcommands;

// all frames will have their vertexes rearranged and expanded
// so they are in the order expected by the command list
int		vertexorder[8192];
int		numorder;

int		allverts, alltris;

int		stripverts[128];
int		striptris[128];
int		stripcount;

/*
================
StripLength
================
*/
int	StripLength(dtriangle_t *tris, int numtris, int starttri, int startv)
{
	int			m1, m2;
	int			j;
	dtriangle_t	*last, *check;
	int			k;

	used[starttri] = 2;

	last = &tris[starttri];

	stripverts[0] = last->vertindex[(startv) % 3];
	stripverts[1] = last->vertindex[(startv + 1) % 3];
	stripverts[2] = last->vertindex[(startv + 2) % 3];

	striptris[0] = starttri;
	stripcount = 1;

	m1 = last->vertindex[(startv + 2) % 3];
	m2 = last->vertindex[(startv + 1) % 3];

	// look for a matching triangle
nexttri:
	for (j = starttri + 1, check = &tris[starttri + 1]; j<numtris; j++, check++)
	{
		if (check->facesfront != last->facesfront)
			continue;
		for (k = 0; k<3; k++)
		{
			if (check->vertindex[k] != m1)
				continue;
			if (check->vertindex[(k + 1) % 3] != m2)
				continue;

			// this is the next part of the fan

			// if we can't use this triangle, this tristrip is done
			if (used[j])
				goto done;

			// the new edge
			if (stripcount & 1)
				m2 = check->vertindex[(k + 2) % 3];
			else
				m1 = check->vertindex[(k + 2) % 3];

			stripverts[stripcount + 2] = check->vertindex[(k + 2) % 3];
			striptris[stripcount] = j;
			stripcount++;

			used[j] = 2;
			goto nexttri;
		}
	}
done:

	// clear the temp used flags
	for (j = starttri + 1; j<numtris; j++)
		if (used[j] == 2)
			used[j] = 0;

	return stripcount;
}

/*
===========
FanLength
===========
*/
int	FanLength(dtriangle_t *tris, int numtris, int starttri, int startv)
{
	int		m1, m2;
	int		j;
	dtriangle_t	*last, *check;
	int		k;

	used[starttri] = 2;

	last = &tris[starttri];

	stripverts[0] = last->vertindex[(startv) % 3];
	stripverts[1] = last->vertindex[(startv + 1) % 3];
	stripverts[2] = last->vertindex[(startv + 2) % 3];

	striptris[0] = starttri;
	stripcount = 1;

	m1 = last->vertindex[(startv + 0) % 3];
	m2 = last->vertindex[(startv + 2) % 3];


	// look for a matching triangle
nexttri:
	for (j = starttri + 1, check = &tris[starttri + 1]; j<numtris; j++, check++)
	{
		if (check->facesfront != last->facesfront)
			continue;
		for (k = 0; k<3; k++)
		{
			if (check->vertindex[k] != m1)
				continue;
			if (check->vertindex[(k + 1) % 3] != m2)
				continue;

			// this is the next part of the fan

			// if we can't use this triangle, this tristrip is done
			if (used[j])
				goto done;

			// the new edge
			m2 = check->vertindex[(k + 2) % 3];

			stripverts[stripcount + 2] = m2;
			striptris[stripcount] = j;
			stripcount++;

			used[j] = 2;
			goto nexttri;
		}
	}
done:

	// clear the temp used flags
	for (j = starttri + 1; j<numtris; j++)
		if (used[j] == 2)
			used[j] = 0;

	return stripcount;
}


/*
================
BuildTris

Generate a list of trifans or strips
for the model, which holds for all frames
================
*/
void BuildTris(q1Mdl *mdl, dtriangle_t *tris, int numtris, mstvert_t *stverts)
{
	int		i, j, k;
	int		startv;
	dtriangle_t	*last, *check;
	int		m1, m2;
	int		striplength;
	trivertx_t	*v;
	dtriangle_t *tv;
	float	s, t;
	int		index;
	int		len, bestlen, besttype;
	int		bestverts[1024];
	int		besttris[1024];
	int		type;

	//
	// build tristrips
	//
	numorder = 0;
	numcommands = 0;
	memset(used, 0, sizeof(used));
	for (i = 0; i<numtris; i++)
	{
		// pick an unused triangle and start the trifan
		if (used[i])
			continue;

		bestlen = 0;
		for (type = 0; type < 2; type++)
			//	type = 1;
		{
			for (startv = 0; startv < 3; startv++)
			{
				if (type == 1)
					len = StripLength(tris, numtris, i, startv);
				else
					len = FanLength(tris, numtris, i, startv);
				if (len > bestlen)
				{
					besttype = type;
					bestlen = len;
					for (j = 0; j<bestlen + 2; j++)
						bestverts[j] = stripverts[j];
					for (j = 0; j<bestlen; j++)
						besttris[j] = striptris[j];
				}
			}
		}

		// mark the tris on the best strip as used
		for (j = 0; j<bestlen; j++)
			used[besttris[j]] = 1;

		if (besttype == 1)
			commands[numcommands++] = (bestlen + 2);
		else
			commands[numcommands++] = -(bestlen + 2);

		for (j = 0; j<bestlen + 2; j++)
		{
			// emit a vertex into the reorder buffer
			k = bestverts[j];
			vertexorder[numorder++] = k;

			// emit s/t coords into the commands stream
			s = stverts[k].s.c_float();
			t = stverts[k].t.c_float();
			if (!tris[besttris[0]].facesfront && stverts[k].onseam)
				s += 0.5f;// skinwidth / 2;	// on back side
			//s = (s + 0.5) / skinwidth;
			//t = (t + 0.5) / skinheight;

			*(float *)&commands[numcommands++] = s;
			*(float *)&commands[numcommands++] = t;
		}
	}

	commands[numcommands++] = 0;		// end of list marker

	//printf("%3i tri %3i vert %3i cmd\n", numtris, numorder, numcommands);

	mdl->m_control = new(pool)u32[numcommands];
	memcpy(mdl->m_control, commands, numcommands * 4);

	//allverts += numorder;
	//alltris += pheader->numtris;
}

#ifdef WIN32

void draw_mdl_frame(q1MdlFrame *frame) {
	vboList_t *vbo = ((vboList_t *)&frame->data)->next;

	if (vbo == 0) {
		return;
	}

	do {
		if (vbo->prim == 0) {
			glBegin(GL_TRIANGLE_FAN);
		}
		else {
			glBegin(GL_TRIANGLE_STRIP);
		}
		faceVertex_s *f = (faceVertex_s *)(vbo + 1);
		int n = vbo->count;
		for (int i = 0; i < n; i++, f++) {
			glTexCoord2f(f->texcoord[0], f->texcoord[1]);
			glVertex3f(f->position.x, f->position.y, f->position.z);
		}
		glEnd();
		vbo = vbo->next;
	} while (vbo);
}

#else
void draw_mdl_frame(q1MdlFrame *frame) {
	vboList_t *vbo = frame->m_vboList.next;
	void *data;
	GPU_Primitive_t prim;

	if (vbo == 0) {
		return;
	}

	do {
		data = (vbo + 1);
		prim = vbo->prim == 0 ? GPU_TRIANGLE_FAN : GPU_TRIANGLE_STRIP;
		ctrDrawArrayDirectly(prim, (u8*)data, vbo->count);
		vbo = vbo->next;
	} while (vbo);
}

#endif

int vbo_ls = 0;


#if 1
void build_mdl_frame(q1MdlFrame *frame,
	dtriangle_t *tris,
	int numtris,
	trivertx_t *verts,
	mstvert_t *stverts,
	float skinwidth,
	float skinheight)
{
	mdlVertex_s f;
	float	s, t;
	float l;
	int	j;
	int	index;
	trivertx_t	*v;
	int	list;
	int	*order;
	vec3_t	point;
	float	*normal;
	int	count;
	int cb;
	int prim;
	u8 *data;
	vboList_t *this_vbo, *last_vbo = (vboList_t *)&frame->m_vboList;
	last_vbo->next = 0;

	j = 0;
	order = commands;
	trivertx_t *mverts = frame->m_verts = new(pool) trivertx_t[numorder];

	//printf("vbo_ls: %8d %d\n", vbo_ls, sizeof(vboList_t));
	//FILE *fp = fopen("mdl_ls.txt", "w");
	while (1)
	{
		// get the vertex count and primitive type
		count = *order++;
		if (!count)
			break; // done
		if (count < 0)
		{
			count = -count;
			prim = 0;
			//glBegin(GL_TRIANGLE_FAN);
		}
		else {
			prim = 1;
			//glBegin(GL_TRIANGLE_STRIP);
		}
		cb = sizeof(faceVertex_s) * count;
		vbo_ls += cb + sizeof(vboList_t);
#ifdef WIN32
		data = (byte *)memalign(cb + sizeof(vboList_t), 0x80);
#else
		data = (byte *)linear.alloc(cb + sizeof(vboList_t), 0x80);
#endif
		last_vbo->next = this_vbo = (vboList_t *)data;
		data = (byte *)(this_vbo + 1);
		this_vbo->count = count;
		this_vbo->prim = prim;
		this_vbo->length = cb;
		this_vbo->next = 0;
		//fprintf(fp, "%d %d\r\n", count, prim);
		do {
			*mverts = verts[vertexorder[j]];
			float x = mverts->v[0];
			float y = mverts->v[1];
			float z = mverts->v[2];
			float s = ((float *)order)[0];
			float t = ((float *)order)[1];

			f.position[0] = x;
			f.position[1] = y;
			f.position[2] = z;
			f.position[3] = mverts->lightnormalindex;

			f.texcoord[0] = s;
			f.texcoord[1] = t;
			order += 2;
			//fprintf(fp, "{ %3.4f %3.4f %3.4f } { %1.5f %1.5f } \r\n", x,y,z,s,t);
			memcpy(data, &f, sizeof(f));
			data += sizeof(f);
			j++;
			mverts++;
		} while (--count);
		last_vbo = this_vbo;
		//glEnd();
	}

	//fclose(fp);
}
#else
void build_mdl_frame(q1MdlFrame *frame,
	dtriangle_t *tris,
	int numtris,
	trivertx_t *verts,
	mstvert_t *stverts,
	float skinwidth,
	float skinheight)
{
	faceVertex_s f;
	float	s, t;
	float l;
	int	j;
	int	index;
	trivertx_t	*v;
	int	list;
	int	*order;
	vec3_t	point;
	float	*normal;
	int	count;
	int cb;
	int prim;
	u8 *data;
	vboList_t *this_vbo, *last_vbo = (vboList_t *)&frame->m_vboList;
	last_vbo->next = 0;

	j = 0;
	order = commands;
	trivertx_t *mverts = frame->m_verts = new(pool) trivertx_t[numorder];

	//printf("vbo_ls: %8d %d\n", vbo_ls, sizeof(vboList_t));
	//FILE *fp = fopen("mdl_ls.txt", "w");
	while (1)
	{
		// get the vertex count and primitive type
		count = *order++;
		if (!count)
			break; // done
		if (count < 0)
		{
			count = -count;
			prim = 0;
			//glBegin(GL_TRIANGLE_FAN);
		}
		else {
			prim = 1;
			//glBegin(GL_TRIANGLE_STRIP);
		}
		cb = sizeof(faceVertex_s) * count;
		vbo_ls += cb + sizeof(vboList_t);
#ifdef WIN32
		data = (byte *)memalign(cb + sizeof(vboList_t), 0x80);
#else
		data = (byte *)linear.alloc(cb + sizeof(vboList_t), 0x80);
#endif
		last_vbo->next = this_vbo = (vboList_t *)data;
		data = (byte *)(this_vbo + 1);
		this_vbo->count = count;
		this_vbo->prim = prim;
		this_vbo->length = cb;
		this_vbo->next = 0;
		//fprintf(fp, "%d %d\r\n", count, prim);
		do {
			*mverts = verts[vertexorder[j]];
			float x = mverts->v[0];
			float y = mverts->v[1];
			float z = mverts->v[2];
			float s = ((float *)order)[0];
			float t = ((float *)order)[1];

			f.position.x = x;
			f.position.y = y;
			f.position.z = z;

			f.texcoord[0] = s;
			f.texcoord[1] = t;
			order += 2;
			//fprintf(fp, "{ %3.4f %3.4f %3.4f } { %1.5f %1.5f } \r\n", x,y,z,s,t);
#ifndef GS_NO_NORMALS
			f.normal.x = q1Mdl::m_normals[mverts->lightnormalindex][0];
			f.normal.y = q1Mdl::m_normals[mverts->lightnormalindex][1];
			f.normal.z = q1Mdl::m_normals[mverts->lightnormalindex][2];
#endif
			memcpy(data, &f, sizeof(f));
			data += sizeof(f);
			j++;
			mverts++;
		} while (--count);
		last_vbo = this_vbo;
		//glEnd();
	}

	//fclose(fp);
}
#endif
/*
================
GL_MakeAliasModelDisplayLists
================
*/
/*
void GL_MakeAliasModelDisplayLists(model_t *m, aliashdr_t *hdr)
{
int		i, j;
maliasgroup_t	*paliasgroup;
int			*cmds;
trivertx_t	*verts;
char	cache[MAX_QPATH], fullpath[MAX_OSPATH], *c;
FILE	*f;
int		len;
byte	*data;

aliasmodel = m;
paliashdr = hdr;	// (aliashdr_t *)Mod_Extradata (m);

//
// look for a cached version
//
strcpy(cache, "glquake/");
COM_StripExtension(m->name + strlen("progs/"), cache + strlen("glquake/"));
strcat(cache, ".ms2");

COM_FOpenFile(cache, &f);
if (f)
{
fread(&numcommands, 4, 1, f);
fread(&numorder, 4, 1, f);
fread(&commands, numcommands * sizeof(commands[0]), 1, f);
fread(&vertexorder, numorder * sizeof(vertexorder[0]), 1, f);
fclose(f);
}
else
{
//
// build it from scratch
//
Con_Printf("meshing %s...\n", m->name);

BuildTris();		// trifans or lists

//
// save out the cached version
//
sprintf(fullpath, "%s/%s", com_gamedir, cache);
f = fopen(fullpath, "wb");
if (f)
{
fwrite(&numcommands, 4, 1, f);
fwrite(&numorder, 4, 1, f);
fwrite(&commands, numcommands * sizeof(commands[0]), 1, f);
fwrite(&vertexorder, numorder * sizeof(vertexorder[0]), 1, f);
fclose(f);
}
}


// save the data out

paliashdr->poseverts = numorder;

cmds = Hunk_Alloc(numcommands * 4);
paliashdr->commands = (byte *)cmds - (byte *)paliashdr;
memcpy(cmds, commands, numcommands * 4);

verts = Hunk_Alloc(paliashdr->numposes * paliashdr->poseverts
* sizeof(trivertx_t));
paliashdr->posedata = (byte *)verts - (byte *)paliashdr;
for (i = 0; i<paliashdr->numposes; i++)
for (j = 0; j<numorder; j++)
*verts++ = poseverts[i][vertexorder[j]];
}
*/

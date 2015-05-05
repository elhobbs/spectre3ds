#include "ViewState.h"
#include "Host.h"
#include "q1Sprite.h"
#include "q1Mdl.h"
#include <3ds.h>
#include <3ds/types.h>

#include "math3.h"
#include "gs.h"

#ifdef WIN32
#include <gl\gl.h>
#include <gl\glu.h>
#endif

extern u32* __linear_heap;
extern shaderProgram_s shader;
extern shaderProgram_s q1Bsp_shader;
extern shaderProgram_s q1Mdl_shader;
extern int shader_mode;

cvar_t	r_ambient = { "r_ambient", "0" };
cvar_t	r_fullbright = { "r_fullbright", "0" };

int mdl_mode = -1;
int bsp_mode = -1;
void ViewState::init() {
	m_particles.init();
	m_dlights.clear();
	gsVboInit(&m_vbo);
	gsVboCreate(&m_vbo, sizeof(faceVertex_s)*(4000) * 3);
	//make a special fullbright texture for no lightmap faces
	m_no_light_id = sys.gen_texture_id();
	byte *lightmap_data = new byte[8 * 8];
	memset(lightmap_data, 0xff, 8 * 8);
	sys.load_texture_L8(m_no_light_id, 8, 8, lightmap_data);
	delete[] lightmap_data;
	lightmap_data = 0;
	mdl_mode = sys.renderMode.register_mode(&q1Mdl::set_render_mode);
	bsp_mode = sys.renderMode.register_mode(&q1Bsp::set_render_mode);
}


void ViewState::split_entity_on_node(q1_bsp_node *node, entity_t *ent) {
	efrag_t				*ef;
	q1_plane			*splitplane;
	q1_leaf_node		*leaf;
	int					sides;

	if (node->m_contents == CONTENTS_SOLID)
	{
		return;
	}

	// add an efrag if the node is a leaf

	if (node->m_contents < 0)
	{
		if (!m_pefragtopnode)
			m_pefragtopnode = node;

		leaf = (q1_leaf_node *)node;

		// grab an efrag off the free list
		ef = m_free_efrags;
		if (!ef)
		{
			host.printf("Too many efrags!\n");
			return;		// no free fragments...
		}
		m_free_efrags = m_free_efrags->entnext;

		ef->entity = ent;

		// add the entity link	
		*m_lastlink = ef;
		m_lastlink = &ef->entnext;
		ef->entnext = 0;

		// set the leaf links
		ef->leaf = leaf;
		ef->leafnext = leaf->m_efrags;
		leaf->m_efrags = ef;

		return;
	}

	// NODE_MIXED

	splitplane = node->m_plane;
	sides = BOX_ON_PLANE_SIDE(m_emins, m_emaxs, splitplane);

	if (sides == 3)
	{
		// split on this plane
		// if this is the first splitter of this bmodel, remember it
		if (!m_pefragtopnode)
			m_pefragtopnode = node;
	}

	// recurse down the contacted sides
	if (sides & 1)
		split_entity_on_node(node->m_children[0], ent);

	if (sides & 2)
		split_entity_on_node(node->m_children[1], ent);
}

void ViewState::add_efrags(entity_t *ent) {
	Model		*entmodel;
	int			i;

	if (!ent->model)
		return;



	m_lastlink = &ent->m_efrag;
	m_pefragtopnode = 0;

	entmodel = ent->model;

	for (i = 0; i<3; i++)
	{
		m_emins[i] = ent->origin[i] + entmodel->m_bounds[0][i].c_float();
		m_emaxs[i] = ent->origin[i] + entmodel->m_bounds[0][i].c_float();
	}

	split_entity_on_node(m_worldmodel->m_nodes, ent);

	ent->m_topnode = m_pefragtopnode;
}

void ViewState::remove_efrags(entity_t *ent) {
	efrag_t		*ef, *old, *walk, **prev;

	ef = ent->m_efrag;

	while (ef)
	{
		prev = &ef->leaf->m_efrags;
		while (1)
		{
			walk = *prev;
			if (!walk)
				break;
			if (walk == ef)
			{	// remove this fragment
				*prev = ef->leafnext;
				break;
			}
			else
				prev = &walk->leafnext;
		}

		old = ef;
		ef = ef->entnext;

		// put it on the free list
		old->entnext = m_free_efrags;
		m_free_efrags = old;
	}

	ent->m_efrag = 0;
}

void ViewState::store_efrags(efrag_t *ppefrag)
{
	entity_t	*pent;
	Model		*clmodel;
	efrag_t		*pefrag;


	while ((pefrag = ppefrag) != NULL)
	{
		pent = pefrag->entity;
		clmodel = pent->model;

		switch (clmodel->type())
		{
		case mod_alias:
		case mod_brush:
		case mod_sprite:
			pent = pefrag->entity;

			if ((pent->visframe != m_framecount) &&
				(m_numvisedicts < MAX_VISEDICTS))
			{
				m_visedicts[m_numvisedicts++] = pent;

				pent->next = pent->model->m_ents;
				pent->model->m_ents = pent;
				// mark that we've recorded this entity for this frame
				pent->visframe = m_framecount;
			}

			ppefrag = pefrag->leafnext;
			break;

		default:
			sys.error("R_StoreEfrags: Bad entity type %d\n", clmodel->type());
		}
	}
}
void ViewState::add(entity_t *ent) {
	if (m_numvisedicts < MAX_VISEDICTS)
	{
		m_visedicts[m_numvisedicts] = ent;
		m_numvisedicts++;

		//ent->next = ent->model->m_ents;
		//ent->model->m_ents = ent;
	}
}

void DLights::render_impact(vec3_fixed32 origin, vec3_fixed32 impact, q1_plane &plane, fixed32p16 radius) {
	vec3_fixed32 start;
	vec3_fixed32 end;
	norm3_fixed32 &normal = plane.m_normal;
	fixed32p16 len(4.0f);
	vec3_fixed32 up(0.0f, 0.0f, 1.0f);
	vec3_fixed32 right(0.0f, 1.0f, 0.0f);

	vec3_t p0,p1;

	start = impact + normal.scale(len);
	end = impact - normal.scale(len);

	start.copy_to(p0);
	end.copy_to(p1);

#ifdef WIN32
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES);
	
	//normal
	glColor3f(1, 0, 0);
	glVertex3fv(p0);
	glVertex3fv(p1);

	//horizontal
	start = impact - right.scale(len);
	end = start + right.scale(len);
	start.copy_to(p0);
	end.copy_to(p1);

	glColor3f(1, 1, 0);
	glVertex3fv(p0);
	glVertex3fv(p1);

	//vertical
	start = impact - up.scale(len);
	end = start + up.scale(len);
	start.copy_to(p0);
	end.copy_to(p1);

	glColor3f(0, 0, 1);
	glVertex3fv(p0);
	glVertex3fv(p1);

	//impact to origin
	start = impact;
	end = origin;
	start.copy_to(p0);
	end.copy_to(p1);

	glColor3f(0, 1, 0);
	glVertex3fv(p0);
	glVertex3fv(p1);
	glEnd();
	glEnable(GL_TEXTURE_2D);
#endif
}

int gsVboDrawFan(gsVbo_s* vbo);
void GPU_DrawArrayDirectly(GPU_Primitive_t primitive, u8* data, u32 n);
int gsUpdateTransformation();

unsigned		blocklights[18 * 18];

void ViewState::render_face(q1_face *face, int framenum) {
	if (face->m_texinfo->m_flags & FACE_SKY) {
		m_draw_sky = true;
		return;
	}

	bool depth = false;
	q1_texture* tx = face->m_texinfo->m_miptex->animate(framenum);
	if (tx) {
		tx->bind();
	}
	
	if (m_fullbright || (face->m_texinfo->m_flags & 1) != 0 || face->m_lightmap_id == 0) {
		sys.bind_texture(m_no_light_id, 1);
	}
	else {
		int dynamic = 0;
		if (m_dlights.dynamic()) {
			if (m_dlights.light_face(face)) {
				dynamic = 2;
				face->m_cached_dlight = true;
			}
			else {
				for (int maps = 0; maps < MAXLIGHTMAPS && face->m_styles[maps] != 255; maps++) {
					if (face->m_cached_light[maps] != m_lightstylevalue[face->m_styles[maps]]) {
						dynamic = 1;
						break;
					}
				}
			}
			if (dynamic || face->m_cached_dlight) {
				face->build_lightmap(blocklights, m_lightstylevalue);
				m_dlights.light_face(face, blocklights);
				q1_lightmap *lm = face->m_lightmap_id;
				if (lm) {
					lm->update_block(blocklights, face->m_lightmap_rect, face->m_lightmap_rotated);
				}
				//hack make sure this is dynamic next frame
				face->m_cached_dlight = dynamic == 2 ? true : false;
			}
		}

		sys.bind_texture(face->m_lightmap_id->m_id, 1);
	}
	gsUpdateTransformation();
	GPU_DrawArrayDirectly(GPU_TRIANGLE_FAN, (u8 *)face->m_vertex_array, face->m_num_points);
	
	if (tx == 0 || tx->m_id2 == -1 || m_fullbright || (face->m_texinfo->m_flags & 1) != 0) {
		return;
	}
	//draw again with fullbright texture
	GPU_SetDepthTestAndWriteMask(true, GPU_EQUAL, GPU_WRITE_ALL);
	sys.bind_texture(tx->m_id2);
	sys.bind_texture(m_no_light_id, 1);
	GPU_DrawArrayDirectly(GPU_TRIANGLE_FAN, (u8 *)face->m_vertex_array, face->m_num_points);
	GPU_SetDepthTestAndWriteMask(true, GPU_GREATER, GPU_WRITE_ALL);
	//sys.bind_texture(tx->m_id);
	//face->bind();
	//gsVboDrawFan(&face->vbo);
#ifdef WIN32
	q1_texture* tx = face->m_texinfo->m_miptex->animate(framenum);
	glBindTexture(GL_TEXTURE_2D, tx->m_id);
	if (!m_fullbright) {
		if (face->m_lightmaps && (face->m_texinfo->m_flags & 1) == 0) {
			int light[64];
			int sext = (face->m_extents[0] >> 4) + 1;
			int text = (face->m_extents[1] >> 4) + 1;
			int size = sext * text;
			for (int i = 0; i < face->m_num_points; i++) {
				int colr = m_ambient;
				byte *lightmap = face->m_lightmaps;
				for (int maps = 0; maps < MAXLIGHTMAPS; maps++) {
					if (face->m_styles[maps] == 255) {
						break;
					}
					int scale = m_lightstylevalue[face->m_styles[maps]];	// 8.8 fraction		
					colr += lightmap[face->m_lightmap_ofs[i]] * scale;
					lightmap += size;
					//break;
				}
				light[i] = colr;
			}
			m_dlights.light_face(face, light);

			/*int r = (((unsigned int)face) >> 0) & 0xff;
			int g = (((unsigned int)face) >> 8) & 0xff;
			int b = (((unsigned int)face) >> 16) & 0xff;
			glColor3ub(r, g, b);*/

			glBegin(GL_POLYGON);
			for (int i = 0; i < face->m_num_points; i++) {
				float u = face->m_u[i].c_float();
				float v = face->m_v[i].c_float();
				int colr = light[i] >> 8;
				if (colr > 255) {
					colr = 255;
				}
				glColor3ub(colr, colr, colr);
				glTexCoord2f(u, v);
				glVertex3f(face->m_points[i][0].c_float(), face->m_points[i][1].c_float(), face->m_points[i][2].c_float());
			}
			glEnd();

			//if there is an alternate full bright texture then draw again
			if (tx->m_id2 == -1) {
				return;
			}
			glBindTexture(GL_TEXTURE_2D, tx->m_id2);
			glDepthFunc(GL_EQUAL);
			depth = true;
		}
	}

	glColor3f(0.78f, 0.78f, 0.78f);

	glBegin(GL_POLYGON);
	for (int i = 0; i < face->m_num_points; i++) {
		float u = face->m_u[i].c_float();
		float v = face->m_v[i].c_float();
		glTexCoord2f(u, v);
		glVertex3f(face->m_points[i][0].c_float(), face->m_points[i][1].c_float(), face->m_points[i][2].c_float());
	}
	glEnd();
	if (depth) {
		glDepthFunc(GL_LESS);
	}
#endif
}

void ViewState::render_leaf(q1_leaf_node *leaf) {
	q1_face *face;
	int num_faces = leaf->m_num_faces;
	for (int i = 0; i < num_faces; i++) {
		face = leaf->m_first_face[i];
		if (face->m_visframe != m_framecount) {
			face->m_visframe = m_framecount;
			fixed32p16 dot = *(face->m_plane) * m_camera;
			if ((face->side == 0 && dot < 0) || (face->side != 0 && dot > 0)) {
				continue;
			}
			render_face(face);
		}
	}
}

int ViewState::render_bsp_r(q1_bsp_node *node, unsigned int planebits) {
	if (node == 0) {
		return -1;
	}
	do {
		int nodenum = node - m_worldmodel->m_nodes;
		if (node->m_visframe != m_framecount) {
			return 0;
		}
		if (node->m_contents == CONTENTS_SOLID) {
			return 0;
		}
		if (node->m_contents < 0) {
			break;
		}

		planebits = m_frustum.cull(node->m_bounds, planebits);
		if (planebits == 0) {
			return 0;
		}

		render_bsp_r(node->m_children[0], planebits);

		node = node->m_children[1];

	} while (node);

	if (node && node->m_contents < 0) {
		q1_leaf_node *leaf = (q1_leaf_node *)node;
		if (leaf->m_visframe == m_framecount) {
			render_leaf(leaf);
			store_efrags(leaf->m_efrags);
		}
	}
	return 0;
}

void ViewState::render_world(float *origin, float *angles) {
	m_worldmodel->set_pos(origin);
	m_viewleaf = m_worldmodel->in_leaf();
	m_worldmodel->mark_visible_leafs(*m_viewleaf, m_framecount);

	sys.renderMode.set_mode(bsp_mode);
	render_bsp_r(&m_worldmodel->m_nodes[0], 15);
	sys.renderMode.set_mode(shader_mode);
	
	//update lightmap textures
	load_dirty_lightmaps(m_worldmodel);
}

void DLights::setup_lights() {
	DLight *dl = m_list;
	float cl_time = (float)host.cl_time();
#ifdef WIN32
	glEnable(GL_LIGHTING);
	for (int i = 0; i<8; i++, dl++) {
		if (dl->m_die < cl_time || dl->m_radius == 0) {
			glDisable(GL_LIGHT0 + i);
			continue;
		}
		GLfloat lightColor0[] = { 1.0f, 0.0f, 0.0f, 1.0f }; //Color (0.5, 0.5, 0.5)
		GLfloat lightPos0[] = { 4.0f, 0.0f, 8.0f, 1.0f }; //Positioned at (4, 0, 8)
		for (int j = 0; j < 3; j++) {
			lightPos0[j] = dl->m_origin[j].c_float();
		}
		glLightfv(GL_LIGHT0+i, GL_DIFFUSE, lightColor0);
		glLightfv(GL_LIGHT0+i, GL_POSITION, lightPos0);
		glEnable(GL_LIGHT0+i);
	}
#endif
}

void skybox_render(float *org, int sky);

extern "C" void con_printf(int col, int row, char *format, ...);
void parseTile8(u8 *tile, byte *image, int width, int height, int x, int y);

void update_lm(q1_lightmap *lm) {
	if (lm == 0 || lm->m_data == 0) {
		return;
	}
	tex3ds_t *tex = (tex3ds_t*)(lm->m_id);
	byte *tile = tex->data;
	byte* data = lm->m_data;
	int width = lm->m_width;
	int height = lm->m_height;
	for (int j = 0; j < height; j += 8) {
		byte *map = &lm->m_dirty_map[(j >> 3) * 8];
		for (int i = 0; i < width; i += 8) {
			if (map[(i >> 3)]) {
				map[(i >> 3)] = 0;
				parseTile8(tile, data, width, height, i, j);
			}
			tile += (8 * 8);
		}
	}
	GSPGPU_FlushDataCache(0, tex->data, width * height);
}

void ViewState::load_dirty_lightmaps(q1Bsp *bsp) {
	if(bsp == 0) {
		return;
	}
	//printf("load_dirty_lightmaps\n");
	for (q1_lightmap *lm = bsp->m_lightmaps; lm; lm = lm->m_next) {
		if (!lm->m_dirty) {
			continue;
		}
		//update_lm(lm);
		sys.load_texture_L8(lm->m_id, lm->m_width, lm->m_height, lm->m_data);
		lm->m_dirty = 0;
	}

}

float CalcFov(float fov_x, float width, float height);

void ViewState::render(float *origin, float *angles) {

	vec3_t forward, right, up;

	gsVboClear(&m_vbo);
	m_camera = origin;
	AngleVectors(angles, forward, right, up);
	m_forward = forward;
	m_right = right;
	m_up = up;

	m_frustum.transform(origin, forward, right, up, 80.0f, CalcFov(80.0f,400.0f,240.0f));

	m_draw_sky = false;

	m_ambient = (int)r_ambient.value;
	m_ambient <<= 8;
	if (m_ambient < 0) {
		m_ambient = 0;
	}
	m_fullbright = (int)r_fullbright.value;

#ifdef WIN32
	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	glRotatef(90.0f, 0.0f, 0.0f, 1.0f);
	glPushMatrix();
	glRotatef(-angles[ROLL], 1.0f, 0.0f, 0.0f);
	glRotatef(-angles[PITCH], 0.0f, 1.0f, 0.0f);
	glRotatef(-angles[YAW], 0.0f, 0.0f, 1.0f);
	glTranslatef(-origin[0], -origin[1], -origin[2]);

	float mview[4][4];
	glGetFloatv(GL_MODELVIEW_MATRIX, &mview[0][0]);

	//glDisable(GL_LIGHTING);
	render_world(origin, angles);
	//m_dlights.setup_lights();

	for (int i = 0; i < m_numvisedicts; i++) {
		entity_t *ent = m_visedicts[i];
		if (ent->model == 0) {
			host.printf("error: null entity model\n");
			continue;
		}

		//host.printf("%s %d\n", ent->model->name(), ent->frame);

		glPushMatrix();
		
		render(ent);

		glPopMatrix();
	}

	render_particles();

	m_dlights.decay_lights();
	
	if (m_draw_sky) {
		int sky = m_worldmodel->m_skytexture_id;
		skybox_render(origin, sky);
	}
	glPopMatrix();
#endif

	//host.printf("o: %4.2f %4.2f %4.2f\n", origin[0], origin[1], origin[2]);
	//host.printf("a: %3.2f %3.2f %3.2f\n", angles[0], angles[1], angles[2]);

#if 1
	//draw world
	gsMatrixMode(GS_MODELVIEW);
	gsLoadIdentity();
		gsRotateY(-90.0f*M_PI / 180.0f);
		gsRotateX(90.0f*M_PI / 180.0f);
		gsRotateZ(180.0f*M_PI / 180.0f);

	//push here to save rotated orientation for view ent
	gsPushMatrix();
		gsRotateX((-angles[ROLL]) * M_PI / 180.0f);
		gsRotateY((-angles[PITCH]) * M_PI / 180.0f);
		gsRotateZ((angles[YAW]) * M_PI / 180.0f);
		gsTranslate(-origin[0], -origin[1], -origin[2]);
		//printf("render_world+\n");
		render_world(origin, angles);
		//printf("render_world-\n");

		sys.renderMode.set_mode(bsp_mode);
		for (int i = 0; i < m_numvisedicts; i++) {
			entity_t *ent = m_visedicts[i];
			if (ent->model == 0) {
				host.printf("error: null entity model\n");
				continue;
			}
			if (ent->model->type() != mod_brush) {
				continue;
			}
			gsPushMatrix();
			render(ent);
			gsPopMatrix();
		}

		sys.renderMode.set_mode(mdl_mode);
		for (int i = 0; i < m_numvisedicts; i++) {
			entity_t *ent = m_visedicts[i];
			if (ent->model == 0) {
				host.printf("error: null entity model\n");
				continue;
			}
			if (ent->model->type() != mod_alias) {
				continue;
			}
			gsPushMatrix();
			render(ent);
			gsPopMatrix();
		}
		
		sys.renderMode.set_mode(shader_mode);
		for (int i = 0; i < m_numvisedicts; i++) {
			entity_t *ent = m_visedicts[i];
			if (ent->model == 0) {
				host.printf("error: null entity model\n");
				continue;
		}
			if (ent->model->type() != mod_sprite) {
				continue;
			}
			gsPushMatrix();
			render(ent);
			gsPopMatrix();
		}
		gsPopMatrix();

	//printf("  %d\n", m_framecount);
#else
	render_world(origin, angles);
#endif
	m_framecount++;
}

void ViewState::render_viewent() {
#ifdef WIN32
	glDepthRange(0.0, 0.3);
	glTranslatef(m_viewent->origin[0], m_viewent->origin[1], m_viewent->origin[2]);

	glRotatef(m_viewent->angles[1], 0, 0, 1);
	glRotatef(m_viewent->angles[0], 0, 1, 0);
	glRotatef(m_viewent->angles[2], 1, 0, 0);
	
	render(m_viewent);

	glDepthRange(0.0, 1.0);
#else
	sys.renderMode.set_mode(mdl_mode);

	gsPushMatrix();
	//GPU_DepthRange(-0.3f, 0.0f);
	gsTranslate(m_viewent->origin[0], m_viewent->origin[1], m_viewent->origin[2]);
	//gsScale(0.5f, 0.0f, 0.0f);

	gsRotateZ(m_viewent->angles[1] * M_PI / 180.0f);
	gsRotateY(m_viewent->angles[0] * M_PI / 180.0f);
	gsRotateX(m_viewent->angles[2] * M_PI / 180.0f);
	
	//GPU_SetDepthTestAndWriteMask(true, GPU_ALWAYS, GPU_WRITE_ALL);
	//GPU_DepthMap(-1.0f, -0.1f);
	//render(m_viewent);
	//GPU_SetDepthTestAndWriteMask(true, GPU_GEQUAL, GPU_WRITE_ALL);
	GPU_DepthMap(-0.5f, 0.5f);
	render(m_viewent);
	GPU_DepthMap(-1.0f, 0.0f);
	//GPU_SetDepthTestAndWriteMask(true, GPU_GREATER, GPU_WRITE_ALL);
	gsPopMatrix();

#endif
}

void ViewState::render(entity_t *ent) {
	switch (ent->model->type()) {
	case mod_sprite:
		//glDisable(GL_LIGHTING);
		render_sprite(ent);
		break;
	case mod_alias:
		//glEnable(GL_LIGHTING);
		render_mdl(ent);
		break;
	case mod_brush:
		//glDisable(GL_LIGHTING);
		render_bsp(ent);
		break;
	default:
		host.printf("invalid model type %d\n", ent->model->type());
		break;
	}
}

void ViewState::render_sprite(entity_t *ent) {
	if (ent == 0 || ent->model == 0) {
		return;
	}
	q1Sprite *sprite = reinterpret_cast<q1Sprite *>(ent->model);
	q1SpriteFrame *frame = sprite->get_frame(ent->frame);
	if (frame == 0) {
		return;
	}

#ifdef WIN32
	glTranslatef(ent->origin[0], ent->origin[1], ent->origin[2]);

	vec3_t point;

	int light = light_point(ent->origin);

	glColor3ub(light, light, light);
	glBindTexture(GL_TEXTURE_2D, frame->m_id);
	glBegin(GL_QUADS);

	glTexCoord2f(0, 1);

	(m_up * frame->down + m_right * frame->left).copy_to(point);
	glVertex3fv(point);

	glTexCoord2f(0, 0);
	(m_up * frame->up + m_right * frame->left).copy_to(point);
	glVertex3fv(point);

	glTexCoord2f(1, 0);
	(m_up * frame->up + m_right * frame->right).copy_to(point);
	glVertex3fv(point);

	glTexCoord2f(1, 1);
	(m_up * frame->down + m_right * frame->right).copy_to(point);
	glVertex3fv(point);

	glEnd();
#else
	faceVertex_s f;
	void *p = gsVboGetOffset(&m_vbo);

	sys.bind_texture(frame->m_id);
	gsTranslate(ent->origin[0], ent->origin[1], ent->origin[2]);

#ifndef GS_NO_NORMALS
	m_forward.copy_to(&f.normal.x);
#endif

	//first tri
	(m_up * frame->down + m_right * frame->left).copy_to(&f.position.x);
	f.texcoord[0] = 0.0f;
	f.texcoord[1] = 1.0f;
	if (gsVboAddData(&m_vbo, &f, sizeof(faceVertex_s), 1)) {
		return;
	}

	(m_up * frame->up + m_right * frame->left).copy_to(&f.position.x);
	f.texcoord[0] = 0.0f;
	f.texcoord[1] = 0.0f;
	if (gsVboAddData(&m_vbo, &f, sizeof(faceVertex_s), 1)) {
		return;
	}

	(m_up * frame->up + m_right * frame->right).copy_to(&f.position.x);
	f.texcoord[0] = 1.0f;
	f.texcoord[1] = 0.0f;
	if (gsVboAddData(&m_vbo, &f, sizeof(faceVertex_s), 1)) {
		return;
	}

	//second tri
	(m_up * frame->down + m_right * frame->left).copy_to(&f.position.x);
	f.texcoord[0] = 0.0f;
	f.texcoord[1] = 1.0f;
	if (gsVboAddData(&m_vbo, &f, sizeof(faceVertex_s), 1)) {
		return;
	}

	(m_up * frame->up + m_right * frame->right).copy_to(&f.position.x);
	f.texcoord[0] = 1.0f;
	f.texcoord[1] = 0.0f;
	if (gsVboAddData(&m_vbo, &f, sizeof(faceVertex_s), 1)) {
		return;
	}

	(m_up * frame->down + m_right * frame->right).copy_to(&f.position.x);
	f.texcoord[0] = 1.0f;
	f.texcoord[1] = 1.0f;
	if (gsVboAddData(&m_vbo, &f, sizeof(faceVertex_s), 1)) {
		return;
	}

	int gsUpdateTransformation();
	void GPU_DrawArrayDirectly(GPU_Primitive_t primitive, u8* data, u32 n);

	gsUpdateTransformation();
	GPU_DrawArrayDirectly(GPU_TRIANGLES, (u8*)p, 6);

#endif
}

void draw_mdl_frame(q1MdlFrame *frame);
int gsUpdateTransformation();

void ViewState::render_mdl(entity_t *ent) {
	if (ent == 0 || ent->model == 0) {
		return;
	}
	q1Mdl *mdl = reinterpret_cast<q1Mdl *>(ent->model);
	q1MdlFrame *frame = mdl->get_frame(ent->frame);
	if (frame == 0) {
		return;
	}

	int light;
	if (mdl->m_fullbright) {
		light = 200;
	}
	else {
		if (ent == m_viewent) {
			vec3_t org;

			m_camera.copy_to(org);
			light = light_point(org);
			if (light < 32) {
				light = 32;
			}
		}
		else {
			light = light_point(ent->origin);
		}
	}

	gsTranslate(ent->origin[0], ent->origin[1], ent->origin[2]);
	gsRotateZ((360.0f-ent->angles[1]) * M_PI / 180.0f);
	gsRotateY(-ent->angles[0] * M_PI / 180.0f);
	gsRotateX(ent->angles[2] * M_PI / 180.0f);

	vec3_t scale, scale_origin;
	mdl->scale_origin().copy_to(scale_origin);
	mdl->scale().copy_to(scale);
	gsTranslate(scale_origin[0],scale_origin[1],scale_origin[2]);
	gsScale(scale[0],scale[1],scale[2]);
	sys.bind_texture(mdl->get_skin(ent->skinnum));
	gsUpdateTransformation();
	draw_mdl_frame(frame);
	//gsVboDraw(&frame->m_vbo);
#ifdef WIN32
	glTranslatef(ent->origin[0], ent->origin[1], ent->origin[2]);
	glRotatef(ent->angles[1], 0, 0, 1);
	glRotatef(-ent->angles[0], 0, 1, 0);
	glRotatef(ent->angles[2], 1, 0, 0);
	
	vec3_t scale, scale_origin;
	mdl->scale_origin().copy_to(scale_origin);
	mdl->scale().copy_to(scale);
	glTranslatef(scale_origin[0],scale_origin[1],scale_origin[2]);
	glScalef(scale[0],scale[1],scale[2]);

	glBindTexture(GL_TEXTURE_2D, mdl->get_skin(ent->skinnum));
	glColor3ub(light, light, light);
	glBegin(GL_TRIANGLES);
	int num_tris = mdl->num_tris();
	vec5_t tri[3];
	vec3_t norm[3];
	for (int i = 0; i < num_tris; i++) {
		mdl->get_triangle(frame, i, tri[0],norm[0]);
		for (int j = 0; j < 3; j++) {
			glNormal3fv(norm[j]);
			glTexCoord2fv(&tri[j][3]);
			glVertex3fv(tri[j]);
		}
	}

	glEnd();
#endif
}

void ViewState::render_bsp(entity_t *ent) {
	if (ent == 0 || ent->model == 0) {
		return;
	}
	q1Bsp *bsp = reinterpret_cast<q1Bsp *>(ent->model);

	m_dlights.mark_lights(bsp->m_nodes + bsp->m_hulls[0].firstclipnode);

#ifdef WIN32
	glTranslatef(ent->origin[0], ent->origin[1], ent->origin[2]);
	glRotatef(ent->angles[1], 0, 0, 1);
	glRotatef(ent->angles[0], 0, 1, 0);
	glRotatef(ent->angles[2], 1, 0, 0);
#endif
	//texturing stuff

	//sys.renderMode.set_mode(bsp_mode);

	gsTranslate(ent->origin[0], ent->origin[1], ent->origin[2]);
	gsRotateZ(ent->angles[1] * M_PI / 180.0f);
	gsRotateY(ent->angles[0] * M_PI / 180.0f);
	gsRotateX(ent->angles[2] * M_PI / 180.0f);

	q1_face *face = bsp->get_face_list();
	int num_faces = bsp->get_face_count();
	for (int i = 0; i < num_faces; i++, face++) {
		render_face(face,ent->frame);
	}

	//sys.renderMode.set_mode(shader_mode);

	//update lightmap textures
	load_dirty_lightmaps(bsp);
}

void ViewState::animate_lights(lightstyle_t *lights) {
	int			i, j, k;
	float cl_time = (float)host.cl_time();

	//
	// light animations
	// 'm' is normal light, 'a' is no light, 'z' is double bright
	i = (int)(cl_time * 10);
	for (j = 0; j<MAX_LIGHTSTYLES; j++)
	{
		if (!lights[j].length)
		{
			m_lightstylevalue[j] = 256;
			continue;
		}
		k = i % lights[j].length;
		k = lights[j].map[k] - 'a';
		k = k * 22;
		m_lightstylevalue[j] = k;
	}
}

int ViewState::light_point_r(q1_bsp_node *node, fixed32p16 f0, fixed32p16 f1, vec3_fixed32 &p0, vec3_fixed32 &p1) {

	static const fixed32p16 offset(0.001f);
	static const fixed32p16 one(1.0f);
	fixed32p16 d0;
	fixed32p16 d1;
	int side = 0;

	do {

		//in a leaf
		if (node->m_contents < 0) {
			if (node->m_contents == CONTENTS_SOLID) {
				return -1;
			}
			return 0;
		}

		//node = &hull->clipnodes[nodenum];

		d0 = *(node->m_plane) * p0;
		d1 = *(node->m_plane) * p1;

		//both in front
		if (d0 >= 0 && d1 >= 0) {
			side = 0;
			node = node->m_children[0];
			continue;
		}

		//both in back
		if (d0 < 0 && d1 < 0) {
			side = 1;
			node = node->m_children[1];
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

	ret = light_point_r(node->m_children[side], f0, midf0, p0, midp0);
	if (ret != 0) {
		return ret;
	}

	ret = light_point_r(node->m_children[side ^ 1], midf0, f1, midp0, p1);
	if (ret != -1) {
		return ret;
	}

	//check all faces on node
	int num = node->m_num_faces;
	q1_face *face = node->m_first_face;
	for (int i = 0; i < num; i++, face++) {
		if (face->m_lightmaps == 0 || (face->m_texinfo->m_flags & 1) == 1)
		{
			continue;
		}
		tex3_fixed16 &u_vec = face->m_texinfo->m_vecs[0];
		tex3_fixed16 &v_vec = face->m_texinfo->m_vecs[1];
		fixed32p16 u_ofs(face->m_texinfo->m_ofs[0]);
		fixed32p16 v_ofs(face->m_texinfo->m_ofs[1]);
		int sext = (face->m_extents[0] >> 4) + 1;
		int text = (face->m_extents[1] >> 4) + 1;
		int size = sext * text;
		int smin = face->m_texturemins[0];
		int tmin = face->m_texturemins[1];
		int uu = (midp0.dot(u_vec) + u_ofs).c_int();
		int vv = (midp0.dot(v_vec) + v_ofs).c_int();

		int x = (uu - smin) / 16;
		int y = (vv - tmin) / 16;
		if (x < 0 || y < 0 || x > sext || y > text)
		{
			continue;
		}
		int colr = m_ambient;
		byte *lightmap = face->m_lightmaps;
		for (int maps = 0; maps < MAXLIGHTMAPS; maps++) {
			if (face->m_styles[maps] == 255) {
				break;
			}
			int scale = m_lightstylevalue[face->m_styles[maps]];	// 8.8 fraction		
			colr += lightmap[face->m_lightmap_ofs[i]] * scale;
			lightmap += size;
			//break;
		}
		return colr>>8;
	}

	return -2;
}


int ViewState::light_point(vec3_t p) {
	vec3_fixed32 p0,p1;
	fixed32p16 f0(0.0f);
	fixed32p16 f1(1.0f);

	p0 = p;
	p1 = p0 - vec3_fixed32(0, 0, 2048.0f);

	int ret = light_point_r(m_worldmodel->m_nodes, f0, f1, p0, p1);
	if (ret < 0) {
		ret = 0;
	}
	ret += m_dlights.light_point(p0);
	if (ret > 255) {
		ret = 255;
	}

	return ret;
}


void ViewState::render_particles() {

	Particle *kill;
	float cl_time = (float)host.cl_time();
	float frame_time = (float)host.frame_time();
	float time3 = frame_time * 15;
	float time2 = frame_time * 10; // 15;
	float time1 = frame_time * 5;
	float grav = (float)frame_time * sv_gravity.value * 0.05f;
	float dvel = 4 * frame_time;


	for (;;)
	{
		kill = m_particles.m_active_particles;
		if (kill && kill->die < cl_time)
		{
			m_particles.m_active_particles = kill->next;
			kill->next = m_particles.m_free_particles;
			m_particles.m_free_particles = kill;
			continue;
		}
		break;
	}

#ifdef WIN32
	glBindTexture(GL_TEXTURE_2D, m_particles.m_texture_id);
	glBegin(GL_TRIANGLES);
	for (Particle *p = m_particles.m_active_particles; p; p = p->next) {
		for (;;) {
			kill = p->next;
			if (kill && kill->die < cl_time)
			{
				p->next = kill->next;
				kill->next = m_particles.m_free_particles;
				m_particles.m_free_particles = kill;
				continue;
			}
			break;
		}

		int scale = 1;

		vec3_t v;
		extern unsigned char *q1_palette;
		byte *c = &q1_palette[p->color * 3];

		glColor3ubv(c);

		p->org.copy_to(v);
		glTexCoord2f(0, 0);
		glVertex3f(v[0], v[1], v[2]);

		(p->org + m_up).copy_to(v);
		glTexCoord2f(1, 0);
		glVertex3f(v[0], v[1], v[2]);

		(p->org + m_right).copy_to(v);
		glTexCoord2f(0, 1);
		glVertex3f(v[0], v[1], v[2]);

		p->org += p->vel * frame_time;

		switch (p->type) {
		case pt_static:
			break;
		case pt_fire:
			p->ramp += time1;
			if (p->ramp >= 6)
				p->die = -1;
			else
				p->color = Particles::ramp3[(int)p->ramp];
			p->vel[2] += grav;
			break;

		case pt_explode:
			p->ramp += time2;
			if (p->ramp >= 8)
				p->die = -1;
			else
				p->color = Particles::ramp1[(int)p->ramp];
			p->vel += p->vel * dvel;
			p->vel[2] -= grav;
			break;

		case pt_explode2:
			p->ramp += time3;
			if (p->ramp >= 8)
				p->die = -1;
			else
				p->color = Particles::ramp2[(int)p->ramp];
			p->vel -= p->vel * frame_time;
			p->vel[2] -= grav;
			break;

		case pt_blob:
			p->vel += p->vel * dvel;
			p->vel[2] -= grav;
			break;

		case pt_blob2:
			p->vel -= p->vel * dvel;
			p->vel[2] -= grav;
			break;

		case pt_grav:
		case pt_slowgrav:
			p->vel[2] -= grav;
			break;
		}

	}
	glEnd();
	glColor3f(1, 1, 1);
#endif
}

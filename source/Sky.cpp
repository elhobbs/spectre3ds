#include "ClientState.h"
#include "protocol.h"
#include "Host.h"
#include <stdio.h>
#ifdef WIN32
#include <gl\gl.h>
#include <gl\glu.h>
#endif
#include "gs.h"

vec3_t skybox_vecs[6][2] = {
		{ { 1, 0, 0 }, { 0, 1, 0 } },
		{ { 0, 1, 0 }, { 0, 0, 1 } },
		{ { 0, 0, 1 }, { 1, 0, 0 } },
		{ { -1, 0, 0 }, { 0, -1, 0 } },
		{ { 0, -1, 0 }, { 0, 0, -1 } },
		{ { 0, 0, -1 }, { -1, 0, 0 } }
};
#define SKYBOX_SIZE 8000
#define SKYBOX_DIV 8 //must be even

vec3_t skybox_corners[6] = {
		{ -SKYBOX_SIZE / 2, -SKYBOX_SIZE / 2, SKYBOX_SIZE / 2 },
		{ SKYBOX_SIZE / 2, -SKYBOX_SIZE / 2, -SKYBOX_SIZE / 2 },
		{ -SKYBOX_SIZE / 2, SKYBOX_SIZE / 2, -SKYBOX_SIZE / 2 },
		{ SKYBOX_SIZE / 2, SKYBOX_SIZE / 2, -SKYBOX_SIZE / 2 },
		{ -SKYBOX_SIZE / 2, SKYBOX_SIZE / 2, SKYBOX_SIZE / 2 },
		{ SKYBOX_SIZE / 2, -SKYBOX_SIZE / 2, SKYBOX_SIZE / 2 }
};

int skybox_verts_num = 0;
vec5_t skybox_verts[6 * SKYBOX_DIV * SKYBOX_DIV * 4];
gsVbo_s skybox_vbo;

void skybox_coord(float *org, float *p) {
	vec3_t dir;
	float len;

	VectorSubtract(p, org, dir);
	dir[2] *= 3;

	len = Length(dir);
	VectorScale(dir, 3.0f / len, dir);

	float *v = skybox_verts[skybox_verts_num++];
	VectorCopy(p, v);
	v[3] = dir[0];
	v[4] = dir[1];
}

inline void vec_copy(float *a, faceVertex_s *b)
{
	VectorCopy(&b->position.x, a);
	b->texcoord[0] = a[3];
	b->texcoord[1] = a[4];
}

void skybox_build(float *org) {
	for (int i = 0; i < 6; i++) {
		vec3_t p;
		for (int j = 0; j < SKYBOX_DIV; j++) {
			VectorMA(skybox_corners[i], j * SKYBOX_SIZE / SKYBOX_DIV, skybox_vecs[i][0], p);
			for (int k = 0; k < SKYBOX_DIV; k++) {
				skybox_coord(org, p);

				VectorMA(p, SKYBOX_SIZE / SKYBOX_DIV, skybox_vecs[i][0], p);
				skybox_coord(org, p);

				VectorMA(p, SKYBOX_SIZE / SKYBOX_DIV, skybox_vecs[i][1], p);
				skybox_coord(org, p);

				VectorMA(p, -SKYBOX_SIZE / SKYBOX_DIV, skybox_vecs[i][0], p);
				skybox_coord(org, p);
			}
		}
	}
	gsVboInit(&skybox_vbo);
	gsVboCreate(&skybox_vbo, sizeof(faceVertex_s) * 6 * SKYBOX_DIV * SKYBOX_DIV * 6);
	for (int i = 0; i < skybox_verts_num; i += 4) {
		gsVboAddData(&skybox_vbo, skybox_verts[i + 0], sizeof(faceVertex_s), 1);
		gsVboAddData(&skybox_vbo, skybox_verts[i + 1], sizeof(faceVertex_s), 1);
		gsVboAddData(&skybox_vbo, skybox_verts[i + 2], sizeof(faceVertex_s), 1);
		gsVboAddData(&skybox_vbo, skybox_verts[i + 0], sizeof(faceVertex_s), 1);
		gsVboAddData(&skybox_vbo, skybox_verts[i + 2], sizeof(faceVertex_s), 1);
		gsVboAddData(&skybox_vbo, skybox_verts[i + 3], sizeof(faceVertex_s), 1);
	}
}

int gsUpdateTransformation();
void GPU_DrawArrayDirectly(GPU_Primitive_t primitive, u8* data, u32 n);

void skybox_render(float *org, int sky) {

#ifdef WIN32
	glTranslatef(org[0], org[1], org[2]);
	glDisable(GL_CULL_FACE);
	glBindTexture(GL_TEXTURE_2D, sky);
	glBegin(GL_QUADS);
	for (int i = 0; i < skybox_verts_num; i++) {
		glTexCoord2f(skybox_verts[i][3], skybox_verts[i][4]);
		glVertex3fv(skybox_verts[i]);
	}
	glEnd();
	glEnable(GL_CULL_FACE);
#else

	void *p = (void *)skybox_vbo.data;

	sys.bind_texture(sky);
	gsTranslate(org[0], org[1], org[2]);

	gsUpdateTransformation();
	GPU_DrawArrayDirectly(GPU_TRIANGLES, (u8*)p, 6 * SKYBOX_DIV * SKYBOX_DIV * 6);
#endif
}

#pragma once

#include <3ds/types.h>

typedef struct
{
	u8* data;
	u32 currentSize; // in bytes
	u32 maxSize; // in bytes
	u32 numVertices;
	u32* commands;
	u32 commandsSize;
} ctrVbo_t;

typedef struct  {
	float x, y, z;
} vect3Df_s;

typedef struct
{
	vect3Df_s position;
	float texcoord[2];
}faceVertex_s;

typedef struct
{
	vect3Df_s position;
	float texcoord[2];
	float lightmap[2];
}bspVertex_t;

typedef struct
{
	unsigned char position[4];
	float texcoord[2];
}mdlVertex_s;

int ctrVboInit(ctrVbo_t* vbo);
int ctrVboCreate(ctrVbo_t* vbo, u32 size);
int ctrVboClear(ctrVbo_t* vbo);
void* ctrVboGetOffset(ctrVbo_t* vbo);
int ctrVboAddData(ctrVbo_t* vbo, void* data, u32 size, u32 units);

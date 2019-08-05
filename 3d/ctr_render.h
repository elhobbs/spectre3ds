#pragma once
#include <citro3d.h>
#include "ctr_vbo.h"

void ctrRenderInit();

void ctrPerspectiveProjection(float fovy, float aspect, float near, float far, float iod, float screen);
void ctrPush2d();
void ctrPop2d();

void ctrLoadIdentity();
void ctrPushMatrix();
void ctrPopMatrix();

void ctrTranslate(float x, float y, float z);
void ctrRotateX(float x);
void ctrRotateY(float y);
void ctrRotateZ(float z);
void ctrScale(float x, float y, float z);

void ctrBindShader(shaderProgram_s* shader);
shaderProgram_s* ctrCurrentShader();
void ctrUpdateTransformation();

void ctrSetTex(GPU_TEXUNIT unit,GPU_TEXCOLOR type,int w,int h,void* data);
void ctrDrawArrayDirectly(GPU_Primitive_t primitive, u8* data, u32 n);
int ctrVboDrawDirectly(ctrVbo_t* vbo);

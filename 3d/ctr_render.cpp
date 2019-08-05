#include "ctr_render.h"
#include "ctr_vbo.h"
#include <c3d/mtxstack.h>
#include <3ds/gpu/gpu.h>
#include <stdio.h>

static C3D_MtxStack modelViewStack;
static C3D_MtxStack projectionStack;
static C3D_Mtx *modelView;
static C3D_Mtx *projection;
static int matrix_init = 1;
static shaderProgram_s* current_shader;

void ctrRenderInit() {
	MtxStack_Init(&modelViewStack);
	modelView = MtxStack_Cur(&modelViewStack);
	
	MtxStack_Init(&projectionStack);
	projection = MtxStack_Cur(&projectionStack);

	current_shader = 0;
}

shaderProgram_s* ctrCurrentShader() {
	return current_shader;
}

void ctrBindShader(shaderProgram_s* shader) {
	C3D_BindProgram(shader);
	s8 proj = shaderInstanceGetUniformLocation(shader->vertexShader, "projection");
	s8 model = shaderInstanceGetUniformLocation(shader->vertexShader, "modelView");

	MtxStack_Bind(&modelViewStack, GPU_VERTEX_SHADER, model, 4);
	MtxStack_Bind(&projectionStack, GPU_VERTEX_SHADER, proj, 4);

	current_shader = shader;
}

void ctrPerspectiveProjection(float fovy, float aspect, float near, float far, float iod, float screen) {
	C3D_Mtx* projection = MtxStack_Cur(&projectionStack);
	Mtx_PerspStereoTilt(projection, fovy, aspect, near, far, iod, screen, false);
}

void ctrPush2d() {
	//printf("ctrPush2d\n");
	C3D_Mtx* modelView = MtxStack_Push(&modelViewStack);
	Mtx_Identity(modelView);

	C3D_Mtx* projection = MtxStack_Push(&projectionStack);
	Mtx_OrthoTilt(projection, 0.0f, 400.0f, 0.0f, 240.0f, 1.0f, -1.0f, false);

	C3D_DepthTest(true, GPU_ALWAYS, GPU_WRITE_ALL);
	C3D_CullFace(GPU_CULL_NONE);
}

void ctrPop2d() {
	//printf("ctrPop2d\n");
	MtxStack_Pop(&modelViewStack);
	MtxStack_Pop(&projectionStack);

	C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_ALL);
}

void ctrLoadIdentity() {
	C3D_Mtx* modelView = MtxStack_Cur(&modelViewStack);
	Mtx_Identity(modelView);
}

void ctrPushMatrix() {
	MtxStack_Push(&modelViewStack);
}

void ctrPopMatrix() {
	MtxStack_Pop(&modelViewStack);
}

void ctrTranslate(float x, float y, float z) {
	C3D_Mtx* modelView = MtxStack_Cur(&modelViewStack);
	Mtx_Translate(modelView, x, y, z, true);
}

void ctrRotateX(float x) {
	C3D_Mtx* modelView = MtxStack_Cur(&modelViewStack);
	Mtx_RotateX(modelView, x, true);
}

void ctrRotateY(float y) {
	C3D_Mtx* modelView = MtxStack_Cur(&modelViewStack);
	Mtx_RotateY(modelView, y, true);
}

void ctrRotateZ(float z) {
	C3D_Mtx* modelView = MtxStack_Cur(&modelViewStack);
	Mtx_RotateZ(modelView, z, true);
}

void ctrScale(float x, float y, float z) {
	C3D_Mtx* modelView = MtxStack_Cur(&modelViewStack);
	Mtx_Scale(modelView, x, y, z);
}

void ctrUpdateTransformation() {
	//printf("ctrUpdateTransformation\n");
	//printf("projection %p\n", projection);
	//for (int i = 0; i < 16; i++) {
	//	printf("%.3f ", projection->m[i]);
	//}
	//printf("\n");
	MtxStack_Update(&projectionStack);
	MtxStack_Update(&modelViewStack);
}

void ctrDrawArrayDirectly(GPU_Primitive_t primitive, u8* data, u32 n) {
	//printf("ctrDrawArrayDirectly\n");
	if (!data || !n) {
		return;
	}
	//move base to start
	C3D_BufInfo*info = C3D_GetBufInfo();
	u32 pa = osConvertVirtToPhys(data);
	if (pa < info->base_paddr) {
		//printf("ctrDrawArrayDirectly: %p %d\n", data, n);
		return;
	}
	C3D_BufCfg* buf = &info->buffers[0];
	buf->offset = pa - info->base_paddr;
	C3D_SetBufInfo(info);
	//draw
	C3D_DrawArrays(primitive, 0, n);
	//printf("ctrDrawArrayDirectly: %p %d\n", data, n);
}

int ctrVboDrawDirectly(ctrVbo_t* vbo) {
	if (!vbo || !vbo->data || !vbo->currentSize || !vbo->maxSize) {
		//printf("vbo: %p %p %d %d\n", vbo, (vbo ? vbo->data: 0), (vbo ? vbo->currentSize : 0), (vbo ? vbo->maxSize : 0));
		return -1;
	}

	ctrUpdateTransformation();

	//printf("ctrVboDrawDirectly: %p %d\n", vbo->data, vbo->numVertices);

	ctrDrawArrayDirectly(GPU_TRIANGLES, vbo->data, vbo->numVertices);

	return 0;
}
void ctrSetTex(
	GPU_TEXUNIT unit,
	GPU_TEXCOLOR type,
	int w,
	int h,
	void* data) {
	u32 reg[5];
	reg[0] = 0;
	reg[1] = (w | (h << 16));
	reg[2] = 2;
	reg[3] = 0;
	reg[4] = osConvertVirtToPhys(data) >> 3;

	int tunit = GPU_TEXUNIT0;

	unit = (GPU_TEXUNIT)0;

	switch (unit)
	{
	case 0:
		GPUCMD_AddIncrementalWrites(GPUREG_TEXUNIT0_BORDER_COLOR, reg, 5);
		GPUCMD_AddWrite(GPUREG_TEXUNIT0_TYPE, type);
		break;
	case 1:
		GPUCMD_AddIncrementalWrites(GPUREG_TEXUNIT1_BORDER_COLOR, reg, 5);
		GPUCMD_AddWrite(GPUREG_TEXUNIT1_TYPE, type);
		break;
	case 2:
		GPUCMD_AddIncrementalWrites(GPUREG_TEXUNIT2_BORDER_COLOR, reg, 5);
		GPUCMD_AddWrite(GPUREG_TEXUNIT2_TYPE, type);
		break;
	}
	//GPUCMD_AddSingleParam(0x0002006F, tunit << 8); 			// enables texcoord outputs
	GPUCMD_AddWrite(GPUREG_SH_OUTATTR_CLOCK, tunit<<8);
	GPUCMD_AddWrite(GPUREG_TEXUNIT_CONFIG, tunit | BIT(16) | BIT(12));
	//GPUCMD_AddSingleParam(0x0002006F, tunit << 8); 			// enables texcoord outputs
	//GPUCMD_AddSingleParam(0x000F0080, 0x00011000 | tunit);	// enables texture units
	//printf("ctrSetTex: %p, %d\n", data, type);
}
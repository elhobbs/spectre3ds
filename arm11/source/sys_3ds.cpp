#include "sys.h"
#include <3ds.h>
#include "gs.h"

#include <stdlib.h>

#include "keyboard.h"

#include "memPool.h"
#include "cache.h"

#include "khax.h"

cache textureCache;
cache vboCache;

extern gsVbo_s con_tris;
extern u32* __ctru_linear_heap;

#define TICKS_PER_MSEC 268123.480
#define TICKS_PER_SEC 268123480.0

typedef struct {
	u64 date_time;
	u64 update_tick;
	//...
} datetime2_t;

static volatile u32* __datetime_selector = (u32*)0x1FF81000;
static volatile datetime2_t* __datetime0 = (datetime2_t*)0x1FF81020;
static volatile datetime2_t* __datetime1 = (datetime2_t*)0x1FF81040;

extern Handle gspEvents[GSPGPU_EVENT_MAX];
void __gspWaitForEvent(GSPGPU_Event id, bool nextEvent) {
	if (id >= GSPGPU_EVENT_MAX)return;
	if (nextEvent)
		svcClearEvent(gspEvents[id]);
	Result ret = svcWaitSynchronization(gspEvents[id], 200 * 1000 * 1000);
	if (ret == 0) {
		if (!nextEvent)
			svcClearEvent(gspEvents[id]);
	}
	else {
		printf("WaitForEvent timeout: %d\n", id);
	}
}

#define __gspWaitForPSC0() __gspWaitForEvent(GSPGPU_EVENT_PSC0, false)
#define __gspWaitForPSC1() __gspWaitForEvent(GSPGPU_EVENT_PSC1, false)
#define __gspWaitForVBlank() __gspWaitForEvent(GSPGPU_EVENT_VBlank0, true)
#define __gspWaitForVBlank0() __gspWaitForEvent(GSPGPU_EVENT_VBlank0, true)
#define __gspWaitForVBlank1() __gspWaitForEvent(GSPGPU_EVENT_VBlank1, true)
#define __gspWaitForPPF() __gspWaitForEvent(GSPGPU_EVENT_PPF, false)
#define __gspWaitForP3D() __gspWaitForEvent(GSPGPU_EVENT_P3D, false)
#define __gspWaitForDMA() __gspWaitForEvent(GSPGPU_EVENT_DMA, false)


// Returns number of milliseconds since 1st Jan 1900 00:00.
u64 _osGetTime() {
	u32 s1, s2 = *__datetime_selector & 1;
	volatile datetime2_t dt;

	do {
		s1 = s2;
		if (!s1) {
			dt.date_time = __datetime0->date_time;
			dt.update_tick = __datetime0->update_tick;
		}
		else {
			dt.date_time = __datetime1->date_time;
			dt.update_tick = __datetime1->update_tick;
		}
		s2 = *__datetime_selector & 1;
	} while (s2 != s1);

	u64 delta = svcGetSystemTick() - dt.update_tick;

	// Work around the VFP not supporting 64-bit integer <--> floating point conversion
	double temp = (u32)(delta >> 32);
	temp *= 0x100000000ULL;
	temp += (u32)delta;

	u32 offset = temp / TICKS_PER_MSEC;
	return dt.date_time + offset;
}


int SYS::milliseconds() {
	return osGetTime();
}

double SYS::seconds() {
	u64 delta = svcGetSystemTick();
	double temp = (u32)(delta >> 32);
	temp *= 0x100000000ULL;
	temp += (u32)delta;

	return temp / TICKS_PER_SEC;
	return osGetTime() / 1000.0;
}

__int64_t SYS::microseconds() {
	return svcGetSystemTick();
}

void SYS::timer_init() {
}

void SYS::timer_shutdown() {
}

extern "C" {
	void con_draw();
	void con_init(bool onbottom);
	bool SD_init();
};

bool SYS::quiting() {
	if (!aptMainLoop()) {
		quit();
	}
	return !m_running;
}

u32 gpuCmdSize;
u32* gpuCmdLeft;
u32* gpuCmdRight;

extern "C" {
#include "test_vsh_shbin.h"
#include "q1Bsp_vsh_shbin.h"
#include "q1Mdl_vsh_shbin.h"
#include "text_vsh_shbin.h"
#include "texture_bin.h"
};


DVLB_s* shader_dlvb;
shaderProgram_s shader;
DVLB_s* q1Bsp_dlvb;
shaderProgram_s q1Bsp_shader;
DVLB_s* q1Mdl_dlvb;
shaderProgram_s q1Mdl_shader;
DVLB_s* text_dlvb;
shaderProgram_s text_shader;

u32* texData;

int text_mode = -1;
int shader_mode = -1;

void set_shader_shader() {
	gsShaderSet(&shader);
	u32 bufferOffsets = 0;
	u64 bufferPermutations = 0x210;;
	u8 bufferNumAttributes = 2;
	GPU_SetAttributeBuffers(3, (u32*)osConvertVirtToPhys((void *)__ctru_linear_heap),
		GPU_ATTRIBFMT(0, 3, GPU_FLOAT) | GPU_ATTRIBFMT(1, 2, GPU_FLOAT),
		0xFFC, 0x210, 1, &bufferOffsets, &bufferPermutations, &bufferNumAttributes);
	GPU_SetTextureEnable(GPU_TEXUNIT0);
	GPU_SetTexEnv(0,
		GPU_TEVSOURCES(GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
		GPU_TEVSOURCES(GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_MODULATE, GPU_MODULATE,
		0xFFFFFFFF);
}

void set_text_shader() {
	gsShaderSet(&text_shader);
	u32 bufferOffsets = 0;
	u64 bufferPermutations = 0x210;;
	u8 bufferNumAttributes = 2;
	GPU_SetAttributeBuffers(3, (u32*)osConvertVirtToPhys((void *)__ctru_linear_heap),
		GPU_ATTRIBFMT(0, 3, GPU_FLOAT) | GPU_ATTRIBFMT(1, 2, GPU_FLOAT),
		0xFFC, 0x210, 1, &bufferOffsets, &bufferPermutations, &bufferNumAttributes);
	GPU_SetTextureEnable(GPU_TEXUNIT0);
	GPU_SetTexEnv(0,
		GPU_TEVSOURCES(GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
		GPU_TEVSOURCES(GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_MODULATE, GPU_MODULATE,
		0xFFFFFFFF);
}

int SYS::init() {
	Result r, ret = 0;

	gfxInitDefault();
	consoleInit(GFX_BOTTOM, 0);
	gfxSwapBuffers();

	//only do this on ninjhax 1
	if (!hbInit()) {

		hbExit();
		Result result = khaxInit();
		::printf("khaxInit returned %08lx\n", result);
	}

	u8 isN3DS = 0;
	APT_CheckNew3DS(&isN3DS);
	if (isN3DS){
		osSetSpeedupEnable(true);
		aptOpenSession();
		r = APT_SetAppCpuTimeLimit(80);
		if (r != 0){
			::printf("APT_SetAppCpuTimeLimit: Error\n");
		}
		aptCloseSession();
	}
	
	GPU_Init(NULL);

	gfxSet3D(true);
	
	// clear upper screen (black) instead of junk
	//con_init(true);
	__gspWaitForVBlank();
	keyboard_init();


	//linearSize = 2 * 1024 * 1024;
	//linearBase = (char *)linearMemAlign(linearSize, 0x80);
	//textureCache.init((byte *)linearBase, linearSize);

	//linearSize = 2 * 1024 * 1024;
	//linearBase = (char *)linearMemAlign(linearSize, 0x10);
	//vboCache.init((byte *)linearBase, linearSize);

	gpuCmdSize = 0x40000;
	gpuCmdLeft = (u32*)linearMemAlign(gpuCmdSize * 4, 0x100);
	gpuCmdRight = (u32*)linearMemAlign(gpuCmdSize * 4, 0x100);

	GPU_Reset(NULL, gpuCmdLeft, gpuCmdSize);

#if 1
	shader_dlvb = DVLB_ParseFile((u32*)test_vsh_shbin, test_vsh_shbin_size);
	shaderProgramInit(&shader);
	shaderProgramSetVsh(&shader, &shader_dlvb->DVLE[0]);

	text_dlvb = DVLB_ParseFile((u32*)text_vsh_shbin, text_vsh_shbin_size);
	shaderProgramInit(&text_shader);
	shaderProgramSetVsh(&text_shader, &text_dlvb->DVLE[0]);

	q1Mdl_dlvb = DVLB_ParseFile((u32*)q1Mdl_vsh_shbin, q1Mdl_vsh_shbin_size);
	shaderProgramInit(&q1Mdl_shader);
	shaderProgramSetVsh(&q1Mdl_shader, &q1Mdl_dlvb->DVLE[0]);

	q1Bsp_dlvb = DVLB_ParseFile((u32*)q1Bsp_vsh_shbin, q1Bsp_vsh_shbin_size);
	shaderProgramInit(&q1Bsp_shader);
	shaderProgramSetVsh(&q1Bsp_shader, &q1Bsp_dlvb->DVLE[0]);

	gsInit(&q1Bsp_shader);

	text_mode = renderMode.register_mode(&set_text_shader);
	shader_mode = renderMode.register_mode(&set_shader_shader);

	int linearSize = 26 * 1024 * 1024;
	char *linearBase = (char *)linearMemAlign(linearSize, 0x10);
	linear.initFromBase(linearBase, linearSize, 0x10);

#endif

	timer_init();

	srand(svcGetSystemTick());

	window_create();

	return ret;
}

int SYS::shutdown() {
	int ret = window_shutdown();

	timer_shutdown();
	
	gsExit();
	khaxExit();
	gfxExit();

	return ret;
}
//stolen from staplebutt
void GPU_SetDummyTexEnv(u8 num)
{
	GPU_SetTexEnv(num,
		GPU_TEVSOURCES(GPU_PREVIOUS, 0, 0),
		GPU_TEVSOURCES(GPU_PREVIOUS, 0, 0),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_REPLACE,
		GPU_REPLACE,
		0xFFFFFFFF);
}
#if 1
void ApplyLeftFrustum(float mNearClippingDistance, float mFarClippingDistance, float mFOV, float mAspectRatio, float mConvergence, float mEyeSeparation)
{
	float top, bottom, left, right;

#if 1
	float tanfov = tan(mFOV / 2);
	float fshift = (mEyeSeparation / 2.0) * mNearClippingDistance / mConvergence;

	bottom = -mNearClippingDistance*tanfov + fshift;
	top = mNearClippingDistance*tanfov + fshift;

	right = mAspectRatio * mNearClippingDistance * tanfov;
	left = -right;
#else
	top = mNearClippingDistance * tan(mFOV / 2);
	bottom = -top;

	float a = mAspectRatio * tan(mFOV / 2) * mConvergence;

	float b = a - mEyeSeparation / 2;
	float c = a + mEyeSeparation / 2;

	left = -b * mNearClippingDistance / mConvergence;
	right = c * mNearClippingDistance / mConvergence;
#endif

	// Set the Projection Matrix
	gsMatrixMode(GS_PROJECTION);
	gsLoadIdentity();
	gsFrustum(left, right, bottom, top,
		mNearClippingDistance, mFarClippingDistance);

	// Displace the world to right
	//gsMatrixMode(GS_MODELVIEW);
	//gsLoadIdentity();
	//glTranslatef(mEyeSeparation / 2, 0.0f, 0.0f);
}

void ApplyRightFrustum(float mNearClippingDistance, float mFarClippingDistance, float mFOV, float mAspectRatio, float mConvergence, float mEyeSeparation)
{
	float top, bottom, left, right;
#if 1
	float tanfov = tan(mFOV / 2);
	float fshift = (mEyeSeparation / 2.0) * mNearClippingDistance / mConvergence;

	bottom = -mNearClippingDistance*tanfov - fshift;
	top = mNearClippingDistance*tanfov - fshift;
	
	right = mAspectRatio * mNearClippingDistance * tanfov;
	left = -right;
#else
	top = mNearClippingDistance * tan(mFOV / 2);
	bottom = -top;

	float a = mAspectRatio * tan(mFOV / 2) * mConvergence;

	float b = a - mEyeSeparation / 2;
	float c = a + mEyeSeparation / 2;

	left = -c * mNearClippingDistance / mConvergence;
	right = b * mNearClippingDistance / mConvergence;
#endif

	// Set the Projection Matrix
	gsMatrixMode(GS_PROJECTION);
	gsLoadIdentity();
	gsFrustum(left, right, bottom, top,
		mNearClippingDistance, mFarClippingDistance);

	// Displace the world to left
	//glMatrixMode(GS_MODELVIEW);
	//gsLoadIdentity();
	//glTranslatef(-mEyeSeparation / 2, 0.0f, 0.0f);
}

#else
void ApplyLeftFrustum(float mNearClippingDistance, float mFarClippingDistance, float mFOV, float mAspectRatio, float mConvergence, float mEyeSeparation)
{
	float top, bottom, left, right;

#if 1
	float tanfov = tan(mFOV / 2);
	float a = tanfov * mConvergence;

	float b = a - mEyeSeparation / 2;
	float c = a + mEyeSeparation / 2;

	bottom = -b * mNearClippingDistance / mConvergence;
	top = c * mNearClippingDistance / mConvergence;

	right = mAspectRatio * mNearClippingDistance * tanfov;
	left = -right;
#else
	top = mNearClippingDistance * tan(mFOV / 2);
	bottom = -top;

	float a = mAspectRatio * tan(mFOV / 2) * mConvergence;

	float b = a - mEyeSeparation / 2;
	float c = a + mEyeSeparation / 2;

	left = -b * mNearClippingDistance / mConvergence;
	right = c * mNearClippingDistance / mConvergence;
#endif

	// Set the Projection Matrix
	gsMatrixMode(GS_PROJECTION);
	gsLoadIdentity();
	gsFrustum(left, right, bottom, top,
		mNearClippingDistance, mFarClippingDistance);

	// Displace the world to right
	//gsMatrixMode(GS_MODELVIEW);
	//gsLoadIdentity();
	//glTranslatef(mEyeSeparation / 2, 0.0f, 0.0f);
}

void ApplyRightFrustum(float mNearClippingDistance, float mFarClippingDistance, float mFOV, float mAspectRatio, float mConvergence, float mEyeSeparation)
{
	float top, bottom, left, right;
#if 1
	float tanfov = tan(mFOV / 2);
	float a = tanfov * mConvergence;

	float b = a - mEyeSeparation / 2;
	float c = a + mEyeSeparation / 2;

	bottom = -c * mNearClippingDistance / mConvergence;
	top = b * mNearClippingDistance / mConvergence;
	
	right = mAspectRatio * mNearClippingDistance * tanfov;
	left = -right;
#else
	top = mNearClippingDistance * tan(mFOV / 2);
	bottom = -top;

	float a = mAspectRatio * tan(mFOV / 2) * mConvergence;

	float b = a - mEyeSeparation / 2;
	float c = a + mEyeSeparation / 2;

	left = -c * mNearClippingDistance / mConvergence;
	right = b * mNearClippingDistance / mConvergence;
#endif

	// Set the Projection Matrix
	gsMatrixMode(GS_PROJECTION);
	gsLoadIdentity();
	gsFrustum(left, right, bottom, top,
		mNearClippingDistance, mFarClippingDistance);

	// Displace the world to left
	//glMatrixMode(GS_MODELVIEW);
	//gsLoadIdentity();
	//glTranslatef(-mEyeSeparation / 2, 0.0f, 0.0f);
}
#endif

bool sys_in_frame = false;
#define CONFIG_3D_SLIDERSTATE (*(float*)0x1FF81080)
int SYS::frame_begin(frmType_t type) {
	float slider = CONFIG_3D_SLIDERSTATE;

	sys_in_frame = true;
	
	switch(type) {
	case FRAME_LEFT:
		GPUCMD_SetBuffer(gpuCmdLeft, gpuCmdSize, 0);
		break;
	case FRAME_RIGHT:
		GPUCMD_SetBuffer(gpuCmdRight, gpuCmdSize, 0);
		break;
	}
#ifdef WIN32
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
#endif
	gsStartFrame();
	GPU_SetViewport(
		(u32*)osConvertVirtToPhys((void *)gsGpuDOut), 
		(u32*)osConvertVirtToPhys((void *)gsGpuOut), 
		0, 0, 240 * 2, 400);

	GPU_DepthMap(-1.0f, 0.0f);
	GPU_SetFaceCulling(GPU_CULL_NONE);
	GPU_SetStencilTest(false, GPU_ALWAYS, 0x00, 0xFF, 0x00);
	GPU_SetStencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_KEEP);
	GPU_SetBlendingColor(0, 0, 0, 0);
	GPU_SetDepthTestAndWriteMask(true, GPU_GREATER, GPU_WRITE_ALL);

	u32 zero  = 0;
	GPUCMD_Add(0x00010062, &zero, 1);
	GPUCMD_Add(0x000F0118, &zero, 1);

	GPU_SetAlphaBlending(
		GPU_BLEND_ADD, 
		GPU_BLEND_ADD, 
		GPU_SRC_ALPHA, 
		GPU_ONE_MINUS_SRC_ALPHA, 
		GPU_SRC_ALPHA, 
		GPU_ONE_MINUS_SRC_ALPHA);
	GPU_SetAlphaTest(false, GPU_ALWAYS, 0x00);

	GPU_SetTextureEnable(GPU_TEXUNIT0);

	GPU_SetTexEnv(0,
		GPU_TEVSOURCES(GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
		GPU_TEVSOURCES(GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_MODULATE, GPU_MODULATE,
		0xFFFFFFFF);
	GPU_SetDummyTexEnv(1);
	GPU_SetDummyTexEnv(2);
	GPU_SetDummyTexEnv(3);
	GPU_SetDummyTexEnv(4);
	GPU_SetDummyTexEnv(5);

	renderMode.set_mode(shader_mode);

	//setup lighting
	float lightAngle = 0.1415f;// angles[YAW];
	vect3Df_s lightDir = vnormf(vect3Df(cos(lightAngle), -1.0f, sin(lightAngle)));
	float lightDirection[] = { 0.0f, -lightDir.z, -lightDir.y, -lightDir.x };
	float lightAmbient[] = {
		0.9f, 0.9f, 0.9f, 1.0f
	};
	//GPU_SetUniform(SHDR_GetUniformRegister(q1Bsp_shader, "lightDirection", 0), (u32*)lightDirection, 1);
	//GPU_SetUniform(SHDR_GetUniformRegister(q1Bsp_shader, "lightAmbient", 0), (u32*)lightAmbient, 1);
	GPU_SetFloatUniform(GPU_VERTEX_SHADER, shaderInstanceGetUniformLocation(shader.vertexShader, "lightDirection"), (u32*)lightDirection, 1);
	GPU_SetFloatUniform(GPU_VERTEX_SHADER, shaderInstanceGetUniformLocation(shader.vertexShader, "lightAmbient"), (u32*)lightAmbient, 1);

	GPU_SetFloatUniform(GPU_VERTEX_SHADER, shaderInstanceGetUniformLocation(text_shader.vertexShader, "lightDirection"), (u32*)lightDirection, 1);
	GPU_SetFloatUniform(GPU_VERTEX_SHADER, shaderInstanceGetUniformLocation(text_shader.vertexShader, "lightAmbient"), (u32*)lightAmbient, 1);

	GPU_SetFloatUniform(GPU_VERTEX_SHADER, shaderInstanceGetUniformLocation(q1Bsp_shader.vertexShader, "lightDirection"), (u32*)lightDirection, 1);
	GPU_SetFloatUniform(GPU_VERTEX_SHADER, shaderInstanceGetUniformLocation(q1Bsp_shader.vertexShader, "lightAmbient"), (u32*)lightAmbient, 1);


	//initialize projection matrix to standard perspective stuff
	//gsMatrixMode(GS_PROJECTION);
	//gsProjectionMatrix(80.0f*M_PI / 180.0f, 240.0f / 400.0f, 4.0f, 8192.0f);
	do {
		float convergence = _3ds_convergence.value;
		float seperation = _3ds_seperation.value * slider;
		float fov = _3ds_fov.value * M_PI / 180.0f;
		float near_plane = _3ds_near_plane.value;
		float far_plane = _3ds_far_plane.value;
		float aspect = 240.0f / 400.0f;

		if (convergence == 0.0f) {
			convergence = 1.0f;
		}
		switch (type) {
		case FRAME_LEFT:
			ApplyLeftFrustum(near_plane, far_plane, fov, aspect, convergence, seperation);
			break;
		case FRAME_RIGHT:
			ApplyRightFrustum(near_plane, far_plane, fov, aspect, convergence, seperation);
			break;
		}
	} while (0);
	gsRotateZ(M_PI / 2); //because framebuffer is sideways...

	gsVboClear(&con_tris);

	return 0;
}

int SYS::frame_end(frmType_t type) {

#ifdef WIN32
	SwapBuffers(m_hdc);
#endif

#if 1
	sys.renderMode.set_mode(text_mode);
	push_2d();
		gsVboDrawDirectly(&con_tris);
	pop_2d();

	GPU_FinishDrawing();

	GPUCMD_Finalize();

	GPUCMD_FlushAndRun();
	__gspWaitForP3D();
	
	switch (type) {
	case FRAME_LEFT:
		GX_DisplayTransfer((u32*)gsGpuOut, 0x019001E0, (u32*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL), 0x019001E0, 0x01001000);
		break;
	case FRAME_RIGHT:
		GX_DisplayTransfer((u32*)gsGpuOut, 0x019001E0, (u32*)gfxGetFramebuffer(GFX_TOP, GFX_RIGHT, NULL, NULL), 0x019001E0, 0x01001000);
		break;
	}
	__gspWaitForPPF();
	
	//__gspWaitForPSC0();
	//clear the screen
	GX_MemoryFill((u32*)gsGpuOut, gsBackgroundColor, (u32*)&gsGpuOut[0x2EE00], 0x201, (u32*)gsGpuDOut, 0x00000000, (u32*)&gsGpuDOut[0x2EE00], 0x201);
	__gspWaitForPSC0();

#endif

	return 0;
}

int SYS::frame_final() {
	keyboard_draw();

	gfxSwapBuffersGpu();

	//gspWaitForEvent(GSPEVENT_VBlank0, true);
	if (m_vsync) {
		gspWaitForEvent(GSPGPU_EVENT_VBlank0, true);
	}
	sys_in_frame = false;
	return 0;
}

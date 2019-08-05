#include "sys.h"
#include <3ds.h>
#include <citro3d.h>
#include "ctr_vbo.h"
#include "ctr_render.h"

#include <stdlib.h>
#include <stdio.h>

#include "keyboard.h"

#include "memPool.h"
#include "cache.h"

//#include "khax.h"

cache textureCache;
cache vboCache;

extern ctrVbo_t con_tris;
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

#if CITRO3D
extern "C" {
#include "test_shbin.h"
#include "q1Bsp_shbin.h"
#include "q1Mdl_shbin.h"
#include "text_shbin.h"
#include "particle_shbin.h"
#include "texture_bin.h"
};
#endif

DVLB_s* shader_dlvb;
shaderProgram_s shader;
DVLB_s* q1Bsp_dlvb;
shaderProgram_s q1Bsp_shader;
DVLB_s* q1Mdl_dlvb;
shaderProgram_s q1Mdl_shader;
DVLB_s* text_dlvb;
shaderProgram_s text_shader;
DVLB_s* particle_dlvb;
shaderProgram_s particle_shader;

u32* texData;

int text_mode = -1;
int shader_mode = -1;
int particle_mode = -1;

void set_shader_shader() {
#ifdef CITRO3D
	ctrBindShader(&shader);
	//printf("shader\n");

	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0=position
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2); // v1=texcoord
	
	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, __ctru_linear_heap, sizeof(float)*5, 2, 0x10);

	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvInit(env);
	
	C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);

	C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR);
	C3D_TexEnvOpAlpha(env, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA);

	C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);
#endif

}

void set_text_shader() {
#ifdef CITRO3D
	ctrBindShader(&text_shader);
	//printf("text_shader\n");

	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0=position
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2); // v1=texcoord

	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, __ctru_linear_heap, sizeof(float) * 5, 2, 0x10);

	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvInit(env);

	C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
	C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);

	C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR);
	C3D_TexEnvOpAlpha(env, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA);

#endif
}

void set_particle_shader() {
#ifdef CITRO3D
	ctrBindShader(&particle_shader);
	//printf("text_shader\n");

	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0=position
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 4); // v1=color

	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, __ctru_linear_heap, sizeof(float) * 7, 2, 0x10);

	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvInit(env);

	C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
	C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);

	C3D_TexEnvOpRgb(env, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR);
	C3D_TexEnvOpAlpha(env, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA);
	//C3D_DepthTest(true, GPU_ALWAYS, GPU_WRITE_ALL);
#endif
}

static u32 old_time_limit;
static C3D_RenderTarget* target_left;
static C3D_RenderTarget* target_right;
#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

int SYS::init() {
	Result ret = 0;

	gfxInitDefault();
	consoleInit(GFX_BOTTOM, 0);
	gfxSwapBuffers();

#if 0
	//only do this on ninjhax 1
	if (!hbInit()) {

		hbExit();
		Result result = khaxInit();
		::printf("khaxInit returned %08lx\n", result);
	}
#endif

	osSetSpeedupEnable(true);
	APT_GetAppCpuTimeLimit(&old_time_limit);

	Result cpuRes = APT_SetAppCpuTimeLimit(30);
	if (R_FAILED(cpuRes)) {
		::printf("Failed to set syscore CPU time limit: %08lX", cpuRes);
	}
	
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE*4);
	ctrRenderInit();
	gfxSet3D(true);

	// clear upper screen (black) instead of junk
	gspWaitForAnyEvent();
	keyboard_init();


	int linearSize = 26 * 1024 * 1024;
	char* linearBase = (char*)linearMemAlign(linearSize, 0x10);
	linear.initFromBase(linearBase, linearSize, 0x10);

#ifdef CITRO3D
	target_left = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	target_right = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	
	C3D_RenderTargetSetOutput(target_left, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
	C3D_RenderTargetSetOutput(target_right, GFX_TOP, GFX_RIGHT, DISPLAY_TRANSFER_FLAGS);

	shader_dlvb = DVLB_ParseFile((u32*)test_shbin, test_shbin_size);
	shaderProgramInit(&shader);
	shaderProgramSetVsh(&shader, &shader_dlvb->DVLE[0]);

	text_dlvb = DVLB_ParseFile((u32*)text_shbin, text_shbin_size);
	shaderProgramInit(&text_shader);
	shaderProgramSetVsh(&text_shader, &text_dlvb->DVLE[0]);

	q1Mdl_dlvb = DVLB_ParseFile((u32*)q1Mdl_shbin, q1Mdl_shbin_size);
	shaderProgramInit(&q1Mdl_shader);
	shaderProgramSetVsh(&q1Mdl_shader, &q1Mdl_dlvb->DVLE[0]);

	q1Bsp_dlvb = DVLB_ParseFile((u32*)q1Bsp_shbin, q1Bsp_shbin_size);
	shaderProgramInit(&q1Bsp_shader);
	shaderProgramSetVsh(&q1Bsp_shader, &q1Bsp_dlvb->DVLE[0]);

	particle_dlvb = DVLB_ParseFile((u32*)particle_shbin, particle_shbin_size);
	shaderProgramInit(&particle_shader);
	shaderProgramSetVsh(&particle_shader, &particle_dlvb->DVLE[0]);
	shaderProgramSetGsh(&particle_shader, &particle_dlvb->DVLE[1], 0);

	text_mode = renderMode.register_mode(&set_text_shader);
	shader_mode = renderMode.register_mode(&set_shader_shader);
	particle_mode = renderMode.register_mode(&set_particle_shader);

#endif

	timer_init();

	srand(svcGetSystemTick());

	window_create();

	return ret;
}

int SYS::shutdown() {
	int ret = window_shutdown();

	timer_shutdown();

	Result cpuRes = APT_SetAppCpuTimeLimit(old_time_limit);
	if (R_FAILED(cpuRes)) {
		::printf("Failed to reset syscore CPU time limit: %08lX", cpuRes);
	}
	osSetSpeedupEnable(false);

	gfxSet3D(false);
	C3D_Fini();

	gfxExit();

	return ret;
}

void pause_key(char* text);

#define LOG(_name) 
//pause_key(_name)
//::printf("%s\n",_name)

bool sys_in_frame = false;
#define CLEAR_COLOR 0
int SYS::frame_begin(frmType_t type) {
	float slider = osGet3DSliderState();
	float iod = slider / 4;

	sys_in_frame = true;
	LOG("C3D_FrameSync");
	C3D_FrameSync();
	LOG("C3D_FrameBegin");
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

#ifdef CITRO3D
	switch (type) {
	case FRAME_LEFT:
		LOG("C3D_RenderTargetClear - left");
		C3D_RenderTargetClear(target_left, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
		C3D_FrameDrawOn(target_left);
		break;
	case FRAME_RIGHT:
		LOG("C3D_RenderTargetClear - right");
		C3D_RenderTargetClear(target_right, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
		C3D_FrameDrawOn(target_right);
		break;
	}
	LOG("ctrVboClear");
	ctrVboClear(&con_tris);

	LOG("ctrPerspectiveProjection");
	ctrPerspectiveProjection(C3D_AngleFromDegrees(40.0f), C3D_AspectRatioTop, _3ds_near_plane.value, _3ds_far_plane.value, (type == FRAME_LEFT ? -iod : iod), 2.0f);
	
	LOG("C3D_DepthTest");
	C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_ALL);
#endif
#ifdef WIN32
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
#endif


	return 0;
}

int SYS::frame_end(frmType_t type) {

#ifdef WIN32
	SwapBuffers(m_hdc);
#endif

#ifdef CITRO3D
	push_2d();

	if (con_tris.currentSize) {
		sys.renderMode.set_mode(text_mode);
		ctrVboDrawDirectly(&con_tris);
	}
	pop_2d();
	C3D_FrameEnd(0);
#endif

	return 0;
}

int SYS::frame_final() {
	keyboard_draw();
	sys_in_frame = false;
	return 0;
}

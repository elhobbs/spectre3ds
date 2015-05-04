#include "sys.h"
#include <3ds.h>
#include "gs.h"

#include "keyboard.h"
#include "input.h"

int SYS::set_format(int bpp, int zdepth, int alphadepth, int stencildepth)
{
	m_bpp = bpp;
	m_zdepth = zdepth;
	m_alphadepth = alphadepth;
	m_stencildepth = stencildepth;
#ifdef WIN32
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-5.0, 5.0, -5.0, 5.0, 5.0, 10000.0);
#endif

	return 1;
}

static int buttonMap3ds[] = {
	'a',
	'b',
	K_ESCAPE,
	K_ENTER,
	K_RIGHTARROW,
	K_LEFTARROW,
	K_UPARROW,
	K_DOWNARROW,
	'r',
	'l',
	'x',
	'y',
	'q',
	'e',
	K_ALT,
	K_CTRL,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	K_MOUSE1,
	K_MOUSE2,
	K_MOUSE3,
	K_MOUSE4,
	K_JOY1,
	K_JOY2,
	K_JOY3,
	K_JOY4
};

int SYS::queue_events() {
	hidScanInput();
	u32 down = hidKeysDown();
	u32 up = hidKeysUp();
	for (int i = 0; i < 32; i++) {
		if (buttonMap3ds[i]) {
			if ((down & BIT(i)) != 0) {
				events.queue_event(m_msg_time, EV_KEY_DOWN, buttonMap3ds[i], 0);
			}
			if ((up & BIT(i)) != 0) {
				events.queue_event(m_msg_time, EV_KEY_UP, buttonMap3ds[i], 0);
			}
		}
	}
	static int key_last = 0;
	static int key = 0;

	key_last = key;
	key = keyboard_scankeys();
	if (key_last != 0 && key_last != key)
	{
		//printf("key up: %d %c\n", key_last, key_last);
		events.queue_event(m_msg_time, EV_KEY_UP, key_last, 0);
		//event.type = ev_keyup;
		//event.data1 = key_last;
		//D_PostEvent(&event);
	}

	if (key != 0 && key != key_last)
	{
		//printf("key down: %d %c\n", key, key);
		events.queue_event(m_msg_time, EV_KEY_DOWN, key, 0);
		//event.type = ev_keydown;
		//event.data1 = key;
		//D_PostEvent(&event);
	}
	return 0;
}


void SYS::push_2d() {
#ifdef WIN32
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, m_width, 0, m_height);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
#endif
	gsMatrixMode(GS_PROJECTION);
	gsPushMatrix();
	gsOrthoMatrix(400.0f, 240.0f, -1.0f, 1.0f);
	
	gsMatrixMode(GS_MODELVIEW);
	gsPushMatrix();
	gsLoadIdentity();
	GPU_SetDepthTestAndWriteMask(true, GPU_ALWAYS, GPU_WRITE_ALL);
}

void SYS::pop_2d() {
#ifdef WIN32
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
#endif
	gsMatrixMode(GS_MODELVIEW);
	gsPopMatrix();
	gsMatrixMode(GS_PROJECTION);
	gsPopMatrix();
	gsMatrixMode(GS_MODELVIEW);
	GPU_SetDepthTestAndWriteMask(true, GPU_GREATER, GPU_WRITE_ALL);
}

//#define DS_SCREEN_FULLSCREEN
//#define NDS

#ifndef DS_SCREEN_FULLSCREEN
#ifdef NDS
#define DS_SCREEN_WIDTH 256
#define DS_SCREEN_HEIGHT 192
#define DS_SCREEN_TOP 100
#define DS_SCREEN_LEFT 600
#else
#define DS_SCREEN_WIDTH 480
#define DS_SCREEN_HEIGHT 240
#define DS_SCREEN_TOP 100
#define DS_SCREEN_LEFT 600
#endif

#else
#define DS_SCREEN_WIDTH 1920
#define DS_SCREEN_HEIGHT 1080
#define DS_SCREEN_TOP 0
#define DS_SCREEN_LEFT 0
#endif // 0

float CalcFov(float fov_x, float width, float height)
{
	float	a;
	float	x;

	//if (fov_x < 1 || fov_x > 179)
	//	Com_Error(ERR_DROP, "Bad fov: %f", fov_x);

	x = width / tan(fov_x / 360 * M_PI);

	a = atan(height / x);

	a = a * 360 / M_PI;

	return a;
}

void MYgluPerspective(double fovy, double aspect,
	double zNear, double zFar)
{
	double xmin, xmax, ymin, ymax;

	ymax = zNear * tan(fovy * M_PI / 360.0);
	ymin = -ymax;

	xmin = ymin * aspect;
	xmax = ymax * aspect;

#ifdef WIN32
	glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
#endif
}

int SYS::window_create() {

#ifdef WIN32
	glClearStencil(0);
	glStencilFunc(GL_ALWAYS, 1, 1);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

	//glMTexCoord2fSGIS = (lpMTexFUNC) wglGetProcAddress("glMultiTexCoord2fARB");
	//glSelectTextureSGIS = (lpSelTexFUNC) wglGetProcAddress("glActiveTextureARB");
	//if(glMTexCoord2fSGIS == NULL)
	//	glMTexCoord2fSGIS = (lpMTexFUNC) wglGetProcAddress("glMTexCoord2fSGIS");

	/*GetWindowRect(openGL,&rc);
	ClipCursor(&rc);
	ShowCursor(FALSE);
	GLWindow.Size(rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top);
	*/

	glColor4f(1, 1, 1, 1.0f);

	glViewport(0, 0, DS_SCREEN_WIDTH - 1, DS_SCREEN_HEIGHT - 1);


	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//gluPerspective(73.74, 256.0 / 192.0, 0.005, 40.0);
	//gluPerspective(68.3, (float)800 / (float)600, 0.005, 40.0);
	//gluPerspective(73.74, (float)DS_SCREEN_WIDTH / (float)DS_SCREEN_HEIGHT, 5, 10000.0);
	MYgluPerspective(CalcFov(90, DS_SCREEN_WIDTH, DS_SCREEN_HEIGHT), (float)DS_SCREEN_WIDTH / (float)DS_SCREEN_HEIGHT, 4, 8192);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//glDisable(GL_TEXTURE_2D);


	ShowCursor(0);
#endif
	set_position(0, 0, 400, 240);
	build_font();
	return 0;
}

int SYS::window_shutdown() {
	return 0;
}

int SYS::set_position(int x, int y, int width, int height) {
	m_x = x;
	m_y = y;
	m_width = width;
	m_height = height;

	return 0;
}
int SYS::get_position(int *pos) {
	*pos++ = m_x;
	*pos++ = m_y;
	*pos++ = m_width;
	*pos++ = m_height;

	return 0;
}

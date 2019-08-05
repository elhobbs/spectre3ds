#pragma once

#ifdef WIN32
#pragma warning(disable : 4244)
#endif

typedef unsigned char byte;
typedef float vec3_t[3];
typedef float vec4_t[4];
typedef float vec5_t[5];

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>
#include <Mmsystem.h>
typedef void (APIENTRY *lpSetVsyncFUNC) (unsigned int);
#endif
#include <3ds.h>
#include "mathlib.h"
#include "FileSystem.h"
#include "RenderMode.h"
#include <citro3d.h>

typedef C3D_Tex tex3ds_t;

/*typedef struct {
	int width;
	int height;
	GPU_TEXCOLOR type;
	GPU_TEXTURE_FILTER_PARAM min, mag;
	byte *data;
} tex3ds2_t;*/

typedef enum {
	EV_NONE=0,
	EV_KEY_DOWN,
	EV_KEY_UP,
	EV_MOUSE_MOVE,
	EV_MOUSE_BUTTON
} evType_t;

typedef struct event_s {
	evType_t	type;
	int			value;
	int			value2;
	int			time;
} event_t;

#define	MAX_QUED_EVENTS		256
#define	MASK_QUED_EVENTS	( MAX_QUED_EVENTS - 1 )

class EvHandler {
public:
	virtual int handleEvent(event_t ev) = 0;
};

class EvQueue {
public:
	explicit EvQueue(void);

	void queue_event(int time,evType_t type,int value,int value2);
	event_t get_event();

private:
	event_t		m_eventQue[MAX_QUED_EVENTS];
	int			m_eventHead;
	int			m_eventTail;
};

inline EvQueue::EvQueue(void) {
	m_eventHead = 0;
	m_eventTail = 0;
}

inline void EvQueue::queue_event(int time,evType_t type,int value,int value2) {
	event_t	*ev;

	ev = &m_eventQue[ m_eventHead & MASK_QUED_EVENTS ];

	if ( m_eventHead - m_eventTail >= MAX_QUED_EVENTS ) {
		m_eventTail++;
	}

	m_eventHead++;

	ev->time = time;
	ev->type = type;
	ev->value = value;
	ev->value2 = value2;
}

inline event_t EvQueue::get_event( void ) {
	event_t	ev;

	// return if we have data
	if ( m_eventHead > m_eventTail ) {
		m_eventTail++;
		return m_eventQue[ ( m_eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// return the empty event 
	memset( &ev, 0, sizeof( ev ) );

	return ev;
}

typedef enum {
	FRAME_LEFT,
	FRAME_RIGHT
} frmType_t;

class SYS {
public:
	explicit SYS();

	int init();
	int shutdown();

	void error(char *error, ...);

	int set_position(int x,int y,int width,int height);
	int get_position(int *pos);

	int frame_begin(frmType_t type = FRAME_LEFT);
	int frame_end(frmType_t type = FRAME_LEFT);
	int frame_final();

	void push_2d();
	void pop_2d();
	void print(float x, float y, float scale, char* buffer,int len=999999);
	void printf(float x, float y, float scale, char* format, ...);

	void vsync(bool enable);

	int queue_events();

	int addEvHandler(EvHandler *evh);

	int handle_events();

	int milliseconds();
	__int64_t microseconds();
	double seconds();

	void sleep(int ms);

	bool quiting();
	void quit();

	unsigned int gen_texture_id(GPU_TEXTURE_FILTER_PARAM min = GPU_LINEAR, GPU_TEXTURE_FILTER_PARAM mag=GPU_LINEAR);
	unsigned int last_texture_id();
	int texture_size(int id);
	void bind_texture(int id, int unit = 0);
	void load_texture256(int id, int width, int height, unsigned char *data, unsigned char *palette,int trans = 0);
	void load_texture16(int id, int width, int height, unsigned char *data, unsigned char *palette, int trans = 0);
	void load_texture_byte(int id, int width, int height, unsigned char *data, unsigned char *palette, int trans = 0);
	
	void update_texture_byte(int id, int width, int height, unsigned char *data, unsigned char *palette, int trans = 0);

	void load_texture_L8(int id, int width, int height, unsigned char *data);

	void convert_texture256to16(int width, int height, byte *data_in, byte *pal_in, byte *data_out, byte *pal_out);
	
	//event queue
	EvQueue	events;
	int			m_msg_time;
	
	FileSystem fileSystem;
	RenderMode renderMode;

private:
	int build_font();
	int set_format(int bpp,int zdepth,int alphadepth,int stencildepth);
	int window_create();
	int window_shutdown();
	void timer_init();
	void timer_shutdown();

#ifdef WIN32
	lpSetVsyncFUNC	wglSwapIntervalEXT;
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

	//flag to signal shutdown
	bool		m_running;

	//use vsync
	bool		m_vsync;
	
	//font texture
	unsigned int m_font_texture_id;

#ifdef WIN32
	//window handle
	HWND		m_hwnd;

	//gl context
	HDC			m_hdc;
	HGLRC		m_hglrc;
#endif

	//window size
	int			m_x;
	int			m_y;
	int			m_width;
	int			m_height;
	
	//pixel format
	int			m_bpp;
	int			m_zdepth;
	int			m_stencildepth;
	int			m_alphadepth;

#ifdef WIN32
	//timer
	LARGE_INTEGER	m_pfreq;
	double			m_pfreq_inv;
	int				m_lowshift;
	LARGE_INTEGER	m_start;
#endif

	//handler stuff
	int			m_ev_handlers_cnt;
	int			m_ev_handlers_max;
	EvHandler** m_ev_handlers;
	int			grow_handlers();

	//texture stuff
	void pal16toRGBA(int width, int height, unsigned char *data, unsigned char *palette);
	void pal256toRGBA(int width, int height, unsigned char *data, unsigned char *palette);
	int			m_last_tex_id_allocated;
};

inline SYS::SYS(void) {
#ifdef WIN32
	m_hwnd = 0;
	m_hdc = 0;
	m_hglrc = 0;
	m_x = m_y = m_width = m_height = 0;
	m_bpp = m_zdepth = m_stencildepth = m_alphadepth = 0;;
	wglSwapIntervalEXT = 0;
#endif	
	m_ev_handlers_cnt = 0;
	m_ev_handlers_max = 4;
	m_ev_handlers = new EvHandler*[4];
	m_running = true;
	m_vsync = false;
}

inline void SYS::sleep(int ms) {
#ifdef WIN32
	Sleep(ms);
#endif
	svcSleepThread(ms * 1000000LL);
}

inline unsigned int SYS::last_texture_id() {
	return m_last_tex_id_allocated;
}

inline void SYS::quit() {
	m_running = false;
}

inline void SYS::vsync(bool enable) {
	m_vsync = enable;
#ifdef WIN32
	if (wglSwapIntervalEXT == 0) {
		return;
	}
	wglSwapIntervalEXT(enable ? 1 : 0);
#endif
}


char    *va(char *format, ...);

extern SYS sys;


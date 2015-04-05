#pragma once

#include "quakedef.h"
#include "sys.h"
#include "Net.h"
#include "pr_comp.h"
#include "Model.h"
#include "q1Progs.h"
#include "input.h"
#include "cvar.h"
#include "CmdList.h"
#include "CmdBuffer.h"
#include "Particle.h"
#include "q1Bsp.h"
#include "ViewState.h"
#include "Mixer.h"
#include "mp3.h"
#include "Hud.h"

// entity effects

#define	EF_BRIGHTFIELD			1
#define	EF_MUZZLEFLASH 			2
#define	EF_BRIGHTLIGHT 			4
#define	EF_DIMLIGHT 			8


extern cvar_t	cl_upspeed;
extern cvar_t	cl_forwardspeed;
extern cvar_t	cl_backspeed;
extern cvar_t	cl_sidespeed;

extern cvar_t	cl_movespeedkey;

extern cvar_t	cl_yawspeed;
extern cvar_t	cl_pitchspeed;

extern cvar_t	cl_anglespeedkey;

extern cvar_t	m_pitch;
extern cvar_t	m_yaw;
extern cvar_t	m_forward;
extern cvar_t	m_side;

extern cvar_t	cl_bob;
extern cvar_t	cl_bobcycle;
extern cvar_t	cl_bobup;

extern cvar_t	cl_rollspeed;
extern cvar_t	cl_rollangle;

extern cvar_t	v_kicktime;
extern cvar_t	v_kickroll;
extern cvar_t	v_kickpitch;

typedef enum {
	ca_dedicated, 		// a dedicated server with no ability to start a client
	ca_disconnected, 	// full screen console with no connection
	ca_connected,		// valid netcon, talking to a server
	ca_max = 0xffffffff
} cactive_t;

#define	SIGNONS		4			// signon messages to receive before connected

//
// client to server
//
#define	clc_bad			0
#define	clc_nop 		1
#define	clc_disconnect	2
#define	clc_move		3			// [usercmd_t]
#define	clc_stringcmd	4		// [string] message

typedef struct
{
	vec3_t	viewangles;

	// intended velocities
	float	forwardmove;
	float	sidemove;
	float	upmove;
} usercmd_t;

typedef struct
{
	char	name[MAX_SCOREBOARDNAME];
	float	entertime;
	int		frags;
	int		colors;			// two 4 bit fields
	//byte	translations[VID_GRADES * 256];
} scoreboard_t;

#define	MAX_STYLESTRING	64

typedef struct lightstyle_s
{
	int		length;
	char	map[MAX_STYLESTRING];
} lightstyle_t;

class ClientState {
public:
	ClientState();

	void frame_start();
	void frame_end();
	void render(frmType_t type);

	bool init();
	void shutdown();
	double time();

	int view_entity();

	int intermission();


	void send_cmd();
	int read_from_server(float realtime,float frametime);
	void decay_lights();

	void disconnect();

	void play_demo(char *demo,bool time);
	void stop_demo();

	cactive_t get_state();

	void establish_connection(char *name);
	void reconnect();

	void forward_to_server(char *cmd);

	void music(char *cmdline);

	q1_leaf_node *view_leaf();

	q1Bsp		*m_worldmodel;
	Model		*m_model_precache[MAX_MODELS];


	int			m_num_entities;
	entity_t	*m_entities[MAX_EDICTS];

	int			m_num_statics;
	entity_t	m_static_entities[MAX_STATIC_ENTITIES];

	int			m_num_temp_entities;
	entity_t	m_temp_entities[MAX_TEMP_ENTITIES];

	beam_t		m_beams[MAX_BEAMS];

	int			m_in_impulse;

	bool		demo_running();

	void		bind_key(char *key, char *action);

private:
	entity_t		m_viewent;
	int				m_framecount;

	void render_hud();

	void render_view_model();
	void update_view_model(vec3_t org,vec3_t ang);
	float calc_view_bob();
	float calc_roll(vec3_t angles, vec3_t velocity);
	void calc_view_roll(vec3_t angles);

	void adjust_angles();
	void base_move(usercmd_t &cmd);
	void in_move(usercmd_t &cmd);
	void send_move(usercmd_t &cmd);
	void clear_state();
	void read_demo_message();
	int parse_server_message();
	void parse_server_info();
	void signon_reply();
	void parse_update(int bits);
	entity_t	*entity_num(int num);
	void parse_baseline(entity_t *ent);
	void parse_static();
	void parse_client_data(int bits);
	float lerp_point(void);
	void relink_entities(void);
	void parse_tent();
	void parse_beam(NetBuffer *msg, Model *m);
	void parse_damage(NetBuffer *msg);
	void parse_particle(NetBuffer *msg);
	void parse_static_sound(NetBuffer *msg);
	void parse_start_sound(NetBuffer *msg);
	void update_temp_entities();
	entity_t *new_temp_entity();

	cactive_t		m_state;
	int				m_signon;			// 0 to SIGNONS
	double			m_oldtime;
	double			m_time;
	double			m_last_received_message;
	double			m_mtime[2];
	int				m_movemessages;

	int				m_demo_frame_start;
	bool			m_demo_time;
	float			m_demo_starttime;
	sysFile			*m_demo_file;
	bool			m_demoplayback;

	NetBufferFixed<1024>	m_message;

	Net				*m_netcon;


	bool			m_onground;
	bool			m_paused;
	bool			m_inwater;
	int				m_stats[MAX_CL_STATS];	// health, etc
	int				m_intermission;	// don't change view angle, full screen, etc
	int				m_completed_time;	// latched at intermission start

	char			m_levelname[40];	// for display on solo scoreboard
	int				m_viewentity;		// cl_entitites[cl.viewentity] = player
	int				m_maxclients;
	int				m_gametype;

	int				m_cdtrack, m_looptrack;	// cd audio

	float			v_dmg_time, v_dmg_roll, v_dmg_pitch;

	// information for local display
	//int			stats[MAX_CL_STATS];	// health, etc
	int			items;			// inventory bit flags
	float	item_gettime[32];	// cl.time of aquiring item, for blinking
	float		faceanimtime;	// use anim frame if cl.time < this

	float		rumble_time;
	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  The server sets punchangle when
	// the view is temporarliy offset, and an angle reset commands at the start
	// of each level and after teleporting.
	vec3_t		m_mviewangles[2];	// during demo playback viewangles is lerped
	// between these
	vec3_t		m_viewangles;

	vec3_t		mvelocity[2];	// update by server, used for lean+bob
	// (0 is newest)
	vec3_t		m_velocity;		// lerped between mvelocity[0] and [1]

	vec3_t		punchangle;		// temporary offset
	vec3_t		v_forward, v_right, v_up;


	// pitch drifting vars
	float		idealpitch;
	float		pitchvel;
	bool		nodrift;
	float		driftmove;
	double		laststop;

	float		m_viewheight;
	float		crouch;			// local amount for smoothing stepups

	// personalization data sent to server	
	char		m_mapstring[MAX_QPATH];
	char		m_spawnparms[MAX_MAPSTRING];	// to restart a level
	// frag scoreboard
	scoreboard_t	*m_scores;		// [cl.maxclients]
	lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];

	sEvHandler					m_evh;

	ViewState					m_view;

	Sound						*cl_sfx_wizhit;
	Sound						*cl_sfx_knighthit;
	Sound						*cl_sfx_tink1;
	Sound						*cl_sfx_ric1;
	Sound						*cl_sfx_ric2;
	Sound						*cl_sfx_ric3;
	Sound						*cl_sfx_r_exp3;
	Mixer						m_mixer;
	mp3							m_music;
	Hud							m_hud;
};

inline q1_leaf_node* ClientState::view_leaf() {
	if (m_signon == SIGNONS) {
		return m_view.view_leaf();
	}
	return 0;
}

inline cactive_t ClientState::get_state() {
	return m_state;
}

inline double ClientState::time() {
	return m_time;
}

inline int ClientState::view_entity() {
	return m_viewentity;
}

inline int ClientState::intermission() {
	return m_intermission;
}

inline bool ClientState::init() {
	sys.addEvHandler(&m_evh);
	m_view.init();
	m_mixer.init();
	m_hud.init();

	return false;
}

inline void ClientState::shutdown() {
	m_mixer.shutdown();
	m_music.stop();
}

inline bool ClientState::demo_running() {
	return m_demoplayback;
}
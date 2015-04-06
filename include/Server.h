#pragma once

#include "quakedef.h"
#include "Client.h"
#include "q1Progs.h"
#include "vec3_fixed.h"
#include "Model.h"
#include "q1Bsp.h"
#include "cvar.h"
#include "CmdList.h"

#define	SERVER_VERSION 1.09

#define MAX_DATAGRAM 1024

typedef struct areanode_s
{
	int					axis;		// -1 = leaf node
	fixed16p3			dist;
	struct areanode_s	*children[2];
	link_t				trigger_edicts;
	link_t				solid_edicts;
} areanode_t;

#define	AREA_DEPTH	4
#define	AREA_NODES	32

#define	MOVE_NORMAL		0
#define	MOVE_NOMONSTERS	1
#define	MOVE_MISSILE	2

#define	STEPSIZE	18

typedef struct
{
	bool		allsolid;	// if true, plane is not valid
	bool		startsolid;	// if true, the initial point was in a solid area
	bool		inopen, inwater;
	float		fraction;		// time completed, 1.0 = didn't hit anything
	vec3_t		endpos;			// final position
	q1_plane	plane;			// surface normal at impact
	edict_t		*ent;			// entity the surface is on
} trace_t;

typedef struct
{
	vec3_t		boxmins, boxmaxs;// enclose the test object along entire move
	float		*mins, *maxs;	// size of the moving object
	vec3_t		mins2, maxs2;	// size when clipping against mosnters
	float		*start, *end;
	trace_t		trace;
	int			type;
	edict_t		*passedict;
} moveclip_t;


typedef enum { ss_loading, ss_active, ss_max = 0xffffffff } server_state_t;

class Server {
public:
	Server();
	bool init();
	void frame(float frametime);

	void		save_game(FILE *fp);

	void		pause();
	void		pause(bool on);
	void		loadgame(bool on);
	void		broadcast_printf(char *str, ...);

	void		clear_datagram();
	void		check_for_new_clients();
	void		user_friction(edict_t *ent, float *origin, float *velocity);
	void		air_accelerate(float *velocity, float wishspeed, vec3_t wishveloc);
	void		accelerate(float *velocity, float *wishdir, float wishspeed);
	void		air_move(Client *client, usercmd_t &cmd, float *angles, float *origin, float *velocity, bool onground);
	void		client_think(Client *client);
	void		run_clients();
	void		physics(float frametime);
	void		send_client_messages();
	void		cleanup_ents();
	void		connect_client(int i);
	void		send_server_info(Client *client);
	void		update_to_reliable_messages();
	bool		send_client_datagram(Client *client);
	void		send_nop(Client *client);
	void		drop_client(Client *client, bool crash);
	void		save_spawn_parms();
	void		spawn_server(char *name);
	bool		read_client_message(Client *client);
	void		read_client_move(Client *client);
	void		prespawn(char *cmdline, void *p, ...);
	//void		spawn(Client *client);
	void		spawn(char *cmdline, void *p, ...);
	void		god(char *cmdline, void *p, ...);
	void		fly(char *cmdline, void *p, ...);
	void		noclip(char *cmdline, void *p, ...);
	void		create(char *cmdline, void *p, ...);
	void		create_baseline();
	int			model_index(char *name);
	void		write_client_data_to_message(edict_t *ent, NetBuffer *msg);
	void		write_entities_to_client(edict_t	*clent, NetBuffer *msg);
	void		send_reconnect();
	int			send_to_all(NetBuffer &msg, int timeout);
	void		shutdown();
	
	void		SV_AddToFatPVS(vec3_t org, q1_bsp_node *node);
	byte		*SV_FatPVS(vec3_t org);
	void		SV_PVS(vec3_t org, byte *pvs);

	void		new_chase_dir(edict_t *actor, edict_t *enemy, float dist);
	void		fix_check_bottom(edict_t *ent);
	bool		check_bottom(edict_t *ent);
	bool		move_step(edict_t *ent, vec3_t move, bool relink);
	bool		step_direction(edict_t *ent, float yaw, float dist);
	bool		close_enough(edict_t *ent, edict_t *goal, float dist);
	void		move_to_goal(edict_t *ent, edict_t *goal, float dist);
	void		check_water_transition(edict_t *ent);
	trace_t		push_entity(edict_t *ent, vec3_t push);
	int			try_unstick(edict_t *ent, vec3_t oldvel);
	void		wall_friction(edict_t *ent, trace_t *trace);
	int			clip_velocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce);
	void		impact(edict_t *e1, edict_t *e2);
	int			fly_move(edict_t *ent, float time, trace_t *steptrace);
	void		walk_move(edict_t *ent);
	edict_t		*test_entity_position(edict_t *ent);
	void		check_stuck(edict_t *ent);
	void		add_gravity(edict_t *ent);
	bool		check_water(edict_t *ent);
	bool		run_think(edict_t *ent);
	void		push_move(edict_t *pusher, float movetime);
	void		Physics_None(edict_t *ent);
	void		Physics_Noclip(edict_t *ent);
	void		Physics_Client(edict_t	*ent, int num);
	void		Physics_Toss(edict_t *ent);
	void		Physics_Pusher(edict_t *ent);
	void		Physics_Step(edict_t *ent);
	void		check_velocity(edict_t *ent);
	void		water_jump(edict_t *ent);
	void		water_move(edict_t *ent, usercmd_t &cmd, float *velocity);

	void		link_edict(edict_t *ent, bool touch_triggers);
	void		unlink_edict(edict_t *ent);
	void		find_touched_leafs(edict_t *ent, q1_bsp_node *node);
	void		touch_links(edict_t *ent, areanode_t *node);

	hull_t		*hull_for_box(vec3_t mins, vec3_t maxs);
	hull_t		*hull_for_entity(edict_t *ent, vec3_t mins, vec3_t maxs, vec3_t offset);
	void		init_box_hull();
	areanode_t *CreateAreaNode(int depth, vec3_fixed16 &mins, vec3_fixed16 &maxs);
	void		ClearWorld(void);

	int			hull_point_contents(hull_t *hull, int num, vec3_t p);
	int			hull_point_contents(hull_t *hull, int num, vec3_fixed32 p);
	int			point_contents(vec3_t p);
	int			point_contents(edict_t *ent);

	void SV_MoveBounds(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, vec3_t boxmins, vec3_t boxmaxs);
	void SV_ClipToLinks(areanode_t *node, moveclip_t *clip);
	int			trace_clip_r(hull_t *hull, int parent, int node, traceResults &trace, fixed32p16 f0, fixed32p16 f1, vec3_fixed32 &p0, vec3_fixed32 &p1);
	int			trace_clip(hull_t *hull, trace_t &trace, vec3_t p0, vec3_t p1);
	trace_t		SV_ClipMoveToEntity(edict_t *ent, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);
	trace_t		SV_Move(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int type, edict_t *passedict);

	void		start_sound(edict_t *entity, int channel, char *sample, int volume, float attenuation);
	void		start_sound_static(vec3_t pos, char *sample, int volume, float attenuation);


	Client		*client(int num);
	int			maxclients();
	bool		changelevel_issued;	// cleared when at SV_SpawnServer

	void		start_particle(vec3_t org, vec3_t dir, int color, int count);

	bool		is_active();
	server_state_t state();

	bool		set_active(bool val);
	double		time();
	void		time(float time);
	NetBuffer	*datagram();
	NetBuffer	*reliable_datagram();
	NetBuffer	*signon();

	q1Progs		*m_progs;

	char		*m_sound_precache[MAX_SOUNDS];	// NULL terminated

	char		*m_model_precache[MAX_MODELS];	// NULL terminated
	Model		*m_models[MAX_MODELS];
	q1Bsp		*m_worldmodel;

	char							m_name[64];
	int			lastcheck;			// used by PF_checkclient
	double		lastchecktime;
private:
	double							m_time;
	double							m_frametime;

	bool							m_active;
	bool							m_paused;
	bool							m_loadgame;

	char							m_modelname[64];

	server_state_t					m_state;
	NetBufferFixed<MAX_DATAGRAM>	m_datagram;
	NetBufferFixed<MAX_DATAGRAM>	m_reliable_datagram;
	NetBufferFixed<8192>			m_signon;

	int								m_maxclients;
	Client							*m_clients;

	int								m_serverflags;
	bool							m_changelevel_issued;

	int								m_current_skill;
	areanode_t	sv_areanodes[AREA_NODES];
	int			sv_numareanodes;

	hull_t		box_hull;
	q1_clipnode	box_clipnodes[6];
	q1_plane	box_planes[6];

	CmdList<Server>		m_cmds;
};

extern	cvar_t	sv_maxvelocity;
extern	cvar_t	sv_gravity;
extern	cvar_t	sv_nostep;
extern	cvar_t	sv_friction;
extern	cvar_t	sv_stopspeed;
extern	cvar_t	sv_maxspeed;
extern	cvar_t	sv_accelerate;
extern	cvar_t	sv_edgefriction;

inline Server::Server() : m_cmds(this) {

	Cvar_RegisterVariable(&sv_maxvelocity);
	Cvar_RegisterVariable(&sv_gravity);
	Cvar_RegisterVariable(&sv_friction);
	Cvar_RegisterVariable(&sv_stopspeed);
	Cvar_RegisterVariable(&sv_nostep);
	Cvar_RegisterVariable(&sv_maxspeed);
	Cvar_RegisterVariable(&sv_accelerate);
	Cvar_RegisterVariable(&sv_edgefriction);

	m_active = m_paused = m_loadgame = false;
	changelevel_issued = false;

	m_maxclients = 4;
	m_clients = new Client[m_maxclients];

	m_progs = new q1Progs(*this);

	m_cmds.add("spawn", &Server::spawn);
	m_cmds.add("prespawn", &Server::prespawn);
	m_cmds.add("god", &Server::god);
	m_cmds.add("fly", &Server::fly);
	m_cmds.add("noclip", &Server::noclip);
	m_cmds.add("create", &Server::create);
}

inline bool Server::init() {
	return false;
}

inline Client *Server::client(int num) {
	if (num < 0 || num > m_maxclients) {
		return 0;
	}
	return &m_clients[num];
}

inline NetBuffer *Server::datagram() {
	return &m_datagram;
}

inline NetBuffer *Server::reliable_datagram() {
	return &m_reliable_datagram;
}

inline NetBuffer *Server::signon() {
	return &m_signon;
}

inline int Server::maxclients() {
	return m_maxclients;
}

inline double Server::time() {
	return m_time;
}

inline void Server::time(float time) {
	m_time = time;
}

inline bool Server::is_active() {
	return m_active;
}

inline server_state_t Server::state() {
	return m_state;
}

inline bool Server::set_active(bool val) {
	bool ret = m_active;
	m_active = val;

	//return the original value;
	return ret;
}

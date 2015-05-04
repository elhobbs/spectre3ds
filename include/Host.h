#pragma once

#include "Server.h"
#include "ClientState.h"
#include "CmdList.h"
#include "CmdBuffer.h"
#include "Console.h"
#include <stdarg.h>
#include <stdlib.h>

extern cvar_t	skill;
extern cvar_t	deathmatch;
extern cvar_t	coop;
extern cvar_t	registered;
extern cvar_t	debug;
extern cvar_t	maxfps;
extern cvar_t	v_gamma;
extern cvar_t	hud_alpha;
extern cvar_t	mixahead;
extern cvar_t	dynamic_textures;

class Host {
public:
	Host();
	
	void frame(double time);
	void render();
	bool console_visible();

	bool init();
	void shutdown();
	void show_console(bool show);
	void clear();

	void shutdown_server();

	double real_time();
	double cl_time();
	double frame_time();

	int cl_view_entity();
	q1_leaf_node* cl_view_leaf();
	int cl_intermission();

	bool server_active();
	server_state_t server_state();

	void dprintf(char* format, ...);
	
	void printf(char* format, ...);
	void center_printf(char* format, ...);
	void add_text(char *text, ...);
	void execute_cmds();
	void execute(char*cmd);
	char* auto_complete(char *cmd);

private:
	void add_game_directory(char *dir);
	void choose_game_draw(char *dirlist[], int total, int pos);
	void choose_game_dir();
	bool filter_time(double time);
	void server_frame(float frametime);

	double			m_time;
	double			m_realtime;
	double			m_oldrealtime;
	double			m_frametime;

	Server			m_sv;
	ClientState		m_cl;

	void change_level(char *cmdline, void *p, ...);
	void restart(char *cmdline, void *p, ...);
	void map(char *cmdline, void *p, ...);
	void quit(char *cmdline, void *p, ...);
	void reconnect(char *cmdline, void *p, ...);
	void disconnect(char *cmdline, void *p, ...);
	void god(char *cmdline, void *p, ...);
	void fly(char *cmdline, void *p, ...);
	void impulse(char *cmdline, void *p, ...);
	void connect(char *cmdline, void *p, ...);
	void noclip(char *cmdline, void *p, ...);
	void create(char *cmdline, void *p, ...);
	void playdemo(char *cmdline, void *p, ...);
	void timedemo(char *cmdline, void *p, ...);
	void showpool(char *cmdline, void *p, ...);
	void vsync(char *cmdline, void *p, ...);
	void set(char *cmdline, void *p, ...);
	void clear(char *cmdline, void *p, ...);
	void dir(char *cmdline, void *p, ...);
	void pause(char *cmdline, void *p, ...);
	void music(char *cmdline, void *p, ...);
	void bind(char *cmdline, void *p, ...);
	void game_load(char *cmdline, void *p, ...);
	void game_save(char *cmdline, void *p, ...);

	CmdList<Host>			m_cmds;
	CmdBufferFixed<8192>	m_cmd_text;

	Console					m_console;
	Notify					m_notify;
};

inline Host::Host():m_cmds(this) {
	m_realtime = m_oldrealtime = m_time = 1.0f;
	Cvar_RegisterVariable(&skill);
	Cvar_RegisterVariable(&deathmatch);
	Cvar_RegisterVariable(&coop);
	Cvar_RegisterVariable(&registered);
	Cvar_RegisterVariable(&debug);
	Cvar_RegisterVariable(&maxfps);
	Cvar_RegisterVariable(&v_gamma);
	Cvar_RegisterVariable(&hud_alpha);
	Cvar_RegisterVariable(&mixahead);
	Cvar_RegisterVariable(&dynamic_textures);

	m_cmds.add("changelevel", &Host::change_level);
	m_cmds.add("restart", &Host::restart);
	m_cmds.add("map", &Host::map);
	m_cmds.add("quit", &Host::quit);
	m_cmds.add("god", &Host::god);
	m_cmds.add("fly", &Host::fly);
	m_cmds.add("impulse", &Host::impulse);
	m_cmds.add("connect", &Host::connect);
	m_cmds.add("reconnect", &Host::reconnect);
	m_cmds.add("disconnect", &Host::disconnect);
	m_cmds.add("noclip", &Host::noclip);
	m_cmds.add("playdemo", &Host::playdemo);
	m_cmds.add("timedemo", &Host::timedemo);
	m_cmds.add("showpool", &Host::showpool);
	m_cmds.add("vsync", &Host::vsync);
	m_cmds.add("set", &Host::set);
	m_cmds.add("clear", &Host::clear);
	m_cmds.add("dir", &Host::dir);
	m_cmds.add("create", &Host::create);
	m_cmds.add("pause", &Host::pause);
	m_cmds.add("music", &Host::music);
	m_cmds.add("bind", &Host::bind);
	m_cmds.add("load", &Host::game_load);
	m_cmds.add("save", &Host::game_save);
}

inline bool Host::console_visible() {
	return m_console.visible();
}

inline double Host::real_time() {
	return m_realtime;
}

inline double Host::frame_time() {
	return m_frametime;
}

inline double Host::cl_time() {
	return m_cl.time();
}

inline int Host::cl_view_entity() {
	return m_cl.view_entity();
}

inline int Host::cl_intermission() {
	return m_cl.intermission();
}

inline q1_leaf_node* Host::cl_view_leaf() {
	return m_cl.view_leaf();
}

inline bool Host::server_active() {
	return m_sv.is_active();
}

inline void Host::show_console(bool show) {
	m_console.show(show);
}

inline server_state_t Host::server_state() {
	return m_sv.state();
}

inline void Host::add_text(char *text, ...) {
	va_list         argptr;

	va_start(argptr, text);
	m_cmd_text.write(text, argptr);
	va_end(argptr);
}

inline void Host::change_level(char *cmdline, void *p, ...) {
	cmdArgs cmd(cmdline);
	if (cmd.argc < 2) {
		return;
	}
	m_sv.save_spawn_parms();
	m_sv.spawn_server(cmd.argv[1]);
}

inline void Host::connect(char *cmdline, void *p, ...) {
	cmdArgs cmd(cmdline);
	if (cmd.argc < 2) {
		return;
	}
	m_cl.establish_connection(cmd.argv[1]);
}

inline void Host::quit(char *cmdline, void *p, ...) {
	sys.quit();
}

inline void Host::reconnect(char *cmdline, void *p, ...) {
	m_cl.reconnect();
}

inline void Host::disconnect(char *cmdline, void *p, ...) {
	m_cl.disconnect();
}

inline void Host::god(char *cmdline, void *p, ...) {
	m_cl.forward_to_server(cmdline);
}

inline void Host::fly(char *cmdline, void *p, ...) {
	m_cl.forward_to_server(cmdline);
}

inline void Host::noclip(char *cmdline, void *p, ...) {
	m_cl.forward_to_server(cmdline);
}

inline void Host::create(char *cmdline, void *p, ...) {
	m_cl.forward_to_server(cmdline);
}

inline void Host::music(char *cmdline, void *p, ...) {
	m_cl.music(cmdline);
}

inline void Host::bind(char *cmdline, void *p, ...) {
	cmdArgs cmd(cmdline);
	switch (cmd.argc) {
	case 3:
		m_cl.bind_key(cmd.argv[1], cmd.argv[2]);
		break;
	default:
		printf("usage: bind key action\n");
		return;
	}
}

inline void Host::pause(char *cmdline, void *p, ...) {
	m_sv.pause();
}

inline void Host::impulse(char *cmdline, void *p, ...) {
	cmdArgs cmd(cmdline);
	if (cmd.argc < 2) {
		return;
	}
	m_cl.m_in_impulse = atoi(cmd.argv[1]);
}

inline void Host::playdemo(char *cmdline, void *p, ...) {
	cmdArgs cmd(cmdline);
	if (cmd.argc < 2) {
		return;
	}
	m_cl.play_demo(cmd.argv[1],false);
}

inline void Host::timedemo(char *cmdline, void *p, ...) {
	cmdArgs cmd(cmdline);
	if (cmd.argc < 2) {
		return;
	}
	m_cl.play_demo(cmd.argv[1],true);
}

inline void Host::vsync(char *cmdline, void *p, ...) {
	cmdArgs cmd(cmdline);
	if (cmd.argc < 2) {
		return;
	}
	int enable = atoi(cmd.argv[1]);
	sys.vsync(enable == 0 ? false : true);
}

inline void Host::set(char *cmdline, void *p, ...) {
	cmdArgs cmd(cmdline);
	if (cmd.argc == 2) {
		printf("%s is set to '%s'\n", cmd.argv[1], Cvar_VariableString(cmd.argv[1]));
		return;
	}
	if (cmd.argc < 3) {
		return;
	}
	char old_value[128];
	char *temp = Cvar_VariableString(cmd.argv[1]);
	strcpy(old_value, temp);
	Cvar_Set(cmd.argv[1], cmd.argv[2]);
	printf("%s changed from '%s' to '%s'\n", cmd.argv[1], old_value, Cvar_VariableString(cmd.argv[1]));
}

inline void Host::clear(char *cmdline, void *p, ...) {
	m_console.clear();
}

inline void Host::restart(char *cmdline, void *p, ...) {
	char	mapname[MAX_QPATH];

	if (m_cl.demo_running() || !m_sv.is_active()) {
		return;
	}

#if 0
	if (cmd_source != src_command)
		return;
#endif // 0

	strcpy(mapname, m_sv.m_name);	// must copy out, because it gets cleared in sv_spawnserver
	m_sv.spawn_server(mapname);
}

inline void Host::map(char *cmdline, void *p, ...) {
	cmdArgs cmd(cmdline);

	if (cmd.argc < 2) {
		return;
	}
	m_cl.disconnect();
	m_sv.shutdown();

	m_sv.spawn_server(cmd.argv[1]);

	if (!server_active()) {
		return;
	}
	m_cmds.call("connect local");
}

extern Host host;

#include "Host.h"
#include "sys.h"
#include "memPool.h"
#include <stdlib.h>
#include "procTexture.h"
#include <3ds.h>
#include "cache.h"
#include "timer3DS.h"
#include <sys/dirent.h>
#include <string.h>
#include "ed_parser.h"

unsigned char *q1_palette;
extern cache textureCache;
extern cache vboCache;

cvar_t	skill = { "skill", "1" };						// 0 - 3
cvar_t	deathmatch = { "deathmatch", "0" };			// 0, 1, or 2
cvar_t	coop = { "coop", "0" };			// 0 or 1
cvar_t  registered = { "registered", "0" };
cvar_t  debug = { "debug", "0" };
cvar_t	maxfps = { "maxfps", "60", true };
cvar_t	v_gamma = { "gamma", "0.7", true };
cvar_t	hud_alpha = { "hud_alpha", "0.3", true };
cvar_t	mixahead = { "mixahead", "0.1", false };
cvar_t	dynamic_textures = { "dynamic_textures", "1", false };

cvar_t	_3ds_seperation = { "seperation", "20", true };
cvar_t	_3ds_convergence = { "convergence", "500", true };
cvar_t	_3ds_fov = { "fov", "90", true };
cvar_t	_3ds_near_plane = { "near_plane", "4", true };
cvar_t	_3ds_far_plane = { "far_plane", "8192", true };

void Host::clear() {
	m_notify.clear();
	pool.clear();
	linear.clear();
	Models.reset();
	Sounds.reset();
	//textureCache.reset();
	//vboCache.reset();
	procTextureList::reset();
}

bool Host::filter_time(double time) {
	m_realtime += time;

	if ((m_realtime - m_oldrealtime) < (1.0/maxfps.value)) {
		sys.sleep(1);
		return false;
	}

	m_frametime = m_realtime - m_oldrealtime;
	m_oldrealtime = m_realtime;


	// don't allow really long or short frames
	if (m_frametime > 0.1f) {
		m_frametime = 0.1f;
	}
	else if (m_frametime < 0.001f) {
		m_frametime = 0.001f;
	}
	return true;
}

void Host::execute(char *cmd) {
	m_cmds.call(cmd);
}

char* Host::auto_complete(char *cmd) {
	cmdArgs cmdargs(cmd);
	if (cmdargs.argc == 2 && !strcmp(cmdargs.argv[0], "set")) {
		static char cmdline[100];
		char *cvar = Cvar_CompleteVariable(cmdargs.argv[1]);
		if (cvar) {
			sprintf(cmdline, "set %s ", cvar);
			return cmdline;
		}
		return 0;
	}
	char *list[100];
	int found = m_cmds.matches(cmd,100,list);
	if (found == 0) {
		return 0;
	}
	printf("found %d matches\n", found);
	for (int i = 0; i < found; i++) {
		printf("%c    %s\n", 2, list[i]);
	}
	return found > 0 ? list[0] : 0;
}

void Host::execute_cmds() {
	while (m_cmd_text.read_pos() != m_cmd_text.pos()) {
		char *s = m_cmd_text.read_string();
		if (s == 0) {
			return;
		}
		m_cmds.call(s);
	}
}

void Host::dprintf(char* format, ...) {
	va_list args;
	static char buffer[4096], *p;

	if (Cvar_VariableValue("debug") == 0) {
		return;
	}

	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);

	m_console.print(buffer);

}

void Host::printf(char* format, ...) {
	va_list args;
	static char buffer[4096], *p;

	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);

	if (Cvar_VariableValue("debug") != 0) {
		::printf(buffer);
	}
	m_console.print(buffer);

}

void Host::center_printf(char* format, ...) {
	va_list args;
	static char buffer[4096], *p;

	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);

	m_notify.print(buffer);

}

//#define ENABLE_TIMING

#ifdef ENABLE_TIMING
#define TIMING_RESET() g_timer.reset()
#define TIMING_START(_t,_name) int _t=g_timer.start(_name)
#define TIMING_STOP(_t) g_timer.stop(_t)
#define TIMING_PRINT() g_timer.print()
#else
#define TIMING_RESET()
#define TIMING_START(_t,_name)
#define TIMING_STOP(_t)
#define TIMING_PRINT()
#endif

void Host::frame(double time) {

	//::printf("time: %f\n",time);
	TIMING_RESET();

	TIMING_START(tt, "elapsed");

	TIMING_START(t0, "events");

	rand();

	//::printf("filter_time\n");
	if (!filter_time(time)) {
		return;
	}

	//::printf("queue_events\n");
	if (sys.queue_events()) {
		return;
	}

	//::printf("handle_events\n");
	sys.handle_events();
	TIMING_STOP(t0);

	TIMING_START(t1,"cmds");

	//::printf("execute_cmds\n");
	//execute console commands
	execute_cmds();

	TIMING_STOP(t1);

	// if running the server locally, make intentions now
	//::printf("m_cl.send_cmd\n");
	if (m_sv.is_active()) {
		m_cl.send_cmd();
	}

	TIMING_START(t2,"server");
	//::printf("m_sv.frame\n");
	m_sv.frame(m_frametime);


	// if running the server remotely, send intentions now after
	// the incoming messages have been read
	//::printf("m_cl.send_cmd\n");
	if (!m_sv.is_active()) {
		m_cl.send_cmd();
	}
	TIMING_STOP(t2);

	m_time += m_frametime;

	TIMING_START(t3,"client");
	//::printf("read_from_server\n");
	m_cl.read_from_server(m_realtime, m_frametime);
	TIMING_STOP(t3);


	TIMING_START(t4, "render");
	//::printf("render\n");
	render();

	TIMING_STOP(t4);
	TIMING_STOP(tt);


	TIMING_PRINT();
}

#define CONFIG_3D_SLIDERSTATE (*(float*)0x1FF81080)

void Host::render() {
	float slider = CONFIG_3D_SLIDERSTATE;
	int frames = slider > 0.0f ? 2 : 1;
	frmType_t type[] = { FRAME_LEFT, FRAME_RIGHT };

	if (dynamic_textures.value) {
		TIMING_START(t0, "proctex");

		proc_textures.update();

		TIMING_STOP(t0);
	}


	TIMING_START(t1, "fstart");
	m_cl.frame_start();
	TIMING_STOP(t1);

	for (int i = 0; i < frames; i++) {
		sys.frame_begin(type[i]);

		TIMING_START(t1, "frend");
		m_cl.render(type[i]);
		TIMING_STOP(t1);

		TIMING_START(t2, "crend");
		m_console.render();
		TIMING_STOP(t2);

		TIMING_START(t3, "hrend");
		m_notify.render();
		TIMING_STOP(t3);

		TIMING_START(t4, "fend");
		sys.frame_end(type[i]);
		TIMING_STOP(t4);
	}
	
	TIMING_START(t2, "ffin");
	m_cl.frame_end();

	sys.frame_final();
	TIMING_STOP(t2);
}

#if 0
int Host::connect(char *name) {
	m_sv.spawn_server(name);
	m_sv.set_active(true);
	m_cl.establish_connection(name);

	return 0;
}

#endif // 0

void Host::shutdown_server() {
	if (m_cl.get_state() == ca_connected) {
		m_cl.disconnect();
	}
	m_sv.shutdown();
}

void Host::shutdown() {
	save_config();
	shutdown_server();
	m_cl.shutdown();
}


void Host::showpool(char *cmdline, void *p, ...) {
	printf("pool size: %d used: %d free: %d\n", pool.size(), pool.used(), pool.size() - pool.used());
}

void Host::dir(char *cmdline, void *p, ...) {
	cmdArgs cmd(cmdline);
	if (cmd.argc != 2) {
		return;
	}
	char *list[100];
	int found = sys.fileSystem.dir(cmd.argv[1], 100, list);
	for (int i = 0; i < found; i++) {
		printf("%s\n", list[i]);
	}
}

byte		gammatable[256];	// palette is sent through this

void BuildGammaTable(float g)
{
	int		i, inf;

	if (g == 1.0)
	{
		for (i = 0; i<256; i++)
			gammatable[i] = i;
		return;
	}

	for (i = 0; i<256; i++)
	{
		inf = 255 * pow((i + 0.5f) / 255.5f, g) + 0.5;
		if (inf < 0)
			inf = 0;
		if (inf > 255)
			inf = 255;
		gammatable[i] = inf;
	}
}

extern "C" size_t getMemFree();


void pause_key(char *text) {
#if 0
	printf("debug : %dKB LINEAR, %dKB REGULAR", (int)linearSpaceFree() / 1024, (int)getMemFree() / 1024);
	printf(text);
	printf("press A...");
	do {
		scanKeys();
		gspWaitForEvent(GSPEVENT_VBlank0, false);
	} while ((keysDown() & KEY_A) == 0);
	do {
		scanKeys();
		gspWaitForEvent(GSPEVENT_VBlank0, false);
	} while ((keysDown() & KEY_A) == KEY_A);
	printf("done\n");
#endif
}

#define DIR_LIST_COUNT 20
void Host::choose_game_draw(char *dirlist[], int total, int pos)
{
	int i;
	int start = (pos / DIR_LIST_COUNT)*DIR_LIST_COUNT;
	int end = start + DIR_LIST_COUNT;

	if (end > total)
		end = total;

	// Clear the screen
	iprintf("\x1b[2J");

	iprintf("Choose a mod game directory\nA to select or B to cancel");
	// Move to 2nd row
	iprintf("\x1b[2;0H");
	// Print line of dashes
	iprintf("--------------------------------");
	for (i = start; i < end; i++)
	{
		// Set row
		iprintf("\x1b[%d;0H", i - start + 3);
		iprintf(" [%s]", dirlist[i]);
	}
	if (end < total)
	{
		// Set row
		iprintf("\x1b[%d;0H", i - start + 3);
		iprintf(" more...");
	}
	gfxFlushBuffers();
	gfxSwapBuffers();
	gspWaitForEvent(GSPEVENT_VBlank0, false);
}

void Host::choose_game_dir() {
	char *buf;
	char *dirlist[DIR_LIST_COUNT * 3];
	int start, dircount = 0;
	int pos = 0;
	int len, pressed, ret;


	buf = new char[4096];
	{
		struct stat st;
		char filename[256];
		DIR *dir;
		struct dirent *pent;

		dir = opendir(".");

		if (dir == 0) {
			iprintf("Unable to open the directory.\n");
			return;
		}

		while ((pent = readdir(dir)) != 0) {
			ret = stat(pent->d_name, &st);
			if (S_ISDIR(st.st_mode))
			{
				if (strcasecmp(pent->d_name, "id1") == 0 ||
					strcmp(pent->d_name, "..") == 0 ||
					strcmp(pent->d_name, ".") == 0)
					continue;
				len = strlen(pent->d_name) + 1;
				if (pos + len >= 4096)
					break;

				dirlist[dircount++] = &buf[pos];
				strcpy(&buf[pos], pent->d_name);
				pos += len + 1;

			}
			if (dircount >= DIR_LIST_COUNT * 3)
				break;
		}

		closedir(dir);
	}

	start = pressed = pos = 0;

	choose_game_draw(dirlist, dircount, pos);

	while (true) {
		int i;
		// Clear old cursors
		for (i = 0; i < DIR_LIST_COUNT; i++) {
			iprintf("\x1b[%d;0H ", i + 3);
		}
		// Show cursor
		iprintf("\x1b[%d;0H*", (pos%DIR_LIST_COUNT) + 3);
		
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForEvent(GSPEVENT_VBlank0, false);

		// Power saving loop. Only poll the keys once per frame and sleep the CPU if there is nothing else to do
		do {
			scanKeys();
			pressed = keysDown();
			gspWaitForVBlank();
		} while (!pressed);

		if (pressed & KEY_UP)
		{
			pos -= 1;
			if (pos < 0)
			{
				pos = dircount - 1;		// Wrap around to bottom of list
				choose_game_draw(dirlist, dircount, pos);
			}
			else if (pos % DIR_LIST_COUNT == (DIR_LIST_COUNT - 1))
			{
				choose_game_draw(dirlist, dircount, pos);
			}
		}
		if (pressed & KEY_DOWN)
		{
			pos += 1;
			if (pos >= dircount)
			{
				pos = 0;		// Wrap around to top of list
				choose_game_draw(dirlist, dircount, pos);
			}
			else if (pos % DIR_LIST_COUNT == 0)
			{
				choose_game_draw(dirlist, dircount, pos);
			}
		}


		if (pressed & KEY_A) {
			// Clear the screen
			iprintf("\x1b[2J");
			printf("adding game dir: %s\n", dirlist[pos]);
			add_game_directory(dirlist[pos]);
			break;
		}

		if (pressed & KEY_B) {
			// Clear the screen
			printf("\x1b[2J");
			printf("canceled gamedir selection\n");
			break;
		}
	}
	//eat all of the keys events
	//or the console will get the last button pressed
	do {
		scanKeys();
		pressed = keysDown() | keysHeld();
		gspWaitForVBlank();
	} while (pressed);

	gfxFlushBuffers();
	gfxSwapBuffers();
	gspWaitForEvent(GSPEVENT_VBlank0, false);
}

void Host::add_game_directory(char *name) {
	char fullname[1024];
	for (int i = 0; i < 10; i++) {
		sprintf(fullname,"%s/pak%d.pak", name, i);
		if (!sys.fileSystem.Add(fullname)) {
			break;
		}
	}
	sys.fileSystem.Add(name);
}

bool Host::init() {

	add_game_directory("id1");

	choose_game_dir();

	//sys.fileSystem.Add("c:\\quake\\id1");
	//sys.fileSystem.Add("c:\\quake\\rogue\\pak0.pak");

	pause_key("gfx/palette.lmp");
	sysFile *pal = sys.fileSystem.open("gfx/palette.lmp");
	int len = pal->length();
	q1_palette = new unsigned char[len];
	pal->read((char *)q1_palette, 0, len);
	delete pal;

	pause_key("BuildGammaTable");
	BuildGammaTable(v_gamma.value);

	pause_key("build palette");
	for (int i = 0; i<256; i++)
	{
		int r = q1_palette[i * 3 + 0];
		int g = q1_palette[i * 3 + 1];
		int b = q1_palette[i * 3 + 2];
		q1_palette[i * 3 + 0] = gammatable[r]; 
		q1_palette[i * 3 + 1] = gammatable[g];
		q1_palette[i * 3 + 2] = gammatable[b];
	}

	pause_key("m_console.init");
	m_console.init();
	pause_key("m_sv.init");
	m_sv.init();
	pause_key("m_cl.init");
	m_cl.init();
	pause_key("m_cl init complete\n");

	pause_key("gfx/pop.lmp");
	FileSystemStat pop = sys.fileSystem["gfx/pop.lmp"];
	if (pop.fsp == 0) {
		Cvar_Set("registered", "0");
		printf("%c\nshareware version detected\n", 2);
	}
	else {
		Cvar_Set("registered", "1");
		printf("%c\nregistered version detected\n", 2);
	}

	pause_key("show_console");
	if (!server_active()) {
		show_console(true);
	}

	pool.markLow();
	linear.markLow();

	load_config();

	host.printf("host init complete\n");

	pause_key("host init completed");
	return false;
}

void Host::game_save(char *cmdline, void *p, ...) {
	cmdArgs cmd(cmdline);

	if (cmd.argc != 2) {
		printf("save <savename> : save a game\n");
		return;
	}

	if (!m_sv.is_active()) {
		printf("Not playing a local game.\n");
		return;
	}

	printf("saving game to %s\n", cmd.argv[1]);
	FILE *fp = sys.fileSystem.create(cmd.argv[1]);
	if (!fp) {
		printf("failed to create file: %s\n", cmd.argv[1]);
		return;
	}

	m_sv.save_game(fp);

	fclose(fp);

	printf("complete.\n");
}

void Host::game_load(char *cmdline, void *p, ...) {
	cmdArgs cmd(cmdline);
	
	if (cmd.argc != 2)
	{
		printf("load <savename> : load a game\n");
		return;
	}

	::printf("opening save file %s...\n", cmd.argv[1]);
	sysFile *fp = sys.fileSystem.open(cmd.argv[1]);
	if (!fp) {
		printf("ERROR: couldn't open %s\n", cmd.argv[1]);
		return;
	}
	int len = fp->length();
	char *data = new char[len+1];
	if (!data) {
		printf("ERROR: failed to allocate buffer\n");
		delete fp;
		return;
	}
	::printf("reading save data %d\n", len);
	fp->read(data, 0, len);
	data[len] = 0;

	::printf("disconnecting client...\n");
	m_cl.disconnect();

	::printf("parsing save data...\n");
	ed_parser parser;
	parser.load_save(m_sv, data);

	delete[] data;
	delete fp;

	::printf("connecting...\n");
	if (m_cl.get_state() != ca_dedicated)
	{
		m_cl.establish_connection("local");
		m_cl.reconnect();
	}
	::printf("done\n");
}

void Host::save_config() {
	FILE *cfg = sys.fileSystem.create("spectre3ds.cfg");
	if (cfg == 0) {
		return;
	}
	printf("saving keys\n");
	m_cl.save_keys(cfg);
	printf("saving cvars\n");
	Cvar_WriteVariables(cfg);
	printf("done\n");


	fclose(cfg);
}

void Host::load_config() {
	sysFile *cfg = sys.fileSystem.open("spectre3ds.cfg");
	if (cfg == 0) {
		return;
	}

	int length = cfg->length();
	char *cmd, *data;
	cmd = data = new char[length + 1];
	if (data) {
		cfg->read(data, 0, length);
		data[length] = 0;

		while (cmd && *cmd) {
			char *eol = cmd;
			while (*eol && *eol != '\n') {
				eol++;
			}
			if (*eol == '\n') {
				*eol = 0;
			}
			execute(cmd);
			cmd = ++eol;
		}
		delete [] data;
	}
	cfg->close();
	delete cfg;

}

int Console::handleEvent(event_t ev) {
	int dir = 1;
	int key;

	if (!m_visible) {
		if (ev.type == EV_KEY_DOWN) {
			key = MapKey(ev.value, m_shift);
			switch (key) {
			case K_ESCAPE:
			case KEY_START:
				m_visible = true;
			}
		}
		return 1;
	}

	switch (ev.type) {
	case EV_KEY_DOWN:
		key = MapKey(ev.value, m_shift);
		switch (key) {
		case K_HOME:
			m_text_history = 0;
			break;
		case K_PGUP:
			m_text_history--;
			break;
		case K_PGDN:
			m_text_history++;
			if (m_text_history > 0) {
				m_text_history = 0;
			}
			break;
		case K_UPARROW:
			dir = -1;
		case K_DOWNARROW:
			//dir defaults to 1
			if (!m_cmd_history) {
				m_cmd_history = true;
				m_cmd_history_line = m_line;
			}
			m_cmd_history_line = (m_cmd_history_line + 32 + dir) % 32;
			if (m_cmd_text[m_cmd_history_line] && *m_cmd_text[m_cmd_history_line]) {
				strcpy(m_cmd_text[m_line], m_cmd_text[m_cmd_history_line]);
				m_pos = strlen(m_cmd_text[m_line]) - 1;
			}
			else {
				//undo
				m_cmd_history_line = (m_cmd_history_line + 32 + (dir*-1)) % 32;
			}
			break;
		case K_TAB:
			if (m_pos > 0) {
				m_text_history = 0;
				char *match = host.auto_complete(m_cmd_text[m_line]);
				if (match) {
					strcpy(m_cmd_text[m_line], match);
					m_pos = strlen(m_cmd_text[m_line]);
					if (m_pos > 0 && m_cmd_text[m_line][m_pos - 1] != ' ') {
						m_cmd_text[m_line][m_pos++] = ' ';
						m_cmd_text[m_line][m_pos] = 0;
					}
				}
			}
			break;
		case K_ESCAPE:
		case KEY_START:
			m_text_history = 0;
			m_cmd_history_line = 0;
			m_cmd_history = false;

			if (m_pos > 0) {
				memset(m_cmd_text[m_line], 0, 256);
				m_pos = 0;
				break;
			}
			m_visible = !m_visible;
			break;
		case K_SHIFT:
			m_shift = true;
			break;
		}
		break;
	case EV_KEY_UP:

		key = MapKey(ev.value, m_shift);

		if (!key)
			return 0;
		switch (key) {
		case K_SHIFT:
			m_shift = false;
			break;
		case K_BACKSPACE:
			if (m_pos > 0) {
				m_cmd_text[m_line][--m_pos] = 0;
			}
			break;
		case K_ENTER:
		case '\n':
			m_text_history = 0;
			m_cmd_history_line = 0;
			m_cmd_history = false;

			m_cmd_text[m_line][m_pos] = key;
			m_pos = 0;
			host.add_text(m_cmd_text[m_line]);
			printf(m_cmd_text[m_line]);
			m_line = (m_line + 1) % 32;
			memset(m_cmd_text[m_line], 0, MAX_CONSOLE_LINE_LENGTH);
			break;
		default:
			if (key < 32 || key > 127) {
				break;
			}
			if (m_shift && key >= 'a' && key <= 'z') {
				key += 'A' - 'a';
			}
			if (m_pos < (MAX_CONSOLE_LINE_LENGTH - 1)) {
				m_cmd_text[m_line][m_pos++] = key;
			}
			break;
		}
	}

	//eat all events
	return 0;
}

void Console::printf(char *text, ...) {
	va_list         argptr;
	static char		buffer[MAX_CONSOLE_TEXT];

	va_start(argptr, text);
	vsprintf(buffer, text, argptr);
	va_end(argptr);

	print(buffer);
}

void Console::print(char *buffer) {
	float text_time = sys.seconds();

	int len = strlen(buffer);
	if (m_text_pos + len + 64 > MAX_CONSOLE_TEXT) {
		m_text_pos = 0;
	}
	int pos = 0;
	int line_len = 0;
	char *p;
	int color = 0;

	if (m_text_lines[m_text_line]) {
		line_len = &m_text[m_text_pos] - m_text_lines[m_text_line];
		m_text_times[m_text_line] = text_time;
	}
	p = &m_text[m_text_pos];
	for (int i = 0; i < len; i++) {
		int ch = buffer[i];
		switch (ch) {
		case 1:
		case 2:
			color = ch;
			*p++ = ch;
			line_len++;
			break;
		case '\n':
		case '\r':
			line_len = 0;
			*p++ = 0;
			*p = 0;
			m_text_line = (m_text_line + 1) % MAX_CONSOLE_LINES;
			m_text_lines[m_text_line] = p;
			m_text_times[m_text_line] = text_time;
			if (color && (i + 1) < len) {
				*p++ = color;
			}
			break;
		default:
			if (ch == '\\' && i + 1 < len && buffer[i + 1] == 'n') {
				line_len = 0;
				*p++ = 0;
				*p = 0;
				m_text_line = (m_text_line + 1) % MAX_CONSOLE_LINES;
				m_text_lines[m_text_line] = p;
				m_text_times[m_text_line] = text_time;
				if (color && (i + 1) < len) {
					*p++ = color;
				}
				i++;
				break;
			}
			if (line_len >= 64) {
				line_len = 0;
				*p++ = 0;
				*p = 0;
				m_text_line = (m_text_line + 1) % MAX_CONSOLE_LINES;
				m_text_lines[m_text_line] = p;
				m_text_times[m_text_line] = text_time;
				if (color && (i + 1) < len) {
					*p++ = color;
				}
			}
			if (ch < 32 || ch > 127) {
				break;
			}
			*p++ = ch;
			line_len++;
		}
	}
	*p = 0;
	m_text_pos = p - m_text;

}

extern gsVbo_s con_tris;

void Console::render() {
	sys.push_2d();

	if (!m_visible) {
		float current_time = sys.seconds();
		int show = 4;
		for (int i = 0; i<show; i++) {
			int line = (m_text_line - i);
			if (line < 0) {
				line += 32;
			}
			if (m_text_lines[line] == 0 || (current_time - m_text_times[line]) > 5) {
				continue;
			}
			sys.print(8, 32 - (8 * i), 1.0f, m_text_lines[line]);
		}
	}
	else {

		for (int i = 0; i < 32; i++) {
			int line = (m_text_line + m_text_history + MAX_CONSOLE_LINES - i) % MAX_CONSOLE_LINES;
			if (m_text_lines[line] == 0) {
				continue;
			}
			sys.print(8, -24 - (8 * i), 1.0f, m_text_lines[line]);
		}
		sys.printf(8, -8, 1.0f, "]: %s", m_cmd_text[m_line]);
		if (m_text_history != 0) {
			sys.printf(8, -16, 1.0f, "%s", "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
		}
		if ((sys.milliseconds() / 200) & 1) {
			sys.printf(8 + (m_pos + 3) * 8, -8, 1.0f, "%c", '_');
		}
	}
	sys.pop_2d();
}

void Notify::print(char *str) {
	if (str == 0 || *str == 0) {
		return;
	}
	strcpy(m_text, str);
	int width = 0;
	m_num_lines = 0;
	m_max_width = 0;
	for (; *str; str++) {
		if (*str == '\n') {
			if (width > m_max_width) {
				m_max_width = width;
			}
			width = 0;
			m_num_lines++;
		}
		else {
			width++;
		}
	}
	if (width > m_max_width) {
		m_max_width = width;
	}
	m_num_lines++;

	if (host.cl_intermission() == 2) {
		m_TTL = 0;
	}
	else {
		m_TTL = 2;
	}
}

void Notify::render() {
	int pos[4];

	if (host.cl_intermission() < 2 && m_TTL <= 0) {
		return;
	}
	int		remaining = 9999;

	// the finale prints the characters one at a time
	if (host.cl_intermission() == 2) {
		remaining = -8 * m_TTL;
	}

	m_TTL -= host.frame_time();

	sys.get_position(pos);
	int x = pos[2] / 2 - m_max_width * 8 / 2;
	int y = pos[3] / 2 - m_num_lines * 8 / 2;
	if (x < 8) {
		x = 8;
	}
	if (y < 8) {
		y = 8;
	}

	sys.push_2d();

	sys.print(x, y, 1, m_text, remaining);

	sys.pop_2d();
}


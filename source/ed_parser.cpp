#include "ed_parser.h"
#include "cvar.h"
#include "stdio.h"
#include "Server.h"

extern cvar_t	skill;
extern cvar_t	deathmatch;
extern cvar_t	coop;

void ed_parser::skip_white_space() {
	int incomment = 0;

	while (position < length) {
		switch (buffer[position]) {
		case '/':
			incomment++;
			if (incomment == 2) {
				while (position < length) {
					switch (buffer[position]) {
					case '\n':
						goto done;
					default:
						position++;
					}
				}
			done:
				incomment = 0;
			}
			break;
		case '\n':
			line++; //as good a place as any to count the lines
		case ' ':
		case '\r':
		case '\t':
			position++;
			break;
		default:
			return;
		}
	}
}

bool ed_parser::skip_past(char *token, int len) {

	while (length - position > len) {
		if (strncmp(&buffer[position], token, len) == 0) {
			position += len;
			return true;
		}
		position++;
	}
	return false;
}

float ed_parser::parse_float() {
	float val;
	int whole = 0, num = 0, den = 0;
	bool fin = false, neg;

	skip_white_space();
	if ((neg = (buffer[position] == '-'))) {
		position++;
	}

	while (!fin && position < length) {
		switch (buffer[position]) {
		case '.':
			den = 1;
			position++;
		case ' ':
		case '\r':
		case '\n':
		case '\t':
			fin = true;
			break;
		case '9':
		case '8':
		case '7':
		case '6':
		case '5':
		case '4':
		case '3':
		case '2':
		case '1':
		case '0':
			whole *= 10;
			whole += buffer[position++] - '0';
			break;
		default:
			fin = true;
		}
	}

	if (den == 1) {
		fin = false;
		while (!fin && position < length) {
			switch (buffer[position]) {
			case ' ':
			case '\r':
			case '\n':
			case '\t':
				fin = true;
				break;
			case '9':
			case '8':
			case '7':
			case '6':
			case '5':
			case '4':
			case '3':
			case '2':
			case '1':
			case '0':
				num *= 10;
				num += buffer[position++] - '0';
				den *= 10;
				break;
			default:
				fin = true;
			}
		}
	}

	val = (float)(neg ? -whole : whole);

	if (den) {
		val += ((float)num / (float)den);
	}

	return val;
}

bool ed_parser::parse_vec3(float p[3]) {
	skip_white_space();
	if (!check("(", 1)) {
		return false;
	}
	float f0 = parse_float();
	float f1 = parse_float();
	float f2 = parse_float();
	if (!check(")", 1)) {
		return false;
	}
	p[0] = f0;
	p[1] = f1;
	p[2] = f2;
	return true;
}

char* ed_parser::parse_string() {
	static char str[1024];

	skip_white_space();


	bool fin = false, in_quote;

	if ((in_quote = (buffer[position] == '\"'))) {
		position++;
	}
	int end = position;

	if (end >= length - 1)
		return 0;

	while (!fin && end < length) {
		switch (buffer[end]) {
		case ' ':
		case '\r':
		case '\n':
		case '\t':
			if (in_quote) {
				end++;
			}
			else {
				fin = true;
			}
			break;
		case '\"':
			fin = true;
			break;
		default:
			end++;
		}
	}
	int len = end - position;
	memcpy(str, &buffer[position],len);
	str[len] = 0;
	position = end + (in_quote ? 1 : 0);

	return str;
}

bool ed_parser::check(char *token, int len) {

	skip_white_space();

	if (length - position < len) {
		return false;
	}
	if (strncmp(&buffer[position], token, len) == 0) {
		position += len;
		return true;
	}
	return false;
}

bool ed_parser::parse_ed(int entnum, bool allocate) {
	if (!check("{", 1)) {
		return false;
	}

	edict_t		*ent;
	if (!entnum) {
		ent = m_progs->EDICT_NUM(0);
	}
	else if (!allocate) {
		while (entnum >= m_progs->m_num_edicts) {
			m_progs->ED_Alloc();
		}
		ent = m_progs->EDICT_NUM(entnum);
		ent->free = false;
		memset(&ent->v, 0, m_progs->m_programs.entityfields * 4);
	}
	else {
		ent = m_progs->ED_Alloc();
	}

	do {
		//check for end
		if (check("}", 1)) {
			if (allocate) {
				// remove things from different skill levels or deathmatch
				if (deathmatch.value)
				{
					if (((int)ent->v.spawnflags & SPAWNFLAG_NOT_DEATHMATCH))
					{
						m_progs->ED_Free(ent);
						return true;
					}
				}
				else if ((skill.value == 0 && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_EASY))
					|| (skill.value == 1 && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_MEDIUM))
					|| (skill.value >= 2 && ((int)ent->v.spawnflags & SPAWNFLAG_NOT_HARD)))
				{
					m_progs->ED_Free(ent);
					return true;
				}
				if (!ent->v.classname)
				{
					printf("No classname for:\n");
					//ED_Print(ent);
					m_progs->ED_Free(ent);
					return true;
				}
				// look for the spawn function
				dfunction_t *func = m_progs->ED_FindFunction(m_progs->m_strings + ent->v.classname);
				if (!func)
				{
					printf("No spawn function for:\n");
					//ED_Print(ent);
					m_progs->ED_Free(ent);
					return true;
				}

				m_progs->m_global_struct->self = m_progs->edict_to_prog(ent);
				m_progs->program_execute(func - m_progs->m_functions);
			}
			else {
				if (!ent->free) {
					m_server->link_edict(ent, false);
				}
			}

			return true;
		}

		//look for key pairs
		char keyname[1024];
		char valstr[1024];
		strcpy(keyname, parse_string());
		strcpy(valstr, parse_string());

		if (keyname[0] == '_') {
			continue;
		}

		if (!strcmp(keyname, "light")) {
			strcpy(keyname, "light_lev");	// hack for single light def
		}

		if (!strcmp(keyname, "angle")) {
			char	temp[32];
			strcpy(keyname, "angles");
			strcpy(temp, valstr);
			sprintf(valstr, "0 %s 0", temp);
		}

		ddef_t *key = m_progs->ED_FindField(keyname);
		if (!key)
		{
			printf("'%s' is not a field\n", keyname);
			continue;
		}

		if (!m_progs->ED_ParseEpair((void *)&ent->v, key, valstr)) {
			sys.error("ED_ParseEdict: parse error");
		}

	} while (position < length);


	return false;
}

void ed_parser::parse(bool allocate) {

	line = 1;
	int entnum = 0;
	while (parse_ed(entnum++,allocate));
}

int ed_parser::load(q1Progs *progs, char *data) {
	m_progs = progs;
	length = strlen(data);
	buffer = data;
	position = 0;

	parse();

	return 0;
}

bool ed_parser::parse_globaldefs() {
	if (!check("{", 1)) {
		return false;
	}
	do {
		//check for end
		if (check("}", 1)) {
			return true;
		}
		//look for key pairs
		char keyname[1024];
		char valstr[1024];
		strcpy(keyname, parse_string());
		strcpy(valstr, parse_string());
		ddef_t	*key = m_progs->ED_FindGlobal(keyname);
		if (!key)
		{
			printf("'%s' is not a global\n", keyname);
			continue;
		}

		if (!m_progs->ED_ParseEpair((void *)m_progs->m_globals, key, valstr)) {
			sys.error("ED_ParseGlobals: parse error");
		}

	} while (position < length);

	return false;
}

int ed_parser::load_save(Server &server, char *data){
	m_progs = server.m_progs;
	m_server = &server;
	length = strlen(data);
	buffer = data;
	position = 0;

	float version = parse_float();
	if (version != 5) {
		return -1;
	}

	char *comment = parse_string();
	float spawn_parms[NUM_SPAWN_PARMS];
	for (int i = 0; i < NUM_SPAWN_PARMS; i++) {
		spawn_parms[i] = parse_float();
	}
	float skill = parse_float();
	int current_skill = (int)(skill + 0.1);
	Cvar_SetValue("skill", (float)current_skill);

	char mapname[128];
	strcpy(mapname, parse_string());
	float time = parse_float();

	server.spawn_server(mapname);

	if (!server.is_active()) {
		printf("Couldn't load map: %s\n", mapname);
		return -1;
	}

	server.pause(true);
	server.loadgame(true);

	for (int i = 0; i<MAX_LIGHTSTYLES; i++) {
		char *str = parse_string();
		server.m_progs->m_lightstyles[i] = new(pool) char[strlen(str) + 1];
		strcpy(server.m_progs->m_lightstyles[i], str);
	}

	if (!parse_globaldefs()) {
		sys.error("parse_globaldefs ERROR\n");
	}

	parse(false);

	Client *client = server.client(0);
	server.time(time);
	for (int i = 0; i < NUM_SPAWN_PARMS; i++) {
		client->m_spawn_parms[i] = spawn_parms[i];
	}

	return 0;
}

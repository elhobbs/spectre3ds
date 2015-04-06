#pragma once
#include "string.h"
#include "q1Progs.h"

#define	SPAWNFLAG_NOT_EASY			256
#define	SPAWNFLAG_NOT_MEDIUM		512
#define	SPAWNFLAG_NOT_HARD			1024
#define	SPAWNFLAG_NOT_DEATHMATCH	2048

class ed_parser {
public:
	int load(q1Progs *progs,char *data);
	int load_save(Server &server, char *data);
private:
	int line;
	int position;
	int length;
	char *buffer;

	bool check(char *token, int len);
	bool skip_past(char *token, int len);
	void skip_white_space();
	char* parse_string();
	float parse_float();
	bool parse_vec3(float p[3]);

	bool parse_ed(int entnum, bool allocate);
	void parse(bool allocate = true);

	bool parse_globaldefs();

	q1Progs *m_progs;
	Server	*m_server;
};


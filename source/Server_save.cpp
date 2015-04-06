#include "Host.h"

#define	SAVEGAME_VERSION	5
#define	SAVEGAME_COMMENT_LENGTH	39

void Server::save_game(FILE *fp) {
	if (!fp) {
		return;
	}
	char	comment[SAVEGAME_COMMENT_LENGTH + 1];
	int current_skill = (int)(skill.value + 0.5);
	Client *client = &m_clients[0];
	
	fprintf(fp, "%i\n", SAVEGAME_VERSION);
	//Host_SavegameComment(comment);
	memset(comment, ' ', sizeof(SAVEGAME_COMMENT_LENGTH));
	comment[SAVEGAME_COMMENT_LENGTH] = 0;

	fprintf(fp, "%s\n", comment);
	for (int i = 0; i<NUM_SPAWN_PARMS; i++)
		fprintf(fp, "%f\n", client->m_spawn_parms[i]);

	fprintf(fp, "%d\n", current_skill);
	fprintf(fp, "%s\n", m_name);
	fprintf(fp, "%f\n", m_time);
	for (int i = 0; i<MAX_LIGHTSTYLES; i++)
	{
		if (m_progs->m_lightstyles[i])
			fprintf(fp, "%s\n", m_progs->m_lightstyles[i]);
		else
			fprintf(fp, "m\n");
	}
	m_progs->ED_WriteGlobals(fp);
	m_progs->ED_Write(fp);
}

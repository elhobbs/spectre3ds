#include "Host.h"
#include "protocol.h"
#include "cmd.h"
#include "cvar.h"
#include "q1Bsp.h"
#include "ed_parser.h"
#include "Sound.h"

extern int vbo_cb;
extern int vbo_tx;
extern int vbo_ls;
extern int mdl_cb;
extern int mdl_tx;

void Server::clear_datagram() {
	m_datagram.clear();
}

void Server::send_server_info(Client *client) {
	char message[256];

	client->m_msg.write_byte(svc_print);
	sprintf(message, "%c\nVERSION %4.2f SERVER (%i CRC)\n", (char)2, SERVER_VERSION, 0);// VERSION, pr_crc);
	client->m_msg.write_string(message);

	client->m_msg.write_byte(svc_serverinfo);
	client->m_msg.write_long(PROTOCOL_VERSION);
	client->m_msg.write_byte(m_maxclients);

	if (!coop.value && deathmatch.value)
		client->m_msg.write_byte(GAME_DEATHMATCH);
	else
		client->m_msg.write_byte(GAME_COOP);

	sprintf(message, m_progs->m_strings + m_progs->m_edicts[0]->v.message);

	client->m_msg.write_string(message);

	for (char **s = m_model_precache + 1; *s; s++) {
		client->m_msg.write_string(*s);
	}
	client->m_msg.write_byte(0);

	for (char **s = m_sound_precache + 1; *s; s++) {
		client->m_msg.write_string(*s);
	}
	client->m_msg.write_byte(0);

	// send music
	client->m_msg.write_byte(svc_cdtrack);
	client->m_msg.write_byte(m_progs->m_edicts[0]->v.sounds);// sv.edicts[0]->v.sounds);
	client->m_msg.write_byte(m_progs->m_edicts[0]->v.sounds);// sv.edicts[0]->v.sounds);

	// set view	
	client->m_msg.write_byte(svc_setview);
	client->m_msg.write_short(1);// NUM_FOR_EDICT(client->edict));

	client->m_msg.write_byte(svc_signonnum);
	client->m_msg.write_byte(1);

	client->m_sendsignon = true;
	client->m_spawned = false;		// need prespawn, spawn, etc
}

void Server::connect_client(int clientnum) {

	Client *client = &m_clients[clientnum];
	edict_t			*ent;
	int				edictnum = clientnum + 1;

	ent = m_progs->EDICT_NUM(edictnum);
	edictnum = clientnum + 1;

	client->set_active(true);
	client->m_edict = ent;
#if 0
	if (sv.loadgame)
		memcpy(client->spawn_parms, spawn_parms, sizeof(spawn_parms));
	else
#endif
	{
		// call the progs to get default spawn parms for the new client
		m_progs->program_execute(m_progs->m_global_struct->SetNewParms);
		for (int i = 0; i<NUM_SPAWN_PARMS; i++)
			client->m_spawn_parms[i] = (&m_progs->m_global_struct->parm1)[i];
	}
	send_server_info(client);
}

void Server::check_for_new_clients() {
	Net *net = Net::check_new_connections();
	if (net == 0) {
		return;
	}
	for (int i = 0; i < m_maxclients; i++) {
		if (m_clients[i].is_active()) {
			continue;
		}
		m_clients[i].set_connection(net);
		connect_client(i);
		break;
	}
}

void Server::read_client_move(Client *client) {
	NetBuffer *msg = client->m_netconnection->m_receiveMessage;
	usercmd_t *move = &client->m_cmd;

	int		i;
	vec3_t	angle;
	int		bits;

	// read ping time
	client->m_ping_times[client->m_num_pings%NUM_PING_TIMES]
		= m_time - msg->read_float();
	client->m_num_pings++;

	// read current angles	
	for (i = 0; i<3; i++)
		angle[i] = msg->read_angle();

	VectorCopy(angle, client->m_edict->v.v_angle);

	// read movement
	move->forwardmove = msg->read_short();
	move->sidemove = msg->read_short();
	move->upmove = msg->read_short();

	// read buttons
	bits = msg->read_byte();
	client->m_edict->v.button0 = (bits & 1);
	client->m_edict->v.button2 = (bits & 2) >> 1;

	i = msg->read_byte();
	if (i)
		client->m_edict->v.impulse = i;
}

void Server::prespawn(char *cmdline, void *p, ...) {
	Client *client = reinterpret_cast<Client *>(p);
	if (client->m_spawned) {
		printf("prespawn not valid -- allready spawned\n");
		return;
	}
	NetBuffer *msg = &client->m_msg;
	msg->write(m_signon);
	msg->write_byte(svc_signonnum);
	msg->write_byte(2);
	client->m_sendsignon = true;
}

void Server::god(char *cmdline, void *p, ...) {
	Client *client = reinterpret_cast<Client *>(p);
	edict_t *ent = client->m_edict;
	NetBuffer *msg = &client->m_msg;

	ent->v.flags = (int)ent->v.flags ^ FL_GODMODE;
	msg->write_byte(svc_print);
	if (!((int)ent->v.flags & FL_GODMODE))
		msg->write_string("godmode OFF\n");
	else
		msg->write_string("godmode ON\n");

}

void Server::fly(char *cmdline, void *p, ...) {
	Client *client = reinterpret_cast<Client *>(p);
	edict_t *ent = client->m_edict;
	NetBuffer *msg = &client->m_msg;

	msg->write_byte(svc_print);
	if (ent->v.movetype != MOVETYPE_FLY) {
		ent->v.movetype = MOVETYPE_FLY;
		msg->write_string("flymode ON\n");
	}
	else {
		ent->v.movetype = MOVETYPE_WALK;
		msg->write_string("flymode OFF\n");
	}

}

void Server::noclip(char *cmdline, void *p, ...) {
	Client *client = reinterpret_cast<Client *>(p);
	edict_t *ent = client->m_edict;
	NetBuffer *msg = &client->m_msg;

	if (m_progs->m_global_struct->deathmatch)
		return;

	msg->write_byte(svc_print);
	if (ent->v.movetype != MOVETYPE_NOCLIP)
	{
		//noclip_anglehack = true;
		ent->v.movetype = MOVETYPE_NOCLIP;
		msg->write_string("noclip ON\n");
	}
	else
	{
		ent->v.movetype = MOVETYPE_WALK;
		msg->write_string("noclip OFF\n");
	}
}

void Server::create(char *cmdline, void *p, ...) {
	cmdArgs cmd(cmdline);
	if (cmd.argc < 2) {
		return;
	}
	// look for the spawn function
	dfunction_t *func = m_progs->ED_FindFunction(cmd.argv[1]);
	if (!func)
	{
		host.printf("No spawn function for:\n");
		return;
	}
	edict_t *ent = m_progs->ED_Alloc();
	memset(&ent->v, 0, m_progs->m_programs.entityfields * 4);
	
	ddef_t *key = m_progs->ED_FindField("classname");
	if (!key)
	{
		host.printf("'%s' is not a field\n", "classname");
		return;
	}

	if (!m_progs->ED_ParseEpair((void *)&ent->v, key, cmd.argv[1])) {
		sys.error("ED_ParseEdict: parse error");
	}

	edict_t *ed = m_progs->find("info_player_start");
	if (ed) {
		VectorCopy(ed->v.origin, ent->v.origin);
	}

	m_progs->m_global_struct->self = m_progs->edict_to_prog(ent);
	if (cmd.argc > 2) {
		ent->v.spawnflags = atoi(cmd.argv[2]);
	}
	m_progs->program_execute(func - m_progs->m_functions);

	ent->baseline.modelindex = 0;
	ent->v.modelindex = model_index(m_progs->m_strings + ent->v.model);
}

extern "C" size_t getMemFree();

void Server::spawn(char *cmdline, void *p, ...) {
	Client *client = reinterpret_cast<Client *>(p);
	if (client->m_spawned) {
		printf("Spawn not valid -- allready spawned\n");
		return;
	}

	if (m_loadgame) {
		m_paused = false;
	} else {
		edict_t *ent = client->m_edict;
		memset(&ent->v, 0, m_progs->m_programs.entityfields * 4);
		ent->v.colormap = m_progs->NUM_FOR_EDICT(ent);
		ent->v.team = (client->m_colors & 15) + 1;
		ent->v.netname = client->m_name - m_progs->m_strings;

		// copy spawn parms out of the client_t

		for (int i = 0; i< NUM_SPAWN_PARMS; i++)
			(&m_progs->m_global_struct->parm1)[i] = client->m_spawn_parms[i];

		// call the spawn function

		m_progs->m_global_struct->time = m_time;
		m_progs->m_global_struct->self = m_progs->edict_to_prog(ent);
		m_progs->program_execute(m_progs->m_global_struct->ClientConnect);

		if ((sys.seconds() - client->m_netconnection->m_connecttime) <= m_time)
			printf("%s entered the game\n", client->m_name);

		m_progs->program_execute(m_progs->m_global_struct->PutClientInServer);
	}

	NetBuffer *msg = &client->m_msg;
	msg->clear();

	// send time of update
	msg->write_byte(svc_time);
	msg->write_float(m_time);

	for (int i = 0; i<m_maxclients; i++)
	{
		Client *client2 = &m_clients[i];
		msg->write_byte(svc_updatename);
		msg->write_byte(i);
		msg->write_string(client2->m_name);
		msg->write_byte(svc_updatefrags);
		msg->write_byte(i);
		msg->write_short(client2->m_old_frags);
		msg->write_byte(svc_updatecolors);
		msg->write_byte(i);
		msg->write_byte(client2->m_colors);
	}

	// send all current light styles
	for (int i = 0; i<MAX_LIGHTSTYLES; i++)
	{
		msg->write_byte(svc_lightstyle);
		msg->write_byte((char)i);
		msg->write_string(m_progs->m_lightstyles[i]);
	}

	//
	// send some stats
	//
	msg->write_byte(svc_updatestat);
	msg->write_byte(STAT_TOTALSECRETS);
	msg->write_long((int)m_progs->m_global_struct->total_secrets);

	msg->write_byte(svc_updatestat);
	msg->write_byte(STAT_TOTALMONSTERS);
	msg->write_long((int)m_progs->m_global_struct->total_monsters);

	msg->write_byte(svc_updatestat);
	msg->write_byte(STAT_SECRETS);
	msg->write_long((int)m_progs->m_global_struct->found_secrets);

	msg->write_byte(svc_updatestat);
	msg->write_byte(STAT_MONSTERS);
	msg->write_long((int)m_progs->m_global_struct->killed_monsters);


	//
	// send a fixangle
	// Never send a roll angle, because savegames can catch the server
	// in a state where it is expecting the client to correct the angle
	// and it won't happen if the game was just loaded, so you wind up
	// with a permanent head tilt
	edict_t *ent = m_progs->EDICT_NUM(1 + (client - m_clients));
	msg->write_byte(svc_setangle);
	for (int i = 0; i < 2; i++) {
		msg->write_angle(ent->v.angles[i]);
	}
	msg->write_angle(0);

	write_client_data_to_message(ent, msg);

	//SV_WriteClientdataToMessage(sv_player, &host_client->message);

	msg->write_byte(svc_signonnum);
	msg->write_byte(3);
	client->m_sendsignon = true;

	host.printf("pool  : %d used: %d free: %d\n", pool.size(), pool.used(), pool.size() - pool.used());
	host.printf("linear: %d used: %d free: %d\n", linear.size(), linear.used(), linear.size() - linear.used());
	host.printf("LINEAR  free: %dKB\n", (int)linearSpaceFree() / 1024);
	host.printf("REGULAR free: %dKB\n", (int)getMemFree() / 1024);
	host.printf("vbo_cb   : %d\n", vbo_cb);
	host.printf("vbo_tx   : %d\n", vbo_tx);
	host.printf("vbo_ls   : %d\n", vbo_ls);
	host.printf("mdl_cb   : %d\n", mdl_cb);
	host.printf("mdl_tx   : %d\n", mdl_tx);

}

bool Server::read_client_message(Client *client) {
	NetBuffer *msg = client->m_netconnection->m_receiveMessage;
	char *s;
	int ret;

	do {
		if (msg->pos() == 0 || msg->read_pos() == msg->pos()) {
			break;
		}
		int r = msg->read_byte();
		if (r == -1) {
			return false;
		}
		int len = msg->read_short();
		/*int skip = */msg->read_byte();

		int end = msg->read_pos() + len;

		do {
			if (len == 0 || !client->is_active()) {
				return false;
			}
			int cmd = msg->read_byte();

			//check end of message
			if (cmd == -1) {
				break;
			}
			switch (cmd)
			{
			case -1:
				//goto nextmsg;		// end of message

			default:
				printf("SV_ReadClientMessage: unknown command char\n");
				return false;

			case clc_nop:
				printf("clc_nop\n");
				break;

			case clc_stringcmd:
				s = msg->read_string();
				ret = 0;
				if (strncasecmp(s, "status", 6) == 0)
					ret = 1;
				else if (strncasecmp(s, "god", 3) == 0)
					ret = 1;
				else if (strncasecmp(s, "notarget", 8) == 0)
					ret = 1;
				else if (strncasecmp(s, "fly", 3) == 0)
					ret = 1;
				else if (strncasecmp(s, "name", 4) == 0)
					ret = 1;
				else if (strncasecmp(s, "noclip", 6) == 0)
					ret = 1;
				else if (strncasecmp(s, "create", 6) == 0)
					ret = 1;
				else if (strncasecmp(s, "say", 3) == 0)
					ret = 1;
				else if (strncasecmp(s, "say_team", 8) == 0)
					ret = 1;
				else if (strncasecmp(s, "tell", 4) == 0)
					ret = 1;
				else if (strncasecmp(s, "color", 5) == 0)
					ret = 1;
				else if (strncasecmp(s, "kill", 4) == 0)
					ret = 1;
				else if (strncasecmp(s, "pause", 5) == 0)
					ret = 1;
				else if (strncasecmp(s, "spawn", 5) == 0) {
					ret = 1;
				}
				else if (strncasecmp(s, "begin", 5) == 0) {
					ret = 1;
					client->m_spawned = true;
					break;
				}
				else if (strncasecmp(s, "prespawn", 8) == 0) {
					ret = 1;
				}
				else if (strncasecmp(s, "kick", 4) == 0)
					ret = 1;
				else if (strncasecmp(s, "ping", 4) == 0)
					ret = 1;
				else if (strncasecmp(s, "give", 4) == 0)
					ret = 1;

				if (ret == 2)
					Cbuf_InsertText(s);
				else if (ret == 1) {
					m_cmds.call(s, client);
					Cmd_ExecuteString(s, src_client);
				}
				else
					printf("%s tried to %s\n", client->m_name, s);
				break;

			case clc_disconnect:
				printf("SV_ReadClientMessage: client disconnected\n");
				return false;

			case clc_move:
				read_client_move(client);
				break;
			}

		} while (msg->read_pos() < end);
		//read alignment bytes
		while (msg->read_pos() & 0x3) {
			msg->read_byte();
		}
		//break;
	} while (1);


	client->m_netconnection->clear();

	return true;
}

void Server::user_friction(edict_t *ent, float *origin, float *velocity)
{
	float	*vel;
	float	speed, newspeed, control;
	vec3_t	start, stop;
	float	friction;
	trace_t	trace;

	vel = velocity;

	speed = sqrt(vel[0] * vel[0] + vel[1] * vel[1]);
	if (!speed)
		return;

	// if the leading edge is over a dropoff, increase friction
	start[0] = stop[0] = origin[0] + vel[0] / speed * 16;
	start[1] = stop[1] = origin[1] + vel[1] / speed * 16;
	start[2] = origin[2] + ent->v.mins[2];
	stop[2] = start[2] - 34;

	trace = SV_Move(start, vec3_origin, vec3_origin, stop, true, ent);

	if (trace.fraction == 1.0)
		friction = sv_friction.value*sv_edgefriction.value;
	else
		friction = sv_friction.value;

	// apply friction	
	control = speed < sv_stopspeed.value ? sv_stopspeed.value : speed;
	newspeed = speed - m_frametime*control*friction;

	if (newspeed < 0)
		newspeed = 0;
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}

void Server::accelerate(float *velocity, float *wishdir, float wishspeed)
{
	int			i;
	float		addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct(velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = sv_accelerate.value*m_frametime*wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i = 0; i<3; i++)
		velocity[i] += accelspeed*wishdir[i];
}

void Server::air_accelerate(float *velocity,float wishspeed, vec3_t wishveloc)
{
	int			i;
	float		addspeed, wishspd, accelspeed, currentspeed;

	wishspd = VectorNormalize(wishveloc);
	if (wishspd > 30)
		wishspd = 30;
	currentspeed = DotProduct(velocity, wishveloc);
	addspeed = wishspd - currentspeed;
	if (addspeed <= 0)
		return;
	//	accelspeed = sv_accelerate.value * host_frametime;
	accelspeed = sv_accelerate.value*wishspeed * m_frametime;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i = 0; i<3; i++)
		velocity[i] += accelspeed*wishveloc[i];
}

bool Server::close_enough(edict_t *ent, edict_t *goal, float dist)
{
	int		i;

	for (i = 0; i<3; i++)
	{
		if (goal->v.absmin[i] > ent->v.absmax[i] + dist)
			return false;
		if (goal->v.absmax[i] < ent->v.absmin[i] - dist)
			return false;
	}
	return true;
}

/*
=============
SV_CheckBottom

Returns false if any part of the bottom of the entity is off an edge that
is not a staircase.

=============
*/
int c_yes, c_no;

bool Server::check_bottom(edict_t *ent)
{
	vec3_t	mins, maxs, start, stop;
	trace_t	trace;
	int		x, y;
	float	mid, bottom;

	VectorAdd(ent->v.origin, ent->v.mins, mins);
	VectorAdd(ent->v.origin, ent->v.maxs, maxs);

	// if all of the points under the corners are solid world, don't bother
	// with the tougher checks
	// the corners must be within 16 of the midpoint
	start[2] = mins[2] - 1;
	for (x = 0; x <= 1; x++)
	for (y = 0; y <= 1; y++)
	{
		start[0] = x ? maxs[0] : mins[0];
		start[1] = y ? maxs[1] : mins[1];
		if (point_contents(start) != CONTENTS_SOLID)
			goto realcheck;
	}

	c_yes++;
	return true;		// we got out easy

realcheck:
	c_no++;
	//
	// check it for real...
	//
	start[2] = mins[2];

	// the midpoint must be within 16 of the bottom
	start[0] = stop[0] = (mins[0] + maxs[0])*0.5;
	start[1] = stop[1] = (mins[1] + maxs[1])*0.5;
	stop[2] = start[2] - 2 * STEPSIZE;
	trace = SV_Move(start, vec3_origin, vec3_origin, stop, true, ent);

	if (trace.fraction == 1.0)
		return false;
	mid = bottom = trace.endpos[2];

	// the corners must be within 16 of the midpoint	
	for (x = 0; x <= 1; x++)
	for (y = 0; y <= 1; y++)
	{
		start[0] = stop[0] = x ? maxs[0] : mins[0];
		start[1] = stop[1] = y ? maxs[1] : mins[1];

		trace = SV_Move(start, vec3_origin, vec3_origin, stop, true, ent);

		if (trace.fraction != 1.0 && trace.endpos[2] > bottom)
			bottom = trace.endpos[2];
		if (trace.fraction == 1.0 || mid - trace.endpos[2] > STEPSIZE)
			return false;
	}

	c_yes++;
	return true;
}

/*
=============
SV_movestep

Called by monster program code.
The move will be adjusted for slopes and stairs, but if the move isn't
possible, no move is done, false is returned, and
pr_global_struct->trace_normal is set to the normal of the blocking wall
=============
*/
bool Server::move_step(edict_t *ent, vec3_t move, bool relink)
{
	float		dz;
	vec3_t		oldorg, neworg, end;
	trace_t		trace;
	int			i;
	edict_t		*enemy;

	// try the move	
	VectorCopy(ent->v.origin, oldorg);
	VectorAdd(ent->v.origin, move, neworg);

	// flying monsters don't step up
	if ((int)ent->v.flags & (FL_SWIM | FL_FLY))
	{
		// try one move with vertical motion, then one without
		for (i = 0; i<2; i++)
		{
			VectorAdd(ent->v.origin, move, neworg);
			enemy = m_progs->prog_to_edict(ent->v.enemy);
			if (i == 0 && enemy != m_progs->m_edicts[0])
			{
				dz = ent->v.origin[2] - m_progs->prog_to_edict(ent->v.enemy)->v.origin[2];
				if (dz > 40)
					neworg[2] -= 8;
				if (dz < 30)
					neworg[2] += 8;
			}
			trace = SV_Move(ent->v.origin, ent->v.mins, ent->v.maxs, neworg, false, ent);

			if (trace.fraction == 1)
			{
				if (((int)ent->v.flags & FL_SWIM) && point_contents(trace.endpos) == CONTENTS_EMPTY)
					return false;	// swim monster left water

				VectorCopy(trace.endpos, ent->v.origin);
				if (relink)
					link_edict(ent, true);
				return true;
			}

			if (enemy == m_progs->m_edicts[0])
				break;
		}

		return false;
	}

	// push down from a step height above the wished position
	neworg[2] += STEPSIZE;
	VectorCopy(neworg, end);
	end[2] -= STEPSIZE * 2;

	trace = SV_Move(neworg, ent->v.mins, ent->v.maxs, end, false, ent);

	if (trace.allsolid)
		return false;

	if (trace.startsolid)
	{
		neworg[2] -= STEPSIZE;
		trace = SV_Move(neworg, ent->v.mins, ent->v.maxs, end, false, ent);
		if (trace.allsolid || trace.startsolid)
			return false;
	}
	if (trace.fraction == 1)
	{
		// if monster had the ground pulled out, go ahead and fall
		if ((int)ent->v.flags & FL_PARTIALGROUND)
		{
			VectorAdd(ent->v.origin, move, ent->v.origin);
			if (relink)
				link_edict(ent, true);
			ent->v.flags = (int)ent->v.flags & ~FL_ONGROUND;
			//	Con_Printf ("fall down\n"); 
			return true;
		}

		return false;		// walked off an edge
	}

	// check point traces down for dangling corners
	VectorCopy(trace.endpos, ent->v.origin);

	if (!check_bottom(ent))
	{
		if ((int)ent->v.flags & FL_PARTIALGROUND)
		{	// entity had floor mostly pulled out from underneath it
			// and is trying to correct
			if (relink)
				link_edict(ent, true);
			return true;
		}
		VectorCopy(oldorg, ent->v.origin);
		return false;
	}

	if ((int)ent->v.flags & FL_PARTIALGROUND)
	{
		//		Con_Printf ("back on ground\n"); 
		ent->v.flags = (int)ent->v.flags & ~FL_PARTIALGROUND;
	}
	ent->v.groundentity = m_progs->edict_to_prog(trace.ent);

	// the move is ok
	if (relink)
		link_edict(ent, true);
	return true;
}


bool Server::step_direction(edict_t *ent, float yaw, float dist)
{
	vec3_t		move, oldorigin;
	float		delta;

	ent->v.ideal_yaw = yaw;
	m_progs->PF_changeyaw();

	yaw = yaw*M_PI * 2 / 360;
	move[0] = cos(yaw)*dist;
	move[1] = sin(yaw)*dist;
	move[2] = 0;

	VectorCopy(ent->v.origin, oldorigin);
	if (move_step(ent, move, false))
	{
		delta = ent->v.angles[YAW] - ent->v.ideal_yaw;
		if (delta > 45 && delta < 315)
		{		// not turned far enough, so don't take the step
			VectorCopy(oldorigin, ent->v.origin);
		}
		link_edict(ent, true);
		return true;
	}
	link_edict(ent, true);

	return false;
}

void Server::fix_check_bottom(edict_t *ent)
{
	//	Con_Printf ("SV_FixCheckBottom\n");

	ent->v.flags = (int)ent->v.flags | FL_PARTIALGROUND;
}

#define	DI_NODIR	-1
void Server::new_chase_dir(edict_t *actor, edict_t *enemy, float dist)
{
	float		deltax, deltay;
	float			d[3];
	float		tdir, olddir, turnaround;

	olddir = anglemod((int)(actor->v.ideal_yaw / 45) * 45);
	turnaround = anglemod(olddir - 180);

	deltax = enemy->v.origin[0] - actor->v.origin[0];
	deltay = enemy->v.origin[1] - actor->v.origin[1];
	if (deltax>10)
		d[1] = 0;
	else if (deltax<-10)
		d[1] = 180;
	else
		d[1] = DI_NODIR;
	if (deltay<-10)
		d[2] = 270;
	else if (deltay>10)
		d[2] = 90;
	else
		d[2] = DI_NODIR;

	// try direct route
	if (d[1] != DI_NODIR && d[2] != DI_NODIR)
	{
		if (d[1] == 0)
			tdir = d[2] == 90 ? 45 : 315;
		else
			tdir = d[2] == 90 ? 135 : 215;

		if (tdir != turnaround && step_direction(actor, tdir, dist))
			return;
	}

	// try other directions
	if (((rand() & 3) & 1) || abs(deltay)>abs(deltax))
	{
		tdir = d[1];
		d[1] = d[2];
		d[2] = tdir;
	}

	if (d[1] != DI_NODIR && d[1] != turnaround
		&& step_direction(actor, d[1], dist))
		return;

	if (d[2] != DI_NODIR && d[2] != turnaround
		&& step_direction(actor, d[2], dist))
		return;

	/* there is no direct path to the player, so pick another direction */

	if (olddir != DI_NODIR && step_direction(actor, olddir, dist))
		return;

	if (rand() & 1) 	/*randomly determine direction of search*/
	{
		for (tdir = 0; tdir <= 315; tdir += 45)
		if (tdir != turnaround && step_direction(actor, tdir, dist))
			return;
	}
	else
	{
		for (tdir = 315; tdir >= 0; tdir -= 45)
		if (tdir != turnaround && step_direction(actor, tdir, dist))
			return;
	}

	if (turnaround != DI_NODIR && step_direction(actor, turnaround, dist))
		return;

	actor->v.ideal_yaw = olddir;		// can't move

	// if a bridge was pulled out from underneath a monster, it may not have
	// a valid standing position at all

	if (!check_bottom(actor))
		fix_check_bottom(actor);

}

void Server::move_to_goal(edict_t *ent, edict_t *goal, float dist)
{

	if (!((int)ent->v.flags & (FL_ONGROUND | FL_FLY | FL_SWIM)))
	{
		return;
	}

	// if the next step hits the enemy, return immediately

	if (m_progs->prog_to_edict(ent->v.enemy) != m_progs->m_edicts[0] && close_enough(ent, goal, dist))
		return;

	// bump around...
	if ((rand() & 3) == 1 ||
		!step_direction(ent, ent->v.ideal_yaw, dist))
	{
		new_chase_dir(ent, goal, dist);
	}
}

/*
===================
SV_AirMove

===================
*/
void Server::air_move(Client *client, usercmd_t &cmd, float *angles, float *origin, float *velocity, bool onground)
{
	edict_t *ent = client->m_edict;
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		forward, right, up;
	vec3_t		wishdir;
	float		wishspeed;

	AngleVectors(ent->v.angles, forward, right, up);

	fmove = cmd.forwardmove;
	smove = cmd.sidemove;

	// hack to not let you back into teleporter
	if (m_time < ent->v.teleport_time && fmove < 0)
		fmove = 0;

	for (i = 0; i<3; i++)
		wishvel[i] = forward[i] * fmove + right[i] * smove;

	if ((int)ent->v.movetype != MOVETYPE_WALK)
		wishvel[2] = cmd.upmove;
	else
		wishvel[2] = 0;

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	if (wishspeed > sv_maxspeed.value)
	{
		VectorScale(wishvel, sv_maxspeed.value / wishspeed, wishvel);
		wishspeed = sv_maxspeed.value;
	}

	if (ent->v.movetype == MOVETYPE_NOCLIP)
	{	// noclip
		VectorCopy(wishvel, velocity);
	}
	else if (onground)
	{
		user_friction(ent,origin,velocity);
		accelerate(velocity,wishdir,wishspeed);
	}
	else
	{	// not on ground, so little effect on velocity
		air_accelerate(velocity,wishspeed,wishvel);
	}
}

void Server::water_jump(edict_t *ent)
{
	if (m_time > ent->v.teleport_time
		|| !ent->v.waterlevel)
	{
		ent->v.flags = (int)ent->v.flags & ~FL_WATERJUMP;
		ent->v.teleport_time = 0;
	}
	ent->v.velocity[0] = ent->v.movedir[0];
	ent->v.velocity[1] = ent->v.movedir[1];
}

/*
===================
SV_WaterMove

===================
*/
void Server::water_move(edict_t *ent, usercmd_t &cmd,float *velocity)
{
	int		i;
	vec3_t	wishvel;
	float	speed, newspeed, wishspeed, addspeed, accelspeed;
	vec3_t forward, right, up;

	//
	// user intentions
	//
	AngleVectors(ent->v.v_angle, forward, right, up);

	for (i = 0; i<3; i++)
		wishvel[i] = forward[i] * cmd.forwardmove + right[i] * cmd.sidemove;

	if (!cmd.forwardmove && !cmd.sidemove && !cmd.upmove)
		wishvel[2] -= 60;		// drift towards bottom
	else
		wishvel[2] += cmd.upmove;

	wishspeed = Length(wishvel);
	if (wishspeed > sv_maxspeed.value)
	{
		VectorScale(wishvel, sv_maxspeed.value / wishspeed, wishvel);
		wishspeed = sv_maxspeed.value;
	}
	wishspeed *= 0.7f;

	//
	// water friction
	//
	speed = Length(velocity);
	if (speed)
	{
		newspeed = speed - m_frametime * speed * sv_friction.value;
		if (newspeed < 0)
			newspeed = 0;
		VectorScale(velocity, newspeed / speed, velocity);
	}
	else
		newspeed = 0;

	//
	// water acceleration
	//
	if (!wishspeed)
		return;

	addspeed = wishspeed - newspeed;
	if (addspeed <= 0)
		return;

	VectorNormalize(wishvel);
	accelspeed = sv_accelerate.value * wishspeed * m_frametime;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i = 0; i<3; i++)
		velocity[i] += accelspeed * wishvel[i];
}


/*
===================
SV_ClientThink

the move fields specify an intended velocity in pix/sec
the angle fields specify an exact angular motion in degrees
===================
*/
void Server::client_think(Client *client)
{
	usercmd_t	cmd;
	vec3_t		v_angle;
	edict_t *ent = client->m_edict;
	bool onground;
	float	*angles;
	float	*origin;
	float	*velocity;

	if (ent->v.movetype == MOVETYPE_NONE)
		return;

	if (((int)(ent->v.flags)) & FL_ONGROUND) {
		onground = true;
	}
	else {
		onground = false;
	}

	origin = ent->v.origin;
	velocity = ent->v.velocity;

	//DropPunchAngle();

	//
	// if dead, behave differently
	//
	if (ent->v.health <= 0)
		return;

	//
	// angles
	// show 1/3 the pitch angle and all the roll angle
	cmd = client->m_cmd;
	angles = ent->v.angles;

	VectorAdd(ent->v.v_angle, ent->v.punchangle, v_angle);
	//angles[ROLL] = V_CalcRoll(sv_player->v.angles, sv_player->v.velocity) * 4;
	if (!ent->v.fixangle)
	{
		angles[PITCH] = -v_angle[PITCH] / 3;
		angles[YAW] = v_angle[YAW];
	}

	if ((int)ent->v.flags & FL_WATERJUMP)
	{
		water_jump(ent);
		return;
	}
	//
	// walk
	//
	if ((ent->v.waterlevel >= 2)
		&& (ent->v.movetype != MOVETYPE_NOCLIP))
	{
		water_move(ent,cmd,velocity);
		return;
	}

	air_move(client,cmd,angles,origin,velocity,onground);
}

void Server::broadcast_printf(char *str, ...) {
}

void Server::pause() {
	m_paused ^= 1;
	if (m_paused)
	{
		//broadcast_printf("%s paused the game\n", m_progs->m_strings + sv_player->v.netname);
	}
	else
	{
		//broadcast_printf("%s unpaused the game\n", pr_strings + sv_player->v.netname);
	}

	// send notification to all clients
	m_reliable_datagram.write_byte(svc_setpause);
	m_reliable_datagram.write_byte(m_paused);
}

void Server::run_clients() {
	for (int i = 0; i < m_maxclients; i++) {
		Client *client = &m_clients[i];

		if (!client->m_active) {
			continue;
		}

		if (!read_client_message(client)) {
			drop_client(client, false);
			continue;
		}

		if (!client->m_spawned) {
			memset(&client->m_cmd, 0, sizeof(client->m_cmd));
			continue;
		}

		if (!m_paused && (m_maxclients > 1)) {// || key_dest == key_game)) {
			client_think(client);
		}
	}
}

void Server::update_to_reliable_messages() {

	// check for changes to be sent over the reliable streams
	for (int i = 0; i < m_maxclients; i++) {
		Client *host_client = &m_clients[i];
		if (host_client->m_old_frags != host_client->m_edict->v.frags) {
			for (int j = 0; j < m_maxclients; j++) {
				Client *client = &m_clients[j];
				if (!client->is_active()) {
					continue;
				}
				client->m_msg.write_byte(svc_updatefrags);
				client->m_msg.write_byte(i);
				client->m_msg.write_short(host_client->m_edict->v.frags);
			}
			host_client->m_old_frags = host_client->m_edict->v.frags;
		}
	}
	for (int j = 0; j < m_maxclients; j++) {
		Client *client = &m_clients[j];
		if (!client->is_active()) {
			continue;
		}
		client->m_msg.write(m_reliable_datagram);
	}
	m_reliable_datagram.clear();
}

/*
=============================================================================

The PVS must include a small area around the client to allow head bobbing
or other small motion on the client side.  Otherwise, a bob might cause an
entity that should be visible to not show up, especially when the bob
crosses a waterline.

=============================================================================
*/

int		fatbytes;
byte	fatpvs[8192 / 8];

void Server::SV_AddToFatPVS(vec3_t org, q1_bsp_node *node)
{
	int			i;
	byte		*pvs;
	q1_plane	*plane;
	vec3_fixed16 forg;
	fixed32p16 dd;
	
	forg = org;

	while (1)
	{
		// if this is a leaf, accumulate the pvs bits
		if (node->m_contents < 0)
		{
			if (node->m_contents != CONTENTS_SOLID)
			{
				q1_leaf_node *leaf = reinterpret_cast < q1_leaf_node *>(node);
				pvs = m_worldmodel->decompress_vis(leaf->m_compressed_vis);
				for (i = 0; i<fatbytes; i++)
					fatpvs[i] |= pvs[i];
			}
			return;
		}

		plane = node->m_plane;
		dd = (*plane) * forg;
		if (dd > fixed32p16(8<<16))
			node = node->m_children[0];
		else if (dd < fixed32p16(-8 << 16))
			node = node->m_children[1];
		else
		{	// go down both
			SV_AddToFatPVS(org, node->m_children[0]);
			node = node->m_children[1];
		}
	}
}

/*
=============
SV_FatPVS

Calculates a PVS that is the inclusive or of all leafs within 8 pixels of the
given point.
=============
*/
byte *Server::SV_FatPVS(vec3_t org)
{
	fatbytes = (m_worldmodel->m_num_leafs + 31) >> 3;
	memset(fatpvs, 0, fatbytes);
	SV_AddToFatPVS(org, m_worldmodel->m_nodes);
	return fatpvs;
}

void Server::SV_PVS(vec3_t org, byte *pvs)
{
	fatbytes = (m_worldmodel->m_num_leafs + 31) >> 3;
	q1_leaf_node *leaf = m_worldmodel->in_leaf();
	byte *p = m_worldmodel->decompress_vis(leaf->m_compressed_vis);
	memcpy(pvs, p, fatbytes);
}


void Server::write_entities_to_client(edict_t	*clent, NetBuffer *msg)
{
	int		e, i;
	int		bits;
	byte	*pvs;
	vec3_t	org;
	float	miss;
	edict_t	*ent;

	// find the client's PVS
	VectorAdd(clent->v.origin, clent->v.view_ofs, org);
	pvs = SV_FatPVS(org);

	// send over all entities (excpet the client) that touch the pvs
	ent = m_progs->NEXT_EDICT(m_progs->m_edicts[0]);
	for (e = 1; e<m_progs->m_num_edicts; e++, ent = m_progs->NEXT_EDICT(ent))
	{
		// ignore if not touching a PV leaf
		if (ent != clent)	// clent is ALLWAYS sent
		{
			// ignore ents without visible models
			if (!ent->v.modelindex || !m_progs->m_strings[ent->v.model]) {
				continue;
			}

			for (i = 0; i < ent->num_leafs; i++) {
				if (pvs[ent->leafnums[i] >> 3] & (1 << (ent->leafnums[i] & 7))) {
					break;
				}
			}

			if (i == ent->num_leafs) {
				continue;		// not visible
			}

		}

		if (msg->len() - msg->pos() < 16)
		{
			printf("packet overflow\n");
			return;
		}

		// send an update
		bits = 0;

		for (i = 0; i<3; i++)
		{
			miss = ent->v.origin[i] - ent->baseline.origin[i];
			if (miss < -0.1 || miss > 0.1)
				bits |= U_ORIGIN1 << i;
		}

		if (ent->v.angles[0] != ent->baseline.angles[0])
			bits |= U_ANGLE1;

		if (ent->v.angles[1] != ent->baseline.angles[1])
			bits |= U_ANGLE2;

		if (ent->v.angles[2] != ent->baseline.angles[2])
			bits |= U_ANGLE3;

		if (ent->v.movetype == MOVETYPE_STEP)
			bits |= U_NOLERP;	// don't mess up the step animation

		if (ent->baseline.colormap != ent->v.colormap)
			bits |= U_COLORMAP;

		if (ent->baseline.skin != ent->v.skin)
			bits |= U_SKIN;

		if (ent->baseline.frame != ent->v.frame)
			bits |= U_FRAME;

		if (ent->baseline.effects != ent->v.effects)
			bits |= U_EFFECTS;

		if (ent->baseline.modelindex != ent->v.modelindex)
			bits |= U_MODEL;

		if (e >= 256)
			bits |= U_LONGENTITY;

		if (bits >= 256)
			bits |= U_MOREBITS;

		//
		// write the message
		//
		msg->write_byte(bits | U_SIGNAL);

		if (bits & U_MOREBITS)
			msg->write_byte(bits >> 8);
		if (bits & U_LONGENTITY)
			msg->write_short(e);
		else
			msg->write_byte(e);

		if (bits & U_MODEL)
			msg->write_byte(ent->v.modelindex);
		if (bits & U_FRAME)
			msg->write_byte(ent->v.frame);
		if (bits & U_COLORMAP)
			msg->write_byte(ent->v.colormap);
		if (bits & U_SKIN)
			msg->write_byte(ent->v.skin);
		if (bits & U_EFFECTS)
			msg->write_byte(ent->v.effects);
		if (bits & U_ORIGIN1)
			msg->write_coord(ent->v.origin[0]);
		if (bits & U_ANGLE1)
			msg->write_angle(ent->v.angles[0]);
		if (bits & U_ORIGIN2)
			msg->write_coord(ent->v.origin[1]);
		if (bits & U_ANGLE2)
			msg->write_angle(ent->v.angles[1]);
		if (bits & U_ORIGIN3)
			msg->write_coord(ent->v.origin[2]);
		if (bits & U_ANGLE3)
			msg->write_angle(ent->v.angles[2]);
	}
}

bool Server::send_client_datagram(Client *client) {
	NetBufferFixed<MAX_DATAGRAM> msg;

	msg.write_byte(svc_time);
	msg.write_float((float)m_time);

	// add the client specific data to the datagram
	write_client_data_to_message(client->m_edict, &msg);

	write_entities_to_client(client->m_edict, &msg);

	// copy the server datagram if there is space
	msg.write(m_datagram);

	// send the datagram
	if(client->m_netconnection->send_unreliable_message(msg) == -1) {
		drop_client(client,true);// if the message couldn't send, kick off
		return false;
	}

	return true;
}

void Server::drop_client(Client *client, bool crash) {
	if (!crash) {
		if (client->m_netconnection->can_send()) {
			client->m_msg.write_byte(svc_disconnect);
			client->m_netconnection->send_message(client->m_msg);
		}
		if (client->m_edict && client->m_spawned)
		{
			// call the prog function for removing a client
			// this will set the body to a dead frame, among other things
			int saveSelf = m_progs->m_global_struct->self;
			m_progs->m_global_struct->self = m_progs->edict_to_prog(client->m_edict);
			m_progs->program_execute(m_progs->m_global_struct->ClientDisconnect);
			m_progs->m_global_struct->self = saveSelf;
		}

		host.printf("Client %s removed\n", client->m_name);
	}

	client->m_netconnection->close();
	client->m_netconnection = 0;
	
	// free the client (the body stays around)
	client->m_active = false;
	client->m_name[0] = 0;
	client->m_old_frags = -999999;

	for (int i = 0; i < m_maxclients; i++) {
		Client *client = &m_clients[i];

		if (!client->is_active()) {
			continue;
		}
		client->m_msg.write_byte(svc_updatename);
		client->m_msg.write_byte(i);
		client->m_msg.write_string("");
		
		client->m_msg.write_byte(svc_updatefrags);
		client->m_msg.write_byte(i);
		client->m_msg.write_short(0);

		client->m_msg.write_byte(svc_updatecolors);
		client->m_msg.write_byte(i);
		client->m_msg.write_byte(0);

	}

}

int Server::send_to_all(NetBuffer &msg, int blocktime) {
	double		start;
	int			i;
	int			count = 0;
	bool		state1[MAX_SCOREBOARD];
	bool		state2[MAX_SCOREBOARD];

	for (i = 0; i < m_maxclients; i++) {
		Client *client = &m_clients[i];
		Net *net = client->m_netconnection;
		if (client->is_active())
		{
			if (net)
			{
				net->send_message(msg);
				state1[i] = true;
				state2[i] = true;
				continue;
			}
			count++;
			state1[i] = false;
			state2[i] = false;
		}
		else
		{
			state1[i] = true;
			state2[i] = true;
		}
	}

	start = sys.seconds();
	while (count)
	{
		count = 0;
		for (i = 0; i < m_maxclients; i++) {
			Client *client = &m_clients[i];
			Net *net = client->m_netconnection;
			if (!state1[i])
			{
				if (net->can_send())
				{
					state1[i] = true;
					net->send_message(msg);
				}
				else
				{
					//NET_GetMessage(host_client->netconnection);
				}
				count++;
				continue;
			}

			if (!state2[i])
			{
				if (net->can_send())
				{
					state2[i] = true;
				}
				else
				{
					//NET_GetMessage(host_client->netconnection);
				}
				count++;
				continue;
			}
		}
		if ((sys.seconds() - start) > blocktime)
			break;
	}

	return count;
}

void Server::send_reconnect() {
	//fixme
	//dont send in single player as this causes the single netcon buffer to 
	//mix messages with client and server
	if (!m_active) {
		NetBufferFixed<32> msg;

		msg.write_char(svc_stufftext);
		msg.write_string("reconnect\n");

		send_to_all(msg,5);
	}
	//if (cls.state != ca_dedicated)
	host.execute("reconnect\n");
}


void Server::send_nop(Client *client) {
	NetBufferFixed<4> msg;

	msg.write_char(svc_nop);
	if (client->m_netconnection->send_unreliable_message(msg) == -1) {
		drop_client(client, true);
	}
}

void Server::cleanup_ents() {
	edict_t *check = m_progs->NEXT_EDICT(m_progs->m_edicts[0]);
	for (int e = 1; e < m_progs->m_num_edicts; e++, check = m_progs->NEXT_EDICT(check))
	{
		check->v.effects = (int)check->v.effects & ~EF_MUZZLEFLASH;
	}
}

void Server::send_client_messages() {

	update_to_reliable_messages();

	for (int i = 0; i < m_maxclients; i++) {
		Client *client = &m_clients[i];
		if (!client->is_active()) {
			continue;
		}

		if (client->m_spawned) {
			if (!send_client_datagram(client)) {
				continue;
			}
		}
		else {
			// the player isn't totally in the game yet
			// send small keepalive messages if too much time has passed
			// send a full message when the next signon stage has been requested
			// some other message data (name changes, etc) may accumulate 
			// between signon stages
			if (!client->m_sendsignon)
			{
				if (host.real_time() - client->m_netconnection->last_send() > 5)
					send_nop(client);
				continue;	// don't send out non-signon messages
			}
		}
		// check for an overflowed message.  Should only happen
		// on a very fucked up connection that backs up a lot, then
		// changes level
		/*if (host_client->message.overflowed)
		{
			SV_DropClient(true);
			host_client->message.overflowed = false;
			continue;
		}*/

		if (client->m_msg.pos() || client->m_dropasap)
		{
			if (!client->m_netconnection->can_send())
			{
				//				I_Printf ("can't write\n");
				continue;
			}

			if (client->m_dropasap)
				drop_client(client,false);	// went to another level
			else
			{
				if (client->m_netconnection->send_message(client->m_msg) == -1)
					drop_client(client,true);	// if the message couldn't send, kick off
				client->m_msg.clear();
				client->m_sendsignon = false;
			}
		}
	}

	cleanup_ents();
}

void Server::save_spawn_parms() {
	m_serverflags = m_progs->m_global_struct->serverflags;
	for (int i = 0; i < m_maxclients; i++) {
		Client *client = &m_clients[i];

		if (!client->m_active) {
			continue;
		}
		// call the progs to get default spawn parms for the new client
		m_progs->m_global_struct->self = m_progs->edict_to_prog(client->m_edict);
		m_progs->program_execute(m_progs->m_global_struct->SetChangeParms);
		for (int j = 0; j<NUM_SPAWN_PARMS; j++)
			client->m_spawn_parms[j] = (&m_progs->m_global_struct->parm1)[j];
	}
}

void ED_ResetPool();
void Server::spawn_server(char *server) {
	//mem counters
	vbo_cb = 0;
	vbo_tx = 0;
	vbo_ls = 0;
	mdl_cb = 0;
	mdl_tx = 0;
	host.clear();
	host.dprintf("free mem : %dKB LINEAR, %dKB REGULAR\n", (int)linearSpaceFree() / 1024, (int)getMemFree() / 1024);
	
	printf("spawn server: %s\n", server);
	changelevel_issued = false;

	//
	// tell all connected clients that we are going to a new level
	//
	if (m_active) {
		send_reconnect();
	}

	//
	// make cvars consistant
	//
	if (coop.value)
		Cvar_SetValue("deathmatch", 0);
	m_current_skill = (int)(skill.value + 0.5);
	if (m_current_skill < 0)
		m_current_skill = 0;
	if (m_current_skill > 3)
		m_current_skill = 3;

	Cvar_SetValue("skill", (float)m_current_skill);

	//
	// set up the new server
	//
	host.clear();
	ED_ResetPool();

	strcpy(m_name, server);

	printf("loading programs...\n");
	m_progs->load();

	m_datagram.clear();
	m_reliable_datagram.clear();
	m_signon.clear();

	host.dprintf("alloc edicts\n");
	edict_t		*ent = m_progs->ED_Alloc();
	for (int i = 0; i<m_maxclients; i++)
	{
		ent = m_progs->ED_Alloc();
		m_clients[i].m_edict = ent;
	}

	m_state = ss_loading;
	m_paused = false;


	m_time = 1.0f;

	printf("loading world...\n");
	strcpy(m_name, server);
	sprintf(m_modelname, "maps/%s.bsp", server);
	q1Bsp *q1 = m_worldmodel = reinterpret_cast<q1Bsp*>(Models.find(m_modelname));

	if (!m_worldmodel)
	{
		host.printf("Couldn't spawn server %s\n", m_modelname);
		m_active = false;
		return;
	}
	host.dprintf("world loaded\n");
	ClearWorld();

	printf("loading models...\n");
	memset(m_model_precache, 0, sizeof(m_model_precache));
	m_model_precache[0] = m_progs->m_strings;
	m_model_precache[1] = m_modelname;
	m_models[1] = q1;
	for (int i = 1; i < q1->num_submodels(); i++) {
		m_model_precache[1 + i] = new(pool) char[5];
		sprintf(m_model_precache[1 + i], "*%i", i);
		m_models[i + 1] = Models.find(m_model_precache[1 + i]);
	}

	host.dprintf("models loaded\n");

	memset(m_sound_precache, 0, sizeof(m_sound_precache));
	m_sound_precache[0] = m_progs->m_strings;



	printf("loading entities...\n");
	//
	// load the rest of the entities
	//	
	ent = m_progs->EDICT_NUM(0);
	memset(&ent->v, 0, m_progs->m_programs.entityfields * 4);
	ent->free = false;
	ent->v.model = m_worldmodel->name() - m_progs->m_strings;
	ent->v.modelindex = 1;		// world model
	ent->v.solid = SOLID_BSP;
	ent->v.movetype = MOVETYPE_PUSH;

	if (coop.value)
		m_progs->m_global_struct->coop = coop.value;
	else
		m_progs->m_global_struct->deathmatch = deathmatch.value;

	m_progs->m_global_struct->mapname = m_name - m_progs->m_strings;

	// serverflags are for cross level information (sigils)
	m_progs->m_global_struct->serverflags = m_serverflags;

	char *entities = q1->load_entities();
	if (!entities)
	{
		host.printf("Couldn't load entities for world model %s\n", m_modelname);
		m_active = false;
		return;
	}

	printf("spawning entities...\n");
	ed_parser parser;
	parser.load(m_progs,entities);
	delete[] entities;
	host.dprintf("entities loaded\n");

	m_active = true;

	// all setup is completed, any further precache statements are errors
	m_state = ss_active;

	printf("starting physics...\n");
	physics(0.1f);
	physics(0.1f);

	host.dprintf("creating baseline\n");
	create_baseline();

	printf("connecting clients...\n");
	// send serverinfo to all connected clients
	for (int i = 0; i < m_maxclients; i++) {
		Client *client = &m_clients[i];
		if (client->m_active) {
			send_server_info(client);
		}
	}

	printf("server spawned\n");
	host.printf("server spawned\n");
}

int Server::model_index(char *name) {
	int		i;

	if (!name || !name[0])
		return 0;

	for (i = 0; i<MAX_MODELS && m_model_precache[i]; i++)
	if (!strcmp(m_model_precache[i], name))
		return i;
	if (i == MAX_MODELS || !m_model_precache[i])
		sys.error("SV_ModelIndex: model %s not precached", name);
	return i;
}

void Server::create_baseline() {
	int			i;
	edict_t			*svent;
	int				entnum;

	for (entnum = 0; entnum < m_progs->m_num_edicts; entnum++)
	{
		// get the current server version
		svent = m_progs->EDICT_NUM(entnum);
		if (svent->free)
			continue;
		if (entnum > m_maxclients && !svent->v.modelindex)
			continue;

		//
		// create entity baseline
		//
		VectorCopy(svent->v.origin, svent->baseline.origin);
		VectorCopy(svent->v.angles, svent->baseline.angles);
		svent->baseline.frame = svent->v.frame;
		svent->baseline.skin = svent->v.skin;
		if (entnum > 0 && entnum <= m_maxclients)
		{
			svent->baseline.colormap = entnum;
			svent->baseline.modelindex = model_index("progs/player.mdl");
		}
		else
		{
			svent->baseline.colormap = 0;
			svent->baseline.modelindex = model_index(m_progs->m_strings + svent->v.model);
		}

		//
		// add to the message
		//
		m_signon.write_byte(svc_spawnbaseline);
		m_signon.write_short(entnum);

		m_signon.write_byte(svent->baseline.modelindex);
		m_signon.write_byte(svent->baseline.frame);
		m_signon.write_byte(svent->baseline.colormap);
		m_signon.write_byte(svent->baseline.skin);
		for (i = 0; i<3; i++)
		{
			m_signon.write_coord(svent->baseline.origin[i]);
			m_signon.write_angle(svent->baseline.angles[i]);
		}
	}
}

void Server::write_client_data_to_message(edict_t *ent, NetBuffer *msg) {
	int		bits;
	int		i;
	edict_t	*other;
	int		items;
	eval_t	*val;

	//
	// send a damage message
	//
	if (ent->v.dmg_take || ent->v.dmg_save)
	{
		other = m_progs->prog_to_edict(ent->v.dmg_inflictor);
		msg->write_byte(svc_damage);
		msg->write_byte(ent->v.dmg_save);
		msg->write_byte(ent->v.dmg_take);
		for (i = 0; i < 3; i++) {
			msg->write_coord(other->v.origin[i] + 0.5*(other->v.mins[i] + other->v.maxs[i]));
		}

		ent->v.dmg_take = 0;
		ent->v.dmg_save = 0;
	}

	//
	// send the current viewpos offset from the view entity
	//
	//SV_SetIdealPitch ();		// how much to look up / down ideally

	// a fixangle might get lost in a dropped packet.  Oh well.
	if (ent->v.fixangle)
	{
		msg->write_byte(svc_setangle);
		for (i = 0; i < 3; i++) {
			msg->write_angle(ent->v.angles[i]);
		}
		ent->v.fixangle = 0;
	}

	bits = 0;

	if (ent->v.view_ofs[2] != DEFAULT_VIEWHEIGHT)
		bits |= SU_VIEWHEIGHT;

	if (ent->v.idealpitch)
		bits |= SU_IDEALPITCH;

	// stuff the sigil bits into the high bits of items for sbar, or else
	// mix in items2
	val = m_progs->GetEdictFieldValue(ent, "items2");

	if (val)
		items = (int)ent->v.items | ((int)val->_float << 23);
	else
		items = (int)ent->v.items | ((int)m_progs->m_global_struct->serverflags << 28);

	bits |= SU_ITEMS;

	if ((int)ent->v.flags & FL_ONGROUND)
		bits |= SU_ONGROUND;

	if (ent->v.waterlevel >= 2)
		bits |= SU_INWATER;

	for (i = 0; i<3; i++)
	{
		if (ent->v.punchangle[i])
			bits |= (SU_PUNCH1 << i);
		if (ent->v.velocity[i])
			bits |= (SU_VELOCITY1 << i);
	}

	if (ent->v.weaponframe)
		bits |= SU_WEAPONFRAME;

	if (ent->v.armorvalue)
		bits |= SU_ARMOR;

	//	if (ent->v.weapon)
	bits |= SU_WEAPON;

	// send the data

	msg->write_byte(svc_clientdata);
	msg->write_short(bits);

	if (bits & SU_VIEWHEIGHT)
		msg->write_char(ent->v.view_ofs[2]);

	if (bits & SU_IDEALPITCH)
		msg->write_char(ent->v.idealpitch);

	for (i = 0; i<3; i++)
	{
		if (bits & (SU_PUNCH1 << i))
			msg->write_char(ent->v.punchangle[i]);
		if (bits & (SU_VELOCITY1 << i))
			msg->write_char(ent->v.velocity[i] / 16);
	}

	// [always sent]	if (bits & SU_ITEMS)
	msg->write_long(items);

	if (bits & SU_WEAPONFRAME)
		msg->write_byte(ent->v.weaponframe);
	if (bits & SU_ARMOR)
		msg->write_byte(ent->v.armorvalue);
	if (bits & SU_WEAPON)
		msg->write_byte(model_index(m_progs->m_strings + ent->v.weaponmodel));

	msg->write_short(ent->v.health);
	msg->write_byte(ent->v.currentammo);
	msg->write_byte(ent->v.ammo_shells);
	msg->write_byte(ent->v.ammo_nails);
	msg->write_byte(ent->v.ammo_rockets);
	msg->write_byte(ent->v.ammo_cells);

	if (true)//standard_quake)
	{
		msg->write_byte(ent->v.weapon);
	}
	else
	{
		for (i = 0; i<32; i++)
		{
			if (((int)ent->v.weapon) & (1 << i))
			{
				msg->write_byte(i);
				break;
			}
		}
	}
}

void Server::start_particle(vec3_t org, vec3_t dir, int color, int count)
{
	int		i, v;

	if (m_datagram.pos() > MAX_DATAGRAM - 16)
		return;
	m_datagram.write_byte(svc_particle);
	m_datagram.write_coord(org[0]);
	m_datagram.write_coord(org[1]);
	m_datagram.write_coord(org[2]);
	for (i = 0; i<3; i++)
	{
		v = dir[i] * 16;
		if (v > 127)
			v = 127;
		else if (v < -128)
			v = -128;
		m_datagram.write_char(v);
	}
	m_datagram.write_byte(count);
	m_datagram.write_byte(color);
}

void Server::shutdown() {
	int		i;
	int		count;
	double	start;

	if (!m_active)
		return;

	m_active = false;


	// flush any pending messages - like the score!!!
	start = sys.seconds();
	do
	{
		count = 0;
		for (i = 0; i < m_maxclients;i++)
		{
			Client *host_client = &m_clients[i];
			if (host_client->m_active && host_client->m_msg.pos())
			{
				if (host_client->m_netconnection->can_send())
				{
					host_client->m_netconnection->send_message(host_client->m_msg);
					host_client->m_msg.clear();
				}
				else
				{
					//NET_GetMessage(host_client->netconnection);
					count++;
				}
			}
		}
		if ((sys.seconds() - start) > 3.0)
			break;
	} while (count);

	// make sure all the clients know we're disconnecting
	NetBufferFixed<4> msg;
	msg.write_byte(svc_disconnect);
	count = send_to_all(msg, 5);
	if (count)
		host.printf("Host_ShutdownServer: NET_SendToAll failed for %u clients\n", count);

	for (i = 0; i < m_maxclients; i++) {
		Client *client = &m_clients[i];
		if (client->m_active)
			drop_client(client,false);
	}

	//
	// clear structures
	//
	//memset(&sv, 0, sizeof(sv));
	//memset(svs.clients, 0, svs.maxclientslimit*sizeof(client_t));

}

void Server::frame(float frametime) {

	if (!m_active) {
		return;
	}

	clear_datagram();

	check_for_new_clients();

	run_clients();

	physics(frametime);

	send_client_messages();
}


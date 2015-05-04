#include "ClientState.h"
#include "protocol.h"
#include "Host.h"
#include "Sound.h"

cvar_t	cl_upspeed = { "cl_upspeed", "200" };
cvar_t	cl_forwardspeed = { "cl_forwardspeed", "200", true };
cvar_t	cl_backspeed = { "cl_backspeed", "200", true };
cvar_t	cl_sidespeed = { "cl_sidespeed", "350" };

cvar_t	cl_movespeedkey = { "cl_movespeedkey", "2.0" };

cvar_t	cl_yawspeed = { "cl_yawspeed", "140" };
cvar_t	cl_pitchspeed = { "cl_pitchspeed", "150" };

cvar_t	cl_anglespeedkey = { "cl_anglespeedkey", "1.5" };

cvar_t	m_pitch = { "m_pitch", "0.022", true };
cvar_t	m_yaw = { "m_yaw", "0.022", true };
cvar_t	m_forward = { "m_forward", "1", true };
cvar_t	m_side = { "m_side", "0.8", true };

cvar_t	cl_bob = { "cl_bob", "0.02", false };
cvar_t	cl_bobcycle = { "cl_bobcycle", "0.6", false };
cvar_t	cl_bobup = { "cl_bobup", "0.5", false };

cvar_t	cl_rollspeed = { "cl_rollspeed", "200" };
cvar_t	cl_rollangle = { "cl_rollangle", "2.0" };

cvar_t	v_kicktime = { "v_kicktime", "0.5", false };
cvar_t	v_kickroll = { "v_kickroll", "0.6", false };
cvar_t	v_kickpitch = { "v_kickpitch", "0.6", false };

cvar_t	dynamic_lights = { "dynamic_lights", "1", false };

cvar_t	cstick_sensitivity = { "cstick_sensitivity", "0.6", true };
cvar_t	nub_sensitivity = { "nub_sensitivity", "0.6", true };

ClientState::ClientState() {
	m_signon = 0;
	m_state = ca_disconnected;
	m_demoplayback = false;
	m_viewangles[0] = m_viewangles[1] = m_viewangles[2] = 0.0f;
	m_mtime[0] = m_mtime[1] = 0.0f;
	m_movemessages = 0;
	m_time = m_oldtime = 1.0f;
	m_last_received_message = 0.0f;
	m_netcon = 0;
	m_in_impulse = 0;
	m_framecount = 1;
	m_evh.bind("UPARROW", "forward");
	m_evh.bind("DOWNARROW", "backward");
	m_evh.bind("LEFTARROW", "strafeleft");
	m_evh.bind("RIGHTARROW", "straferight");
	m_evh.bind("b", "jump");
	m_evh.bind("l", "attack");
	m_evh.bind("r", "speed");

	Cvar_RegisterVariable(&cl_upspeed);
	Cvar_RegisterVariable(&cl_forwardspeed);
	Cvar_RegisterVariable(&cl_backspeed);
	Cvar_RegisterVariable(&cl_sidespeed);
	Cvar_RegisterVariable(&cl_movespeedkey);
	Cvar_RegisterVariable(&cl_yawspeed);
	Cvar_RegisterVariable(&cl_pitchspeed);
	Cvar_RegisterVariable(&cl_anglespeedkey);
	Cvar_RegisterVariable(&m_pitch);
	Cvar_RegisterVariable(&m_yaw);
	Cvar_RegisterVariable(&m_forward);
	Cvar_RegisterVariable(&m_side);

	Cvar_RegisterVariable(&cl_bob);
	Cvar_RegisterVariable(&cl_bobcycle);
	Cvar_RegisterVariable(&cl_bobup);

	Cvar_RegisterVariable(&cl_rollspeed);
	Cvar_RegisterVariable(&cl_rollangle);
	Cvar_RegisterVariable(&v_kicktime);
	Cvar_RegisterVariable(&v_kickroll);
	Cvar_RegisterVariable(&v_kickpitch);

	Cvar_RegisterVariable(&dynamic_lights);

	Cvar_RegisterVariable(&cstick_sensitivity);
	Cvar_RegisterVariable(&nub_sensitivity);
}

void ClientState::bind_key(char *key, char *action) {
	if (m_evh.bind(key, action)) {
		host.printf("invalid bind: %s %s\n", key, action);
	}
}

void ClientState::disconnect() {

	if (m_state == ca_connected) {
		m_message.clear();
		m_message.write_byte(clc_disconnect);
		m_netcon->send_unreliable_message(m_message);
		m_message.clear();
		m_netcon->close();

		m_state = ca_disconnected;
		if (host.server_active())
			host.shutdown_server();
	}
	m_signon = 0;
	m_mixer.stop_all();
	m_music.stop();
}

void  ClientState::adjust_angles() {
	//TODO adjust angle based on input
}

void  ClientState::base_move(usercmd_t &cmd) {
	if (m_signon != SIGNONS) {
		return;
	}
	adjust_angles();

	memset(&cmd, 0, sizeof(cmd));

	//TODO adjust movement based on input
	if (m_evh.actionState(ACTION_FORWARD)) {
		cmd.forwardmove += cl_forwardspeed.value;
	}
	if (m_evh.actionState(ACTION_BACKWARD)) {
		cmd.forwardmove -= cl_backspeed.value;
	}
	if (m_evh.actionState(ACTION_STRAFE_RIGHT)) {
		cmd.sidemove += cl_sidespeed.value;
	}
	if (m_evh.actionState(ACTION_STRAFE_LEFT)) {
		cmd.sidemove -= cl_sidespeed.value;
	}
	if (m_evh.actionState(ACTION_SPEED)) {
		cmd.forwardmove *= cl_movespeedkey.value;
		cmd.sidemove *= cl_movespeedkey.value;
		cmd.upmove *= cl_movespeedkey.value;
	}
}

touchPosition	g_lastTouch = { 0, 0 };
touchPosition	g_currentTouch = { 0, 0 };

void  ClientState::in_move(usercmd_t &cmd) {
	if (m_signon != SIGNONS) {
		return;
	}
#ifdef WIN32
	int pos[4];
	POINT current_pos;
	int mx, my;

	sys.get_position(pos);
	int window_center_x = pos[0] + pos[2] / 2;
	int window_center_y = pos[1] + pos[3] / 2;
	GetCursorPos(&current_pos);
	mx = current_pos.x - window_center_x;
	my = current_pos.y - window_center_y;

	m_viewangles[YAW] -= mx * m_yaw.value;

	m_viewangles[PITCH] += m_pitch.value * my;
	if (m_viewangles[PITCH] > 80)
		m_viewangles[PITCH] = 80;
	if (m_viewangles[PITCH] < -70)
		m_viewangles[PITCH] = -70;

	SetCursorPos(window_center_x, window_center_y);
#endif
	if (keysDown() & KEY_TOUCH)
	{
		touchRead(&g_lastTouch);// = touchReadXY();
		g_lastTouch.px <<= 7;
		g_lastTouch.py <<= 7;
	}
	if (keysHeld() & KEY_TOUCH)
	{
		int dx, dy;
		touchRead(&g_currentTouch);// = touchReadXY();
		// let's use some fixed point magic to improve touch smoothing accuracy
		g_currentTouch.px <<= 7;
		g_currentTouch.py <<= 7;

		dx = (g_currentTouch.px - g_lastTouch.px) >> 6;
		dy = (g_currentTouch.py - g_lastTouch.py) >> 6;


		dx *= 3;
		dy *= 3;

		// add mouse X/Y movement to cmd
		m_viewangles[YAW] -= dx * m_yaw.value;

		m_viewangles[PITCH] += m_pitch.value * dy;
		if (m_viewangles[PITCH] > 80)
			m_viewangles[PITCH] = 80;
		if (m_viewangles[PITCH] < -70)
			m_viewangles[PITCH] = -70;

		// some simple averaging / smoothing through weightened (.5 + .5) accumulation
		g_lastTouch.px = (g_lastTouch.px + g_currentTouch.px) / 2;
		g_lastTouch.py = (g_lastTouch.py + g_currentTouch.py) / 2;
	}
	circlePosition nubPos = { 0, 0 };
	irrstCstickRead(&nubPos);
	if (abs(nubPos.dx) > 2 || abs(nubPos.dy) > 2) {
		m_viewangles[YAW] -= nubPos.dx * m_yaw.value * nub_sensitivity.value;

		m_viewangles[PITCH] -= nubPos.dy * m_pitch.value * nub_sensitivity.value;
		if (m_viewangles[PITCH] > 80)
			m_viewangles[PITCH] = 80;
		if (m_viewangles[PITCH] < -70)
			m_viewangles[PITCH] = -70;
	}
	circlePosition cstickPos = { 0, 0 };
	circleRead(&cstickPos);
	float speed = m_evh.actionState(ACTION_SPEED) ? cl_movespeedkey.value : 1;
	float aspeed = speed * host.frame_time();
	//host.printf("cs: %d %d %f\n", cstickPos.dx, cstickPos.dy, aspeed);
	if (abs(cstickPos.dx) > 20) {
		cmd.sidemove += m_side.value * cstickPos.dx * cstick_sensitivity.value * aspeed * cl_sidespeed.value;
	}
	if (abs(cstickPos.dy) > 20) {
			cmd.forwardmove += m_forward.value * cstickPos.dy * cstick_sensitivity.value * aspeed * cl_forwardspeed.value;
	}
}
void  ClientState::send_move(usercmd_t &cmd) {
	NetBufferFixed<128> buf;

	buf.write_byte(clc_move);
	buf.write_float(m_mtime[0]); //so server can get ping times

	for (int i = 0; i < 3; i++) {
		buf.write_angle(m_viewangles[i]);
	}

	buf.write_short((int)cmd.forwardmove);
	buf.write_short((int)cmd.sidemove);
	buf.write_short((int)cmd.upmove);

	int bits = 0;
	if (m_evh.actionState(ACTION_ATTACK)) {
		bits |= 1;
	}
	if (m_evh.actionState(ACTION_JUMP)) {
		bits |= 2;
	}
	//TODO calculate button bits
	buf.write_byte(bits);

	buf.write_byte(m_in_impulse);
	m_in_impulse = 0;

	if (m_demoplayback) {
		return;
	}

	//
	// allways dump the first two message, because it may contain leftover inputs
	// from the last level
	//
	if (++m_movemessages <= 2) {
		return;
	}

	//TODO net send unreliable
	if (m_netcon) {
		m_netcon->send_unreliable_message(buf);
	}
}

void ClientState::send_cmd() {
	if (m_state != ca_connected) {
		return;
	}

	if (m_signon == SIGNONS) {
		usercmd_t cmd;

		base_move(cmd);

		//TODO process keys
		in_move(cmd);

		send_move(cmd);
	}
	if (m_demoplayback) {
		m_message.clear();
		return;
	}

	if (!m_message.pos()) {
		return;
	}

	if (!m_netcon->can_send()) {
		printf("CL_WriteToServer: can't send\n");
		return;
	}

	if (m_netcon->send_message(m_message) == -1) {
		printf("CL_WriteToServer: lost server connection");
	}

	m_message.clear();
}

void ClientState::reconnect() {
	m_signon = 0;
	m_mixer.stop_all();
	m_music.stop();
}

void ClientState::music(char *cmdline) {
	cmdArgs cmd(cmdline);
	if (cmd.argc < 2) {
		return;
	}
	if (cmd.argc == 3 && !strcmp("track", cmd.argv[1])) {
		int track = atoi(cmd.argv[2]);
		m_music.play(track, true);
		return;
	}
	if (cmd.argc == 3 && !strcmp("play", cmd.argv[1])) {
		m_music.play(cmd.argv[2], true);
		return;
	}
	if (!strcmp("stop", cmd.argv[1])) {
		m_music.stop();
		return;
	}
}

void ClientState::forward_to_server(char *cmd) {
	m_message.write_byte(clc_stringcmd);
#if 1
	m_message.write_string(va("%s\n",cmd));
#else
	if (Q_strcasecmp(Cmd_Argv(0), "cmd") != 0)
	{
		SZ_Print(&cls.message, Cmd_Argv(0));
		SZ_Print(&cls.message, " ");
	}
	if (Cmd_Argc() > 1)
		SZ_Print(&cls.message, Cmd_Args());
	else
		SZ_Print(&cls.message, "\n");
#endif // 1
}

/*
===============
CL_LerpPoint

Determines the fraction between the last two messages that the objects
should be put at.
===============
*/
float	ClientState::lerp_point(void)
{
	float	f, frac;

	f = m_mtime[0] - m_mtime[1];

	if (!f || host.server_active() || m_demo_time )//|| cl_nolerp.value || cls.timedemo)
	{
		m_time = m_mtime[0];
		return 1;
	}

	if (f > 0.1)
	{	// dropped packet, or start of demo
		m_mtime[1] = m_mtime[0] - 0.1;
		f = 0.1f;
	}
	frac = (m_time - m_mtime[1]) / f;
	//Con_Printf ("frac: %f\n",frac);
	if (frac < 0)
	{
		if (frac < -0.01)
		{
			m_time = m_mtime[1];
			//				Con_Printf ("low frac\n");
		}
		frac = 0;
	}
	else if (frac > 1)
	{
		if (frac > 1.01)
		{
			m_time = m_mtime[0];
			//				Con_Printf ("high frac\n");
		}
		frac = 1;
	}

	return frac;
}

entity_t *ClientState::new_temp_entity(void)
{
	entity_t	*ent;

	if (m_num_temp_entities == MAX_TEMP_ENTITIES)
		return 0;
	ent = &m_temp_entities[m_num_temp_entities++];
	memset(ent, 0, sizeof(*ent));

	m_view.add(ent);

	//ent->colormap = vid.colormap;
	return ent;
}

void ClientState::update_temp_entities() {
	int			i;
	beam_t		*b;
	vec3_t		dist, org;
	float		d;
	entity_t	*ent;
	float		yaw, pitch;
	float		forward;
	float		cl_time = host.cl_time();

	m_num_temp_entities = 0;

	// update lightning
	for (i = 0, b = m_beams; i< MAX_BEAMS; i++, b++)
	{
		if (!b->model || b->endtime < cl_time)
			continue;

		// if coming from the player, update the start position
		if (b->entity == m_viewentity)
		{
			ent = m_entities[m_viewentity];
			VectorCopy(ent->origin, b->start);
		}

		// calculate pitch and yaw
		VectorSubtract(b->end, b->start, dist);

		if (dist[1] == 0 && dist[0] == 0)
		{
			yaw = 0;
			if (dist[2] > 0)
				pitch = 90;
			else
				pitch = 270;
		}
		else
		{
			yaw = (int)(atan2(dist[1], dist[0]) * 180 / M_PI);
			if (yaw < 0)
				yaw += 360;

			forward = sqrt(dist[0] * dist[0] + dist[1] * dist[1]);
			pitch = (int)(atan2(dist[2], forward) * 180 / M_PI);
			if (pitch < 0)
				pitch += 360;
			//sprintf(tent_buffer,"tent: %f %f\n%d %d\n",dist[0],dist[1],(int)yaw,(int)pitch);
			//Con_Printf(tent_buffer);
		}

		// add new entities for the lightning
		VectorCopy(b->start, org);
		d = VectorNormalize(dist);
		while (d > 0)
		{
			ent = new_temp_entity();
			if (!ent)
				return;

			VectorCopy(org, ent->origin);
			ent->model = b->model;
			ent->angles[0] = pitch;
			ent->angles[1] = yaw;
			ent->angles[2] = rand() % 360;

			//ent->next = ent->model->ents;
			//ent->model->ents = ent;

			for (i = 0; i<3; i++)
				org[i] += dist[i] * 30;
			d -= 30;
		}
	}

}
/*
===============
CL_RelinkEntities
===============
*/
void ClientState::relink_entities(void)
{
	entity_t	*ent;
	int			i, j;
	float		frac, f, d;
	vec3_t		delta;
	float		bobjrotate;
	vec3_t		oldorg;
	//dlight_t	*dl;

	// determine partial update time	
	frac = lerp_point();

	m_view.clear();

	//
	// interpolate player info
	//
	for (i = 0; i < 3; i++) {
		m_velocity[i] = mvelocity[1][i] +
			frac * (mvelocity[0][i] - mvelocity[1][i]);
	}

	if (m_demoplayback)
	{
		// interpolate the angles	
		for (j = 0; j<3; j++)
		{
			d = m_mviewangles[0][j] - m_mviewangles[1][j];
			if (d > 180)
				d -= 360;
			else if (d < -180)
				d += 360;
			m_viewangles[j] = m_mviewangles[1][j] + frac*d;
		}
	}

	bobjrotate = anglemod(100 * m_time);

	// start on the entity after the world
	for (i = 1; i<m_num_entities; i++)
	{
		ent = m_entities[i];
		if (!ent->model)
		{	// empty slot
			if (ent->forcelink) {
				m_view.remove_efrags(ent);
				//	R_RemoveEfrags(ent);	// just became empty
			}
			continue;
		}

		// if the object wasn't included in the last packet, remove it
		if (ent->msgtime != m_mtime[0])
		{
			ent->model = NULL;
			continue;
		}

		VectorCopy(ent->origin, oldorg);

		if (ent->forcelink)
		{	// the entity was not updated in the last message
			// so move to the final spot
			VectorCopy(ent->msg_origins[0], ent->origin);
			VectorCopy(ent->msg_angles[0], ent->angles);
		}
		else
		{	// if the delta is large, assume a teleport and don't lerp
			f = frac;
			for (j = 0; j<3; j++)
			{
				delta[j] = ent->msg_origins[0][j] - ent->msg_origins[1][j];
				if (delta[j] > 100 || delta[j] < -100)
					f = 1;		// assume a teleportation, not a motion
			}

			// interpolate the origin and angles
			for (j = 0; j<3; j++)
			{
				ent->origin[j] = ent->msg_origins[1][j] + f*delta[j];

				d = ent->msg_angles[0][j] - ent->msg_angles[1][j];
				if (d > 180)
					d -= 360;
				else if (d < -180)
					d += 360;
				ent->angles[j] = ent->msg_angles[1][j] + f*d;
			}

		}

		if (ent->model->m_flags)
		{
			// rotate binary objects locally
			if (ent->model->m_flags & EF_ROTATE)
				ent->angles[1] = bobjrotate;

			if (ent->model->m_flags & EF_GRENADE)
				m_view.m_particles.rocket_trail(oldorg, ent->origin, 1);
			else if (ent->model->m_flags & EF_GIB)
				m_view.m_particles.rocket_trail(oldorg, ent->origin, 2);
			else if (ent->model->m_flags & EF_ZOMGIB)
				m_view.m_particles.rocket_trail(oldorg, ent->origin, 4);
			else if (ent->model->m_flags & EF_TRACER)
				m_view.m_particles.rocket_trail(oldorg, ent->origin, 3);
			else if (ent->model->m_flags & EF_TRACER2)
				m_view.m_particles.rocket_trail(oldorg, ent->origin, 5);
			else if (ent->model->m_flags & EF_TRACER3)
				m_view.m_particles.rocket_trail(oldorg, ent->origin, 6);
			else if (ent->model->m_flags & EF_ROCKET)
			{
				m_view.m_particles.rocket_trail(oldorg, ent->origin, 0);
				m_view.m_dlights.alloc(i, ent->origin, 200, 0.01f, 0, 0);
			}
		}

		if (ent->effects)
		{
			//if (ent->effects & EF_BRIGHTFIELD)
			//	R_EntityParticles(ent);

			if (ent->effects & EF_MUZZLEFLASH)
			{
				vec3_t		fv, rv, uv, org;

				VectorCopy(ent->origin, org);
				org[2] += 16;
				AngleVectors(ent->angles, fv, rv, uv);

				VectorMA(org, 18, fv, org);
				m_view.m_dlights.alloc(i, org, 200 + (rand() & 31), 0.1f, 0, 32);
			}

			if (ent->effects & EF_BRIGHTLIGHT)
			{
				m_view.m_dlights.alloc(i, ent->origin, 400 + (rand() & 31), 0.001f, 0, 0);
			}

			if (ent->effects & EF_DIMLIGHT)
			{
				m_view.m_dlights.alloc(i, ent->origin, 200 + (rand() & 31), 0.001f, 0, 0);
			}
		}

		ent->forcelink = false;

		if (i == m_viewentity)// && !chase_active.value)
			continue;

		m_view.add(ent);

	}
}

int ClientState::read_from_server(float realtime, float frametime) {


	if (m_state != ca_connected) {
		return -1;
	}

	int ret;

	m_oldtime = m_time;
	m_time += frametime;

	do {
		read_demo_message();
		/*ret = get_message(&msg);
		if (ret == -1) {
			//TODO lost connection
		}
		if (!ret) {
			break;
		}
		m_last_received_message = realtime;*/
		ret = parse_server_message();
	} while(ret && m_state == ca_connected);

	m_netcon->clear();

	relink_entities();
	update_temp_entities();

	return 0;
}

void ClientState::parse_server_info() {
	NetBuffer *msg = m_netcon->m_receiveMessage;
	int i;
	int		nummodels, numsounds;
	char	*model_precache[MAX_MODELS];//[MAX_QPATH];
	char	*sound_precache[MAX_SOUNDS];//[MAX_QPATH];

	clear_state();

	// parse protocol version number
	i = msg->read_long();
	if (i != PROTOCOL_VERSION)
	{
		printf("Server returned version %i, not %i\n", i, PROTOCOL_VERSION);
		return;
	}

	// parse maxclients
	m_maxclients = msg->read_byte();
	if (m_maxclients < 1 || m_maxclients > MAX_SCOREBOARD)
	{
		printf("Bad maxclients (%u) from server\n", m_maxclients);
		return;
	}
	m_scores = new(pool) scoreboard_t[m_maxclients];

	// parse gametype
	m_gametype = msg->read_byte();

	// parse signon message
	char *str = msg->read_string();
	strncpy(m_levelname, str, sizeof(m_levelname)-1);

	// seperate the printfs so the server message can have a color
	host.printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	host.printf("%c%s\n", 2, str);

	//
	// first we go through and touch all of the precache data that still
	// happens to be in the cache, so precaching something else doesn't
	// needlessly purge it
	//
	// precache models
	memset(m_model_precache, 0, sizeof(m_model_precache));
	for (nummodels = 1;; nummodels++) {
		str = msg->read_string();
		if (!str[0])
			break;
		if (nummodels == MAX_MODELS) {
			printf("Server sent too many model precaches\n");
			return;
		}
		int len = strlen(str) + 1;
		char *name = new(pool) char[len];
		memcpy(name, str, len);
		model_precache[nummodels] = name;
		//Mod_TouchModel(name);
	}

	
	// precache sounds
	//memset(cl.sound_precache, 0, sizeof(cl.sound_precache));
	for (numsounds = 1;; numsounds++)
	{
		str = msg->read_string();
		if (!str[0])
			break;
		if (numsounds == MAX_SOUNDS)
		{
			printf("Server sent too many sound precaches\n");
			return;
		}
		int len = strlen(str) + 1;
		char *name = new(pool) char[len];
		memcpy(name, str, len);
		sound_precache[numsounds] = name;
		//S_TouchSound(str);
	}
	
	//
	// now we try to load everything else until a cache allocation fails
	//

	for (i = 1; i<nummodels; i++)
	{
		m_model_precache[i] = Models.find(model_precache[i]);// , false);
		if (m_model_precache[i] == NULL)
		{
			host.dprintf("Model %s not found\n", model_precache[i]);
			//return;
		}
		//CL_KeepaliveMessage();
	}
	//we need a dummy sound to align the index across server/client
	Sound *sound = new(pool)Sound("");
	Sounds.add(sound);
	for (i = 1; i < numsounds; i++)
	{
		Sound *sound = Sounds.find(sound_precache[i]);
		if (sound == 0) {
			break;
		}
		byte *data = sound->data();
		//cl.sound_precache[i] = S_PrecacheSound(sound_precache[i]);
		//CL_KeepaliveMessage();
	}
	cl_sfx_wizhit = Sounds.find("wizard/hit.wav");
	cl_sfx_knighthit = Sounds.find("hknight/hit.wav");
	cl_sfx_tink1 = Sounds.find("weapons/tink1.wav");
	cl_sfx_ric1 = Sounds.find("weapons/ric1.wav");
	cl_sfx_ric2 = Sounds.find("weapons/ric2.wav");
	cl_sfx_ric3 = Sounds.find("weapons/ric3.wav");
	cl_sfx_r_exp3 = Sounds.find("weapons/r_exp3.wav");
	m_mixer.cache_ambient_sounds();
	/*
	S_BeginPrecaching();
	for (i = 1; i<numsounds; i++)
	{
		cl.sound_precache[i] = S_PrecacheSound(sound_precache[i]);
		CL_KeepaliveMessage();
	}
	S_EndPrecaching();*/


	// local state
	//ent = cl_entities[0];
	//ent->model = 
	
	m_worldmodel = reinterpret_cast<q1Bsp *>(m_model_precache[1]);

	m_view.clear();

	m_view.set_world(m_worldmodel);

	m_mixer.reset();
	/*
	R_NewMap();

	Hunk_Check();		// make sure nothing is hurt

	noclip_anglehack = false;		// noclip is turned off at start	*/

	if (m_demoplayback) {
		m_demo_endload_time = sys.seconds();
	}
}

void ClientState::signon_reply() {
	char 	str[2048];

	if (m_demoplayback) {
		return;
	}
	host.dprintf("signon_reply: %i\n", m_signon);

	switch (m_signon)
	{
	case 1:
		m_message.write_byte(clc_stringcmd);
		m_message.write_string("prespawn");
		break;

	case 2:
		m_message.write_byte(clc_stringcmd);
		m_message.write_string(va("name \"%s\"\n", "player"));// cl_name.string));

		m_message.write_byte(clc_stringcmd);
		m_message.write_string(va("color %i %i\n", 0, 0));// ((int)cl_color.value) >> 4, ((int)cl_color.value) & 15));

		m_message.write_byte(clc_stringcmd);
		sprintf(str, "spawn %s", m_spawnparms);
		m_message.write_string(str);
		break;

	case 3:
		m_message.write_byte(clc_stringcmd);
		m_message.write_string("begin");
		//Cache_Report();		// print remaining memory
		break;

	case 4:
		//SCR_EndLoadingPlaque();		// allow normal screen updates
		break;
	}
}

void ClientState::parse_beam(NetBuffer *msg,Model *m)
{
	int		ent;
	vec3_t	start, end;
	beam_t	*b;
	int		i;
	float	cl_time = host.cl_time();

	ent = msg->read_short();

	start[0] = msg->read_coord();
	start[1] = msg->read_coord();
	start[2] = msg->read_coord();

	end[0] = msg->read_coord();
	end[1] = msg->read_coord();
	end[2] = msg->read_coord();

#if 1
	// override any beam with the same entity
	for (i = 0, b = m_beams; i< MAX_BEAMS; i++, b++)
	if (b->entity == ent)
	{
		b->entity = ent;
		b->model = m;
		b->endtime = cl_time + 0.2;
		VectorCopy(start, b->start);
		VectorCopy(end, b->end);
		return;
	}

	// find a free beam
	for (i = 0, b = m_beams; i< MAX_BEAMS; i++, b++)
	{
		if (!b->model || b->endtime < cl_time)
		{
			b->entity = ent;
			b->model = m;
			b->endtime = cl_time + 0.2;
			VectorCopy(start, b->start);
			VectorCopy(end, b->end);
			return;
		}
	}
#endif
	host.printf("beam list overflow!\n");
}

void ClientState::parse_tent() {
	NetBuffer *msg = m_netcon->m_receiveMessage;
	int		type;
	vec3_t	pos;
	int rnd;
	//dlight_t	*dl;
	int		colorStart, colorLength;

	type = msg->read_byte();
	switch (type)
	{
	case TE_WIZSPIKE:			// spike hitting wall
		pos[0] = msg->read_coord();
		pos[1] = msg->read_coord();
		pos[2] = msg->read_coord();
		m_view.m_particles.effect(pos, vec3_origin, 20, 30);
		m_mixer.start(-1, 0, cl_sfx_wizhit, pos, 1, 1);
		break;

	case TE_KNIGHTSPIKE:			// spike hitting wall
		pos[0] = msg->read_coord();
		pos[1] = msg->read_coord();
		pos[2] = msg->read_coord();
		m_view.m_particles.effect(pos, vec3_origin, 226, 20);
		m_mixer.start(-1, 0, cl_sfx_knighthit, pos, 1, 1);
		break;

	case TE_SPIKE:			// spike hitting wall
		pos[0] = msg->read_coord();
		pos[1] = msg->read_coord();
		pos[2] = msg->read_coord();

		m_view.m_particles.effect(pos, vec3_origin, 0, 10);

		if (rand() % 5)
			m_mixer.start(-1, 0, cl_sfx_tink1, pos, 1, 1);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				m_mixer.start(-1, 0, cl_sfx_ric1, pos, 1, 1);
			else if (rnd == 2)
				m_mixer.start(-1, 0, cl_sfx_ric2, pos, 1, 1);
			else
				m_mixer.start(-1, 0, cl_sfx_ric3, pos, 1, 1);
		}
		break;
	case TE_SUPERSPIKE:			// super spike hitting wall
		pos[0] = msg->read_coord();
		pos[1] = msg->read_coord();
		pos[2] = msg->read_coord();
		m_view.m_particles.effect(pos, vec3_origin, 0, 20);

		if (rand() % 5)
			m_mixer.start(-1, 0, cl_sfx_tink1, pos, 1, 1);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				m_mixer.start(-1, 0, cl_sfx_ric1, pos, 1, 1);
			else if (rnd == 2)
				m_mixer.start(-1, 0, cl_sfx_ric2, pos, 1, 1);
			else
				m_mixer.start(-1, 0, cl_sfx_ric3, pos, 1, 1);
		}
		break;

	case TE_GUNSHOT:			// bullet hitting wall
		pos[0] = msg->read_coord();
		pos[1] = msg->read_coord();
		pos[2] = msg->read_coord();
		m_view.m_particles.effect(pos, vec3_origin, 0, 20);
		break;

	case TE_EXPLOSION:			// rocket explosion
		pos[0] = msg->read_coord();
		pos[1] = msg->read_coord();
		pos[2] = msg->read_coord();
		m_view.m_particles.explosion(pos);
		m_view.m_dlights.alloc(0, pos, 350, 0.5f, 300, 0);
		m_mixer.start(-1, 0, cl_sfx_r_exp3, pos, 1, 1);
		break;

	case TE_TAREXPLOSION:			// tarbaby explosion
		pos[0] = msg->read_coord();
		pos[1] = msg->read_coord();
		pos[2] = msg->read_coord();
		//R_BlobExplosion(pos);

		m_mixer.start(-1, 0, cl_sfx_r_exp3, pos, 1, 1);
		break;

	case TE_LIGHTNING1:				// lightning bolts
		parse_beam(msg, Models.find("progs/bolt.mdl"));//(cl_bolt1_mod);
		break;

	case TE_LIGHTNING2:				// lightning bolts
		parse_beam(msg, Models.find("progs/bolt2.mdl"));//(cl_bolt2_mod);
		break;

	case TE_LIGHTNING3:				// lightning bolts
		parse_beam(msg, Models.find("progs/bolt3.mdl"));//(cl_bolt3_mod);
		break;

		// PGM 01/21/97 
	case TE_BEAM:				// grappling hook beam
		parse_beam(msg, 0);// cl_beam_mod);
		break;
		// PGM 01/21/97

	case TE_LAVASPLASH:
		pos[0] = msg->read_coord();
		pos[1] = msg->read_coord();
		pos[2] = msg->read_coord();
		m_view.m_particles.lava_splash(pos);
		break;

	case TE_TELEPORT:
		pos[0] = msg->read_coord();
		pos[1] = msg->read_coord();
		pos[2] = msg->read_coord();
		m_view.m_particles.teleport_splash(pos);
		break;

	case TE_EXPLOSION2:				// color mapped explosion
		pos[0] = msg->read_coord();
		pos[1] = msg->read_coord();
		pos[2] = msg->read_coord();
		colorStart = msg->read_byte();
		colorLength = msg->read_byte();
		m_view.m_particles.explosion2(pos, colorStart, colorLength);
		m_view.m_dlights.alloc(0, pos, 350, 0.5f, 300, 0);
		m_mixer.start(-1, 0, cl_sfx_r_exp3, pos, 1, 1);
		break;

	default:
		sys.error("CL_ParseTEnt: bad type");
	}
}

void ClientState::parse_damage(NetBuffer *msg) {
	int		armor, blood;
	vec3_t	from;
	int		i;
	vec3_t	forward, right, up;
	entity_t	*ent;
	float	side;
	float	count;

	armor = msg->read_byte();
	blood = msg->read_byte();
	for (i = 0; i<3; i++)
		from[i] = msg->read_coord();

	count = blood*0.5 + armor*0.5;
	if (count < 10)
		count = 10;

#if 0
	cl.faceanimtime = cl.time + 0.2;		// but sbar face into pain frame

	cl.cshifts[CSHIFT_DAMAGE].percent += 3 * count;
	if (cl.cshifts[CSHIFT_DAMAGE].percent < 0)
		cl.cshifts[CSHIFT_DAMAGE].percent = 0;
	if (cl.cshifts[CSHIFT_DAMAGE].percent > 150)
		cl.cshifts[CSHIFT_DAMAGE].percent = 150;

	if (armor > blood)
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 200;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 100;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 100;
	}
	else if (armor)
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 220;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 50;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 50;
	}
	else
	{
		cl.cshifts[CSHIFT_DAMAGE].destcolor[0] = 255;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[1] = 0;
		cl.cshifts[CSHIFT_DAMAGE].destcolor[2] = 0;
	}
#endif
	//
	// calculate view angle kicks
	//
	ent = m_entities[m_viewentity];

	VectorSubtract(from, ent->origin, from);
	VectorNormalize(from);

	AngleVectors(ent->angles, forward, right, up);

	side = DotProduct(from, right);
	v_dmg_roll = count*side*v_kickroll.value;

	side = DotProduct(from, forward);
	v_dmg_pitch = count*side*v_kickpitch.value;

	v_dmg_time = v_kicktime.value;
}

#define DEFAULT_SOUND_PACKET_VOLUME 255
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0

void ClientState::parse_start_sound(NetBuffer *msg) {
	vec3_t  pos;
	int 	channel, ent;
	int 	sound_num;
	int 	volume;
	int 	field_mask;
	float 	attenuation;
	int		i;

	field_mask = msg->read_byte();

	if (field_mask & SND_VOLUME)
		volume = msg->read_byte();
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;

	if (field_mask & SND_ATTENUATION)
		attenuation = msg->read_byte() / 64.0f;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;

	channel = msg->read_short();
	sound_num = msg->read_byte();

	ent = channel >> 3;
	channel &= 7;

	if (ent > MAX_EDICTS)
		sys.error("CL_ParseStartSoundPacket: ent = %i", ent);

	for (i = 0; i<3; i++)
		pos[i] = msg->read_coord();

	Sound* sound = Sounds.index(sound_num);
	if (!sound) {
		return;
	}
	//byte *data = sound->data();

	m_mixer.start(ent, channel, sound, pos, volume / 255.0f, attenuation);
}

void ClientState::parse_static_sound(NetBuffer *msg) {
	vec3_t		org;
	int			sound_num, vol, atten;
	int			i;

	for (i = 0; i<3; i++)
		org[i] = msg->read_coord();
	sound_num = msg->read_byte();
	vol = msg->read_byte();
	atten = msg->read_byte();

	Sound* sound = Sounds.index(sound_num);
	if (!sound) {
		return;
	}
	//S_StaticSound(cl.sound_precache[sound_num], org, (float)vol, (float)atten);
	m_mixer.start_static(sound, org, vol, atten);
}

void ClientState::parse_particle(NetBuffer *msg) {
	vec3_t		org, dir;
	int			i, count, msgcount, color;

	for (i = 0; i<3; i++)
		org[i] = msg->read_coord();
	for (i = 0; i<3; i++)
		dir[i] = msg->read_char() * (1.0 / 16);
	msgcount = msg->read_byte();
	color = msg->read_byte();

	if (msgcount == 255)
		count = 1024;
	else
		count = msgcount;

	//R_RunParticleEffect(org, dir, color, count);
}

int ClientState::parse_server_message() {
	NetBuffer *msg = m_netcon->m_receiveMessage;
	char *str;
	//TODO parse stuff

	int r = msg->read_byte();
	if (r == -1) {
		return 0;
	}
	int len = msg->read_short();
	int skip = msg->read_byte();

	int end = msg->read_pos() + len;
	int i;

	do {
		int cmd = msg->read_byte();

		//check end of message
		if (cmd == -1) {
			break;
		}
		if (cmd & 128) {
			//fast update
			parse_update(cmd & 127);
			continue;
		}
		switch (cmd) {
		default:
			printf("CL_ParseServerMessage: Illegible server message\n");
			break;

		case svc_nop:
			printf("<-- server to client keepalive\n");
			break;

		case svc_time:
			m_mtime[1] = m_mtime[0];
			m_mtime[0] = msg->read_float();
			break;

		case svc_clientdata:
			i = msg->read_short();
			parse_client_data(i);
			break;

		case svc_version:
			i = msg->read_long();
			if (i != PROTOCOL_VERSION)
				sys.error("CL_ParseServerMessage: Server is protocol %i instead of %i\n", i, PROTOCOL_VERSION);
			break;

		case svc_disconnect:
			host.printf("Server disconnected\n");
			return 0;

		case svc_print:
			host.printf("%s",msg->read_string());
			break;

		case svc_centerprint:
			host.center_printf("%s", msg->read_string());
			break;

		case svc_stufftext:
			str = msg->read_string();
			host.add_text(str);
			break;

		case svc_damage:
			parse_damage(msg);
			break;

		case svc_serverinfo:
			parse_server_info();
			//vid.recalc_refdef = true;	// leave intermission full screen
			break;

		case svc_setangle:
			for (i = 0; i<3; i++)
				m_viewangles[i] = msg->read_angle();
			break;

		case svc_setview:
			m_viewentity = msg->read_short();
			break;

		case svc_lightstyle:
			i = msg->read_byte();
			if (i >= MAX_LIGHTSTYLES)
				sys.error("svc_lightstyle > MAX_LIGHTSTYLES");
			strcpy(cl_lightstyle[i].map, msg->read_string());
			cl_lightstyle[i].length = strlen(cl_lightstyle[i].map);
			break;

		case svc_sound:
			parse_start_sound(msg);
			//CL_ParseStartSoundPacket();
			break;

		case svc_stopsound:
			i = msg->read_byte();
			//S_StopSound(i >> 3, i & 7);
			break;

		case svc_updatename:
			//Sbar_Changed();
			i = msg->read_byte();
			if (i >= m_maxclients)
				printf("CL_ParseServerMessage: svc_updatename > MAX_SCOREBOARD");
			else
				strcpy(m_scores[i].name, msg->read_string());
			break;

		case svc_updatefrags:
			//Sbar_Changed();
			i = msg->read_byte();
			if (i >= m_maxclients)
				printf("CL_ParseServerMessage: svc_updatefrags > MAX_SCOREBOARD");
			m_scores[i].frags = msg->read_short();
			break;

		case svc_updatecolors:
			//Sbar_Changed();
			i = msg->read_byte();
			if (i >= m_maxclients)
				printf("CL_ParseServerMessage: svc_updatecolors > MAX_SCOREBOARD");
			m_scores[i].colors = msg->read_byte();
			//CL_NewTranslation(i);
			break;

		case svc_particle:
			parse_particle(msg);
			break;

		case svc_spawnbaseline:
			i = msg->read_short();
			// must use CL_EntityNum() to force cl.num_entities up
			parse_baseline(entity_num(i));
			break;
		case svc_spawnstatic:
			parse_static();
			break;
		case svc_temp_entity:
			parse_tent();
			break;

		case svc_setpause:
		{
			m_paused = msg->read_byte() ? true : false;

			if (m_paused)
			{
				//CDAudio_Pause();
#ifdef _WIN32
				//VID_HandlePause(true);
#endif
			}
			else
			{
				//CDAudio_Resume();
#ifdef _WIN32
				//VID_HandlePause(false);
#endif
			}
		}
			break;

		case svc_signonnum:
			i = msg->read_byte();
			if (i <= m_signon)
				host.printf("Received signon %i when at %i\n", i, m_signon);
			m_signon = i;
			signon_reply();
			break;

		case svc_killedmonster:
			m_stats[STAT_MONSTERS]++;
			break;

		case svc_foundsecret:
			m_stats[STAT_SECRETS]++;
			break;

		case svc_updatestat:
			i = msg->read_byte();
			if (i < 0 || i >= MAX_CL_STATS)
				sys.error("svc_updatestat: %i is invalid", i);
			m_stats[i] = msg->read_long();
			break;

		case svc_spawnstaticsound:
			parse_static_sound(msg);
			break;

		case svc_cdtrack:
			m_cdtrack = msg->read_byte();
			m_looptrack = msg->read_byte();
			m_music.play(m_cdtrack, true);
			//if ((m_demoplayback || m_demorecording) && (cls.forcetrack != -1))
			//	CDAudio_Play((byte)cls.forcetrack, true);
			//else
			//	CDAudio_Play((byte)cl.cdtrack, true);
			break;

		case svc_intermission:
			m_intermission = 1;
			m_completed_time = (int)m_time;
			//vid.recalc_refdef = true;	// go to full screen
			break;

		case svc_finale:
			m_intermission = 2;
			m_completed_time = (int)m_time;
			host.center_printf(msg->read_string());
			//vid.recalc_refdef = true;	// go to full screen
			//SCR_CenterPrint(MSG_ReadString());
			break;

		case svc_cutscene:
			m_intermission = 3;
			m_completed_time = (int)m_time;
			host.center_printf(msg->read_string());
			//vid.recalc_refdef = true;	// go to full screen
			//SCR_CenterPrint(MSG_ReadString());
			break;

		case svc_sellscreen:
			//Cmd_ExecuteString("help", src_command);
			break;
		}
	} while (msg->read_pos() < end);

	//read alignment bytes
	while (msg->read_pos() & 0x3) {
		i = msg->read_byte();
		if (i == -1) {
			break;
		}
	}

	return 1;
}

void ClientState::establish_connection(char *name) {
	m_netcon = Net::connect(name);
	m_state = ca_connected;
	m_signon = 0;
	m_netcon->clear();
}

entity_t	*ClientState::entity_num(int num)
{
	entity_t *ent;
	int n;
	if (num >= m_num_entities)
	{
		if (num >= MAX_EDICTS)
			sys.error("CL_EntityNum: %i is an invalid number", num);
		n = num - m_num_entities + 1;
		ent = new(pool) entity_t[n];
		memset(ent, 0, sizeof(entity_t)*n);
		while (m_num_entities <= num)
		{
			ent->colormap = 0;// vid.colormap;
			m_entities[m_num_entities++] = ent++;
		}
	}

	return m_entities[num];
}

void ClientState::parse_baseline(entity_t *ent) {
	int			i;
	NetBuffer	*msg = m_netcon->m_receiveMessage;

	ent->baseline.modelindex = msg->read_byte();
	ent->baseline.frame = msg->read_byte();
	ent->baseline.colormap = msg->read_byte();
	ent->baseline.skin = msg->read_byte();
	for (i = 0; i<3; i++)
	{
		ent->baseline.origin[i] = msg->read_coord();
		ent->baseline.angles[i] = msg->read_angle();
	}
}

void ClientState::parse_update(int bits) {
	int			i;
	Model		*model;
	int			modnum;
	bool		forcelink;
	entity_t	*ent;
	int			num;
	NetBuffer	*msg = m_netcon->m_receiveMessage;

	if (m_signon == SIGNONS - 1)
	{	// first update is the final signon stage
		m_signon = SIGNONS;
		signon_reply();
	}

	if (bits & U_MOREBITS)
	{
		i = msg->read_byte();
		bits |= (i << 8);
	}

	if (bits & U_LONGENTITY)
		num = msg->read_short();
	else
		num = msg->read_byte();

	ent = entity_num(num);

	//for (i = 0; i<16; i++)
	//if (bits&(1 << i))
	//	bitcounts[i]++;

	if (ent->msgtime != m_mtime[1])
		forcelink = true;	// no previous frame to lerp from
	else
		forcelink = false;

	ent->msgtime = m_mtime[0];

	if (bits & U_MODEL)
	{
		modnum = msg->read_byte();
		if (modnum >= MAX_MODELS)
			sys.error("CL_ParseModel: bad modnum");
	}
	else
		modnum = ent->baseline.modelindex;

	model = m_model_precache[modnum];
	if (model != ent->model)
	{
		ent->model = model;
		// automatic animation (torches, etc) can be either all together
		// or randomized
		if (model)
		{
			//if (model->synctype == ST_RAND)
			//	ent->syncbase = (float)(rand() & 0x7fff) / 0x7fff;
			//else
				ent->syncbase = 0.0;
		}
		else
			forcelink = true;	// hack to make null model players work
#ifdef GLQUAKE
		if (num > 0 && num <= cl.maxclients)
			R_TranslatePlayerSkin(num - 1);
#endif
	}

	if (bits & U_FRAME)
		ent->frame = msg->read_byte();
	else
		ent->frame = ent->baseline.frame;

	if (bits & U_COLORMAP)
		i = msg->read_byte();
	else
		i = ent->baseline.colormap;
	if (!i)
		ent->colormap = 0;// vid.colormap;
	else
	{
		if (i > m_maxclients)
			sys.error("i >= cl.maxclients");
		ent->colormap = 0;// cl.scores[i - 1].translations;
	}

#ifdef GLQUAKE
	if (bits & U_SKIN)
		skin = MSG_ReadByte();
	else
		skin = ent->baseline.skin;
	if (skin != ent->skinnum) {
		ent->skinnum = skin;
		if (num > 0 && num <= cl.maxclients)
			R_TranslatePlayerSkin(num - 1);
	}

#else

	if (bits & U_SKIN)
		ent->skinnum = msg->read_byte();
	else
		ent->skinnum = ent->baseline.skin;
#endif

	if (bits & U_EFFECTS)
		ent->effects = msg->read_byte();
	else
		ent->effects = ent->baseline.effects;

	// shift the known values for interpolation
	VectorCopy(ent->msg_origins[0], ent->msg_origins[1]);
	VectorCopy(ent->msg_angles[0], ent->msg_angles[1]);

	if (bits & U_ORIGIN1)		ent->msg_origins[0][0] = msg->read_coord();
	else						ent->msg_origins[0][0] = ent->baseline.origin[0];
	if (bits & U_ANGLE1)		ent->msg_angles[0][0] = msg->read_angle();
	else						ent->msg_angles[0][0] = ent->baseline.angles[0];

	if (bits & U_ORIGIN2)		ent->msg_origins[0][1] = msg->read_coord();
	else						ent->msg_origins[0][1] = ent->baseline.origin[1];
	if (bits & U_ANGLE2)		ent->msg_angles[0][1] = msg->read_angle();
	else						ent->msg_angles[0][1] = ent->baseline.angles[1];

	if (bits & U_ORIGIN3)		ent->msg_origins[0][2] = msg->read_coord();
	else						ent->msg_origins[0][2] = ent->baseline.origin[2];
	if (bits & U_ANGLE3)		ent->msg_angles[0][2] = msg->read_angle();
	else						ent->msg_angles[0][2] = ent->baseline.angles[2];

	if (bits & U_NOLERP)
		ent->forcelink = true;

	if (forcelink)
	{	// didn't have an update last message
		VectorCopy(ent->msg_origins[0], ent->msg_origins[1]);
		VectorCopy(ent->msg_origins[0], ent->origin);
		VectorCopy(ent->msg_angles[0], ent->msg_angles[1]);
		VectorCopy(ent->msg_angles[0], ent->angles);
		ent->forcelink = true;
		host.dprintf("ent(%3i): %04.2f %04.2f %04.2f\n",
			num,
			ent->origin[0],
			ent->origin[1],
			ent->origin[2]);
	}
}

void ClientState::parse_static(void)
{
	entity_t *ent;
	int		i;

	i = m_num_statics;
	if (i >= MAX_STATIC_ENTITIES)
		sys.error("Too many static entities");
	ent = &m_static_entities[i];
	m_num_statics++;
	parse_baseline(ent);

	// copy it to the current state
	ent->model = m_model_precache[ent->baseline.modelindex];
	ent->frame = ent->baseline.frame;
	ent->colormap = 0;// vid.colormap;
	ent->skinnum = ent->baseline.skin;
	ent->effects = ent->baseline.effects;

	VectorCopy(ent->baseline.origin, ent->origin);
	VectorCopy(ent->baseline.angles, ent->angles);
	m_view.add_efrags(ent);
}


void ClientState::clear_state() {

	m_num_entities = 0;
	m_num_statics = 0;

	m_intermission = 0;

	memset(m_static_entities, 0, sizeof(m_static_entities));
	memset(m_beams, 0, sizeof(m_beams));

	m_view.reset();
	m_mixer.reset();
	m_hud.reset();
	m_music.stop();
}

void ClientState::parse_client_data(int bits)
{
	NetBuffer	*msg = m_netcon->m_receiveMessage;
	int		i, j;

	if (bits & SU_VIEWHEIGHT)
		m_viewheight = (float)msg->read_char();
	else
		m_viewheight = DEFAULT_VIEWHEIGHT;

	if (bits & SU_IDEALPITCH)
		idealpitch = (float)msg->read_char();
	else
		idealpitch = 0;

	VectorCopy(mvelocity[0], mvelocity[1]);
	for (i = 0; i<3; i++)
	{
		if (bits & (SU_PUNCH1 << i))
			punchangle[i] = (float)msg->read_char();
		else
			punchangle[i] = 0;
		if (bits & (SU_VELOCITY1 << i))
			mvelocity[0][i] = (float)msg->read_char() * 16;
		else
			mvelocity[0][i] = 0;
	}

	// [always sent]	if (bits & SU_ITEMS)
	i = msg->read_long();

	if (items != i)
	{	// set flash times
		//Sbar_Changed();
		for (j = 0; j<32; j++)
		if ((i & (1 << j)) && !(items & (1 << j)))
			item_gettime[j] = (float)m_time;
		items = i;
	}

	m_onground = (bits & SU_ONGROUND) != 0;
	m_inwater = (bits & SU_INWATER) != 0;

	if (bits & SU_WEAPONFRAME)
		m_stats[STAT_WEAPONFRAME] = msg->read_byte();
	else
		m_stats[STAT_WEAPONFRAME] = 0;

	if (bits & SU_ARMOR)
		i = msg->read_byte();
	else
		i = 0;
	if (m_stats[STAT_ARMOR] != i)
	{
		m_stats[STAT_ARMOR] = i;
		//Sbar_Changed();
	}

	if (bits & SU_WEAPON)
		i = msg->read_byte();
	else
		i = 0;
	if (m_stats[STAT_WEAPON] != i)
	{
		m_stats[STAT_WEAPON] = i;
		//Sbar_Changed();
	}

	i = msg->read_short();
	if (m_stats[STAT_HEALTH] != i)
	{
		m_stats[STAT_HEALTH] = i;
		//Sbar_Changed();
	}

	i = msg->read_byte();
	if (m_stats[STAT_AMMO] != i)
	{
		m_stats[STAT_AMMO] = i;
		//Sbar_Changed();
	}

	for (i = 0; i<4; i++)
	{
		j = msg->read_byte();
		if (m_stats[STAT_SHELLS + i] != j)
		{
			m_stats[STAT_SHELLS + i] = j;
			//Sbar_Changed();
		}
	}

	i = msg->read_byte();

	if (true)//standard_quake)
	{
		if (m_stats[STAT_ACTIVEWEAPON] != i)
		{
			m_stats[STAT_ACTIVEWEAPON] = i;
			//Sbar_Changed();
		}
	}
	else
	{
		if (m_stats[STAT_ACTIVEWEAPON] != (1 << i))
		{
			m_stats[STAT_ACTIVEWEAPON] = (1 << i);
			//Sbar_Changed();
		}
	}
}

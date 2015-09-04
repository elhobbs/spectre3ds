#include "ClientState.h"
#include "Host.h"
#include "q1Pic.h"
//#include <gl\gl.h>
//#include <gl\glu.h>

float ClientState::calc_view_bob() {
	float	bob;
	float	cycle;

	cycle = m_time - (int)(m_time / cl_bobcycle.value)*cl_bobcycle.value;
	cycle /= cl_bobcycle.value;
	if (cycle < cl_bobup.value)
		cycle = M_PI * cycle / cl_bobup.value;
	else
		cycle = M_PI + M_PI*(cycle - cl_bobup.value) / (1.0 - cl_bobup.value);

	// bob is proportional to velocity in the xy plane
	// (don't count Z, or jumping messes it up)

	bob = sqrt(m_velocity[0] * m_velocity[0] + m_velocity[1] * m_velocity[1]) * cl_bob.value;
	//Con_Printf ("speed: %5.1f\n", Length(cl.velocity));
	bob = bob*0.3 + bob*0.7*sin(cycle);
	if (bob > 4)
		bob = 4;
	else if (bob < -7)
		bob = -7;
	return bob;
}


float ClientState::calc_roll(vec3_t angles, vec3_t velocity)
{
	float	sign;
	float	side;
	float	value;

	side = DotProduct(velocity, v_right);
	sign = side < 0 ? -1 : 1;
	side = fabs(side);

	value = cl_rollangle.value;
	//	if (cl.inwater)
	//		value *= 6;

	if (side < cl_rollspeed.value)
		side = side * value / cl_rollspeed.value;
	else
		side = value;

	return side*sign;

}

void ClientState::calc_view_roll(vec3_t angles)
{
	entity_t *ent = m_entities[m_viewentity];

	/*float side = */calc_roll(ent->angles, m_velocity);
	//r_refdef.viewangles[ROLL] += side;

	if (v_dmg_time > 0)
	{
		angles[ROLL] += v_dmg_time / v_kicktime.value*v_dmg_roll;
		angles[PITCH] += v_dmg_time / v_kicktime.value*v_dmg_pitch;
		v_dmg_time -= host.frame_time();
		//ds_rumble(rum_pain, ds_rpain.value, 0.01);
	}

	if (m_stats[STAT_HEALTH] <= 0)
	{
		angles[ROLL] = 80;	// dead view angle
		return;
	}
}

void ClientState::update_view_model(vec3_t org, vec3_t ang) {
	// ent is the player model (visible when out of body)
	entity_t *ent = m_entities[m_viewentity];

	// never let it sit exactly on a node line, because a water plane can
	// dissapear when viewed with the eye exactly on it.
	// the server protocol only specifies to 1/16 pixel, so add 1/32 in each axis
	org[0] += 1.0 / 32;
	org[1] += 1.0 / 32;
	org[2] += 1.0 / 32;

	// transform the view offset by the model's matrix to get the offset from
	// model origin for the view
	m_viewent.angles[YAW] = m_viewangles[YAW];	// the model should face
	// the view dir
	m_viewent.angles[PITCH] = -m_viewangles[PITCH];	// the model should face
	// the view dir

	float bob = calc_view_bob();

	calc_view_roll(ang);

#if 0
	V_CalcViewRoll();
	V_AddIdle();

#endif // 0

	// set up gun position
	m_viewent.angles[0] = 0;
	m_viewent.angles[1] = 0;
	m_viewent.angles[2] = 0;

	VectorCopy(org, m_viewent.origin);

	m_viewent.origin[0] += bob *0.4;

	m_viewent.model = m_model_precache[m_stats[STAT_WEAPON]];
	m_viewent.frame = m_stats[STAT_WEAPONFRAME];
	m_viewent.colormap = 0;

	// smooth out stair step ups
	static float oldz = 0;
	if (m_onground && m_viewent.origin[2] - oldz > 0)
	{
		float steptime;

		steptime = m_time - m_oldtime;
		if (steptime < 0)
			//FIXME		I_Error ("steptime < 0");
			steptime = 0;

		oldz += steptime * 80;
		if (oldz > m_viewent.origin[2])
			oldz = m_viewent.origin[2];
		if (m_viewent.origin[2] - oldz > 12)
			oldz = m_viewent.origin[2] - 12;
		org[2] += oldz - ent->origin[2];
	}
	else
		oldz = m_viewent.origin[2];

}

void ClientState::render_view_model() {
	if (m_intermission) {
		return;
	}
#if 0
	if (chase_active.value)
		return;

	if (!r_drawentities.value)
		return;
#endif

	if (items & IT_INVISIBILITY)
		return;

	if (m_stats[STAT_HEALTH] <= 0)
		return;

	if (!m_viewent.model)
		return;

	/*j = R_LightPoint (currententity->origin);

	if (j < 24)
	j = 24;		// allways give some light on gun
	ambientlight = j;
	shadelight = j;

	// add dynamic lights
	for (lnum=0 ; lnum<MAX_DLIGHTS ; lnum++)
	{
	dl = &cl_dlights[lnum];
	if (!dl->radius)
	continue;
	if (!dl->radius)
	continue;
	if (dl->die < cl.time)
	continue;

	VectorSubtract (currententity->origin, dl->origin, dist);
	add = dl->radius - Length(dist);
	if (add > 0)
	ambientlight += add;
	}

	ambient[0] = ambient[1] = ambient[2] = ambient[3] = (float)ambientlight / 128;
	diffuse[0] = diffuse[1] = diffuse[2] = diffuse[3] = (float)shadelight / 128;

	// hack the depth range to prevent view model from poking into walls
	*/
#if 0
	if (crosshair.value)
	{
		R_DrawCrosshair();
	}

#endif // 0
	m_view.render_viewent();

}

int sigil = -1;
sysFileDir *gfxwad = 0;
q1Pic *sig1;


void ClientState::render_hud() {
	if (m_intermission) {
		return;
	}

	if (false && gfxwad == 0) {
		FileSystemStat pgfx = sys.fileSystem["gfx.wad"];
		gfxwad = sys.fileSystem.Add((char *)sys.fileSystem.Get(pgfx), pgfx.len);
		sig1 = Pics.find("ibar");
		int sig = gfxwad->Find("ibar");
		sig1->parse(gfxwad->Get(sig));

	}

	sys.push_2d();

	m_hud.frame(m_stats, items);
	//sig1->render(400, 300);

	sys.pop_2d();
}

#define CONFIG_3D_SLIDERSTATE (*(float*)0x1FF81080)

void ClientState::render(frmType_t type) {
	if (m_signon != SIGNONS) {
		m_mixer.update(vec3_origin, vec3_origin, vec3_origin, vec3_origin);
		return;
	}
	//extra update
	m_mixer.update();
	m_music.update();

	float slider = CONFIG_3D_SLIDERSTATE;
	entity_t *cl = entity_num(m_viewentity);
	vec3_t org, ang, vieworg;

	VectorCopy(cl->origin, org);
	VectorCopy(m_viewangles, ang);
	org[2] += m_viewheight;

	AngleVectors(ang, v_forward, v_right, v_up);

	float vofs = 0;
	if (slider > 0.0f) {
		float ofs = 0;
		switch (type) {
		case FRAME_LEFT:
			ofs = slider*-5.0f;
			vofs = slider * -0.3;
			break;
		case FRAME_RIGHT:
			ofs = slider*5.0f;
			vofs = slider * 0.3;
			break;
		}
		VectorMA(org, ofs, v_right, org);
	}

	vieworg[0] = 0;
	vieworg[1] = vofs;
	vieworg[2] = 0;

	update_view_model(vieworg, ang);
	m_view.set_viewent(&m_viewent);
	
	m_view.render(org, ang);

	render_view_model();
	//turn off dynmic lights - it will get turned back on
	//for the next frame. this should prevent recalculating lights in stereo
	m_view.m_dlights.dynamic(false);
	m_framecount++;

	//extra update
	m_mixer.update();
	m_music.update();
	
	if (!host.console_visible()) {
		render_hud();
	}

}

void ClientState::frame_start() {


	m_music.update();

	if (m_signon != SIGNONS) {
		m_mixer.update(vec3_origin, vec3_origin, vec3_origin, vec3_origin);
		return;
	}

	bool dynamic = dynamic_lights.value ? true : false;

	m_view.animate_lights(cl_lightstyle);
	m_view.m_dlights.dynamic(dynamic);
	m_view.m_dlights.mark_lights(m_worldmodel->m_nodes);

	entity_t *cl = entity_num(m_viewentity);
	AngleVectors(m_viewangles, v_forward, v_right, v_up);
	if (cl) {
		m_mixer.update(cl->origin, v_forward, v_right, v_up);
	}



	//if (!host.console_visible()) {
	//	render_hud();
	//}

	//m_framecount++;

	if (m_paused) {
		host.center_printf("paused");
	}
}
void ClientState::frame_end() {
	m_view.m_dlights.dynamic(true);
}
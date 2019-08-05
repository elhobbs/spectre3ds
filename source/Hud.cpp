#include "Hud.h"
#include "Host.h"
#include "quakedef.h"
#ifdef WIN32
#include <gl\gl.h>
#include <gl\glu.h>
#endif
#include "ctr_render.h"

static byte hud_pal[] = { 0, 0, 0, 0, 0xff, 0, 0xff, 0, 0, 0xff, 0xd7, 0x00, 0x00, 0x00, 0xff, 0xcc, 0xcc, 0xcc }; //clear, green, red, gold, blue, grey

void arc_percent(byte *image, int x0, int y0, int rad, int thickness, int color, int border, int percent);
void hline_percent(byte *image, int x0, int y0, int length, int thickness, int percent, int color);
void vline_percent(byte *image, int x0, int y0, int length, int thickness, int percent, int color);
void render_pic(int id, int x, int y, int width, int height);

void Hud::init() {
	if (m_image_id == -1) {
		m_image_id = sys.gen_texture_id();
		memset(m_image, 0, sizeof(m_image));
		//hack to fix transparency - assumes first pixel is transparent color
		//m_image[0] = 0;
		sys.load_texture256(m_image_id, HUD_WIDTH, HUD_HEIGHT, m_image, hud_pal, 1);
	}
}

void Hud::frame(int *stats, int items) {
	int health = stats[STAT_HEALTH];
	int armor = stats[STAT_ARMOR];
	int ammo = stats[STAT_AMMO];
	bool update = false;

	m_last_health = m_health;
	m_health = health;
	m_last_items = m_items;
	m_items = items;
	m_last_armor = m_armor;
	m_armor = armor;
	m_last_ammo = m_ammo;
	m_ammo = ammo;

	int new_items = m_items&~m_last_items;
	int end_items = (m_items ^ m_last_items) & (~m_items);

	if (m_health != m_last_health) {
		arc_percent(m_image, 32, 92, 28, 4, 1, 2, health);
		//arc_percent(m_image, 32, 32, 8, 2, 1, health);
		update = true;
	}

	int armor_max = 100;
	int armor_color = 0;
	if (m_armor != m_last_armor) {
		if (items & IT_ARMOR1) {
			armor_max = 100;
			armor_color = 4;
		}
		else if (items & IT_ARMOR2) {
			armor_max = 150;
			armor_color = 3;
		}
		else if (items & IT_ARMOR3) {
			armor_max = 200;
			armor_color = 2;
		}
		arc_percent(m_image, 32, 92, 18, 2, armor_color, 0, m_armor * 100 / armor_max);
		update = true;
	}

	if (new_items & IT_INVULNERABILITY) {
		m_time_invulnerable = host.cl_time() + 30;
	}
	if (new_items & IT_SUIT) {
		m_time_suit = host.cl_time() + 30;
	}
	if (new_items & IT_QUAD) {
		m_time_quad = host.cl_time() + 30;
	}
	if (new_items & IT_INVISIBILITY) {
		m_time_invisibility = host.cl_time() + 30;
	}

	//invulnerability
	float diff = m_time_invulnerable - host.cl_time();
	if (end_items & IT_INVULNERABILITY) {
		hline_percent(m_image,3, 10, 56, 2, 100, 0);
		update = true;
		m_time_invulnerable = 0;
	}
	else if (diff > 0) {
		hline_percent(m_image,3, 10, 56, 2, diff * 100 / 30, 3);
		update = true;
	}
	//suit
	diff = m_time_suit - host.cl_time();
	if (end_items & IT_SUIT) {
		hline_percent(m_image, 3, 16, 56, 2, 100, 0);
		update = true;
		m_time_suit = 0;
	}
	else if (diff > 0) {
		hline_percent(m_image, 3, 16, 56, 2, diff * 100 / 30, 1);
		update = true;
	}
	//quad
	diff = m_time_quad - host.cl_time();
	if (end_items & IT_QUAD) {
		hline_percent(m_image, 3, 22, 56, 2, 100, 0);
		update = true;
		m_time_quad = 0;
	}
	else if (diff > 0) {
		hline_percent(m_image, 3, 22, 56, 2, diff * 100 / 30, 4);
		update = true;
	}
	//invisiblity
	diff = m_time_invisibility - host.cl_time();
	if (end_items & IT_INVISIBILITY) {
		hline_percent(m_image, 3, 28, 56, 2, 100, 0);
		update = true;
		m_time_invisibility = 0;
	}
	else if (diff > 0) {
		hline_percent(m_image, 3, 28, 56, 2, diff * 100 / 30, 4);
		update = true;
	}

	int max_ammo = 100;
	if (m_ammo != m_last_ammo) {
		vline_percent(m_image, 64, 70, 46, 3, m_ammo * 100/max_ammo, 5);
		update = true;
	}

	//upload new texture if needed
	if (update) {
		sys.update_texture_byte(m_image_id, 128, 128, m_image, hud_pal, 1);
	}

	if (health < 50) {
		if (m_hud_alpha_pulse < 0.1) {
			m_hud_alpha_pulse = 1.0;
		}
		else {
			m_hud_alpha_pulse -= host.frame_time() * (health < 30 ? 2 : 1);
		}
	}
	else {
		m_hud_alpha_pulse = hud_alpha.value;
	}

#ifdef WIN32
	glColor4f(1, 1, 1, m_hud_alpha_pulse);
	render_pic(m_image_id, 32, 96, HUD_WIDTH, HUD_HEIGHT);
	glColor4f(1, 1, 1, 1.0f);
#else

	int x_pos = 2;
	int y_pos = 2;
	float s1 = 0.0f;
	float s2 = 1.0f;
	float t1 = 0.0f;
	float t2 = 1.0f;
	faceVertex_s tri0[3] = {
		{ { 0.0f, 0.0f, -0.01f }, { 0.0f, 0.0f } },
		{ { 399.0f, 0.0f, -0.01f }, { 1.0f, 0.0f } },
		{ { 399.0f, 64.0f, -0.01f }, { 1.0f, 1.0f } },
	};
	faceVertex_s tri1[3] = {
		{ { 399.0f, 64.0f, -0.01f }, { 1.0f, 1.0f } },
		{ { 0.0f, 64.0f, -0.01f }, { 0.0f, 1.0f } },
		{ { 0.0f, 0.0f, -0.01f }, { 0.0f, 0.0f } }
	};
	//first tri
	tri0[0].position.x = x_pos;
	tri0[0].position.y = y_pos;
	tri0[0].texcoord[0] = s1;
	tri0[0].texcoord[1] = t2;

	tri0[1].position.x = x_pos + HUD_WIDTH;
	tri0[1].position.y = y_pos;
	tri0[1].texcoord[0] = s2;
	tri0[1].texcoord[1] = t2;

	tri0[2].position.x = x_pos + HUD_WIDTH;
	tri0[2].position.y = y_pos + HUD_HEIGHT;
	tri0[2].texcoord[0] = s2;
	tri0[2].texcoord[1] = t1;

	//second tri
	tri1[0].position.x = x_pos + HUD_WIDTH;
	tri1[0].position.y = y_pos + HUD_HEIGHT;
	tri1[0].texcoord[0] = s2;
	tri1[0].texcoord[1] = t1;

	tri1[1].position.x = x_pos;
	tri1[1].position.y = y_pos + HUD_HEIGHT;
	tri1[1].texcoord[0] = s1;
	tri1[1].texcoord[1] = t1;

	tri1[2].position.x = x_pos;
	tri1[2].position.y = y_pos;
	tri1[2].texcoord[0] = s1;
	tri1[2].texcoord[1] = t2;

#if CITRO3D
	//extern shaderProgram_s text_shader;
	//float hudambient[] = {
	//	m_hud_alpha_pulse, 1.0f, 1.0f, 1.0f
	//};
	sys.renderMode.set_mode(shader_mode);
	//GPU_SetFloatUniform(GPU_VERTEX_SHADER, shaderInstanceGetUniformLocation(text_shader.vertexShader, "lightAmbient"), (u32*)hudambient, 1);

	ctrVboClear(&m_vbo);
	sys.bind_texture(m_image_id);
	ctrVboAddData(&m_vbo, tri0, sizeof(tri0), 3);
	ctrVboAddData(&m_vbo, tri1, sizeof(tri1), 3);
	ctrVboDrawDirectly(&m_vbo);
	//printf("health: %3d\n", health);

	//float lightAmbient[] = {
	//	0.9f, 0.9f, 0.9f, 1.0f
	//};
	//GPU_SetFloatUniform(GPU_VERTEX_SHADER, shaderInstanceGetUniformLocation(text_shader.vertexShader, "lightAmbient"), (u32*)lightAmbient, 1);
#endif
#endif
}

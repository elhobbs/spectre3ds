#pragma once

#include "sys.h"

#define HUD_WIDTH 128
#define HUD_HEIGHT 128

class Hud {
public:
	Hud();
	void init();
	void frame(int *stats, int items);
	void reset();

private:
	int		m_health;
	int		m_last_health;
	int		m_items;
	int		m_last_items;
	int		m_armor;
	int		m_last_armor;
	int		m_ammo;
	int		m_last_ammo;

	float	m_hud_alpha_pulse;
	float	m_time_invulnerable;
	float	m_time_suit;
	float	m_time_quad;
	float	m_time_invisibility;

	int		m_image_id;
	byte	m_image[HUD_WIDTH * HUD_HEIGHT];
};

inline Hud::Hud() {
	reset();

	m_image_id = -1;
}

inline void Hud::reset() {
	m_health = 0;
	m_last_health = -1;
	m_items = 0;
	m_last_items = -1;
	m_armor = 0;
	m_last_armor = -1;
	m_ammo = 0;
	m_last_ammo = -1;

	m_hud_alpha_pulse = 0;

	m_time_invulnerable = 0;
	m_time_suit = 0;
	m_time_quad = 0;
	m_time_invisibility = 0;

	memset(m_image, 0, sizeof(m_image));
}
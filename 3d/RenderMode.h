#pragma once
#include <string.h>

typedef void (*RenderModeSetter)(void);

class RenderMode {
public:
	RenderMode();

	int register_mode(RenderModeSetter setter);
	void set_mode(int mode);
private:
	int m_num_modes;
	int m_max_modes;
	int m_current_mode;
	RenderModeSetter *m_modes;
};

inline RenderMode::RenderMode() {
	m_current_mode = -1;
	m_num_modes = 0;
	m_max_modes = 8;
	m_modes = new RenderModeSetter[m_max_modes];
}

inline int RenderMode::register_mode(RenderModeSetter setter) {
	if (m_num_modes == m_max_modes) {
		RenderModeSetter *temp = new RenderModeSetter[m_num_modes + 4];
		memcpy(temp, m_modes, sizeof(RenderModeSetter)*m_num_modes);
		delete [] m_modes;
		m_modes = temp;
		m_max_modes += 4;
	}
	m_modes[m_num_modes++] = setter;
	return m_num_modes-1;
}

inline void RenderMode::set_mode(int mode) {
	if (mode == m_current_mode || mode < 0 || mode >= m_num_modes || m_modes[mode] == 0) {
		return;
	}
	(m_modes[mode])();
	//printf("mode set to %d\n", mode);
	m_current_mode = mode;
}
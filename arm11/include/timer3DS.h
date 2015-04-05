#pragma once
#include <3ds.h>

class timer3DS {
public:
	timer3DS();
	int start(char *name);
	int start(int id);
	u64 stop(int id);
	u64 elapsed(int id);
	void clear(int id);
	void reset();
	void print(int id);
	void print();
private:
	int		m_max_timers;
	int		m_num_timers;
	u64		m_start[1000];
	u64		m_elapsed[1000];
	char	*m_name[1000];
};

inline timer3DS::timer3DS() {
	m_num_timers = 0;
	m_max_timers = 1000;
}

inline int timer3DS::start(char *name) {
	if (m_num_timers >= m_max_timers) {
		return -1;
	}
	int id = m_num_timers++;
	u64 tm = svcGetSystemTick();
	m_start[id] = tm;
	m_name[id] = name;
	m_elapsed[id] = 0;
	return id;
}

inline u64 timer3DS::stop(int id) {
	if (id < 0 || id >= m_num_timers) {
		return 0;
	}
	u64 tm = svcGetSystemTick();
	m_elapsed[id] += (tm - m_start[id]);
	return  m_elapsed[id];
}

inline u64 timer3DS::elapsed(int id) {
	if (id < 0 || id >= m_num_timers) {
		return 0;
	}
	return  m_elapsed[id];
}

inline void timer3DS::clear(int id) {
	m_elapsed[id] = 0;
}

inline void timer3DS::reset() {
	m_num_timers = 0;
}

extern timer3DS g_timer;
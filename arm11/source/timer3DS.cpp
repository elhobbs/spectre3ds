#include "timer3DS.h"
#include <stdio.h>

#define TICKS_PER_MSEC 268123.480f

void timer3DS::print(int id) {
	if (id < 0 || id >= m_num_timers) {
		return;
	}
	printf("%8s: %6.2f\n", m_name[id], (float)m_elapsed[id] / TICKS_PER_MSEC);
}

void timer3DS::print() {
	for (int i = 0; i < m_num_timers; i++) {
		print(i);
	}
}

timer3DS g_timer;
#pragma once

#include "memPool.h"

template<typename T,int len>
class LoadList {
public:
	LoadList();
	T* load(char *name);
	T* find(char *name);
	int index(char *name);
	T* index(int index);
	void add(T *mod);
	void reset();
private:
	int		m_num_known;
	T		*m_known[len];
};

template<typename T, int len>
LoadList<T,len>::LoadList() {
	m_num_known = 0;
	memset(m_known, 0, sizeof(m_known));
}

template<typename T, int len>
T *LoadList<T, len>::find(char *name) {
	for (int i = 0; i < m_num_known; i++) {
		if (!strcmp(m_known[i]->name(), name)) {
			return m_known[i];
		}
	}
	T *tptr = load(name);
	add(tptr);
	return tptr;
}

template<typename T, int len>
int LoadList<T, len>::index(char *name) {
	for (int i = 0; i < m_num_known; i++) {
		if (!strcmp(m_known[i]->name(), name)) {
			return i;
		}
	}
	return -1;
}

template<typename T, int len>
T *LoadList<T, len>::index(int index) {
	if (index < 0 || index >= m_num_known) {
		return 0;
	}
	return m_known[index];
}

template<typename T, int len>
void LoadList<T, len>::add(T *tptr) {
	if (tptr == 0 || m_num_known < 0 || m_num_known >= len) {
		return;
	}
	m_known[m_num_known++] = tptr;
}

#if 0
template<typename T, int len>
void LoadList<T, len>::reset() {
	m_num_known = 0;
}
#endif
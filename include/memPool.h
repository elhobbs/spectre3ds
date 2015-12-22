#pragma once
#include <sys/types.h>

#include <3ds.h>
#include <stdio.h>

class memPool {
public:
	memPool(char *base, int size);
	memPool(int size);
	memPool();
	void* alloc(ssize_t nbytes);
	void* alloc(ssize_t nbytes, ssize_t align);
	void dealloc(void* p);
	void clear();
	int size();
	int used();
	void initFromBase(char *base, int size, int align);
	void markLow();//sets the low mark to the current position
private:
	int m_pos, m_size;
	int m_align;
	int m_lowMark; //this is the clear start
	char *m_data;
	int m_failed_size;
};

inline memPool::memPool(int size) {
	m_pos = 0;
	m_size = size;
	m_align = 4 - 1;
	m_lowMark = 0;
	m_data = new char[size];
	m_failed_size = 0;
}

inline memPool::memPool() {
	m_pos = 0;
	m_size = 0;
	m_align = 0;
	m_lowMark = 0;
	m_data = 0;
	m_failed_size = 0;
}

inline memPool::memPool(char *base, int size) {
	m_pos = 0;
	m_size = size;
	m_data = base;
	m_align = 4 - 1;
	m_lowMark = 0;
	m_failed_size = 0;
}

inline void memPool::initFromBase(char *base, int size, int align) {
	m_pos = 0;
	m_size = size;
	m_data = base;
	m_align = align - 1;
	m_lowMark = 0;
	m_failed_size = 0;
}

inline void memPool::clear() {
	m_pos = m_lowMark;
	m_failed_size = 0;
}

inline void memPool::markLow() {
	m_lowMark = m_pos;
}

inline int memPool::size() {
	return m_size;
}

inline int memPool::used() {
	return m_pos;
}

inline void* memPool::alloc(ssize_t nbytes)
{
	int aligned_size = ((nbytes + m_align) & ~m_align);
	if (m_pos + aligned_size > m_size) {
		m_failed_size += aligned_size;
		printf("memPool alloc failed: %d free %d align %d\n", nbytes, m_pos, m_align);
		while (1) {
			gspWaitForEvent(GSPGPU_EVENT_VBlank0, true);
		};
		return 0;
	}
	char *p = m_data + m_pos;
	m_pos += aligned_size;
	return p;
}

inline void* memPool::alloc(ssize_t nbytes, ssize_t align)
{
	int aligned_size = ((nbytes + m_align) & ~m_align);
	int aligned_start = m_pos + (align - 1);
	aligned_start &= (~(align - 1));
	if (aligned_start + aligned_size > m_size) {
		m_failed_size += aligned_size;
		printf("memPool alloc failed: %d free %d align %d\n", nbytes, m_pos, align);
		while (1) {
			gspWaitForEvent(GSPGPU_EVENT_VBlank0, true);
		};
		return 0;
	}
	m_pos = aligned_start;
	char *p = m_data + m_pos;
	m_pos += aligned_size;
	return p;
}

inline void memPool::dealloc(void* p)
{
	//do nothing
}

inline void* operator new(size_t nbytes, memPool& pool)
{
	return pool.alloc(nbytes);
}

inline void* operator new[](size_t nbytes, memPool& pool)
{
	return pool.alloc(nbytes);
}

inline void operator delete(void *a, memPool& pool)
{
	//do nothing
}

inline void operator delete[](void *a, memPool& pool)
{
	//do nothing
}


extern memPool pool;
extern memPool linear;

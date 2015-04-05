#pragma once

#include "memPool.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

typedef unsigned char byte;

class CmdBuffer {
public:
	CmdBuffer();

	int read_pos();
	int pos();
	int len();

	void clear();
	void reset_read();
	
	void write_long(int c);
	void write(char *format, ...);
	void write(char *format, va_list _list);

	int read_long();
	char* read_string();

protected:
	int m_pos, m_size;
	int m_read_pos;
	bool m_bad_read;
	byte *m_data;

private:
	byte* alloc(int len);
	byte* read(int len);
};

inline CmdBuffer::CmdBuffer() {
	m_read_pos = m_pos = m_size = 0;
	m_data = 0;
	m_bad_read = false;
}

inline byte* CmdBuffer::alloc(int len) {
	if (m_pos + len > m_size) {
		//error out?
		return 0;
	}
	byte *p = &m_data[m_pos];
	m_pos += len;
	return p;
}


inline void CmdBuffer::clear() {
	m_pos = m_read_pos = 0;
	m_bad_read = false;
}

inline void CmdBuffer::reset_read() {
	m_read_pos = 0;
	m_bad_read = false;
}

inline int CmdBuffer::read_pos() {
	return m_read_pos;
}

inline int CmdBuffer::pos() {
	return m_pos;
}

inline int CmdBuffer::len() {
	return m_size;
}

inline byte* CmdBuffer::read(int len) {
	if (m_read_pos + len > m_pos) {
		return 0;
	}
	byte *p = &m_data[m_read_pos];
	m_read_pos += len;
	return p;
}

inline void CmdBuffer::write_long(int c) {
	byte *p = alloc(4);
	p[0] = c & 0xff;
	p[1] = (c >> 8) & 0xff;
	p[2] = (c >> 16) & 0xff;
	p[3] = (c >> 24);
}

inline void CmdBuffer::write(char *format, ...) {
	va_list			argptr;
	static char		s[1024];

	va_start(argptr, format);
	vsprintf(s, format, argptr);
	va_end(argptr);

	int len = strlen(s) + 1;

	write_long(len);
	byte *p = alloc(len);
	if (p) {
		memcpy(p, s, len);
	}
}

inline void CmdBuffer::write(char *format, va_list _list) {
	static char		s[1024];

	vsprintf(s, format, _list);
	int len = strlen(s) + 1;

	write_long(len);
	byte *p = alloc(len);
	if (p) {
		memcpy(p, s, len);
	}
}

inline int CmdBuffer::read_long() {
	byte *p = read(4);
	return (int)(p[0] +
		(p[1] << 8) +
		(p[2] << 16) +
		(p[3] << 24));
}

inline char* CmdBuffer::read_string() {
	int len = read_long();
	if (len > 2048) {
		return 0;
	}
	byte *p = read(len);

	//if we read everything then reset
	if (m_pos == m_read_pos) {
		clear();
	}

	return (char *)p;
}

template <int size>
class CmdBufferFixed : public CmdBuffer {
public:
	CmdBufferFixed();
private:
	unsigned char m_fixedbuffer[size];
};

template <int size>
inline CmdBufferFixed<size>::CmdBufferFixed() {
	m_read_pos = m_pos = 0;
	m_size = size;
	m_data = &m_fixedbuffer[0];
	m_bad_read = false;
}

template <int size>
class CmdBufferPool : public CmdBuffer {
public:
	CmdBufferPool();
};

template <int size>
inline CmdBufferPool<size>::CmdBufferPool() {
	m_read_pos = m_pos = 0;
	m_size = size;
	m_data = new(pool)byte[size];
	m_bad_read = false;
}
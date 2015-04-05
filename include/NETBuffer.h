#pragma once

#include "memPool.h"
#include "FileSystem.h"
#include <string.h>

typedef unsigned char byte;

class NetBuffer {
public:
	NetBuffer();

	int read_pos();
	int pos();
	int len();

	void clear();
	void reset_read();
	void write_char(int c);
	void write_byte(int c);
	void write_short(int c);
	void write_long(int c);
	void write_float(float f);
	void write_string(char *s);
	void write_coord(float f);
	void write_angle(float f);
	void write(byte *data, int len);
	void write(NetBuffer &buf,int pad=0);
	int  write_from_file(sysFile* fp, int pos, int len);

	int read_char();
	int read_byte();
	int read_short();
	int read_long();
	float read_float();
	int read_string(char *s, int max_len);
	char* read_string();
	float read_coord();
	float read_angle();
	void read(NetBuffer &buf, int len, int pad = 0);
	int copy(char *buf, int maxlen);

protected:
	int m_pos, m_size;
	int m_read_pos;
	bool m_bad_read;
	byte *m_data;

private:
	byte* alloc(int len);
	byte* read(int len);
};

inline NetBuffer::NetBuffer() {
	m_read_pos = m_pos = m_size = 0;
	m_data = 0;
	m_bad_read = false;
}

inline byte* NetBuffer::alloc(int len) {
	if (m_pos + len > m_size) {
		//error out?
		return 0;
	}
	byte *p = &m_data[m_pos];
	m_pos += len;
	return p;
}


inline void NetBuffer::clear() {
	m_pos = m_read_pos = 0;
	m_bad_read = false;
}

inline void NetBuffer::reset_read() {
	m_read_pos = 0;
	m_bad_read = false;
}

inline int NetBuffer::read_pos() {
	return m_read_pos;
}

inline int NetBuffer::pos() {
	return m_pos;
}

inline int NetBuffer::len() {
	return m_size;
}

inline byte* NetBuffer::read(int len) {
	if (m_read_pos + len > m_pos) {
		return 0;
	}
	byte *p = &m_data[m_read_pos];
	m_read_pos += len;
	return p;
}

inline int NetBuffer::copy(char *buf,int maxlen) {
	int len = m_pos;
	if (len > maxlen) {
		len = maxlen;
	}
	memcpy(buf, m_data, len);

	return len;
}

inline void NetBuffer::write_char(int c) {
	byte *p = alloc(1);
	p[0] = c;
}

inline void NetBuffer::write_byte(int c) {
	byte *p = alloc(1);
	p[0] = c;
}

inline void NetBuffer::write_short(int c) {
	byte *p = alloc(2);
	p[0] = c & 0xff;
	p[1] = c >> 8;
}

inline void NetBuffer::write_long(int c) {
	byte *p = alloc(4);
	p[0] = c & 0xff;
	p[1] = (c >> 8) & 0xff;
	p[2] = (c >> 16) & 0xff;
	p[3] = (c >> 24);
}

inline void NetBuffer::write_float(float f) {
	byte *p = alloc(4);
	union
	{
		float   f;
		int     l;
	} dat;

	dat.f = f;
	int c = dat.l;

	p[0] = c & 0xff;
	p[1] = (c >> 8) & 0xff;
	p[2] = (c >> 16) & 0xff;
	p[3] = (c >> 24);
}

inline void NetBuffer::write(byte *s, int len) {
	byte *p = alloc(len);
	if (p) {
		memcpy(p, s, len);
	}
}

inline void NetBuffer::write(NetBuffer &buf, int pad /*=0*/) {
	int len = buf.m_pos;
	if (pad) {
		len += (pad - 1);
		len &= ~(pad - 1);
	}
	byte *p = alloc(len);
	if (p) {
		memcpy(p, buf.m_data, buf.m_pos);
		//0 extra bytes
		//if (len - buf.m_pos > 0) {
		//	memset(&p[buf.m_pos], 0, len - buf.m_pos);
		//}
	}
}

inline void NetBuffer::write_string(char *s) {
	if (!s) {
		byte *p = alloc(1);
		*p = 0;
		return;
	}
	int len = strlen(s) + 1;
	byte *p = alloc(len);
	if (p) {
		memcpy(p, s, len);
	}
}

inline void NetBuffer::write_coord(float f) {
	write_short((int)(f * 8));
}

inline void NetBuffer::write_angle(float f) {
	write_byte(((int)f * 256 / 360) & 0xff);
}

inline int NetBuffer::read_char() {
	byte *p = read(1);
	if (p == 0) {
		return -1;
	}
	return (signed char)p[0];
}

inline int NetBuffer::read_byte() {
	byte *p = read(1);
	if (p == 0) {
		return -1;
	}
	return p[0];
}

inline int NetBuffer::read_short() {
	byte *p = read(2);
	return (short)(p[0] + (p[1] << 8));
}

inline int NetBuffer::read_long() {
	byte *p = read(4);
	return (int)(p[0] +
		(p[1] << 8) +
		(p[2] << 16) +
		(p[3] << 24));
}

inline float NetBuffer::read_float() {
	union
	{
		byte    b[4];
		float   f;
		int     l;
	} dat;
	byte *p = read(4);
	dat.b[0] = p[0];
	dat.b[1] = p[1];
	dat.b[2] = p[2];
	dat.b[3] = p[3];

	return dat.f;
}

inline int NetBuffer::read_string(char *s, int max_len) {

	int l = 0;
	do {
		int c = read_char();
		if (c == -1 || c == 0) {
			break;
		}
		s[l++] = c;

	} while (l < (max_len - 1));
	s[l] = 0;
	return l;
}

inline char* NetBuffer::read_string() {
	static char s[2048];
	int l = 0;
	do {
		int c = read_char();
		if (c == -1 || c == 0) {
			break;
		}
		s[l++] = c;

	} while (l < (2048 - 1));
	s[l] = 0;
	return s;
}

inline float NetBuffer::read_coord() {
	return read_short() * 0.125f;
}

inline float NetBuffer::read_angle() {
	return read_char() * (360.0f/256);
}

template <int size>
class NetBufferFixed : public NetBuffer {
public:
	NetBufferFixed();
private:
	unsigned char m_fixedbuffer[size];
};

template <int size>
inline NetBufferFixed<size>::NetBufferFixed() {
	m_read_pos = m_pos = 0;
	m_size = size;
	m_data = &m_fixedbuffer[0];
	m_bad_read = false;
}

template <int size>
class NetBufferPool : public NetBuffer {
public:
	NetBufferPool();
};

template <int size>
inline NetBufferPool<size>::NetBufferPool() {
	m_read_pos = m_pos = 0;
	m_size = size;
	m_data = new(pool) byte[size];
	m_bad_read = false;
}
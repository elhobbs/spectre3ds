#pragma once
#include "sys.h"
#include "LoadList.h"
#include "wav_parser.h"

#define MAX_SOUND_KNOWN 256

#define DEFAULT_SOUND_PACKET_VOLUME 255
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0

class Sound {
public:
	Sound(char *name);
	char *name();
	byte *data();
	int length();
	int loop();
	int id;
private:
	void	resample(wavinfo_t &info, byte *src);
	char	*m_name;
	int		m_length;
	int		m_loopstart;
	byte	*m_data;
};

inline Sound::Sound(char *name) {
	m_data = 0;
	m_length = 0;
	m_loopstart = -1;

	int len = strlen(name);
	m_name = new(pool) char[len + 1];
	if (m_name == 0)
		return;
	memcpy(m_name, name, len);
	m_name[len] = 0;
}

inline char *Sound::name() {
	return m_name;
}

inline int Sound::length() {
	return m_length;
}

inline int Sound::loop() {
	return m_loopstart;
}

extern LoadList<Sound, 256> Sounds;

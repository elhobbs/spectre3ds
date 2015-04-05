#pragma once
#include "MixerHardware.h"
#include <3ds.h>

typedef unsigned char byte;

class MixerHardware3DS : public MixerHardware {
public:
	MixerHardware3DS();
	MixerHardware3DS(int channel, int speed, int channels);
	~MixerHardware3DS();

	void init();
	void shutdown();
	void flush();
	void update(int *pAudioData, int count);
	void update(short *pAudioData, int count);
	int samplepos();
	void clear();
	byte *buffer();
private:

	int m_num_channels;
	int m_speed;
	bool m_initialized;

	u32				m_lastPos;
	u64				m_start;
	u64				m_soundPos;		//the sound position
	u64				m_bufferPos;	//the write position
	int				m_bufferSize;	// the size of soundBuffer
	byte			*m_soundBuffer;	// the buffer itself
	u32				m_soundBufferPHY;
	int				m_channel;
};

inline MixerHardware3DS::MixerHardware3DS() {
	m_speed = 11025;// MIXER_SPEED;
	m_num_channels = 2;
	m_channel = -1;
	m_initialized = false;
}

inline MixerHardware3DS::MixerHardware3DS(int channel, int speed, int channels) {
	//m_count++;
	m_speed = speed;
	m_num_channels = channels;
	m_channel = channel;
	m_initialized = false;
}

inline MixerHardware3DS::~MixerHardware3DS() {
}
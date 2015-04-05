#include "Mixer_3ds.h"
#include <3ds.h>
#include <stdio.h>
#include <string.h>

#include "Host.h"

static int audio_initialized = 0;

void MixerHardware3DS::init() {
	Result ret = 0;
	m_bufferSize = 8192;

	if (audio_initialized == 0) {
		if (csndInit() == 0) {
			host.printf("Sound init ok!\n");
			audio_initialized++;
		}
		else {
			host.printf("Sound init failed!\n");
			return;
		}
	}
	else {
		audio_initialized++;
	}


	m_soundBuffer = (byte *)linearAlloc(m_bufferSize*m_num_channels);
	m_soundBufferPHY = osConvertVirtToPhys((u32)m_soundBuffer);
	m_bufferPos = 0;
	m_soundPos = 0;
	m_lastPos = 0;
	clear();

	ret = csndPlaySound(m_channel, SOUND_REPEAT | SOUND_FORMAT_8BIT, m_speed, (u32*)m_soundBuffer, (u32*)m_soundBuffer, m_bufferSize);
	if (ret != 0) {
		printf("csndPlaySound failed %d on channel %d\n", ret, m_channel);
	}
	if (m_num_channels > 1) {
		ret = csndPlaySound(m_channel + 1, SOUND_REPEAT | SOUND_FORMAT_8BIT, m_speed, (u32*)m_soundBuffer + m_bufferSize, (u32*)m_soundBuffer + m_bufferSize, m_bufferSize);
		if (ret != 0) {
			printf("csndPlaySound failed %d on channel %d\n", ret, m_channel + 1);
		}
		CSND_SetVol(m_channel, 0x7fff, 0);
		csndExecCmds(true);
		CSND_SetVol(m_channel + 1, 0, 0x7fff);
		csndExecCmds(true);
	}

	//try to start stalled channels
	u8 playing = 0;
	csndIsPlaying(m_channel, &playing);
	if (playing == 0) {
		CSND_SetPlayState(m_channel, 1);
	}
	if (m_num_channels > 1) {
		playing = 0;
		csndIsPlaying(m_channel+1, &playing);
		if (playing == 0) {
			CSND_SetPlayState(m_channel + 1, 1);
		}
	}

	//flush csnd command buffers
	csndExecCmds(true);

	m_start = svcGetSystemTick();
	m_initialized = true;
	//printf("MixerHardware3DS::init on %d %d %08x\naudio_initialized == %d\n", m_channel, m_num_channels, m_soundBuffer, audio_initialized);
	//svcSleepThread(5000000000);
}

void MixerHardware3DS::clear() {
	if (!m_initialized) {
		return;
	}
	memset(m_soundBuffer, 0, m_bufferSize*m_num_channels);
	GSPGPU_FlushDataCache(NULL, m_soundBuffer, m_bufferSize*m_num_channels);
}

void MixerHardware3DS::shutdown() {
	if (audio_initialized) {
		flush();
		CSND_SetPlayState(m_channel, 0);
		if (m_num_channels > 1) {
			CSND_SetPlayState(m_channel + 1, 0);
		}

		//flush csnd command buffers
		csndExecCmds(true);
		if (m_soundBuffer) {
			linearFree(m_soundBuffer);
			m_soundBuffer = 0;
		}
		m_initialized = false;
	}
	audio_initialized--;
	if (audio_initialized == 0) {
		Result ret = csndExit();
		printf("csndExit %d\n", ret);
		svcSleepThread(5000000000);
	}
}

void MixerHardware3DS::flush() {
	if (!m_initialized) {
		return;
	}
	memset(m_soundBuffer, 0, m_bufferSize * m_num_channels);
	GSPGPU_FlushDataCache(NULL, m_soundBuffer, m_bufferSize*m_num_channels);
}

byte* MixerHardware3DS::buffer() {
	return m_soundBuffer;
}

#define TICKS_PER_SEC 268123480.0

int MixerHardware3DS::samplepos() {
	int pos, diff;
	CSND_ChnInfo musInfo;

	if (!m_initialized) {
		return 0;
	}
	csndGetState(m_channel, &musInfo);
	pos = musInfo.samplePAddr - m_soundBufferPHY;
	diff = pos - m_lastPos;
	//check for wrap
	if (diff < 0) diff += m_bufferSize;
	m_lastPos = pos;
	m_soundPos += diff;

	//u8 playing = 0;
	//csndIsPlaying(m_channel, &playing);

	//printf("pos: %6d %6d\n", m_soundPos, (int)playing);
	return m_soundPos;
}

void MixerHardware3DS::update(int *pAudioData, int count) {
	int volume = 1;
	int mask = m_bufferSize - 1;

	if (!m_initialized) {
		return;
	}
	if (m_bufferPos < m_soundPos) {
		m_bufferPos = m_soundPos;
	}

	if (m_channel > 1) {
		int *p = pAudioData;
		byte *outp1 = m_soundBuffer;
		byte *outp2 = m_soundBuffer + m_bufferSize;
		int val;
		int pos = m_bufferPos & mask;
		m_bufferPos += count;
		while (count--) {
			val = (*p * volume) >> 0;
			p += 1;//step;
			if (val > 0x7fff)
				val = 0x7fff;
			else if (val < (short)0x8000)
				val = (short)0x8000;
			outp1[pos] = val >> 8;

			val = (*p * volume) >> 0;
			p += 1;//step;
			if (val > 0x7fff)
				val = 0x7fff;
			else if (val < (short)0x8000)
				val = (short)0x8000;
			outp2[pos] = val >> 8;
			pos = (pos + 1) & mask;
		}
	}
	else {
		int *p = pAudioData;
		byte *outp = m_soundBuffer;
		int val;
		int pos = m_bufferPos & mask;
		m_bufferPos += count;
		while (count--) {
			val = (*p * volume) >> 0;
			p += 2;//step;
			if (val > 0x7fff)
				val = 0x7fff;
			else if (val < (short)0x8000)
				val = (short)0x8000;
			outp[pos] = (val >> 8);
			pos = (pos + 1) & mask;
		}
	}
	GSPGPU_FlushDataCache(NULL, m_soundBuffer, m_bufferSize*m_num_channels);
	//printf("upd int: %08x %8d\n", pAudioData, count);
}

void MixerHardware3DS::update(short *pAudioData, int count)
{
	int volume = 1;
	int mask = m_bufferSize - 1;

	if (!m_initialized) {
		return;
	}
	if (m_bufferPos < m_soundPos) {
		m_bufferPos = m_soundPos;
	}

	short *p = pAudioData;
	int val;
	int pos = m_bufferPos & mask;
	m_bufferPos += count;
	if (m_num_channels > 1) {
		byte *outp1 = m_soundBuffer;
		byte *outp2 = m_soundBuffer + m_bufferSize;
		while (count--) {
			val = (*p * volume) >> 0;
			p += 1;//step;
			if (val > 0x7fff)
				val = 0x7fff;
			else if (val < (short)0x8000)
				val = (short)0x8000;
			outp1[pos] = (val >> 8);
			
			val = (*p * volume) >> 0;
			p += 1;//step;
			if (val > 0x7fff)
				val = 0x7fff;
			else if (val < (short)0x8000)
				val = (short)0x8000;
			outp2[pos] = (val >> 8);
			
			pos = (pos + 1) & mask;
		}
	}
	else {
		byte *outp = m_soundBuffer;
		while (count--) {
			val = (*p * volume) >> 0;
			p += 1;//step;
			if (val > 0x7fff)
				val = 0x7fff;
			else if (val < (short)0x8000)
				val = (short)0x8000;
			outp[pos] = (val >> 8) + 128;
			pos = (pos + 1) & mask;
		}
	}
	GSPGPU_FlushDataCache(NULL, m_soundBuffer, m_bufferSize);
	//printf("upd short: %08x %6d %6d\n", pAudioData, count, m_bufferPos);
}

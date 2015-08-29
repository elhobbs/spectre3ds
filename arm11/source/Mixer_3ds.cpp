#include "Mixer_3ds.h"
#include <3ds.h>
#include <stdio.h>
#include <string.h>

#include "Host.h"

static int audio_initialized = 0;

void MixerHardware3DS::init() {
	m_bufferSize = 16384;

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
	m_soundPos2 = 0;
	clear();

	m_initialized = true;

	//try to adjust for the csnd timer precision to match the system timer
	u32 temp = CSND_TIMER(m_speed);
	m_speed = CSND_TIMER(temp);

	start();

	//printf("MixerHardware3DS::init on %d %d %08x\naudio_initialized == %d\n", m_channel, m_num_channels, m_soundBuffer, audio_initialized);
	//svcSleepThread(5000000000);
}

Result play_sound(int chn, u32 flags, u32 sampleRate, float vol, float pan, void* data0, void* data1, u32 size)
{
	if (!(csndChannels & BIT(chn)))
		return 1;

	u32 paddr0 = 0, paddr1 = 0;

	int encoding = (flags >> 12) & 3;
	int loopMode = (flags >> 10) & 3;

	if (!loopMode) flags |= SOUND_ONE_SHOT;

	if (data0) paddr0 = osConvertVirtToPhys((u32)data0);
	if (data1) paddr1 = osConvertVirtToPhys((u32)data1);

	u32 timer = CSND_TIMER(sampleRate);
	if (timer < 0x0042) timer = 0x0042;
	else if (timer > 0xFFFF) timer = 0xFFFF;
	flags &= ~0xFFFF001F;
	flags |= SOUND_ENABLE | SOUND_CHANNEL(chn) | (timer << 16);

	u32 volumes = CSND_VOL(vol, pan);
	CSND_SetChnRegs(flags, paddr0, paddr1, size, volumes, volumes);

	if (loopMode == CSND_LOOPMODE_NORMAL && paddr1 > paddr0)
	{
		// Now that the first block is playing, configure the size of the subsequent blocks
		size -= paddr1 - paddr0;
		CSND_SetBlock(chn, 1, paddr1, size);
	}

	return 0;
}

void MixerHardware3DS::start() {
	Result ret = 0;
	if (!m_initialized) {
		return;
	}
	clear();
	float pan[] = { -1, 1 };
	if (m_num_channels < 2) {
		pan[0] = 0;
	}
	
	m_start = svcGetSystemTick();
	m_lastPos = 0;
	m_soundPos = 0;
	m_soundPos2 = 0;
	for (int i = 0; i < m_num_channels; i++) {
		ret = play_sound(m_channel + i, SOUND_REPEAT | SOUND_FORMAT_8BIT, m_speed, 1.0f, pan[i], (u32*)(m_soundBuffer + i*m_bufferSize), (u32*)(m_soundBuffer + i*m_bufferSize), m_bufferSize);
		if (ret != 0) {
			printf("csndPlaySound failed %d on channel %d\n", ret, m_channel+i);
		}
	}
	CSND_UpdateInfo(true);


	for (int i = 0; i < m_num_channels; i++) {
		u8 playing = 0;
		csndIsPlaying(m_channel + i, &playing);
		if (!playing) {
			printf("forcing: %d\n", m_channel + i);
			CSND_SetPlayState(m_channel + i, 1);
		}
	}
	//flush csnd command buffers
	CSND_UpdateInfo(true);

	for (int i = 0; i < m_num_channels; i++) {
		CSND_SetVol(m_channel+i, CSND_VOL(1.0, pan[i]), CSND_VOL(1.0, pan[i]));
	}
	CSND_UpdateInfo(true);

	//printf("mix start: %d %f %f\n", m_channel, (float)m_start,(float)svcGetSystemTick());
}

void MixerHardware3DS::stop() {
	Result ret = 0;
	if (!m_initialized) {
		return;
	}

	clear();

	for (int i = 0; i < m_num_channels; i++) {
		//start the sound
		u32 flags = SOUND_CHANNEL(m_channel + i);

		CSND_SetPlayState(m_channel + i, 0);
		CSND_SetChnRegs(flags, 0, 0, 0, 0, 0);
	}
	//flush csnd command buffers
	CSND_UpdateInfo(true);
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
		stop();
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
		svcSleepThread(3000000000);
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
//#define TICKS_PER_SEC_LL 268123480LL
#define TICKS_PER_SEC_LL 268111856LL

// Work around the VFP not supporting 64-bit integer <--> floating point conversion
static inline double u64_to_double(u64 value) {
	return (((double)(u32)(value >> 32)) * 0x100000000ULL + (u32)value);
}

u64 MixerHardware3DS::samplepos() {
	u64 delta = (svcGetSystemTick() - m_start);
	u64 samples = delta * m_speed / TICKS_PER_SEC_LL;

	m_soundPos = samples;

	//printf("%2d %8d %10lld %10lld\n", m_channel, m_speed, speed, delta);

	return m_soundPos;
}

void MixerHardware3DS::update(int *pAudioData, int count) {
	int volume = 1;
	int mask = m_bufferSize - 1;

	if (!m_initialized) {
		return;
	}
	if (m_bufferPos < m_soundPos) {
		printf("ibuffer underflow %08x\n", (int)(m_soundPos - m_bufferPos));
		m_bufferPos = m_soundPos;
	}

	if (m_num_channels > 1) {
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
		printf("sbuffer underflow %08x\n", (int)(m_soundPos - m_bufferPos));
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
	GSPGPU_FlushDataCache(NULL, m_soundBuffer, m_bufferSize * m_num_channels);
	//printf("upd short: %08x %6d %6d\n", pAudioData, count, m_bufferPos);
}

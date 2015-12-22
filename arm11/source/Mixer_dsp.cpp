#include "Mixer_dsp.h"
#include <3ds.h>
#include <stdio.h>
#include <string.h>

#include "Host.h"

static int audio_initialized = 0;

void MixerHardwareDSP::init() {
	m_bufferSize = (1<<15)*m_num_channels;

	if (audio_initialized == 0) {
		if (ndspInit() == 0) {
			sys.sleep(10);
			ndspSetOutputMode(NDSP_OUTPUT_STEREO);
			ndspSetOutputCount(1); // Num of buffers
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

	m_soundBuffer = (byte *)linearAlloc(m_bufferSize);
	m_soundBufferPHY = osConvertVirtToPhys((void *)m_soundBuffer);
	m_bufferPos = 0;
	m_soundPos = 0;
	m_soundPos2 = 0;
	clear();

	m_initialized = true;

	start();

	//printf("MixerHardwareDSP::init on %d %d %08x\naudio_initialized == %d\n", m_channel, m_num_channels, m_soundBuffer, audio_initialized);
	//svcSleepThread(5000000000);
}

void MixerHardwareDSP::start() {
	Result ret = 0;
	if (m_initialized == false || m_playing == true) {
		return;
	}
	clear();
	
	m_start2 = svcGetSystemTick();
	m_start = 0;
	m_lastPos = 0;
	m_soundPos = 0;
	m_soundPos2 = 0;

	ndspChnSetInterp(m_channel, NDSP_INTERP_NONE);
	ndspChnSetRate(m_channel, float(m_speed));
	ndspChnSetFormat(m_channel, 
		(m_num_channels == 1) ? NDSP_FORMAT_MONO_PCM8 : NDSP_FORMAT_STEREO_PCM8);

	// Create and play a wav buffer
	memset(&m_WavBuf, 0, sizeof(ndspWaveBuf));

	m_WavBuf.data_vaddr = reinterpret_cast<void *>(m_soundBuffer);
	m_WavBuf.nsamples = m_bufferSize/m_num_channels;
	m_WavBuf.looping = true; // Loop enabled
	m_WavBuf.status = NDSP_WBUF_FREE;

	DSP_FlushDataCache(m_soundBuffer, m_bufferSize);

	ndspChnWaveBufAdd(m_channel, &m_WavBuf);
	m_soundPos = m_soundPos = m_lastPos = 0;
	sys.sleep(20);
	m_bufferPos = m_soundPos = samplepos();
	m_playing = true;
}

void MixerHardwareDSP::stop() {
	if (!m_initialized) {
		return;
	}

	flush();
	clear();

}

void MixerHardwareDSP::clear() {
	if (!m_initialized) {
		return;
	}
	memset(m_soundBuffer, 0, m_bufferSize);
	DSP_FlushDataCache(m_soundBuffer, m_bufferSize);
}

void MixerHardwareDSP::shutdown() {
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
		ndspExit();
	}
}

void MixerHardwareDSP::flush() {
	if (m_initialized == false || m_playing == false) {
		return;
	}
	ndspChnReset(m_channel);
	sys.sleep(20);

	memset(m_soundBuffer, 0, m_bufferSize);
	DSP_FlushDataCache(m_soundBuffer, m_bufferSize);
	m_playing = false;
}

byte* MixerHardwareDSP::buffer() {
	return m_soundBuffer;
}

#define TICKS_PER_SEC 268123480.0
//#define TICKS_PER_SEC_LL 268123480LL
#define TICKS_PER_SEC_LL 268111856LL

u64 MixerHardwareDSP::samplepos() {
	if (!m_playing) {
		return 0;
	}
	u32 temp = ndspChnGetSamplePos(m_channel);
	int diff = temp - m_lastPos;
	if (diff < 0) {
		//printf("diff: %d %d %d %d\n", diff, temp, m_lastPos, m_bufferSize / m_num_channels);
		diff += m_bufferSize/m_num_channels;
	}

	m_soundPos += diff;
	m_lastPos = temp;

	u64 delta = (svcGetSystemTick() - m_start2);
	u64 samples = delta * m_speed / TICKS_PER_SEC_LL;
	u32 keys = hidKeysHeld();
	if(m_channel == 0 && ((keys & KEY_R) == KEY_R)) printf("%10lld %10lld \n",samples, m_soundPos);

	return m_soundPos;
}

void MixerHardwareDSP::update(int *pAudioData, int count) {
	int volume = 1;
	int mask = m_bufferSize - 1;

	if (!m_initialized) {
		return;
	}
	if (count <= 0) {
		return;
	}

	if (!m_playing) {
		start();
	}

	byte *outp = m_soundBuffer;
	int pos = (m_bufferPos*m_num_channels) & mask;
	int *p = pAudioData;
	int val;
	m_bufferPos += count;
	if (m_num_channels > 1) {
		while (count--) {
			val = (*p * volume) >> 0;
			p += 1;//step;
			if (val > 0x7fff)
				val = 0x7fff;
			else if (val < (short)0x8000)
				val = (short)0x8000;
			outp[pos++] = val >> 8;

			val = (*p * volume) >> 0;
			p += 1;//step;
			if (val > 0x7fff)
				val = 0x7fff;
			else if (val < (short)0x8000)
				val = (short)0x8000;
			outp[pos++] = val >> 8;
			pos &= mask;
		}
	}
	else {
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
	DSP_FlushDataCache(m_soundBuffer, m_bufferSize);
	//printf("upd int: %08x %8d\n", pAudioData, count);
}

void MixerHardwareDSP::update(short *pAudioData, int count)
{
	int volume = 1;
	int mask = m_bufferSize - 1;

	if (!m_initialized) {
		return;
	}
	if (count <= 0) {
		return;
	}

	if (!m_playing) {
		start();
	}

	int pos = (m_bufferPos*m_num_channels) & mask;
	m_bufferPos += count;
	short *p = pAudioData;
	byte *outp = m_soundBuffer;
	int val;
	if (m_num_channels > 1) {
		while (count--) {
			val = (*p * volume) >> 0;
			p += 1;//step;
			if (val > 0x7fff)
				val = 0x7fff;
			else if (val < (short)0x8000)
				val = (short)0x8000;
			outp[pos++] = (val >> 8);
			
			val = (*p * volume) >> 0;
			p += 1;//step;
			if (val > 0x7fff)
				val = 0x7fff;
			else if (val < (short)0x8000)
				val = (short)0x8000;
			outp[pos++] = (val >> 8);
			
			pos &= mask;
		}
	}
	else {
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
	DSP_FlushDataCache(m_soundBuffer, m_bufferSize);
	//printf("upd short: %08x %6d %6d\n", pAudioData, count, m_bufferPos);
}

#include "Sound.h"

#include "cache.h"

LoadList<Sound, 256> Sounds;

cache sound_cache(1024 * 1024);

template<>
void LoadList<class Sound, 256>::reset() {
	m_num_known = 0;
	sound_cache.reset();
}

template<>
Sound* LoadList<class Sound, 256>::load(char *name) {
	Sound *sound = new(pool) Sound(name);
	if (sound == 0 || sound->name() == 0) {
		return 0;
	}

	return sound;
}

void Sound::resample(wavinfo_t &info, byte *src) {

	if (info.width == 1 && info.rate == MIXER_SPEED) {
		byte *data_in = src + info.dataofs;
		int len = info.samples;

		m_data = sound_cache.alloc(&m_data, len);
		if (!m_data)
			return;

		for (int i = 0; i < len; i++) {
			((signed char *)m_data)[i] = (int)((unsigned char)(data_in[i]) - 128);
		}

		return;
	}

	int inwidth = info.width;
	float stepscale = (float)info.rate / MIXER_SPEED;	// this is usually 0.5, 1, or 2
	int srcsample, sample;
	int samplefrac = 0;
	int fracstep = stepscale * 256;
	int outcount = info.samples / stepscale;
	byte *data_in = src + info.dataofs;

	m_data = sound_cache.alloc(&m_data, outcount);
	if (!m_data)
		return;

	if (inwidth == 2) {
		for (int i = 0; i<outcount; i++)
		{
			srcsample = samplefrac >> 8;
			samplefrac += fracstep;
			sample = ((short *)data_in)[srcsample];
			((signed char *)m_data)[i] = sample >> 8;
		}
	}
	else {
		for (int i = 0; i<outcount; i++)
		{
			srcsample = samplefrac >> 8;
			samplefrac += fracstep;
			sample = (int)((unsigned char)(data_in[srcsample]) - 128) << 8;
			((signed char *)m_data)[i] = sample >> 8;
		}
	}

	if (info.loopstart != -1) {
		info.loopstart = info.loopstart / stepscale;
	}
	info.rate = MIXER_SPEED;
	info.width = 1;
	info.samples = outcount;
}

byte *Sound::data() {
	if (m_data) {
		sound_cache.touch(m_data);
		return m_data;
	}
	//open the file
	char name[128];
	sprintf(name, "sound/%s", m_name);
	sysFile *file = sys.fileSystem.open(name);
	if (file == 0) {
		return 0;
	}
	//read the file to a temp buffer
	int len = file->length();
	byte *data = new byte[len + 1];
	if (!data) {
		delete file;
		return 0;
	}
	if (file->read((char *)data, 0, len) != len) {
		delete[] data;
		delete file;
		return 0;

	}

	//parse the wav file
	wav_parser wav(data, len);
	wavinfo_t info = wav.parse();
	if (info.channels != 1) {
		return 0;
	}

	resample(info, data);

	m_length = info.samples;
	m_loopstart = info.loopstart;

	//cleanup
	delete[] data;
	delete file;

	return m_data;
}


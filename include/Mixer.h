#pragma once
#include "Sound.h"
#include "vec3_fixed.h"

typedef struct
{
	Sound	*sfx;			// sfx number
	int		leftvol;		// 0-255 volume
	int		rightvol;		// 0-255 volume
	u64		end;			// end time in global paintsamples
	int 	pos;			// sample position in sfx
	int		looping;		// where to loop, -1 = no looping
	int		entnum;			// to allow overriding a specific sound
	int		entchannel;		//
	vec3_fixed32	origin;			// origin of sound effect
	fixed32p16		dist_mult;		// distance multiplier (attenuation/clipK)
	int		master_vol;		// 0-255 master volume
} channel_t;

typedef struct
{
	int left;
	int right;
} portable_samplepair_t;

#define	PAINTBUFFER_SIZE		512

#define	AMBIENT_WATER	0
#define	AMBIENT_SKY		1
#define	AMBIENT_SLIME	2
#define	AMBIENT_LAVA	3

#define	NUM_AMBIENTS			4		// automatic ambient sounds
#define	MAX_CHANNELS			128
#define	MAX_DYNAMIC_CHANNELS	8

#define MIXER_BUFFER_LEN		16384

#ifdef WIN32
#define MIXER_SPEED				11000
#else
#define MIXER_SPEED				11025
#endif

#include "MixerHardware.h"

class Mixer {
public:
	Mixer();

	void init();
	void shutdown();
	void reset();
	void update();

	void print();

	void update(vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up);
	void cache_ambient_sounds();
	void stop_all();
	void start(int entnum, int entchannel, Sound *sfx, vec3_t origin, float fvol, float attenuation);
	void start_static(Sound *sfx, vec3_t origin, float fvol, float attenuation);

private:
	int			m_snd_scaletable[32][256];
	u64			m_sound_time;
	u64			m_paint_time;
	int			m_samples;
	int			m_speed;
	portable_samplepair_t m_paintbuffer[PAINTBUFFER_SIZE];

	int			pick_channel(int ent, int entchannel);
	void		init_scale_table(void);
	void		paint_channel(channel_t *ch, int count);
	void		paint_channels(u64 endtime);
	void		transfer_paint_buffer(u64 endtime);
	u64			sample_pos();
	void		spatialize(channel_t *ch);
	void		update_ambient_sounds();


	// 0 to MAX_DYNAMIC_CHANNELS-1	= normal entity sounds
	// MAX_DYNAMIC_CHANNELS to MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS -1 = water, etc
	// MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS to total_channels = static sounds
	Sound		*m_ambients[NUM_AMBIENTS];
	channel_t   m_channels[MAX_CHANNELS];
	int			m_num_channels;
	

	vec3_fixed32	m_origin;
	vec3_fixed32	m_forward, m_right, m_up;

	MixerHardware	*m_hw;
};

inline Mixer::Mixer() {
	m_sound_time = 0;
	m_paint_time = 0;

	m_speed = MIXER_SPEED;
	m_samples = MIXER_BUFFER_LEN;
}

inline void Mixer::init_scale_table(void)
{
	for (int i = 0; i < 32; i++) {
		for (int j = 0; j < 256; j++) {
			m_snd_scaletable[i][j] = ((signed char)j) * i * 8;
		}
	}
}

inline void Mixer::cache_ambient_sounds() {
	m_ambients[AMBIENT_WATER] = Sounds.find("ambience/water1.wav");
	m_ambients[AMBIENT_SKY] = Sounds.find("ambience/wind2.wav");
	//load the data so that looping works
	if (m_ambients[AMBIENT_WATER]) {
		m_ambients[AMBIENT_WATER]->data();
	}
	if (m_ambients[AMBIENT_SKY]) {
		m_ambients[AMBIENT_SKY]->data();
	}
}
#include "Mixer.h"
#include "Host.h"
#ifdef WIN32
#include "Mixer_win32.h"
#endif
#ifdef ARM11
#include "Mixer_3ds.h"
#endif

void Mixer::init() {
	init_scale_table();
#ifdef WIN32
	m_hw = reinterpret_cast<MixerHardware *>(new MixerHardwareWin32());
#endif

#ifdef ARM11
	MixerHardware3DS *p3ds = new MixerHardware3DS(16,11025,2);
	p3ds->init();
	m_hw = reinterpret_cast<MixerHardware *>(p3ds);
#endif
	reset();
}

void Mixer::shutdown() {
	if (m_hw) {
#ifdef ARM11
		MixerHardware3DS *p3ds = reinterpret_cast<MixerHardware3DS *>(m_hw);
		p3ds->shutdown();
		delete p3ds;
#endif
		m_hw = 0;
	}
}

void Mixer::reset() {
	stop_all();
	m_sound_time = m_paint_time = 0;
	for (int i = 0; i < NUM_AMBIENTS; i++) {
		m_ambients[i] = 0;
	}
	//m_hw->stop();
	//m_hw->start();
}


int Mixer::pick_channel(int entnum, int entchannel) {
	int view_entity = host.cl_view_entity();
	int first_to_die = -1;
	s64 life_left = INT64_MAX;
	for (int ch_idx = NUM_AMBIENTS; ch_idx < NUM_AMBIENTS + MAX_DYNAMIC_CHANNELS; ch_idx++) {
		if (entchannel != 0		// channel 0 never overrides
			&& m_channels[ch_idx].entnum == entnum
			&& (m_channels[ch_idx].entchannel == entchannel || entchannel == -1)) {	
			// allways override sound from same entity
			first_to_die = ch_idx;
			break;
		}

		// don't let monster sounds override player sounds
		if (m_channels[ch_idx].entnum == view_entity && entnum != view_entity && m_channels[ch_idx].sfx) {
			continue;
		}

		//printf("%d %lld %lld %016llX\n", ch_idx, m_paint_time, m_channels[ch_idx].end, m_channels[ch_idx].end - m_paint_time);
		if ((s64)(m_channels[ch_idx].end - m_paint_time) < life_left) {
			life_left = m_channels[ch_idx].end - m_paint_time;
			first_to_die = ch_idx;
		}
	}

	if (first_to_die == -1) {
		return -1;
	}

	if (m_channels[first_to_die].sfx) {
		m_channels[first_to_die].sfx = 0;
	}

	return first_to_die;
}

void Mixer::start(int entnum, int entchannel, Sound *sfx, vec3_t origin, float fvol, float attenuation) {

	if (!sfx) {
		return;
	}
	int channelnum = pick_channel(entnum, entchannel);
	if (channelnum == -1) {
		//printf("no channel\n");
		return;
	}
	channel_t *ch = &m_channels[channelnum];

	ch->entnum = entnum;
	ch->entchannel = entchannel;
	ch->sfx = sfx;
	ch->pos = 0;
	ch->end = m_paint_time + sfx->length();
	ch->origin = origin;
	ch->dist_mult = attenuation / 1000.0f;
	ch->master_vol = (int)(fvol * 255);

	spatialize(ch);

	//printf("%s %6d %6d\n", sfx->name(), ch->leftvol, ch->rightvol);
}

void Mixer::start_static(Sound *sfx, vec3_t origin, float fvol, float attenuation) {
	if (m_num_channels == MAX_CHANNELS) {
		return;
	}

	channel_t *ch = &m_channels[m_num_channels++];
	ch->entnum = 0;
	ch->entchannel = 0;
	ch->sfx = sfx;
	ch->pos = 0;
	ch->end = m_paint_time + sfx->length();
	ch->origin = origin;
	ch->dist_mult = (attenuation/64) / 1000.0f;
	ch->master_vol = (int)(fvol);

	spatialize(ch);
}

void Mixer::transfer_paint_buffer(u64 endtime) {

	int count = endtime - m_paint_time;

#if 1
	m_hw->update((int *)m_paintbuffer, count);
#else
	int *p = (int *)m_paintbuffer;
	int out_mask = MIXER_BUFFER_LEN - 1;
	int out_idx = m_paint_time & out_mask;
	byte *out_left = m_buffer_left;
	byte *out_right = m_buffer_right;
	int volume = 256;
	while (count--) {
		val = (*p * volume) >> 8;
		p += 1;//step;
		if (val > 0x7fff)
			val = 0x7fff;
		else if (val < (short)0x8000)
			val = (short)0x8000;
		out_left[out_idx] = (val >> 8) + 128;

		val = (*p * volume) >> 8;
		p += 1;//step;
		if (val > 0x7fff)
			val = 0x7fff;
		else if (val < (short)0x8000)
			val = (short)0x8000;
		out_right[out_idx] = (val >> 8) + 128;

		out_idx = (out_idx + 1) & out_mask;
	}
#endif
}

void Mixer::paint_channel(channel_t *ch, int count) {
	if (ch->leftvol > 255) {
		ch->leftvol = 255;
	}
	else if (ch->leftvol < 0) {
		ch->leftvol = 0;
	}
	if (ch->rightvol > 255) {
		ch->rightvol = 255;
	}
	else if (ch->rightvol < 0) {
		ch->rightvol = 0;
	}

	int *lscale = m_snd_scaletable[ch->leftvol >> 3];
	int *rscale = m_snd_scaletable[ch->rightvol >> 3]; 
	byte *sfx = ch->sfx->data();
	if (!sfx) {
		return;
	}
	
	sfx += ch->pos;
	int data;

	for (int i = 0; i < count; i++) {
		data = sfx[i];
		m_paintbuffer[i].left += lscale[data];
		m_paintbuffer[i].right += rscale[data];
	}

	ch->pos += count;
}

void Mixer::paint_channels(u64 endtime) {
	int count;
	int mixed = 0;
	while (m_paint_time < endtime) {
		u64 end = endtime;
		//if the paint buffer is smaller
		if (endtime - m_paint_time > PAINTBUFFER_SIZE) {
			end = m_paint_time + PAINTBUFFER_SIZE;
		}
		//clear the paint buffer
		memset(m_paintbuffer, 0, (end - m_paint_time) * sizeof(portable_samplepair_t));
		//paint the channels
		for (int i = 0; i < m_num_channels; i++) {
			channel_t *ch = &m_channels[i];
			if (!ch->sfx) {
				continue;
			}
			//printf("mixing: %s %lld %lld\n", ch->sfx->name(), m_paint_time, ch->end);
			if (ch->leftvol <= 0 && ch->rightvol <= 0) {
				continue;
			}
			mixed++;
			u64 ltime = m_paint_time;
			while (ltime < end) {
				// paint up to end
				if (ch->end < end) {
					count = ch->end - ltime;
				}
				else {
					count = end - ltime;
				}

				if (count > 0) {
					paint_channel(ch, count);
					ltime += count;
				}

				// if at end of loop, restart
				if (ltime >= ch->end) {
					if (ch->sfx->loop() >= 0) {
						ch->pos = ch->sfx->loop();
						ch->end = ltime + ch->sfx->length() - ch->pos;
					}
					else {	
						// channel just stopped
						ch->sfx = 0;
						break;
					}
				}
			}
		}
		transfer_paint_buffer(end);
		m_paint_time = end;
	}
	//host.printf("mixed: %d\n", mixed);
}

u64 Mixer::sample_pos() {
	return m_hw->samplepos();
}

void Mixer::update() {
	u64 last = m_sound_time;
	m_sound_time = sample_pos();
	//printf("%8d %8d\n",m_sound_time - last, m_speed/16);

	u64 endtime = m_sound_time + m_speed * mixahead.value;
	if (m_paint_time < m_sound_time) {
		printf("\nsnd %8lld %8lld %8lld\n\n", m_sound_time, m_paint_time, endtime);
		m_paint_time = m_sound_time;
	}

	//printf("%8lld %8lld %8lld %6lld\n", m_sound_time, m_paint_time, endtime, endtime - m_paint_time);

	paint_channels(endtime);
}

void Mixer::spatialize(channel_t *ch) {

	vec3_fixed32 vec = ch->origin - m_origin;
	fixed32p16 dist = vec.normalize() * ch->dist_mult;
	fixed32p16 dot = m_right * vec;
	fixed32p16 rscale = fixed32p16(1<<16) + dot;
	fixed32p16 lscale = fixed32p16(1<<16) - dot;
	fixed32p16 scale;
	
	//add in distance effecf
	scale = (fixed32p16(1 << 16) - dist) * rscale;
	ch->rightvol = (scale.x * ch->master_vol) >> 16;
	if (ch->rightvol < 0) {
		ch->rightvol = 0;
	}

	scale = (fixed32p16(1 << 16) - dist) * lscale;
	ch->leftvol = (scale.x * ch->master_vol) >> 16;
	if (ch->leftvol < 0) {
		ch->leftvol = 0;
	}
}

void Mixer::update_ambient_sounds() {
	q1_leaf_node *leaf = host.cl_view_leaf();
	float host_frametime = host.frame_time();
	if (!leaf) {
		for (int i = 0; i < NUM_AMBIENTS; i++) {
			m_channels[i].sfx = 0;
		}
		return;
	}
	for (int i = 0; i < NUM_AMBIENTS; i++) {
		channel_t *ch = &m_channels[i];
		ch->sfx = m_ambients[i];
		if (!ch->sfx) {
			continue;
		}

		int vol = 0.3f * leaf->m_ambient_sound_level[i];
		if (vol < 8) {
			vol = 0;
		}
		// don't adjust volume too fast
		if (ch->master_vol < vol)
		{
			ch->master_vol += host_frametime * 100;
			if (ch->master_vol > vol)
				ch->master_vol = vol;
		}
		else if (ch->master_vol > vol)
		{
			ch->master_vol -= host_frametime * 100;
			if (ch->master_vol < vol)
				ch->master_vol = vol;
		}

		ch->leftvol = ch->rightvol = ch->master_vol;
	}
}

void Mixer::update(vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up) {
	m_origin = origin;
	m_forward = v_forward;
	m_right = v_right;
	m_up = v_up;

	update_ambient_sounds();

	int viewent = host.cl_view_entity();
	channel_t *combine = 0;
	int i, j;

	for (i = NUM_AMBIENTS; i < m_num_channels; i++) {
		channel_t *ch = &m_channels[i];

		if (!ch->sfx) {
			continue;
		}
		if (ch->entnum == viewent) {
			ch->leftvol = ch->master_vol;
			ch->rightvol = ch->master_vol;
		}
		else {
			spatialize(ch);
		}
		

		if (ch->leftvol <= 0 && ch->rightvol <= 0) {
			continue;
		}

		// try to combine static sounds with a previous channel of the same
		// sound effect so we don't mix five torches every frame
		if (i >= MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS) {
			// see if it can just use the last one
			if (combine && combine->sfx == ch->sfx) {
				combine->leftvol += ch->leftvol;
				combine->rightvol += ch->rightvol;
				ch->leftvol = ch->rightvol = 0;
				continue;
			}
			// search for one
			combine = m_channels + MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS;
			for (j = MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS; j < i; j++, combine++) {
				if (combine->sfx == ch->sfx) {
					break;
				}
			}

			if (j == m_num_channels) {
				combine = 0;
			}
			else {
				if (combine != ch) {
					combine->leftvol += ch->leftvol;
					combine->rightvol += ch->rightvol;
					ch->leftvol = ch->rightvol = 0;
				}
				continue;
			}
		}
	}

	update();
}

void Mixer::stop_all() {
	m_num_channels = MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS;

	for (int i = 0; i < MAX_CHANNELS; i++) {
		if (m_channels[i].sfx) {
			m_channels[i].sfx = 0;
			m_channels[i].end = 0;
		}
	}

	if (m_hw) {
		m_hw->flush();
	}
	//memset(m_buffer_left, 0, sizeof(m_buffer_left));
	//memset(m_buffer_right, 0, sizeof(m_buffer_right));
}
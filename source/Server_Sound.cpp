#include "Server.h"
#include "Sound.h"
#include "protocol.h"

void Server::start_sound(edict_t *entity, int channel, char *sample, int volume, float attenuation) {
	int         sound_num;
	int field_mask;
	int			i;
	int			ent;

	if (volume < 0 || volume > 255)
		sys.error("SV_StartSound: volume = %i", volume);

	if (attenuation < 0 || attenuation > 4)
		sys.error("SV_StartSound: attenuation = %f", attenuation);

	if (channel < 0 || channel > 7)
		sys.error("SV_StartSound: channel = %i", channel);

	if (m_datagram.pos() > MAX_DATAGRAM - 16)
		return;

	// find precache number for sound
	sound_num = Sounds.index(sample);

	if (sound_num == -1)
	{
		//Con_Printf ("SV_StartSound: %s not precacheed\n", sample);
		return;
	}

	ent = m_progs->NUM_FOR_EDICT(entity);

	channel = (ent << 3) | channel;

	field_mask = 0;
	if (volume != DEFAULT_SOUND_PACKET_VOLUME)
		field_mask |= SND_VOLUME;
	if (attenuation != DEFAULT_SOUND_PACKET_ATTENUATION)
		field_mask |= SND_ATTENUATION;

	// directed messages go only to the entity the are targeted on
	m_datagram.write_byte(svc_sound);
	m_datagram.write_byte(field_mask);
	if (field_mask & SND_VOLUME)
		m_datagram.write_byte(volume);
	if (field_mask & SND_ATTENUATION)
		m_datagram.write_byte(attenuation * 64);
	m_datagram.write_short(channel);
	m_datagram.write_byte(sound_num);
	for (i = 0; i < 3; i++) {
		m_datagram.write_coord(entity->v.origin[i] + 0.5*(entity->v.mins[i] + entity->v.maxs[i]));
	}
}

void Server::start_sound_static(vec3_t pos, char *sample, int volume, float attenuation) {
	int         sound_num = -1;
	int			i;

	if (volume < 0 || volume > 255)
		sys.error("SV_StartSound: volume = %i", volume);

	if (attenuation < 0 || attenuation > 4)
		sys.error("SV_StartSound: attenuation = %f", attenuation);

	// find precache number for sound
	for (int i = 0; i < MAX_SOUNDS; i++) {
		if (!m_sound_precache[i]) {
			break;
		}
		if (!strcmp(m_sound_precache[i], sample)) {
			sound_num = i;
			break;
		}
	}

	if (sound_num == -1) {
		//Con_Printf ("SV_StartSound: %s not precacheed\n", sample);
		return;
	}


	m_signon.write_byte(svc_spawnstaticsound);
	for (i = 0; i < 3; i++) {
		m_signon.write_coord(pos[i]);
	}
	m_signon.write_byte(sound_num);
	m_signon.write_byte(volume);
	m_signon.write_byte(attenuation * 64);
}

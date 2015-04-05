#include "wav_parser.h"

wav_parser::wav_parser(byte *data, int length) {
	m_data_p = m_data = data;
	m_data_end = m_data + length;
}

short wav_parser::read_short() {
	short val = 0;
	val = *m_data_p;
	val = val + (*(m_data_p + 1) << 8);
	m_data_p += 2;
	return val;
}

int wav_parser::read_long() {
	int val = 0;
	val = *m_data_p;
	val = val + (*(m_data_p + 1) << 8);
	val = val + (*(m_data_p + 2) << 16);
	val = val + (*(m_data_p + 3) << 24);
	m_data_p += 4;
	return val;
}

byte *wav_parser::find_next_chunk(char *name) {
	//save the start pos
	byte *data_p = m_data_p;
	do {
		if (m_data_p >= m_data_end) {
			m_data_p = data_p;
			return 0;
		}
		m_data_p += 4;
		int chunk_len = read_long();
		if (chunk_len < 0) {
			m_data_p = data_p;
			return 0;
		}

		m_data_p -= 8;
		if (!memcmp(m_data_p, name, 4)) {
			return m_data_p;
		}
		chunk_len += 8 + (chunk_len & 0x1);
		m_data_p += chunk_len;
	} while (1);

	return 0;
}

wavinfo_t wav_parser::parse() {
	wavinfo_t info;
	memset(&info, 0, sizeof(info));

	if (m_data == 0) {
		return info;
	}
	byte *riff = find_next_chunk("RIFF");
	if (!riff || memcmp(riff + 8, "WAVE", 4)) {
		return info;
	}
	m_data_p = riff + 12;
	byte *fmt = find_next_chunk("fmt ");
	if (!fmt) {
		return info;
	}
	m_data_p = fmt + 8;
	short format = read_short();
	if (format != 1) {
		return info;
	}

	info.channels = read_short();
	info.rate = read_long();
	m_data_p += 4 + 2;
	info.width = read_long() / 8;

	byte *cue = find_next_chunk("cue ");
	if (cue) {
		m_data_p += 32;
		info.loopstart = read_long();

		byte *list = find_next_chunk("LIST");
		if (list) {
			if (!memcmp(list + 28, "mark", 4)) {
				m_data_p = list + 24;
				int samples = read_long();
				info.samples = info.loopstart + samples;
			}
		}
		m_data_p = fmt;
	}
	else {
		info.loopstart = -1;
	}

	byte *data = find_next_chunk("data");
	if (!data) {
		return info;
	}

	m_data_p = data + 4;
	int samples = read_long() / info.width;
	if (info.samples) {
		if (samples < info.samples) {
			sys.error("bad loop length");
		}
	}
	else {
		info.samples = samples;
	}
	info.dataofs = m_data_p - m_data;

	return info;
}

#pragma once
#include "sys.h"

typedef struct
{
	int		rate;
	int		width;
	int		channels;
	int		loopstart;
	int		samples;
	int		dataofs;		// chunk starts this many bytes from file start
} wavinfo_t;

class wav_parser {
public:
	wav_parser(byte *data, int length);
	wavinfo_t parse();

private:
	short	read_short();
	int		read_long();
	byte	*find_next_chunk(char *name);
	byte	*m_data_end;
	byte	*m_data_p;
	byte	*m_data;
};


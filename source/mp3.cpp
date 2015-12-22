#include "mp3.h"
#include "Host.h"

#ifdef WIN32
#include "Mixer_win32.h"
#endif
#ifdef ARM11
#include "Mixer_3ds.h"
#include "Mixer_dsp.h"
#endif

void mp3::frame() {
	int offset, err;

	// if mp3 is set to loop indefinitely, don't bother with how many data is left
	if (m_loop && m_bytesLeft < 2 * MAINBUF_SIZE)
		m_bytesLeft += MP3_FILE_BUFFER_SIZE;


	/* find start of next MP3 frame - assume EOF if no sync found */
	offset = MP3FindSyncWord((u8 *)m_readPtr, m_bytesLeft);
	if (offset < 0) {
		host.printf("mp3: finished\n");
		m_state = MP3_IDLE;
		return;
	}
	m_readPtr += offset;
	m_bytesLeft -= offset;

	err = MP3Decode(m_hMP3Decoder, &m_readPtr, &m_bytesLeft, m_outbuf, 0);
	if (err) {
		/* error occurred */
		switch (err) {
		case ERR_MP3_INDATA_UNDERFLOW:
			//outOfData = 1;
			break;
		case ERR_MP3_MAINDATA_UNDERFLOW:
			/* do nothing - next call to decode will provide more mainData */
			return;
		case ERR_MP3_FREE_BITRATE_SYNC:
		default:
			host.printf("mp3: error %d stopping\n", err);
			m_state = MP3_ERROR;
			break;
		}
		return;
	}
	/* no error */
	MP3GetLastFrameInfo(m_hMP3Decoder, &m_mp3FrameInfo);

	//start the mixer if needed
	if (m_hw == 0) {
		//host.printf("no m_hw\n");
		m_speed = m_mp3FrameInfo.samprate;
		m_channels = m_mp3FrameInfo.nChans;
#ifdef WIN32
		m_hw = reinterpret_cast<MixerHardware *>(new MixerHardwareWin32(m_speed, m_channels));
#endif
#ifdef ARM11
		MixerHardwareDSP *p3ds = new MixerHardwareDSP(10, m_speed, m_channels);
		p3ds->init();
		m_hw = reinterpret_cast<MixerHardware *>(p3ds);
#endif
	}

	m_hw->update(m_outbuf, m_mp3FrameInfo.outputSamps>>1);
	m_paint_time += (m_mp3FrameInfo.outputSamps>>1);
}

void mp3::fill_buffer() {
	int n = m_file->read((char *)m_file_buffer + MP3_FILE_BUFFER_SIZE, -1, MP3_FILE_BUFFER_SIZE);
	if (m_loop && n < MP3_FILE_BUFFER_SIZE) {
		n = m_file->read((char *)m_file_buffer + MP3_FILE_BUFFER_SIZE + n, 0, MP3_FILE_BUFFER_SIZE-n);
	}
}

void mp3::frames(u64 endtime)
{

	while (m_paint_time < endtime && m_state != MP3_ERROR)
	{

		frame();

		// check if we moved onto the 2nd file data buffer, if so move it to the 1st one and request a refill
		if (m_readPtr > (m_file_buffer + MP3_FILE_BUFFER_SIZE)) {
			m_readPtr = m_readPtr - MP3_FILE_BUFFER_SIZE;
			memcpy((void *)m_readPtr, (void *)(m_readPtr + MP3_FILE_BUFFER_SIZE), MP3_FILE_BUFFER_SIZE - (m_readPtr - m_file_buffer));
			//mp3->flag = 1;
			fill_buffer();
			//fifoSendValue32(FIFO_USER_02, 0);
		}
	}
}

void mp3::playing() {

	m_sound_time = sample_pos();

	// check to make sure that we haven't overshot
	if (m_paint_time < m_sound_time)
	{
		m_paint_time = m_sound_time;
	}

	// mix ahead of current position
	u64 endtime = m_sound_time + (m_mp3FrameInfo.samprate / 10);

	frames(endtime);
}

void mp3::resume() {

	m_sound_time = m_paint_time = 0;// sample_pos();
	if (m_hw) {
		m_hw->flush();
	}
	//memset((void *)mp3->audio, 0, MP3_AUDIO_BUFFER_SIZE);
	
	frames(MP3_AUDIO_BUFFER_SAMPS / 2);

	/*mp3_channel = getFreeChannel();
	//mp3->audio[4] = mp3->audio[5] = 0x7fff;	// force a pop for debugging
	SCHANNEL_SOURCE(mp3_channel) = (u32)mp3->audio;
	SCHANNEL_REPEAT_POINT(mp3_channel) = 0;
	SCHANNEL_LENGTH(mp3_channel) = (MP3_AUDIO_BUFFER_SIZE) >> 2;
	SCHANNEL_TIMER(mp3_channel) = 0x10000 - (0x1000000 / mp3->rate);
	SCHANNEL_CR(mp3_channel) = SCHANNEL_ENABLE | SOUND_VOL(mp3_volume) | SOUND_PAN(64) | (SOUND_FORMAT_16BIT) | (SOUND_REPEAT);

	ds_set_timer(mp3->rate);
	ds_sound_start = ds_time();*/

	m_state = MP3_PLAYING;
}

void mp3::starting() {
	//host.printf("mp3 starting\n");
	if (m_hMP3Decoder == 0) {
		if ((m_hMP3Decoder = MP3InitDecoder()) == 0) {
			m_state = MP3_IDLE;
			//fifoSendValue32(FIFO_USER_01, 1);
			return;
		}
	}

	resume();

	//fifoSendValue32(FIFO_USER_01, cb);
}

void mp3::resuming() {

	resume();
	//fifoSendValue32(FIFO_USER_01, 0);
}

void mp3::pause() {
	m_hw->flush();
	//ds_set_timer(0);
	//SCHANNEL_CR(mp3_channel) = 0;
	//mp3_channel = -1;
	m_state = MP3_PAUSED;
}

void mp3::pausing() {
	pause();
	//fifoSendValue32(FIFO_USER_01, 0);
}

void mp3::stopping() {
	host.printf("mp3 stopping\n");
	m_hw->flush();
	MP3FreeDecoder(m_hMP3Decoder);
	m_hMP3Decoder = 0;
	m_state = MP3_IDLE;
}

void mp3::update() {
	switch (m_state) {
	case MP3_STARTING:
		starting();
		break;
	case MP3_PLAYING:
		playing();
		break;
	case MP3_PAUSING:
		pausing();
		break;
	case MP3_RESUMING:
		resuming();
		break;
	case MP3_STOPPING:
		stopping();
		break;
	case MP3_IDLE:
	case MP3_PAUSED:
	case MP3_ERROR:
		break;
	}
}

void mp3::play(char *name, bool loop) {
	stop();
	m_file = sys.fileSystem.open(name);
	if (m_file == 0) {
		host.printf("unable to open: %s\n", name);
		m_state = MP3_IDLE;
		return;
	}

	m_length = m_bytesLeft = m_file->length();
	m_loop = loop;

	m_file->read((char *)m_file_buffer, -1, MP3_FILE_BUFFER_SIZE * 2);
	m_readPtr = m_file_buffer;
	m_state = MP3_STARTING;
	host.printf("playing %s\n", name);
}

void mp3::play(int track, bool loop) {
	char name[128];

	sprintf(name, "music/%02d.mp3", track);
	play(name, loop);
}

void mp3::stop() {
	if (m_file) {
		delete m_file;
		m_file = 0;
	}
	if (m_hw) {
		m_hw->flush();
#ifdef ARM11
		MixerHardwareDSP *p3ds = reinterpret_cast<MixerHardwareDSP *>(m_hw);
		p3ds->shutdown();
#endif
		delete m_hw;
		m_hw = 0;
	}
	m_state = MP3_IDLE;
}

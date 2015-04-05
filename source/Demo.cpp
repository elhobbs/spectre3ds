#include "ClientState.h"
#include "protocol.h"
#include "Host.h"
#include "procTexture.h"
#include "Sound.h"

void ClientState::stop_demo() {
	if (!m_demoplayback) {
		return;
	}

	float end = sys.seconds();
	int frame_count = m_framecount;

	if (m_demo_file) {
		delete m_demo_file;
		m_demo_file = 0;
	}
	m_demoplayback = false;
	m_state = ca_disconnected;
	m_signon = 0;

	m_mixer.stop_all();
	m_music.stop();

	if (m_demo_time) {
		host.printf("%ctimedemo: %d frames in %0.4f seconds. %0.4f fps\n", 
			2, 
			frame_count - m_demo_frame_start,
			end - m_demo_starttime,
			(float)(frame_count - m_demo_frame_start) / (end - m_demo_starttime));
		m_demo_time = false;
	}
}

void ClientState::read_demo_message() {
	if (m_demoplayback) {
		m_netcon->clear();
		if (m_signon == SIGNONS) {
			if (m_time <= m_mtime[0]) {
				return;
			}
		}
		int len = 0;

		// get the next message
		int r = m_demo_file->read((char *)&len, -1, 4);
		if (r != 4) {
			stop_demo();
			return;
		}

		VectorCopy(m_mviewangles[0], m_mviewangles[1]);
		r = m_demo_file->read((char *)m_mviewangles[0], -1, 12);
		if (r != 12) {
			stop_demo();
			return;
		}

		//cook up values to read in parse_server_message
		m_netcon->m_receiveMessage->write_byte(1);
		m_netcon->m_receiveMessage->write_short(len);
		m_netcon->m_receiveMessage->write_byte(0);

		r = m_netcon->m_receiveMessage->write_from_file(m_demo_file, -1, len);
		if (r != len) {
			stop_demo();
		}
	}
}

void ClientState::play_demo(char *demo,bool time) {
	char name[256];

	if (demo == 0 || *demo == 0) {
		return;
	}

	strcpy(name, demo);
	strcat(name, ".dem");

	disconnect();

	host.clear();

	m_demo_file = sys.fileSystem.open(name);
	if (m_demo_file == 0) {
		host.printf("play_demo: %s not found\n");
		return;
	}

	m_netcon = Net::connect("demo");
	m_demoplayback = true;
	m_state = ca_connected;
	m_time = 0;
	m_mtime[0] = m_mtime[1] = 0;

	char c;
	bool end = false;
	bool neg = false;
	int track = 0;
	do {
		if (!m_demo_file->read(&c, -1, 1)) {
			break;
		}
		switch (c) {
		case '\n':
			end = true;
			break;
		case '-':
			neg = true;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			track = track * 10 + (c - '0');
		}

	} while (!end);

	track = -track;

	m_demo_frame_start = m_framecount;
	m_demo_time = time;
	m_demo_starttime = 0;
	if (time) {
		m_demo_starttime = sys.seconds();
	}
}
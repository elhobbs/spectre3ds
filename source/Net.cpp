#include "Net.h"
#include "sys.h"

bool NetLoop::m_pending_local_connect;
NetBufferFixed<NET_MAXMESSAGE>	NetLoop::m_sharedbuffer;

int NetLoop::send_message(NetBuffer &buf) {

	if (m_disconnected) {
		return -1;
	}
	m_lastSendTime = (float)sys.seconds();

	//message type
	m_receiveMessage->write_byte(1);

	//length
	m_receiveMessage->write_short(buf.pos());

	//align
	m_receiveMessage->write_byte(0);

	//message
	m_receiveMessage->write(buf, 4);

	m_canSend = false;

	return 1;
}

int NetLoop::send_unreliable_message(NetBuffer &buf) {

	if (m_disconnected) {
		return -1;
	}
	m_lastSendTime = (float)sys.seconds();

	//message type
	m_receiveMessage->write_byte(2);

	//length
	m_receiveMessage->write_short(buf.pos());

	//align
	m_receiveMessage->write_byte(0);

	//message
	m_receiveMessage->write(buf, 4);

	return 1;
}


Net* Net::check_new_connections() {
	static NetLoop server_loop;
	if (!NetLoop::m_pending_local_connect) {
		return 0;
	}
	NetLoop::m_pending_local_connect = false;
	NetLoop *net = &server_loop;
	//Net *net = new NetLoop();
	net->reset();
	return net;
}

Net* Net::connect(char *name) {
	static NetLoop client_loop;
	NetLoop::m_pending_local_connect = true;
	if (name && !strcmp(name, "demo")) {
		NetLoop::m_pending_local_connect = false;
	}
	NetLoop *net = &client_loop;
	//NetLoop *net = new NetLoop();
	net->reset();
	return net;
}

inline void NetLoop::close() {
	m_canSend = false;
	m_disconnected = true;
}

inline void NetLoop::reset() {
	m_receiveMessage->clear();
	m_sendMessage->clear();
	m_canSend = true;
	m_disconnected = false;
}


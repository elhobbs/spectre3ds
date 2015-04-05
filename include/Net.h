#pragma once
#ifndef __NET_H__
#define __NET_H__

#include "NetBuffer.h"
/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// net.h -- quake's interface to the networking layer

struct qsockaddr
{
	short sa_family;
	unsigned char sa_data[14];
};


#define	NET_NAMELEN			64

#define NET_MAXMESSAGE		8192
#define NET_HEADERSIZE		(2 * sizeof(unsigned int))
#define NET_DATAGRAMSIZE	(MAX_DATAGRAM + NET_HEADERSIZE)

// NetHeader flags
#define NETFLAG_LENGTH_MASK	0x0000ffff
#define NETFLAG_DATA		0x00010000
#define NETFLAG_ACK			0x00020000
#define NETFLAG_NAK			0x00040000
#define NETFLAG_EOM			0x00080000
#define NETFLAG_UNRELIABLE	0x00100000
#define NETFLAG_CTL			0x80000000


#define NET_PROTOCOL_VERSION	3

// This is the network info/connection protocol.  It is used to find Quake
// servers, get info about them, and connect to them.  Once connected, the
// Quake game protocol (documented elsewhere) is used.
//
//
// General notes:
//	game_name is currently always "QUAKE", but is there so this same protocol
//		can be used for future games as well; can you say Quake2?
//
// CCREQ_CONNECT
//		string	game_name				"QUAKE"
//		byte	net_protocol_version	NET_PROTOCOL_VERSION
//
// CCREQ_SERVER_INFO
//		string	game_name				"QUAKE"
//		byte	net_protocol_version	NET_PROTOCOL_VERSION
//
// CCREQ_PLAYER_INFO
//		byte	player_number
//
// CCREQ_RULE_INFO
//		string	rule
//
//
//
// CCREP_ACCEPT
//		long	port
//
// CCREP_REJECT
//		string	reason
//
// CCREP_SERVER_INFO
//		string	server_address
//		string	host_name
//		string	level_name
//		byte	current_players
//		byte	max_players
//		byte	protocol_version	NET_PROTOCOL_VERSION
//
// CCREP_PLAYER_INFO
//		byte	player_number
//		string	name
//		long	colors
//		long	frags
//		long	connect_time
//		string	address
//
// CCREP_RULE_INFO
//		string	rule
//		string	value

//	note:
//		There are two address forms used above.  The short form is just a
//		port number.  The address that goes along with the port is defined as
//		"whatever address you receive this reponse from".  This lets us use
//		the host OS to solve the problem of multiple host addresses (possibly
//		with no routing between them); the host will use the right address
//		when we reply to the inbound connection request.  The long from is
//		a full address and port in a string.  It is used for returning the
//		address of a server that is not running locally.

#define CCREQ_CONNECT		0x01
#define CCREQ_SERVER_INFO	0x02
#define CCREQ_PLAYER_INFO	0x03
#define CCREQ_RULE_INFO		0x04

#define CCREP_ACCEPT		0x81
#define CCREP_REJECT		0x82
#define CCREP_SERVER_INFO	0x83
#define CCREP_PLAYER_INFO	0x84
#define CCREP_RULE_INFO		0x85

typedef struct qsocket_s
{
	struct qsocket_s	*next;
	double			connecttime;
	double			lastMessageTime;
	double			lastSendTime;

	bool			disconnected;
	bool			canSend;
	bool			sendNext;

	int				driver;
	int				landriver;
	int				socket;
	void			*driverdata;

	unsigned int	ackSequence;
	unsigned int	sendSequence;
	unsigned int	unreliableSendSequence;
	int				sendMessageLength;
	byte			sendMessage[NET_MAXMESSAGE];

	unsigned int	receiveSequence;
	unsigned int	unreliableReceiveSequence;
	int				receiveMessageLength;
	byte			receiveMessage[NET_MAXMESSAGE];

	struct qsockaddr	addr;
	char				address[NET_NAMELEN];

} qsocket_t;

#define	MAX_MSGLEN		8000		// max length of a reliable message
#define	MAX_DATAGRAM	1024		// max length of unreliable message

class Net {
public:
	Net();

	void init();
	void shutdown();

	virtual int send_message(NetBuffer &msg) = 0;
	virtual int send_unreliable_message(NetBuffer &msg) = 0;
	virtual void close() = 0;
	static Net* connect(char *name);
	static Net* check_new_connections();
	float last_send();
	bool can_send();
	void clear();

	NetBuffer		*m_receiveMessage;
	NetBuffer		*m_sendMessage;
	float			m_connecttime;
	float			m_lastMessageTime;
	float			m_lastSendTime;

protected:
	bool			m_disconnected;
	bool			m_canSend;
	bool			m_sendNext;

	unsigned int	m_ackSequence;
	unsigned int	m_sendSequence;
	unsigned int	m_unreliableSendSequence;
	//int				m_sendMessageLength;
	//byte			m_sendMessage[NET_MAXMESSAGE];

	unsigned int	m_receiveSequence;
	unsigned int	m_unreliableReceiveSequence;
	//int				m_receiveMessageLength;
	//byte			m_receiveMessage[NET_MAXMESSAGE];

	struct qsockaddr	m_addr;
	char				m_address[NET_NAMELEN];
private:

};

inline Net::Net() {
	m_canSend = false;
	m_disconnected = true;
}

inline float Net::last_send() {
	return m_lastSendTime;
}

inline bool Net::can_send() {
	return m_canSend;
}

inline void Net::clear() {
	m_receiveMessage->clear();
	m_sendMessage->clear();
	m_canSend = true;
}

class NetLoop : public Net {
public:
	NetLoop();
	int send_message(NetBuffer &msg);
	int send_unreliable_message(NetBuffer &msg);
	void close();
	void reset();
	static bool m_pending_local_connect;
private:
	//all loops share a static buffer - maybe this will work????
	static NetBufferFixed<NET_MAXMESSAGE>	m_sharedbuffer;
};

inline NetLoop::NetLoop() {
	m_receiveMessage = &m_sharedbuffer;
	m_sendMessage = &m_sharedbuffer;
	m_canSend = true;
	m_disconnected = false;
}

#endif
#pragma once

#include "Net.h"
#include "ClientState.h"

#define	NUM_PING_TIMES		16
#define	NUM_SPAWN_PARMS		16

class Client {
public:
	Client();
	bool is_active();
	bool set_active(bool val);
	void set_connection(Net *net);

	NetBufferFixed<MAX_MSGLEN>	m_msg;

	edict_t		*m_edict;				// EDICT_NUM(clientnum+1)

	bool		m_active;
	bool		m_spawned;
	bool		m_dropasap;
	bool		m_sendsignon;
	char		m_name[32];
	// client known data for deltas	
	int			m_old_frags;

	Net			*m_netconnection;

	// spawn parms are carried from level to level
	float		m_spawn_parms[NUM_SPAWN_PARMS];
	usercmd_t	m_cmd;
	float		m_ping_times[NUM_PING_TIMES];
	int			m_num_pings;
	int			m_colors;
protected:

private:

	float		m_last_message;


	vec3_t		m_wishdir;


};

inline Client::Client() {
	m_active = false;
	m_spawned = false;
	m_dropasap = false;
	m_sendsignon = 0;

	m_last_message = 0;

	m_netconnection = 0;

	m_name[0] = 0;

	m_num_pings = 0;

	m_old_frags = 0;
}

inline bool Client::is_active() {
	return m_active;
}

inline bool Client::set_active(bool val) {
	bool ret = m_active;
	m_active = val;

	//return the original value;
	return ret;
}

inline void Client::set_connection(Net *net) {
	m_netconnection = net;
}

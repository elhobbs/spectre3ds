#include "sys.h"
#ifdef WIN32
#include <gl\gl.h>
#include <gl\glu.h>
#endif
#include <stdio.h>
#include <cstdarg>

int SYS::grow_handlers() {
	//expand the list if needed
	if( (m_ev_handlers_max-m_ev_handlers_cnt) == 0) {
		EvHandler **temp = new EvHandler*[m_ev_handlers_max+4];
		if(temp == 0) {
			return -1;
		}
		m_ev_handlers_max += 4;
		memcpy(temp,m_ev_handlers,m_ev_handlers_cnt*sizeof(EvHandler*));
		delete [] m_ev_handlers;
		m_ev_handlers = temp;
	}

	return 0;
}

int SYS::addEvHandler(EvHandler *evh) {
	if(grow_handlers()) {
		return -1;
	}
	m_ev_handlers[m_ev_handlers_cnt++] = evh;
	
	return 0;
}

int SYS::handle_events() {
	do {
		event_t ev = events.get_event();
		if(ev.type == 0) {
			break;
		}
		for(int i=0;i<m_ev_handlers_cnt;i++) {
			if(m_ev_handlers[i]->handleEvent(ev) == 0) {
				break;
			}
		}
	} while(1);

	return 0;
}

void SYS::error(char *error, ...) {
	va_list		argptr;
	char		string[1024];

	va_start(argptr, error);
	vsprintf(string, error, argptr);
	va_end(argptr);

	//PR_PrintStatement(pr_statements + pr_xstatement);
	//PR_StackTrace();
	::printf("%s\n", string);

	m_running = false;
	//Host_Error("Program error");
}

/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
char    *va(char *format, ...)
{
	va_list         argptr;
	static char             string[1024];

	va_start(argptr, format);
	vsprintf(string, format, argptr);
	va_end(argptr);

	return string;
}


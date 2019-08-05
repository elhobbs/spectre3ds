#include "input.h"
#include "CmdBuffer.h"

#ifndef __CONSOLE_H__
#define __CONSOLE_H__


#define MAX_CONSOLE_LINES 200
#define MAX_CONSOLE_TEXT 4096
#define MAX_CONSOLE_LINE_LENGTH 256

class Notify {
public:
	Notify();
	void render();
	void print(char *str);
	void clear();
private:
	float		m_TTL;
	int			m_num_lines;
	int			m_max_width;
	char		m_text[1024];
	//gsVbo_s		m_vbo;
};

inline Notify::Notify() {
	m_TTL = 0;
	*m_text = 0;
}

inline void Notify::clear() {
	m_TTL = 0;
	*m_text = 0;
}

class Console : public EvHandler {
public:
	Console();
	bool init();

	int handleEvent(event_t ev);
	void render();
	void show(bool visible);
	void clear();
	bool visible();


	void print(char *text);
	void printf(char *text, ...);
private:
	void					add_char(int c);
	void					auto_complete();

	bool					m_visible;
	bool					m_shift;

	int						m_line;
	int						m_pos;
	char					m_cmd_text[32][MAX_CONSOLE_LINE_LENGTH];

	float					m_last_print;

	bool					m_cmd_history;
	int						m_cmd_history_line; //this tracks copying previous commands

	int						m_text_history;
	int						m_text_line;
	int						m_text_pos;
	char					*m_text_lines[MAX_CONSOLE_LINES];

	float					m_text_times[MAX_CONSOLE_LINES];
	char					m_text[MAX_CONSOLE_TEXT];
	//gsVbo_s					m_vbo;
};

inline Console::Console() {

	m_text_history = 0;
	m_cmd_history = false;
	m_cmd_history_line = 0;

	m_shift = false;
	m_visible = false;
	m_line = 0;
	m_pos = 0;
	memset(m_cmd_text[0], 0, MAX_CONSOLE_LINE_LENGTH);
	
	*m_text = 0;
	m_text_line = 0;
	m_text_pos = 0;
	memset(m_text_lines, 0, sizeof(m_text_lines));
	m_text_lines[0] = m_text;
	//gsVboInit(&m_vbo);
	//gsVboCreate(&m_vbo, 256 * 1024);
}

inline bool Console::init() {
	sys.addEvHandler(this);
	show(false);
	return false;
}

inline void Console::show(bool visible) {
	m_visible = visible;
}

inline bool Console::visible() {
	return m_visible;
}

inline void Console::clear() {
	*m_text = 0;
	m_text_line = 0;
	m_text_pos = 0;
	memset(m_text_lines, 0, sizeof(m_text_lines));
	m_text_lines[0] = m_text;
}

#endif

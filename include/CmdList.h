#pragma once

#include <stdarg.h>

template <typename L>
class CmdList {
public:
	typedef void(L::*builtin)(char *cmdline, void *p, ...);
	//CmdList();
	explicit CmdList(L *l);

	void add(char *name, builtin f_ptr);
	void call(char *name,...);
	int matches(char *text, int num, char **list);

private:
	L *m_ref;
	class Cmd {
	public:
		char	*m_name;
		int		m_len;
		builtin m_func;
		Cmd		*m_next;
	};
	Cmd		m_head;
};

#if 0
template <typename L>
inline CmdList<L>::CmdList() {
	m_head.m_next = 0;
}

#endif // 0

template <typename L>
inline CmdList<L>::CmdList(L *l) : m_ref(l) {
	m_head.m_next = 0;
}

template <typename L>
inline void CmdList<L>::add(char *name, builtin f_ptr) {
	if (name == 0) {
		return;
	}
	int len = strlen(name);
	char *str = new char[len+1];
	memcpy(str, name, len+1);

	Cmd *c = new Cmd();
	c->m_name = str;
	c->m_func = f_ptr;
	c->m_len = len;

	c->m_next = m_head.m_next;
	m_head.m_next = c;

}

template <typename L>
inline void CmdList<L>::call(char *name, ...) {
	va_list         argptr;
	for (Cmd *next = m_head.m_next; next; next = next->m_next) {
		int len = next->m_len;
		if (memcmp(name, next->m_name, len) == 0) {
			if (name[len] != 0 && name[len] != ' ' && name[len] != '\r' && name[len] != '\n') {
				continue;
			}
			va_start(argptr, name);
			void *p = va_arg(argptr, void *);
			((*m_ref).*(next->m_func))(name, p, argptr);
			va_end(argptr);
			return;
		}
	}
}

template <typename L>
inline int CmdList<L>::matches(char *text,int num,char **list) {
	int found = 0;
	int len = strlen(text);
	for (Cmd *next = m_head.m_next; next; next = next->m_next) {
		if (memcmp(text, next->m_name, len) == 0) {
			if (found < num) {
				list[found++] = next->m_name;
			}
		}
	}
	return found;
}

class cmdArgs {
public:
	cmdArgs(char *cmd);
	int argc;
	char *argv[16];
private:
	char str[256];
};

inline cmdArgs::cmdArgs(char *cmd) {
	bool in_str = false;
	argc = 0;
	memset(argv, 0, sizeof(argv));
	strcpy(str, cmd);
	str[255] = 0;

	char *b = str;
	for (argc = 0; *b && argc < 8;) {
		while (*b && *b == ' ') {
			b++;
		}
		if (*b == 0 || *b == '\n' || *b == '\r') {
			break;
		}
		argv[argc] = b;
		if (*b == '"') {
			argv[argc] = ++b;
			in_str = true;
		}
		while (*b) {
			if ((*b == '"' && in_str) || (*b == ' ' && !in_str) || *b == '\n' || *b == '\r') {
				*b++ = 0;
				in_str = false;
				break;
			}
			b++;
		}
		argc++;
	}
}
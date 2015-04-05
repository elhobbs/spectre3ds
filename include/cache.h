#pragma once
#include "host.h"
#include <stddef.h>

typedef enum {
	MB_STATE_ERROR = 0,
	MB_STATE_FREE = 1,
	MB_STATE_MRU,
	MB_STATE_MFU,
	MB_STATE_HEAD
} MB_STATE;

typedef struct memblock_s {
	MB_STATE			state;
	int					len;
	byte				**owner;
	struct memblock_s	*prev, *next;
	struct memblock_s	*lru_prev, *lru_next;
} memblock_t;

class cache {
public:
	cache();
	cache(int size);
	cache(byte *base, int size);
	
	void init(byte *base, int size);

	byte *alloc(byte** owner, int size);
	void free(byte *data);
	void touch(byte *data);

	void reset();

	void check();

private:
	memblock_t *try_alloc(int size);
	void free(memblock_t *mb);
	void unlink(memblock_t *mb);
	void link(memblock_t *mb);

	int			m_zone_used;
	int			m_zone_len;
	byte		*m_zone;
	memblock_t	*m_head;
	memblock_t	m_mru;
	memblock_t	m_mfu;
	memblock_t	m_free;
};

inline cache::cache(int size) {
	m_zone = 0;
	m_zone_len = size;
	reset();
}

inline cache::cache() {
	m_zone = 0;
	m_zone_len = 0;
}

inline cache::cache(byte *base, int size) {
	m_zone = base;
	m_zone_len = size;
	reset();
}

inline void cache::init(byte *base, int size) {
	m_zone = base;
	m_zone_len = size;
	reset();
}

inline void memblock_list_init(memblock_t &list) {
	list.state = MB_STATE_HEAD;
	list.next = list.prev = &list;
	list.lru_next = list.lru_prev = &list;
	list.len = 0;
	list.owner = 0;
}

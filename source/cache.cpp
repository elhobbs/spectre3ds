#include "cache.h"

void memblock_list_unlinkLRU(memblock_t &list, memblock_t *mb) {
	if (!mb->lru_next || !mb->lru_prev || mb->state == MB_STATE_HEAD) {
		sys.error("memblock_list_unlink: NULL link");
	}

	list.len -= mb->len;

	mb->lru_next->lru_prev = mb->lru_prev;
	mb->lru_prev->lru_next = mb->lru_next;

	mb->lru_prev = mb->lru_next = 0;
}

void memblock_list_linkLRU(memblock_t &list, memblock_t *mb) {
	if (mb->lru_next || mb->lru_prev) {
		sys.error("memblock_list_linkLRU: active link");
	}
	list.len += mb->len;
	list.lru_next->lru_prev = mb;
	mb->lru_next = list.lru_next;
	mb->lru_prev = &list;
	list.lru_next = mb;
}

memblock_t* memblock_list_getLRU(memblock_t &list) {
	//list is empty
	if (list.lru_prev == &list) {
		return 0;
	}
	return list.lru_prev;
}

void cache::reset() {
	if (m_zone == 0) {
		m_zone = new byte[m_zone_len];
		memset(m_zone, 0, m_zone_len);
	}
	m_zone_used = 0;
	m_head = (memblock_t *)m_zone;
	m_head->len = m_zone_len;
	m_head->next = 0;
	m_head->prev = 0;
	m_head->owner = 0;
	m_head->lru_next = 0;
	m_head->lru_prev = 0;
	m_head->state = MB_STATE_FREE;

	memblock_list_init(m_mru);
	memblock_list_init(m_mfu);
	memblock_list_init(m_free);
	link(m_head);
}

void cache::check() {
#if 0
	memblock_t *next;
	memblock_t *end;
	
	next = m_head;
	end = (memblock_t *)(((byte *)m_head) + m_zone_len);
	do {
		memblock_t *check = (memblock_t *)(((byte *)next) + next->len);
		if (next->next != check && check != end) {
			printf("bad list %08x %08x %08x\n", next->next,check,end);
		}
		if (check != end && check->prev != next) {
			printf("bad list %08x %08x %08x\n", check, end, check->prev);
		}
		next = next->next;
	} while (next);

	next = m_free.lru_next;
	while (next != &m_free) {
		if (next->state != MB_STATE_FREE) {
			sys.error("bad state");
		}
		next = next->lru_next;
	}
	next = m_mru.lru_next;
	while (next != &m_mru) {
		if (next->state != MB_STATE_MRU) {
			sys.error("bad state");
		}
		next = next->lru_next;
	}
	next = m_mfu.lru_next;
	while (next != &m_mfu) {
		if (next->state != MB_STATE_MFU) {
			sys.error("bad state");
		}
		next = next->lru_next;
	}
#endif
}

#if 1

memblock_t *cache::try_alloc(int size) {
	memblock_t *next;
	next = m_free.lru_next;
	check();
	do {
		//at end of list
		if (next == &m_free) {
			break;
		}
		if (next->len >= size) {
			int extra = next->len - size;
			unlink(next);
			//add it back into the free list
			if (extra >= 2048) {
				//create a new block using the extra
				memblock_t *mem = (memblock_t *)(((byte *)next) + size);

				mem->state = MB_STATE_FREE;
				mem->len = extra;
				mem->prev = next;
				mem->next = next->next;
				if (mem->next) {
					mem->next->prev = mem;
				}
				next->next = mem;
				next->len = size;

				mem->lru_next = mem->lru_prev = 0;
				link(mem);
			}
			//mark the block as used and return it
			next->state = MB_STATE_MRU;
			link(next);
			check();
			return next;

		}
		next = next->lru_next;
	} while (1);

	return 0;
}

#else

memblock_t *cache::try_alloc(int size) {
	memblock_t *next;
	next = m_head;
	check();
	do {
		if (next->state == MB_STATE_FREE && next->len >= size) {
			int extra = next->len - size;
			unlink(next);
			//add it back into the free list
			if (extra >= 8192) {
				//create a new block using the extra
				memblock_t *mem = (memblock_t *)(((byte *)next)+size);

				mem->state = MB_STATE_FREE;
				mem->len = extra;
				mem->prev = next;
				mem->next = next->next;
				if (mem->next) {
					mem->next->prev = mem;
				}
				next->next = mem;
				next->len = size;

				mem->lru_next = mem->lru_prev = 0;
				link(mem);
			}
			//mark the block as used and return it
			next->state = MB_STATE_MRU;
			link(next);
			check();
			return next;

		}
		next = next->next;
	} while (next);

	return 0;
}
#endif

void cache::free(byte *data) {
	memblock_t *mb = ((memblock_t *)data) - 1;
	free(mb);
}

void cache::link(memblock_t *mb) {
	switch (mb->state) {
	case MB_STATE_FREE:
		memblock_list_linkLRU(m_free, mb);
		break;
	case MB_STATE_MFU:
		memblock_list_linkLRU(m_mfu, mb);
		break;
	case MB_STATE_MRU:
		memblock_list_linkLRU(m_mru, mb);
		break;
	default:
		sys.error("cache link: bad state\n");
	}
}

void cache::unlink(memblock_t *mb) {
	switch (mb->state) {
	case MB_STATE_FREE:
		memblock_list_unlinkLRU(m_free, mb);
		break;
	case MB_STATE_MFU:
		memblock_list_unlinkLRU(m_mfu, mb);
		break;
	case MB_STATE_MRU:
		memblock_list_unlinkLRU(m_mru, mb);
		break;
	default:
		sys.error("cache unlink: bad state\n");
	}
}


void cache::free(memblock_t *mb) {
	check();

	if (mb->owner) {
		*mb->owner = 0;
	}

	unlink(mb);

	//mark it as free
	mb->state = MB_STATE_FREE;
	check();

	host.dprintf("free %08x %d", mb, mb->len);

	//merge to prev if free
	if (mb->prev && mb->prev->state == MB_STATE_FREE) {
		host.dprintf("-");
		mb = mb->prev;
		unlink(mb);
		mb->len += mb->next->len;
		mb->next = mb->next->next;
		if (mb->next) {
			mb->next->prev = mb;
		}
	}
	check();
	//merge to next if free
	if (mb->next && mb->next->state == MB_STATE_FREE) {
		host.dprintf("+");
		memblock_t *next = mb->next;
		unlink(next);
		mb->len += next->len;
		mb->next = next->next;
		if (mb->next) {
			mb->next->prev = mb;
		}
	}
	check();
	link(mb);
	host.dprintf("\n");
}


void cache::touch(byte *data) {
	memblock_t *mb = ((memblock_t *)data) - 1;

	unlink(mb);
	mb->state = MB_STATE_MFU;
	link(mb);
}

byte *cache::alloc(byte **owner, int size) {
	memblock_t *mb;
	size = (size + sizeof(memblock_t) + 15) & ~15;
	int cb = 0;
	do {
		mb = try_alloc(size);
		if (mb) {
			mb->owner = owner;
			//::printf("alloc: %d %d\n", size, cb);
			return (byte *)(mb + 1);
		}
		//::printf("let's try again\n");
		if (m_mfu.len > m_mru.len) {
			mb = memblock_list_getLRU(m_mfu);
			unlink(mb);
			mb->state = MB_STATE_MRU;
			link(mb);
		}
		mb = memblock_list_getLRU(m_mru);
		if (!mb) {
			return 0;
		}
		cb++;
		//::printf("free %08x %d\n", mb, cb);
		free(mb);
	} while (1);

	::printf("alloc failed: %d %d\n", size, cb);
	return 0;
}


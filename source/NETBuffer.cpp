#include "NetBuffer.h"

NetBufferFixed<1024> msg;

int NetBuffer::write_from_file(sysFile* fp, int pos, int len) {
	if (fp == 0 || !fp->isopen()) {
		return 0;
	}
	byte *p = alloc(len);
	if (p) {
		int r = fp->read((char *)p, pos, len);
		return r;
	}
	return 0;
}

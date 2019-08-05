#include "ctr_vbo.h"
#include "memPool.h"
#include <string.h>

int ctrVboInit(ctrVbo_t* vbo) {
	if (!vbo) {
		return -1;
	}

	vbo->data = 0;
	vbo->currentSize = 0;
	vbo->maxSize = 0;
	vbo->commands = 0;
	vbo->commandsSize = 0;

	return 0;
}

int ctrVboCreate(ctrVbo_t* vbo, u32 size) {
	if (!vbo) {
		return -1;
	}

	vbo->data = (u8*)linear.alloc(size);
	vbo->numVertices = 0;
	vbo->currentSize = 0;
	vbo->maxSize = size;

	return 0;
}

int ctrVboClear(ctrVbo_t* vbo) {
	if (!vbo) {
		return -1;
	}

	vbo->numVertices = 0;
	vbo->currentSize = 0;

	return 0;
}


void* ctrVboGetOffset(ctrVbo_t* vbo) {
	if (!vbo) {
		return 0;
	}

	return (void*)(&((u8*)vbo->data)[vbo->currentSize]);
}

int ctrVboAddData(ctrVbo_t* vbo, void* data, u32 size, u32 units) {
	if (!vbo || !data || !size) {
		return -1;
	}
	if (((s32)vbo->maxSize) - ((s32)vbo->currentSize) < size)return -1;

	memcpy(ctrVboGetOffset(vbo), data, size);
	vbo->currentSize += size;
	vbo->numVertices += units;

	return 0;
}

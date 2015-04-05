#include "sys.h"
#include "q1Mdl.h"
#include "q1Bsp.h"
#include "q1Sprite.h"

extern SYS sys;
extern unsigned char *q1_palette;

template<>
void LoadList<class Model, 256>::reset() {
	m_num_known = 0;
}

template<>
Model* LoadList<class Model, 256>::load(char *name) {
	sysFile *file = sys.fileSystem.open(name);
	if (file == 0) {
		return 0;
	}
	Model *model = 0;
	int id;
	if (file->read(reinterpret_cast<char *>(&id), 0, sizeof(int)) == 0) {
		return 0;
	}
	switch (id) {
	case IDPOLYHEADER:
		model = new(pool) q1Mdl(name,file);
		break;
	case BSPVERSION:
		model = new(pool) q1Bsp(name,file);
		break;
	case IDSPRITEHEADER:
		model = new(pool) q1Sprite(name, file);
		break;
	}
	file->close();

	return model;
}

LoadList<Model, 256> Models;

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

#if 1
int q1_plane::on_side(bbox &box) {
	float	dist1, dist2;
	vec3_t emaxs, emins, normal;
	float dist;
	int		sides;

	/*// fast axial cases
	if (p->type < 3)
	{
	if (p->dist <= emins[p->type])
	return 1;
	if (p->dist >= emaxs[p->type])
	return 2;
	return 3;
	}*/

	box[0].copy_to(emins);
	box[1].copy_to(emaxs);
	m_normal.copy_to(normal);
	dist = m_dist.c_float();

	// general case
	switch (m_signbits)
	{
	case 0:
		dist1 = normal[0] * emaxs[0] + normal[1] * emaxs[1] + normal[2] * emaxs[2];
		dist2 = normal[0] * emins[0] + normal[1] * emins[1] + normal[2] * emins[2];
		break;
	case 1:
		dist1 = normal[0] * emins[0] + normal[1] * emaxs[1] + normal[2] * emaxs[2];
		dist2 = normal[0] * emaxs[0] + normal[1] * emins[1] + normal[2] * emins[2];
		break;
	case 2:
		dist1 = normal[0] * emaxs[0] + normal[1] * emins[1] + normal[2] * emaxs[2];
		dist2 = normal[0] * emins[0] + normal[1] * emaxs[1] + normal[2] * emins[2];
		break;
	case 3:
		dist1 = normal[0] * emins[0] + normal[1] * emins[1] + normal[2] * emaxs[2];
		dist2 = normal[0] * emaxs[0] + normal[1] * emaxs[1] + normal[2] * emins[2];
		break;
	case 4:
		dist1 = normal[0] * emaxs[0] + normal[1] * emaxs[1] + normal[2] * emins[2];
		dist2 = normal[0] * emins[0] + normal[1] * emins[1] + normal[2] * emaxs[2];
		break;
	case 5:
		dist1 = normal[0] * emins[0] + normal[1] * emaxs[1] + normal[2] * emins[2];
		dist2 = normal[0] * emaxs[0] + normal[1] * emins[1] + normal[2] * emaxs[2];
		break;
	case 6:
		dist1 = normal[0] * emaxs[0] + normal[1] * emins[1] + normal[2] * emins[2];
		dist2 = normal[0] * emins[0] + normal[1] * emaxs[1] + normal[2] * emaxs[2];
		break;
	case 7:
		dist1 = normal[0] * emins[0] + normal[1] * emins[1] + normal[2] * emins[2];
		dist2 = normal[0] * emaxs[0] + normal[1] * emaxs[1] + normal[2] * emaxs[2];
		break;
	default:
		dist1 = dist2 = 0;		// shut up compiler
		break;
	}

	sides = 0;
	if ((dist1 - dist) >= 0)
		sides = 1;
	if ((dist2 - dist) < 0)
		sides |= 2;

	return sides;
}
#else
int q1_plane::on_side(bbox &box) {
	fixed32p16	dist1, dist2;
	int		sides;


	/*// fast axial cases
	if (p->type < 3)
	{
	if (p->dist <= emins[p->type])
	return 1;
	if (p->dist >= emaxs[p->type])
	return 2;
	return 3;
	}*/



	// general case
	switch (m_signbits)
	{
	case 0:
		dist1 = m_normal[0].multiply3(box[1][0]) + m_normal[1].multiply3(box[1][1]) + m_normal[2].multiply3(box[1][2]);
		dist2 = m_normal[0].multiply3(box[0][0]) + m_normal[1].multiply3(box[0][1]) + m_normal[2].multiply3(box[0][2]);
		break;
	case 1:
		dist1 = m_normal[0].multiply3(box[0][0]) + m_normal[1].multiply3(box[1][1]) + m_normal[2].multiply3(box[1][2]);
		dist2 = m_normal[0].multiply3(box[1][0]) + m_normal[1].multiply3(box[0][1]) + m_normal[2].multiply3(box[0][2]);
		break;
	case 2:
		dist1 = m_normal[0].multiply3(box[1][0]) + m_normal[1].multiply3(box[0][1]) + m_normal[2].multiply3(box[1][2]);
		dist2 = m_normal[0].multiply3(box[0][0]) + m_normal[1].multiply3(box[1][1]) + m_normal[2].multiply3(box[0][2]);
		break;
	case 3:
		dist1 = m_normal[0].multiply3(box[0][0]) + m_normal[1].multiply3(box[0][1]) + m_normal[2].multiply3(box[1][2]);
		dist2 = m_normal[0].multiply3(box[1][0]) + m_normal[1].multiply3(box[1][1]) + m_normal[2].multiply3(box[0][2]);
		break;
	case 4:
		dist1 = m_normal[0].multiply3(box[1][0]) + m_normal[1].multiply3(box[1][1]) + m_normal[2].multiply3(box[0][2]);
		dist2 = m_normal[0].multiply3(box[0][0]) + m_normal[1].multiply3(box[0][1]) + m_normal[2].multiply3(box[1][2]);
		break;
	case 5:
		dist1 = m_normal[0].multiply3(box[0][0]) + m_normal[1].multiply3(box[1][1]) + m_normal[2].multiply3(box[0][2]);
		dist2 = m_normal[0].multiply3(box[1][0]) + m_normal[1].multiply3(box[0][1]) + m_normal[2].multiply3(box[1][2]);
		break;
	case 6:
		dist1 = m_normal[0].multiply3(box[1][0]) + m_normal[1].multiply3(box[0][1]) + m_normal[2].multiply3(box[0][2]);
		dist2 = m_normal[0].multiply3(box[0][0]) + m_normal[1].multiply3(box[1][1]) + m_normal[2].multiply3(box[1][2]);
		break;
	case 7:
		dist1 = m_normal[0].multiply3(box[0][0]) + m_normal[1].multiply3(box[0][1]) + m_normal[2].multiply3(box[0][2]);
		dist2 = m_normal[0].multiply3(box[1][0]) + m_normal[1].multiply3(box[1][1]) + m_normal[2].multiply3(box[1][2]);
		break;
	default:
		dist1 = dist2 = 0;		// shut up compiler
		break;
	}

	sides = 0;
	if (dist1 >=  m_dist)
		sides = 1;
	if (dist2 < m_dist)
		sides |= 2;

	return sides;
}
#endif

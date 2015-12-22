#include "sys.h"
#include "q1Mdl.h"
#include "Host.h"

extern unsigned char *q1_palette;


/*int q1Mdl::build_uv(int num, dtriangle_t *tris, stvert_t *in_stverts) {
	m_u = new(pool) fixed16p12[num * 3];
	m_v = new(pool) fixed16p12[num * 3];
	for (int i = 0; i < num; i++) {
		for (int j = 0; j < 3; j++) {
			for (int k = 0; k < 3; k++) {
				float s = (float)in_stverts[tris[i].vertindex[j]].s;
				float t = (float)in_stverts[tris[i].vertindex[j]].t;
				if (!tris[i].facesfront && in_stverts[tris[i].vertindex[j]].onseam) {
					s += (m_skin_width/2);
				}
				m_u[i * 3 + j] = s / m_skin_width;
				m_v[i * 3 + j] = t / m_skin_height;
			}
		}
	}
	return 0;
}*/

/*int q1MdlFrame::build(int num, dtriangle_t *tris, trivertx_t *verts) {
	m_triverts = verts;
	m_num_verts = num * 3;
	m_verts = new(pool)vec3_fixed8[num * 3];
	for (int i = 0; i < num; i++) {
		for (int j = 0; j < 3; j++) {
			for (int k = 0; k < 3; k++) {
				m_verts[i * 3 + j][k].x = verts[tris[i].vertindex[j]].v[k];
			}
		}
	}
	return 0;
}*/

void build_mdl_frame(q1MdlFrame *frame,
	dtriangle_t *tris,
	int numtris,
	trivertx_t *verts,
	mstvert_t *stverts,
	float skinwidth,
	float skinheight);

int q1MdlFrame::build(int num, dtriangle_t *tris, trivertx_t *verts, mstvert_t *st, int skinwidth,int skinheight) {
	build_mdl_frame(this,tris,num,verts,st,(float)skinwidth,(float)skinheight);
	return 0;
}

q1MdlFrame* q1Mdl::get_frame(int framenum) {
	if (framenum >= m_num_groups) {
		framenum = 0;
	}
	q1MdlFrameGroup *group = &m_groups[framenum];
	q1MdlFrame *frame = 0;

	if (group->m_intervals) {
		int i;
		int numframes = m_groups[framenum].m_num_frames;
		float *intervals = m_groups[framenum].m_intervals;
		float fullinterval = intervals[numframes - 1];
		float time = host.cl_time();

		float targettime = time - ((int)(time / fullinterval)) * fullinterval;
		for (i = 0; i<(numframes - 1); i++)
		{
			if (intervals[i] > targettime)
				break;
		}
		frame = &m_groups[framenum].m_frames[i];
	}
	else {
		frame = &group->m_frames[0];
	}

	return frame;
}

int q1Mdl::trace(traceResults &trace, int hull, vec3_fixed16 &p0, vec3_fixed16 &p1) {
	return 0;
}

int mdl_cb = 0;
int mdl_tx = 0;
q1Mdl::q1Mdl(char *name, sysFile *file):Model(name) {
	rframe = 0;

	m_type = mod_alias;

	m_file = file;

	m_fullbright = false;
	if (name && (strstr(name, "flame") || strstr(name, "bolt"))) {
		m_fullbright = true;
	}

	mdl_t mdl;
	if (m_file->read(reinterpret_cast<char *>(&mdl), 0, sizeof(mdl)) == 0) {
		return;
	}
	int skin_size = mdl.skinwidth*mdl.skinheight;
	m_skin_width = mdl.skinwidth;
	m_skin_height = mdl.skinheight;
	m_flags = mdl.flags;
	for (int i = 0; i < 3; i++) {
		m_scale[i] = mdl.scale[i];
		m_scale_origin[i] = mdl.scale_origin[i];
	}


	//load skins
	m_num_skins = mdl.numskins;
	m_skin_texture_ids = new unsigned int[mdl.numskins];
	char *in_skin = new char[skin_size];
	for (int i = 0; i < mdl.numskins; i++) {
		daliasskintype_t skin_type;
		if (m_file->read(reinterpret_cast<char *>(&skin_type), -1, sizeof(skin_type)) == 0) {
			return;
		}
		if (skin_type.type == ALIAS_SKIN_SINGLE) {
			if (m_file->read(in_skin, -1, skin_size) == 0) {
				return;
			}
			m_skin_texture_ids[i] = sys.gen_texture_id();
			sys.load_texture256(m_skin_texture_ids[i],m_skin_width,m_skin_height,(unsigned char *)in_skin,q1_palette,1);
			mdl_tx += sys.texture_size(m_skin_texture_ids[i]);
		}
		else {
			//is this used?
			host.printf("Unimplemented: ALIAS_SKIN_GROUP detected in %s\n",m_file->name());
			return;
		}
	}
	delete[] in_skin;

	//load st verts
	stvert_t *in_stverts = new stvert_t[mdl.numverts];
	if (m_file->read(reinterpret_cast<char *>(in_stverts), -1, sizeof(stvert_t)*mdl.numverts) == 0) {
		return;
	}
	//build texture coords
	m_st = new(pool) mstvert_t[mdl.numverts];
	for (int i = 0; i < mdl.numverts; i++) {
		m_st[i].s = (float)in_stverts[i].s/m_skin_width;
		m_st[i].t = (float)in_stverts[i].t/m_skin_height;
		m_st[i].onseam = in_stverts[i].onseam;
	}

	//load triangles
	m_num_tris = mdl.numtris;
	dtriangle_t *in_tris = m_tris = new(pool) dtriangle_t[mdl.numtris];
	if (m_file->read(reinterpret_cast<char *>(in_tris), -1, sizeof(dtriangle_t)*mdl.numtris) == 0) {
		return;
	}

	//build_uv(mdl.numtris, in_tris, in_stverts);

	//done with stverts
	delete [] in_stverts;

	//build the mesh
	void BuildTris(q1Mdl *mdl, dtriangle_t *tris, int numtris, mstvert_t *stverts);
	BuildTris(this, in_tris, mdl.numtris, m_st);

	//load frames
	m_num_groups = mdl.numframes;
	m_groups = new(pool) q1MdlFrameGroup[mdl.numframes];
	for (int i = 0; i < mdl.numframes; i++) {

		//load frame type
		daliasframetype_t frame_type;
		if (m_file->read(reinterpret_cast<char *>(&frame_type), -1, sizeof(frame_type)) == 0) {
			return;
		}

		if (frame_type.type == ALIAS_SINGLE) {
			//load single frames
			daliasframe_t alias_frame;
			if (m_file->read(reinterpret_cast<char *>(&alias_frame), -1, sizeof(alias_frame)) == 0) {
				return;
			}
			trivertx_t *in_vert = new(pool)trivertx_t[mdl.numverts];
			if (m_file->read(reinterpret_cast<char *>(in_vert), -1, sizeof(trivertx_t)*mdl.numverts) == 0) {
				return;
			}
			m_groups[i].allocate_frames(1);
			m_groups[i].m_frames[0].m_triverts = in_vert;
			//gsVboInit(&m_groups[i].m_frames[0].m_vbo);
			//gsVboCreate(&m_groups[i].m_frames[0].m_vbo, sizeof(faceVertex_s)*(m_num_tris)* 3);
			mdl_cb += sizeof(faceVertex_s)*(m_num_tris)* 3;
#if Q1_MDL_FRAME_NAMES
			memcpy(m_groups[i].m_frames[0].m_name, alias_frame.name, 16);
#endif
			m_groups[i].m_frames[0].build(mdl.numtris, in_tris, in_vert, m_st,m_skin_width,m_skin_height);
			//delete[] in_vert;
		}
		else {
			//load frame groups
			daliasgroup_t alias_group;
			if (m_file->read(reinterpret_cast<char *>(&alias_group), -1, sizeof(alias_group)) == 0) {
				return;
			}
			//load intervals
			daliasinterval_t *alias_intervals = new daliasinterval_t[alias_group.numframes];
			if (m_file->read(reinterpret_cast<char *>(alias_intervals), -1, sizeof(daliasinterval_t)*alias_group.numframes) == 0) {
				return;
			}
			m_groups[i].m_intervals = new(pool) float[alias_group.numframes];
			for (int j = 0; j < alias_group.numframes; j++) {
				m_groups[i].m_intervals[j] = alias_intervals[j].interval;
			}
			delete [] alias_intervals;

			//load frames for group
			m_groups[i].allocate_frames(alias_group.numframes);
			for (int j = 0; j < alias_group.numframes; j++) {
				daliasframe_t alias_frame;
				if (m_file->read(reinterpret_cast<char *>(&alias_frame), -1, sizeof(alias_frame)) == 0) {
					return;
				}
				trivertx_t *in_vert = new(pool)trivertx_t[mdl.numverts];
				if (m_file->read(reinterpret_cast<char *>(in_vert), -1, sizeof(trivertx_t)*mdl.numverts) == 0) {
					return;
				}
				m_groups[i].m_frames[j].m_triverts = in_vert;
				//gsVboInit(&m_groups[i].m_frames[j].m_vbo);
				//gsVboCreate(&m_groups[i].m_frames[j].m_vbo, sizeof(faceVertex_s)*(m_num_tris)* 3);
#if Q1_MDL_FRAME_NAMES
				memcpy(m_groups[i].m_frames[j].m_name, alias_frame.name, 16);
#endif
				m_groups[i].m_frames[j].build(mdl.numtris, in_tris, in_vert, m_st, m_skin_width, m_skin_height);
				mdl_cb += sizeof(faceVertex_s)*(m_num_tris)* 3;
				//delete[] in_vert;
			}
		}
	}
	//m_file->
	//delete[] in_tris;

	// FIXME: do this right
	for (int i = 0; i < 3; i++) {
		m_bounds[0][i] = -16.0f;
		m_bounds[1][i] = 16.0f;
	}
	m_radius = 0;
}

extern u32* __ctru_linear_heap;
extern shaderProgram_s q1Mdl_shader;

void q1Mdl::set_render_mode() {
	u32 bufferOffsets = 0;
	u64 bufferPermutations = 0x210;
	u8 bufferNumAttributes = 2;

	gsShaderSet(&q1Mdl_shader);

	GPU_SetAttributeBuffers(2, (u32*)osConvertVirtToPhys((void *)__ctru_linear_heap),
		GPU_ATTRIBFMT(0, 4, GPU_UNSIGNED_BYTE) | GPU_ATTRIBFMT(1, 2, GPU_FLOAT),
		0xFFC, 0x210, 1, &bufferOffsets, &bufferPermutations, &bufferNumAttributes);
	
	GPU_SetTextureEnable(GPU_TEXUNIT0);
	GPU_SetTexEnv(0,
		GPU_TEVSOURCES(GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
		GPU_TEVSOURCES(GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_MODULATE, GPU_MODULATE,
		0xFFFFFFFF);
}

vec3_t q1Mdl::m_normals[162] = {
	{ -0.525731f, 0.000000f, 0.850651f },
	{ -0.442863f, 0.238856f, 0.864188f },
	{ -0.295242f, 0.000000f, 0.955423f },
	{ -0.309017f, 0.500000f, 0.809017f },
	{ -0.162460f, 0.262866f, 0.951056f },
	{ 0.000000f, 0.000000f, 1.000000f },
	{ 0.000000f, 0.850651f, 0.525731f },
	{ -0.147621f, 0.716567f, 0.681718f },
	{ 0.147621f, 0.716567f, 0.681718f },
	{ 0.000000f, 0.525731f, 0.850651f },
	{ 0.309017f, 0.500000f, 0.809017f },
	{ 0.525731f, 0.000000f, 0.850651f },
	{ 0.295242f, 0.000000f, 0.955423f },
	{ 0.442863f, 0.238856f, 0.864188f },
	{ 0.162460f, 0.262866f, 0.951056f },
	{ -0.681718f, 0.147621f, 0.716567f },
	{ -0.809017f, 0.309017f, 0.500000f },
	{ -0.587785f, 0.425325f, 0.688191f },
	{ -0.850651f, 0.525731f, 0.000000f },
	{ -0.864188f, 0.442863f, 0.238856f },
	{ -0.716567f, 0.681718f, 0.147621f },
	{ -0.688191f, 0.587785f, 0.425325f },
	{ -0.500000f, 0.809017f, 0.309017f },
	{ -0.238856f, 0.864188f, 0.442863f },
	{ -0.425325f, 0.688191f, 0.587785f },
	{ -0.716567f, 0.681718f, -0.147621f },
	{ -0.500000f, 0.809017f, -0.309017f },
	{ -0.525731f, 0.850651f, 0.000000f },
	{ 0.000000f, 0.850651f, -0.525731f },
	{ -0.238856f, 0.864188f, -0.442863f },
	{ 0.000000f, 0.955423f, -0.295242f },
	{ -0.262866f, 0.951056f, -0.162460f },
	{ 0.000000f, 1.000000f, 0.000000f },
	{ 0.000000f, 0.955423f, 0.295242f },
	{ -0.262866f, 0.951056f, 0.162460f },
	{ 0.238856f, 0.864188f, 0.442863f },
	{ 0.262866f, 0.951056f, 0.162460f },
	{ 0.500000f, 0.809017f, 0.309017f },
	{ 0.238856f, 0.864188f, -0.442863f },
	{ 0.262866f, 0.951056f, -0.162460f },
	{ 0.500000f, 0.809017f, -0.309017f },
	{ 0.850651f, 0.525731f, 0.000000f },
	{ 0.716567f, 0.681718f, 0.147621f },
	{ 0.716567f, 0.681718f, -0.147621f },
	{ 0.525731f, 0.850651f, 0.000000f },
	{ 0.425325f, 0.688191f, 0.587785f },
	{ 0.864188f, 0.442863f, 0.238856f },
	{ 0.688191f, 0.587785f, 0.425325f },
	{ 0.809017f, 0.309017f, 0.500000f },
	{ 0.681718f, 0.147621f, 0.716567f },
	{ 0.587785f, 0.425325f, 0.688191f },
	{ 0.955423f, 0.295242f, 0.000000f },
	{ 1.000000f, 0.000000f, 0.000000f },
	{ 0.951056f, 0.162460f, 0.262866f },
	{ 0.850651f, -0.525731f, 0.000000f },
	{ 0.955423f, -0.295242f, 0.000000f },
	{ 0.864188f, -0.442863f, 0.238856f },
	{ 0.951056f, -0.162460f, 0.262866f },
	{ 0.809017f, -0.309017f, 0.500000f },
	{ 0.681718f, -0.147621f, 0.716567f },
	{ 0.850651f, 0.000000f, 0.525731f },
	{ 0.864188f, 0.442863f, -0.238856f },
	{ 0.809017f, 0.309017f, -0.500000f },
	{ 0.951056f, 0.162460f, -0.262866f },
	{ 0.525731f, 0.000000f, -0.850651f },
	{ 0.681718f, 0.147621f, -0.716567f },
	{ 0.681718f, -0.147621f, -0.716567f },
	{ 0.850651f, 0.000000f, -0.525731f },
	{ 0.809017f, -0.309017f, -0.500000f },
	{ 0.864188f, -0.442863f, -0.238856f },
	{ 0.951056f, -0.162460f, -0.262866f },
	{ 0.147621f, 0.716567f, -0.681718f },
	{ 0.309017f, 0.500000f, -0.809017f },
	{ 0.425325f, 0.688191f, -0.587785f },
	{ 0.442863f, 0.238856f, -0.864188f },
	{ 0.587785f, 0.425325f, -0.688191f },
	{ 0.688191f, 0.587785f, -0.425325f },
	{ -0.147621f, 0.716567f, -0.681718f },
	{ -0.309017f, 0.500000f, -0.809017f },
	{ 0.000000f, 0.525731f, -0.850651f },
	{ -0.525731f, 0.000000f, -0.850651f },
	{ -0.442863f, 0.238856f, -0.864188f },
	{ -0.295242f, 0.000000f, -0.955423f },
	{ -0.162460f, 0.262866f, -0.951056f },
	{ 0.000000f, 0.000000f, -1.000000f },
	{ 0.295242f, 0.000000f, -0.955423f },
	{ 0.162460f, 0.262866f, -0.951056f },
	{ -0.442863f, -0.238856f, -0.864188f },
	{ -0.309017f, -0.500000f, -0.809017f },
	{ -0.162460f, -0.262866f, -0.951056f },
	{ 0.000000f, -0.850651f, -0.525731f },
	{ -0.147621f, -0.716567f, -0.681718f },
	{ 0.147621f, -0.716567f, -0.681718f },
	{ 0.000000f, -0.525731f, -0.850651f },
	{ 0.309017f, -0.500000f, -0.809017f },
	{ 0.442863f, -0.238856f, -0.864188f },
	{ 0.162460f, -0.262866f, -0.951056f },
	{ 0.238856f, -0.864188f, -0.442863f },
	{ 0.500000f, -0.809017f, -0.309017f },
	{ 0.425325f, -0.688191f, -0.587785f },
	{ 0.716567f, -0.681718f, -0.147621f },
	{ 0.688191f, -0.587785f, -0.425325f },
	{ 0.587785f, -0.425325f, -0.688191f },
	{ 0.000000f, -0.955423f, -0.295242f },
	{ 0.000000f, -1.000000f, 0.000000f },
	{ 0.262866f, -0.951056f, -0.162460f },
	{ 0.000000f, -0.850651f, 0.525731f },
	{ 0.000000f, -0.955423f, 0.295242f },
	{ 0.238856f, -0.864188f, 0.442863f },
	{ 0.262866f, -0.951056f, 0.162460f },
	{ 0.500000f, -0.809017f, 0.309017f },
	{ 0.716567f, -0.681718f, 0.147621f },
	{ 0.525731f, -0.850651f, 0.000000f },
	{ -0.238856f, -0.864188f, -0.442863f },
	{ -0.500000f, -0.809017f, -0.309017f },
	{ -0.262866f, -0.951056f, -0.162460f },
	{ -0.850651f, -0.525731f, 0.000000f },
	{ -0.716567f, -0.681718f, -0.147621f },
	{ -0.716567f, -0.681718f, 0.147621f },
	{ -0.525731f, -0.850651f, 0.000000f },
	{ -0.500000f, -0.809017f, 0.309017f },
	{ -0.238856f, -0.864188f, 0.442863f },
	{ -0.262866f, -0.951056f, 0.162460f },
	{ -0.864188f, -0.442863f, 0.238856f },
	{ -0.809017f, -0.309017f, 0.500000f },
	{ -0.688191f, -0.587785f, 0.425325f },
	{ -0.681718f, -0.147621f, 0.716567f },
	{ -0.442863f, -0.238856f, 0.864188f },
	{ -0.587785f, -0.425325f, 0.688191f },
	{ -0.309017f, -0.500000f, 0.809017f },
	{ -0.147621f, -0.716567f, 0.681718f },
	{ -0.425325f, -0.688191f, 0.587785f },
	{ -0.162460f, -0.262866f, 0.951056f },
	{ 0.442863f, -0.238856f, 0.864188f },
	{ 0.162460f, -0.262866f, 0.951056f },
	{ 0.309017f, -0.500000f, 0.809017f },
	{ 0.147621f, -0.716567f, 0.681718f },
	{ 0.000000f, -0.525731f, 0.850651f },
	{ 0.425325f, -0.688191f, 0.587785f },
	{ 0.587785f, -0.425325f, 0.688191f },
	{ 0.688191f, -0.587785f, 0.425325f },
	{ -0.955423f, 0.295242f, 0.000000f },
	{ -0.951056f, 0.162460f, 0.262866f },
	{ -1.000000f, 0.000000f, 0.000000f },
	{ -0.850651f, 0.000000f, 0.525731f },
	{ -0.955423f, -0.295242f, 0.000000f },
	{ -0.951056f, -0.162460f, 0.262866f },
	{ -0.864188f, 0.442863f, -0.238856f },
	{ -0.951056f, 0.162460f, -0.262866f },
	{ -0.809017f, 0.309017f, -0.500000f },
	{ -0.864188f, -0.442863f, -0.238856f },
	{ -0.951056f, -0.162460f, -0.262866f },
	{ -0.809017f, -0.309017f, -0.500000f },
	{ -0.681718f, 0.147621f, -0.716567f },
	{ -0.681718f, -0.147621f, -0.716567f },
	{ -0.850651f, 0.000000f, -0.525731f },
	{ -0.688191f, 0.587785f, -0.425325f },
	{ -0.587785f, 0.425325f, -0.688191f },
	{ -0.425325f, 0.688191f, -0.587785f },
	{ -0.425325f, -0.688191f, -0.587785f },
	{ -0.587785f, -0.425325f, -0.688191f },
	{ -0.688191f, -0.587785f, -0.425325f } };
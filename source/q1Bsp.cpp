#include "sys.h"
#include "Host.h"
#include "q1Bsp.h"
#include "procTexture.h"
#include "cache.h"
#include "MaxRectsBinPack.h"

extern cache textureCache;
extern cache vboCache;

extern SYS sys;
extern unsigned char *q1_palette;
procTextureList proc_textures(8);

int vbo_cb = 0;
int vbo_tx = 0;

void _pal256toRGBA(tex3ds_t *tx, unsigned char *data, unsigned char *palette, int trans);

void q1_texture::bind() {
	sys.bind_texture(m_id);
	return;

	if (!m_cachable) {
		sys.bind_texture(m_id);
		return;
	}
	if (m_cache) {
		textureCache.touch(m_cache);
		sys.bind_texture(m_id);
		return;
	}

	tex3ds_t *tex = (tex3ds_t*)m_id;
	if (!tex) {
		return;
	}
	::printf("loading texture\n");
	int width = tex->width;
	int height = tex->height;
	int cb = width * height * 2 + 127;
	m_cache = textureCache.alloc(&m_cache, cb);
	if (m_cache == 0) {
		return;
	}
	byte *data = (byte *)((((u32)m_cache) + 127) & (~127));


	tex->data = data;
	tex->type = GPU_RGBA5551;
	_pal256toRGBA(tex, m_data, q1_palette, 0);
	sys.bind_texture(m_id);
}

void build_face(q1_face *face);

void q1_face::bind() {
	/*if (m_cache) {
		vboCache.touch(m_cache);
		return;
	}
	gsVboInit(&vbo);
	int cb = sizeof(faceVertex_s)*m_num_points;
	m_cache = vboCache.alloc(&m_cache, cb + 15);
	byte *data = (byte *)((((u32)m_cache) + 15) & (~15));
	vbo.data = data;
	vbo.maxSize = cb;
	build_face(this);*/
}


q1_texture * q1_texture::animate(int frame) {
	int		reletive;
	int		count;
	q1_texture *base = this;

	//if(strcmp(base->ds.name,"+0basebtn")==0) {
	//	int kasjd = 0;
	//}

	if (frame)
	{
		if (alternate_anims)
			base = alternate_anims;
	}

	if (!base->anim_total)
		return base;

	reletive = (int)(host.cl_time() * 10) % base->anim_total;

	count = 0;
	while (base->anim_min > reletive || base->anim_max <= reletive)
	{
		base = base->anim_next;
		if (!base)
			sys.error("R_TextureAnimation: broken cycle");
		if (++count > 100)
			sys.error("R_TextureAnimation: infinite cycle");
	}

	return base;
}

extern u32* __linear_heap;
extern shaderProgram_s q1Bsp_shader;

void q1Bsp::set_render_mode() {
	//texturing stuff
	u32 bufferOffsets = 0;
	u64 bufferPermutations = 0x210;
	u8 bufferNumAttributes = 3;

	gsShaderSet(&q1Bsp_shader);

	GPU_SetAttributeBuffers(3, (u32*)osConvertVirtToPhys((u32)__linear_heap),
		GPU_ATTRIBFMT(0, 3, GPU_FLOAT) | GPU_ATTRIBFMT(1, 2, GPU_FLOAT) | GPU_ATTRIBFMT(2, 2, GPU_FLOAT),
		0xFFC, 0x210, 1, &bufferOffsets, &bufferPermutations, &bufferNumAttributes);
	
	GPU_SetTextureEnable((GPU_TEXUNIT)(GPU_TEXUNIT0 | GPU_TEXUNIT1));
	GPU_SetTexEnv(0,
		GPU_TEVSOURCES(GPU_TEXTURE0, GPU_TEXTURE1, GPU_PRIMARY_COLOR),
		GPU_TEVSOURCES(GPU_TEXTURE0, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
		GPU_TEVOPERANDS(0, 2, 0),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_MODULATE, GPU_MODULATE,
		0xFFFFFFFF);
}

unsigned char *q1Bsp::decompress_vis(unsigned char *in)
{
	static unsigned char decompressed[1024];
	int		c;
	unsigned char *out;
	int		row;

	row = (m_num_leafs + 7) >> 3;
	out = decompressed;

	if (m_visibility == 0 || in == 0) {
		memset(decompressed, 0xff, row);
		return decompressed;
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);

	return decompressed;
}

int q1Bsp::mark_visible_leafs(q1_leaf_node &leaf) {

	unsigned char *vis = decompress_vis(leaf.m_compressed_vis);

	for (int i = 0; i < m_num_leafs; i++) {
		if (vis[i >> 3] & (1 << (i & 7)))
		{
			if (m_leafs[i + 1].m_visframe != m_visframe) {
				m_leafs[i + 1].m_visframe = m_visframe;
			}
			/*q1_node *node = m_leafs[i + 1].m_parent;
			do
			{
			if (node->m_visframe == m_visframe)
			break;
			node->m_visframe = m_visframe;
			node = node->m_parent;
			} while (node);*/
		}
	}
	return 0;
}

int q1Bsp::mark_visible_leafs(q1_leaf_node &leaf,int frame) {

	unsigned char *vis = decompress_vis(leaf.m_compressed_vis);

	for (int i = 0; i < m_num_leafs; i++) {
		if (vis[i >> 3] & (1 << (i & 7)))
		{
			m_leafs[i + 1].m_visframe = frame;
			q1_node *node = m_leafs[i + 1].m_parent;
			do
			{
				if (node->m_visframe == frame) {
					break;
				}
				node->m_visframe = frame;
				node = node->m_parent;
			} while (node);
		}
	}
	return 0;
}

q1_leaf_node* q1Bsp::in_leaf() {
	q1_bsp_node *node = &m_nodes[0];
	do {
		if (node->m_contents != 0) {
			return (q1_leaf_node *)node;
		}
		fixed32p16 dot = *(node->m_plane) * m_pos;

		node = node->m_children[dot < 0 ? 1 : 0];

	} while (node);

	return 0;
}

q1_leaf_node* q1Bsp::in_leaf(vec3_fixed16 &pos) {
	q1_bsp_node *node = &m_nodes[0];
	do {
		if (node->m_contents != 0) {
			return (q1_leaf_node *)node;
		}
		fixed32p16 dot = *(node->m_plane) * pos;

		node = node->m_children[dot < 0 ? 1 : 0];

	} while (node);

	return 0;
}

q1_leaf_node* q1Bsp::in_leaf(vec3_t vpos) {
	vec3_fixed16 pos;
	pos = vpos;
	q1_bsp_node *node = &m_nodes[0];
	do {
		if (node->m_contents != 0) {
			return (q1_leaf_node *)node;
		}
		fixed32p16 dot = *(node->m_plane) * pos;

		node = node->m_children[dot < 0 ? 1 : 0];

	} while (node);

	return 0;
}

int q1Bsp::leaf_num(q1_leaf_node *leaf) {
	int num = leaf - m_leafs;
	return num;
}

int q1Bsp::trace(traceResults &trace, int hull, vec3_fixed16 &p0, vec3_fixed16 &p1) {
	trace.fraction = 1.0f;
	//trace.plane = 0;
	vec3_fixed32 pp0;
	vec3_fixed32 pp1;
	fixed32p16 f0(0.0f);
	fixed32p16 f1(1.0f);
	int ret;

	for (int i = 0; i < 3; i++) {
		pp0[i].x = p0[i].x << 13;
		pp1[i].x = p1[i].x << 13;
	}

	if (hull == 0) {
		ret = trace_r(0, &m_nodes[0], trace, f0, f1, pp0, pp1);
	}
	else {
		ret = trace_clip_r(0, 0, trace, f0, f1, pp0, pp1);
	}
	if (trace.fraction == 1) {
		trace.end = p1;
	}
	else if (trace.fraction == 0) {
		trace.end = p0;
	}
	else {
		vec3_fixed32 midp;
		midp = pp0 + (pp1 - pp0) * trace.fraction;
		for (int i = 0; i < 3; i++) {
			trace.end[i].x = midp[i].x >> 13;
		}
	}
	trace.start = p0;

	return ret;
}

int q1Bsp::trace_r(q1_bsp_node *parent, q1_bsp_node *pnode, traceResults &trace, fixed32p16 f0, fixed32p16 f1, vec3_fixed32 &p0, vec3_fixed32 &p1) {

	static const fixed32p16 offset(0.001f);
	static const fixed32p16 one(1.0f);
	fixed32p16 d0;
	fixed32p16 d1;

	// already hit something nearer
	if (trace.fraction <= f0) {
		return 1;		
	}

	do {
		//alread at end
		if (trace.fraction == 0) {
			return 0;
		}

		//in a leaf
		if (pnode->m_contents) {
			if (pnode->m_contents == CONTENTS_SOLID) {
				trace.fraction = f0;
				if (parent) {
					trace.plane = *parent->m_plane;
				}
				return 1;
			}
			//in water
			return 0;
		}
		
		d0 = *(pnode->m_plane) * p0;
		d1 = *(pnode->m_plane) * p1;

		//both in front
		if (d0 >= 0 && d1 >= 0) {
			pnode = pnode->m_children[0];
			//return trace_r(*node.m_children[0], fraction, f0, f1, p0, p1);
			continue;
		}

		//both in back
		if (d0 < 0 && d1 < 0) {
			pnode = pnode->m_children[1];
			//return trace_r(*node.m_children[1], fraction, f0, f1, p0, p1);
			continue;
		}

		//found a split point
		break;
	} while (1);

#if 1
	int ret;
	fixed32p16 frac0;
	int side;
	if (d0 < d1) {
		side = 1;
		frac0 = d0 / (d0 - d1);
	}
	else if (d0 > d1) {
		side = 0;
		frac0 = d0 / (d0 - d1);
	}
	else {
		side = 0;
		frac0 = 1.0f;
	}

	//make sure 0 to 1
	if (frac0 < 0) {
		frac0 = 0;
	} else if (frac0 > one) {
		frac0 = one;
	}

	//generate a split
	vec3_fixed32 midp;
	fixed32p16 midf;

	midp = p0 + (p1 - p0) * frac0;
	midf = f0 + (f1 - f0) * frac0;

	ret = trace_r(pnode, pnode->m_children[side], trace, f0, midf, p0, midp);
	if (ret != 0) {
		return ret;
	}
	ret = trace_r(pnode, pnode->m_children[side^1], trace, midf, f1, midp, p1);
	if (ret != 1) {
		return ret;
	}
	//fraction = midf;
	return 2;
#else
	fixed32p16 frac0;
	fixed32p16 frac1;
	int side;
	if (d0 < d1) {
		side = 1;
		frac1 = (d0 + offset) / (d0 - d1);
		frac0 = (d0 - offset) / (d0 - d1);
	}
	else if (d0 > d1) {
		side = 0;
		frac1 = (d0 - offset) / (d0 - d1);
		frac0 = (d0 + offset) / (d0 - d1);
	}
	else {
		side = 0;
		frac0 = 1.0f;
		frac1 = 0;
	}

	//make sure 0 to 1
	if (frac0 < 0) {
		frac0 = 0;
	} else if (frac0 > one) {
		frac0 = one;
	}

	//generate a split
	vec3_fixed32 midp;
	fixed32p16 midf;

	midp = p0 + (p1 - p0) * frac0;
	midf = f0 + (f1 - f0) * frac0;

	trace_r(pnode->m_children[side], fraction, f0, midf, p0, midp);

	//make sure 0 to 1
	if (frac1 < 0) {
		frac1 = 0;
	} else if (frac1 > one) {
		frac1 = one;
	}

	midp = p0 + (p1 - p0) * frac1;
	midf = f0 + (f1 + f0) * frac1;

	trace_r(pnode->m_children[side^1], fraction, midf, f1, midp, p1);
	return 2;
#endif
}

int q1Bsp::trace_clip_r(short parentnum, short nodenum, traceResults &trace, fixed32p16 f0, fixed32p16 f1, vec3_fixed32 &p0, vec3_fixed32 &p1) {

	static const fixed32p16 offset(0.001f);
	static const fixed32p16 one(1.0f);
	fixed32p16 d0;
	fixed32p16 d1;
	q1_clipnode *node;

	// already hit something nearer
	if (trace.fraction <= f0) {
		return 1;
	}

	do {
		//alread at end
		if (trace.fraction == 0) {
			return 0;
		}

		//in a leaf
		if (nodenum < 0) {
			if (nodenum == CONTENTS_SOLID) {
				trace.fraction = f0;
				//if (parent) {
				//	trace.plane = parent->m_plane;
				//}
				return 1;
			}
			//in water
			return 0;
		}

		node = &m_clipnodes[nodenum];

		d0 = *(node->m_plane) * p0;
		d1 = *(node->m_plane) * p1;

		//both in front
		if (d0 >= 0 && d1 >= 0) {
			parentnum = nodenum;
			nodenum = node->m_children[0];
			//return trace_r(*node.m_children[0], fraction, f0, f1, p0, p1);
			continue;
		}

		//both in back
		if (d0 < 0 && d1 < 0) {
			parentnum = nodenum;
			nodenum = node->m_children[1];
			//return trace_r(*node.m_children[1], fraction, f0, f1, p0, p1);
			continue;
		}

		//found a split point
		break;
	} while (1);

	int ret;
	fixed32p16 frac0;
	int side;
	if (d0 < d1) {
		side = 1;
		frac0 = d0 / (d0 - d1);
	}
	else if (d0 > d1) {
		side = 0;
		frac0 = d0 / (d0 - d1);
	}
	else {
		side = 0;
		frac0 = 1.0f;
	}

	//make sure 0 to 1
	if (frac0 < 0) {
		frac0 = 0;
	}
	else if (frac0 > one) {
		frac0 = one;
	}

	//generate a split
	vec3_fixed32 midp;
	fixed32p16 midf;

	midp = p0 + (p1 - p0) * frac0;
	midf = f0 + (f1 - f0) * frac0;

	ret = trace_clip_r(nodenum, node->m_children[side], trace, f0, midf, p0, midp);
	if (ret != 0) {
		return ret;
	}
	ret = trace_clip_r(nodenum, node->m_children[side ^ 1], trace, midf, f1, midp, p1);
	if (ret != 1) {
		return ret;
	}
	//fraction = midf;
	return 2;
}

/*
int q1_leaf_node::render(int frame) {
	for (int i = 0; i < m_num_faces; i++) {
		m_first_face[i]->render(frame);
	}
	return 0;
}

int q1Bsp::render_bsp_r(q1_bsp_node *node) {
	if (node == 0) {
		return -1;
	}
	do {
		if (node->m_contents != 0) {
			q1_leaf_node *leaf = (q1_leaf_node *)node;
			if (leaf->m_visframe == m_visframe) {
				leaf->render(m_frame);
				//host.m_cl.store_efrags(&leaf->m_efrags);
			}
			return -1;
		}

		render_bsp_r(node->m_children[1]);

		node = node->m_children[0];

	} while (node);

	return 0;
}
*/

q1Bsp::q1Bsp(char *name, sysFile* file) :Model(name) {

	host.dprintf("loading bsp\n");
	m_type = mod_brush;
	m_file = file;
	m_lightmaps = 0;
	if (m_file == 0) {
		return;
	}
	host.dprintf("reading header\n");
	if (m_file->read(reinterpret_cast<char *>(&m_header), 0, sizeof(m_header)) == 0) {
		return;
	}
	m_skytexture = -1;
	m_skytexture_id = 0;

	host.dprintf("planes\n");
	load_planes();

	host.dprintf("textures\n");
	load_textures();

	host.dprintf("texinfos\n");
	load_texinfos();

	host.dprintf("lighting\n");
	load_lighting();

	host.dprintf("faces\n");
	load_faces();

	//needs to happen after faces and lighting
	host.dprintf("building lightmaps...\n");
	build_lightmaps();

	host.dprintf("markfaces\n");
	load_markfaces();

	host.dprintf("visibility\n");
	load_visibility();

	host.dprintf("leafs\n");
	load_leafs();

	host.dprintf("nodes\n");
	load_nodes();

	host.dprintf("clipnodes\n");
	load_clipnodes();

	host.dprintf("subnodels\n");
	load_submodels();

	host.dprintf("hulls\n");
	make_hulls();


	host.dprintf("creating submodels\n");
	//create all the sub models
	for (int i = 1; i < m_num_submodels; i++) {
		Model *mod = new(pool)q1Bsp(i, this);
		Models.add(mod);
	}

	host.dprintf("fixup\n");
	m_bounds = m_submodels[0].m_bounds;
	m_num_leafs = m_submodels[0].m_visleafs;
	m_firstmodelsurface = m_submodels[0].m_firstface;
	m_nummodelsurfaces = m_submodels[0].m_numfaces;

	m_visframe = 0;
	host.dprintf("bsp load complete\n");
}

q1Bsp::q1Bsp(int submodel, q1Bsp *bsp) :Model(submodel) {

	m_type = mod_brush;
	m_header = bsp->m_header;

	m_skytexture = bsp->m_skytexture;
	m_skytexture_id = bsp->m_skytexture_id;

	m_num_planes = bsp->m_num_planes;
	m_planes = bsp->m_planes;

	m_num_textures = bsp->m_num_textures;
	m_textures = bsp->m_textures;

	m_num_texinfos = bsp->m_num_texinfos;
	m_texinfos = bsp->m_texinfos;

	m_len_lighting = bsp->m_len_lighting;
	m_lighting = bsp->m_lighting;

	m_num_faces = bsp->m_num_faces;
	m_faces = bsp->m_faces;

	m_num_markfaces = bsp->m_num_markfaces;
	m_markfaces = bsp->m_markfaces;

	m_len_visibility = bsp->m_len_visibility;
	m_visibility = bsp->m_visibility;

	m_num_submodels = bsp->m_num_submodels;
	m_submodels = bsp->m_submodels;

	m_num_leafs = m_submodels[submodel].m_visleafs;
	m_leafs = bsp->m_leafs;

	m_num_nodes = bsp->m_num_nodes;
	m_nodes = bsp->m_nodes;

	m_num_clipnodes = bsp->m_num_clipnodes;
	m_clipnodes = bsp->m_clipnodes;

	m_visframe = 0;

	m_bounds = m_submodels[submodel].m_bounds;
	m_num_leafs = m_submodels[submodel].m_visleafs;
	m_firstmodelsurface = m_submodels[submodel].m_firstface;
	m_nummodelsurfaces = m_submodels[submodel].m_numfaces;

	for (int i = 0; i < MAX_MAP_HULLS; i++) {
		m_hulls[i] = bsp->m_hulls[i];
		m_hulls[i].firstclipnode = m_submodels[submodel].m_headnode[i];
		if (i) {
			m_hulls[i].lastclipnode = bsp->m_num_clipnodes - 1;
		}
	}

	m_lightmaps = bsp->m_lightmaps;
}

int q1Bsp::load_planes() {
	lump_t &lump = m_header.lumps[LUMP_PLANES];
	int len = lump.filelen;
	int num = len / sizeof(dplane_t);
	m_num_planes = num;
	m_planes = new(pool) q1_plane[num];
	dplane_t *in = new dplane_t[num];

	m_file->read(reinterpret_cast<char *>(in), lump.fileofs, len);
	for (int i = 0; i < num; i++) {
		for (int j = 0; j < 3; j++) {
			m_planes[i] = in[i].normal;
			m_planes[i].m_type = in[i].type;
		}
	}
	delete[] in;

	return 0;
}

char* q1Bsp::load_entities() {
	lump_t &lump = m_header.lumps[LUMP_ENTITIES];
	int len = lump.filelen;
	char *in = new char[len+1];

	m_file->read(in, lump.fileofs, len);
	in[len] = 0;

	return in;
}

int q1Bsp::load_texinfos() {
	lump_t &lump = m_header.lumps[LUMP_TEXINFO];
	int len = lump.filelen;
	int num = len / sizeof(texinfo_t);
	m_num_texinfos = num;
	m_texinfos = new(pool)q1_texinfo[num];
	texinfo_t *in = new texinfo_t[num];

	m_file->read(reinterpret_cast<char *>(in), lump.fileofs, len);
	for (int i = 0; i < num; i++) {
		m_texinfos[i].m_flags = in[i].flags;
		if (m_textures[in[i].miptex].m_width) {
			m_texinfos[i].m_miptex = &m_textures[in[i].miptex];
		}
		else {
			m_texinfos[i].m_miptex = &m_textures[0];
		}
		for (int j = 0; j < 2; j++) {
			for (int k = 0; k < 3; k++) {
				m_texinfos[i].m_vecs[j][k] = in[i].vecs[j][k];
			}
			m_texinfos[i].m_ofs[j] = in[i].vecs[j][3];
		}
		if (in[i].miptex == m_skytexture) {
			m_texinfos[i].m_flags |= FACE_SKY;
		}
	}
	delete[] in;

	return 0;
}

void q1Bsp::make_texture_animations() {
	int		i, j, num, max, altmax;
	q1_texture	*tx, *tx2;
	q1_texture	*anims[10];
	q1_texture	*altanims[10];
	//
	// sequence the animations
	//
	for (i = 0; i<m_num_textures; i++)
	{
		tx = &m_textures[i];
		if (!tx || tx->m_name[0] != '+')
			continue;
		if (tx->anim_next)
			continue;	// allready sequenced

		// find the number of frames in the animation
		memset(anims, 0, sizeof(anims));
		memset(altanims, 0, sizeof(altanims));

		max = tx->m_name[1];
		altmax = 0;
		if (max >= 'a' && max <= 'z')
			max -= 'a' - 'A';
		if (max >= '0' && max <= '9')
		{
			max -= '0';
			altmax = 0;
			anims[max] = tx;
			max++;
		}
		else if (max >= 'A' && max <= 'J')
		{
			altmax = max - 'A';
			max = 0;
			altanims[altmax] = tx;
			altmax++;
		}
		else
			sys.error("Bad animating texture %s", tx->m_name);

		for (j = i + 1; j<m_num_textures; j++)
		{
			tx2 = &m_textures[j];
			if (!tx2 || tx2->m_name[0] != '+')
				continue;
			if (strcmp(tx2->m_name + 2, tx->m_name + 2))
				continue;

			num = tx2->m_name[1];
			if (num >= 'a' && num <= 'z')
				num -= 'a' - 'A';
			if (num >= '0' && num <= '9')
			{
				num -= '0';
				anims[num] = tx2;
				if (num + 1 > max)
					max = num + 1;
			}
			else if (num >= 'A' && num <= 'J')
			{
				num = num - 'A';
				altanims[num] = tx2;
				if (num + 1 > altmax)
					altmax = num + 1;
			}
			else
				sys.error("Bad animating texture %s", tx->m_name);
		}

#define	ANIM_CYCLE	2
		// link them all together
		for (j = 0; j<max; j++)
		{
			tx2 = anims[j];
			if (!tx2)
				sys.error("Missing frame %i of %s", j, tx->m_name);
			tx2->anim_total = max * ANIM_CYCLE;
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j + 1) * ANIM_CYCLE;
			tx2->anim_next = anims[(j + 1) % max];
			if (altmax)
				tx2->alternate_anims = altanims[0];
		}
		for (j = 0; j<altmax; j++)
		{
			tx2 = altanims[j];
			if (!tx2)
				sys.error("Missing frame %i of %s", j, tx->m_name);
			tx2->anim_total = altmax * ANIM_CYCLE;
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j + 1) * ANIM_CYCLE;
			tx2->anim_next = altanims[(j + 1) % altmax];
			if (max)
				tx2->alternate_anims = anims[0];
		}
	}
}

void cquant16(unsigned char *data, int width, int height, unsigned char *palette, unsigned char *dest, unsigned char *dest_palette);
#ifdef WIN32
static unsigned char dest32[512 * 512 * 4];
#endif

extern int tex_dim(int x);

byte* create_tex_copy(int id, byte *in, int inwidth, int inheight)
{
	tex3ds_t *tx = (tex3ds_t*)id;
	if (!tx) {
		return 0;
	}
	int i, j;
	int width = tex_dim(inwidth);
	int height = tex_dim(inheight);
	byte *out = new(pool) byte[width*height];

	tx->width = width;
	tx->height = height;

	if (inwidth == width && inheight == height) {
		memcpy(out, in, width*height);
		return out;
	}


	byte *outrow = out;
	byte *inrow = in;
	{
		unsigned	frac, fracstep;
		fracstep = (inwidth << 16) / width;
		for (i = 0; i < height; i++, outrow += width)
		{
			inrow = (in + inwidth*(i*inheight / height));
			frac = fracstep >> 1;
			for (j = 0; j < width; j++)
			{
				outrow[j] = inrow[frac >> 16];
				frac += fracstep;
			}
		}
	}
	return out;
}

int q1Bsp::load_textures() {
	lump_t &lump = m_header.lumps[LUMP_TEXTURES];
	//int len = lump.filelen;
	int ofs = lump.fileofs;
	m_file->read(reinterpret_cast<char *>(&m_num_textures), ofs, sizeof(int));
	int num = m_num_textures;
	int *tex_ofs = new int[num+1];
	m_textures = new(pool) q1_texture[num];
	tex_ofs[num] = lump.filelen;
	ofs += sizeof(int);
	m_file->read(reinterpret_cast<char *>(tex_ofs), ofs, sizeof(int)*num);

	int skytexture = -1;

	host.dprintf("loading %d textures %08x\n", num, tex_ofs);

	for (int i = 0; i < num; i++) {
		if (tex_ofs[i] == -1) {
			memset(&m_textures[i], 0, sizeof(q1_texture));
			continue;
		}
		int level = 0;
		host.dprintf("reading %d %d\n", i, tex_ofs[i + 1] - tex_ofs[i]);

		//read the mip header
		miptex_t _mip;
		miptex_t *mip = &_mip;
		m_file->read((char *)mip, lump.fileofs + tex_ofs[i], sizeof(miptex_t));

		//calculate size of data
		int tx_bytes = (mip->width >> level) * (mip->height >> level);
		byte *tx = new byte[tx_bytes];

		//read the tex data
		m_file->read((char *)tx, lump.fileofs + tex_ofs[i] + mip->offsets[level], tx_bytes);

		//copy values
		memcpy(m_textures[i].m_name,mip->name,16);
		m_textures[i].m_width = mip->width;
		m_textures[i].m_height = mip->height;
		m_textures[i].anim_total = 0;
		m_textures[i].anim_next = 0;
		m_textures[i].alternate_anims = 0;
		m_textures[i].m_id = sys.gen_texture_id();
		m_textures[i].m_id2 = -1;
		m_textures[i].m_cachable = false;
		host.dprintf("loading %d %16s\n", i, mip->name);

		//cache a scaled copy of the original texture
		m_textures[i].m_data = create_tex_copy(m_textures[i].m_id, tx, mip->width, mip->height);

		//generate texture
		if (*mip->name == '*') {
			warpTexture *warp = new(pool) warpTexture(m_textures[i].m_id, m_textures[i].m_width >> level, m_textures[i].m_height >> level, tx);
			proc_textures.add(warp);
			vbo_tx += sys.texture_size(m_textures[i].m_id);
		}
		else if (!strncmp(mip->name, "sky", 3)) {
			m_skytexture = i;
			m_skytexture_id = m_textures[i].m_id;
			skytexture = i;
			skyTexture *sky = new(pool) skyTexture(m_textures[i].m_id, m_textures[i].m_width >> level, m_textures[i].m_height >> level, tx);
			proc_textures.add(sky);
			vbo_tx += sys.texture_size(m_textures[i].m_id);
		}
		else {
			//m_textures[i].m_cachable = true;
			vbo_tx += ((mip->width >> level) * (mip->height >> level) * 2);

			sys.load_texture256(m_textures[i].m_id, mip->width >> level, mip->height >> level, tx, q1_palette);
			//host.printf("load_texture256 %d complete\n",i);
			if (sys.last_texture_id() != m_textures[i].m_id) {
				m_textures[i].m_id2 = sys.last_texture_id();
			}
		}

		delete[] tx;
		host.dprintf("finished %d\n",i);
	}

	delete[] tex_ofs;

	host.dprintf("texture animations\n");
	make_texture_animations();

	//PROC proc = wglGetProcAddress("glColorTableEXT");

	host.dprintf("texture load finished\n");
	return 0;
}

#if 0
void calculate_face(q1_face *face) {
	q1_texinfo *tex = face->m_texinfo;
	q1_texture *tx = tex->m_miptex;
	vec3_fixed16 *points = face->m_points;
	tex3_fixed16 &u_vec = face->m_texinfo->m_vecs[0];
	tex3_fixed16 &v_vec = face->m_texinfo->m_vecs[1];
	fixed32p16 u_ofs(face->m_texinfo->m_ofs[0]);
	fixed32p16 v_ofs(face->m_texinfo->m_ofs[1]);
	fixed32p16 *u = face->m_u;
	fixed32p16 *v = face->m_v;
	fixed32p16 mins[2];
	fixed32p16 maxs[2];
	mins[0] = mins[1] = 30000.0f;
	maxs[0] = maxs[1] = -30000.0f;
	fixed32p16 save_u[64];
	fixed32p16 save_v[64];
	int num_edges = face->m_num_points;
	for (int j = 0; j < num_edges; j++) {
		fixed32p16 uu = (points[j].dot(u_vec) + u_ofs);
		fixed32p16 vv = (points[j].dot(v_vec) + v_ofs);

		if (uu < mins[0]) {
			mins[0] = uu;
		}
		if (uu > maxs[0]) {
			maxs[0] = uu;
		}
		if (vv < mins[1]) {
			mins[1] = vv;
		}
		if (vv > maxs[1]) {
			maxs[1] = vv;
		}
		save_u[j] = uu;
		save_v[j] = vv;
		u[j] = (uu / tx->m_width);
		v[j] = (vv / tx->m_height);
	}

	for (int j = 0; j < 2; j++) {
		mins[j] = floor((mins[j].c_float()) / 16);
		maxs[j] = ceil((maxs[j].c_float()) / 16);
		face->m_texturemins[j] = (short)(mins[j].c_int() * 16);
		face->m_extents[j] = ((maxs[j] - mins[j]) * 16.0f).c_short();
	}

	int smax = (face->m_extents[0] >> 4) + 1;
	int tmax = (face->m_extents[1] >> 4) + 1;
	int smin = face->m_texturemins[0];
	int tmin = face->m_texturemins[1];

	for (int j = 0; j < num_edges; j++) {
		int x = (save_u[j].c_int() - smin) / 16;
		int y = (save_v[j].c_int() - tmin) / 16;
		face->m_lightmap_ofs[j] = (smax*y) + x;
	}
}
#endif

void q1_lightmap::mark_dirty(short *rect) {
	/*int x1 = rect[0] >> 3;
	int y1 = rect[1] >> 3;
	int x2 = (rect[0] + rect[2] + 7) >> 3;
	int y2 = (rect[1] + rect[3] + 7) >> 3;
	byte *row = &m_dirty_map[y1 * 8 + x1];
	for (int i = y1; i < y2; i++) {
		for (int j = x1; j < x2; j++) {
			row[j] = 1;
		}
		row += 8;
	}*/
	m_dirty = 1;
}

void q1_lightmap::update_block(unsigned int *blocklights, short *rect, bool rotated) {
	if (m_data == 0 || rect[1] > 63 || rect[0] > 63 ||
		rect[1] < 0 || rect[0] < 0) {
		printf("lm: BAD %08x %d %d\n", m_data, rect[0], rect[1]);
	}
	else {
		unsigned *bl = blocklights;
		unsigned *chk = (unsigned *)m_data;
		if (*(chk - 1) != 0xDEADBEEF) {
			printf("bad lead\n");
		}
		if (*(chk + (64 * 64 / 4)) != 0xDEADBEEF) {
			printf("bad tail\n");
		}
		int x = rect[0];
		int y = rect[1];
		int w = rect[2];
		int h = rect[3];
		//printf("lm: %08x %d %d %d %d\n%d %d %d\n", lm->m_data, x, y, w, h, sext, text, size);
		int stride = 64;
		int i, j, t;
		byte *dest = &m_data[(y * 64) + x];
		if (rotated) {
			for (i = 0; i < w; i++, dest++)
			{
				for (j = 0; j < h; j++)
				{
					t = *bl++;
					t >>= 7;
					if (t > 255)
						t = 255;
					dest[j * 64] = t;
				}
			}
		}
		else {
			for (i = 0; i < h; i++, dest += stride)
			{
				for (j = 0; j < w; j++)
				{
					t = *bl++;
					t >>= 7;
					if (t > 255)
						t = 255;
					dest[j] = t;
				}
			}
		}
		mark_dirty(rect);
	}
}


void q1_face::build_lightmap(unsigned int *blocklights, int *lightstyles) {
	if (m_lightmaps && m_lightmap_id && (m_texinfo->m_flags & 1) == 0) {

		int sext = (m_extents[0] >> 4) + 1;
		int text = (m_extents[1] >> 4) + 1;
		int size = sext * text;
		// clear to no light
		for (int i = 0; i < size; i++) {
			blocklights[i] = 0;
		}
		byte *lightmap = m_lightmaps;
		for (int maps = 0; maps < MAXLIGHTMAPS; maps++) {
			if (m_styles[maps] == 255) {
				break;
			}
			int scale = lightstyles[m_styles[maps]];	// 8.8 fraction
			m_cached_light[maps] = scale;
			//colr += lightmap[face->m_lightmap_ofs[i]] * scale;
			for (int j = 0; j < size; j++) {
				blocklights[j] += lightmap[j] * scale;
			}
			lightmap += size;
			//break;
		}
	}

}

void q1_face::build_vertex_array() {
	q1_texture *tx = m_texinfo->m_miptex;
	tex3_fixed16 &u_vec = m_texinfo->m_vecs[0];
	tex3_fixed16 &v_vec = m_texinfo->m_vecs[1];
	fixed32p16 u_ofs(m_texinfo->m_ofs[0]);
	fixed32p16 v_ofs(m_texinfo->m_ofs[1]);
	fixed32p16 *u = m_u;
	fixed32p16 *v = m_v;
	fixed32p16 mins[2];
	fixed32p16 maxs[2];
	mins[0] = mins[1] = 30000.0f;
	maxs[0] = maxs[1] = -30000.0f;
	fixed32p16 save_u[64];
	fixed32p16 save_v[64];

	for (int j = 0; j < m_num_points; j++) {
		fixed32p16 uu = (m_points[j].dot(u_vec) + u_ofs);
		fixed32p16 vv = (m_points[j].dot(v_vec) + v_ofs);

		if (uu < mins[0]) {
			mins[0] = uu;
		}
		if (uu > maxs[0]) {
			maxs[0] = uu;
		}
		if (vv < mins[1]) {
			mins[1] = vv;
		}
		if (vv > maxs[1]) {
			maxs[1] = vv;
		}
		save_u[j] = uu;
		save_v[j] = vv;
		u[j] = (uu / tx->m_width);
		v[j] = (vv / tx->m_height);
	}

	for (int j = 0; j < 2; j++) {
		mins[j] = (float)floor(mins[j].c_float() / 16);
		maxs[j] = (float)ceil(maxs[j].c_float() / 16);
		m_texturemins[j] = (short)(mins[j].c_int() * 16);
		m_extents[j] = ((maxs[j] - mins[j]) * 16.0f).c_short();
	}

	int smax = (m_extents[0] >> 4) + 1;
	//int tmax = (m_extents[1] >> 4) + 1;
	int smin = m_texturemins[0];
	int tmin = m_texturemins[1];

	for (int j = 0; j < m_num_points; j++) {
		int x = (save_u[j].c_int() - smin) / 16;
		int y = (save_v[j].c_int() - tmin) / 16;
		m_lightmap_ofs[j] = (smax*y) + x;
	}

	//build vbo for face
	m_vertex_array = linear.alloc(sizeof(bspVertex_t)*m_num_points);
	bspVertex_t *fv = (bspVertex_t *)m_vertex_array;

	for (int j = 0; j < m_num_points; j++, fv++) {
		int x = (save_u[j].c_int() - smin);
		int y = (save_v[j].c_int() - tmin);
		m_points[j].copy_to(&fv->position.x);
		fv->texcoord[0] = m_u[j].c_float();
		fv->texcoord[1] = m_v[j].c_float();
		//save the offsets now
		//then use this when building the lightmaps
		fv->lightmap[0] = x;
		fv->lightmap[1] = y;
#ifndef GS_NO_NORMALS
		m_plane->m_normal.copy_to(&fv->normal.x);
#endif
	}
	
	fv = (bspVertex_t *)m_vertex_array;
	for (int i = 0; i < m_num_points; ++i)
	{
		vec3_t v1, v2;
		float *prev, *vert, *next;
		float f;

		prev = &fv[(i + m_num_points - 1) % m_num_points].position.x;
		vert = &fv[i].position.x;
		next = &fv[(i + 1) % m_num_points].position.x;

		VectorSubtract(vert, prev, v1);
		VectorNormalize(v1);
		VectorSubtract(next, prev, v2);
		VectorNormalize(v2);

		// skip co-linear points
#define COLINEAR_EPSILON 0.001
		if ((fabs(v1[0] - v2[0]) <= COLINEAR_EPSILON) &&
			(fabs(v1[1] - v2[1]) <= COLINEAR_EPSILON) &&
			(fabs(v1[2] - v2[2]) <= COLINEAR_EPSILON))
		{
			for (int j = i + 1; j < m_num_points; ++j)
			{
					fv[j - 1] = fv[j];
			}
			--m_num_points;
			// retry next vertex next time, which is now current vertex
			--i;
		}
	}
}

/*
void build_face(q1_face *face) {
	q1_texinfo *tex = face->m_texinfo;
	q1_texture *tx = tex->m_miptex;
	vec3_fixed16 *points = face->m_points;
	tex3_fixed16 &u_vec = face->m_texinfo->m_vecs[0];
	tex3_fixed16 &v_vec = face->m_texinfo->m_vecs[1];
	fixed32p16 u_ofs(face->m_texinfo->m_ofs[0]);
	fixed32p16 v_ofs(face->m_texinfo->m_ofs[1]);
	fixed32p16 *u = face->m_u;
	fixed32p16 *v = face->m_v;
	fixed32p16 mins[2];
	fixed32p16 maxs[2];
	mins[0] = mins[1] = 30000.0f;
	maxs[0] = maxs[1] = -30000.0f;
	fixed32p16 save_u[64];
	fixed32p16 save_v[64];
	int num_edges = face->m_num_points;

	if (face->m_cache == 0 || face->m_vbo.data == 0) {
		printf("face empty vbo\n");
		return;
	}
	for (int j = 0; j < num_edges; j++) {
		fixed32p16 uu = (points[j].dot(u_vec) + u_ofs);
		fixed32p16 vv = (points[j].dot(v_vec) + v_ofs);

		if (uu < mins[0]) {
			mins[0] = uu;
		}
		if (uu > maxs[0]) {
			maxs[0] = uu;
		}
		if (vv < mins[1]) {
			mins[1] = vv;
		}
		if (vv > maxs[1]) {
			maxs[1] = vv;
		}
		save_u[j] = uu;
		save_v[j] = vv;
		u[j] = (uu / tx->m_width);
		v[j] = (vv / tx->m_height);
	}

	for (int j = 0; j < 2; j++) {
		mins[j] = (float)floor(mins[j].c_float() / 16);
		maxs[j] = (float)ceil(maxs[j].c_float() / 16);
		face->m_texturemins[j] = (short)(mins[j].c_int() * 16);
		face->m_extents[j] = ((maxs[j] - mins[j]) * 16.0f).c_short();
	}

	int smax = (face->m_extents[0] >> 4) + 1;
	int tmax = (face->m_extents[1] >> 4) + 1;
	int smin = face->m_texturemins[0];
	int tmin = face->m_texturemins[1];

	for (int j = 0; j < num_edges; j++) {
		int x = (save_u[j].c_int() - smin) / 16;
		int y = (save_v[j].c_int() - tmin) / 16;
		face->m_lightmap_ofs[j] = (smax*y) + x;
	}

	//build vbo for face
	faceVertex_s fv0;// , fv1, fv2;
	face->m_plane->m_normal.copy_to(&fv0.normal.x);
#if 1
	for (int j = 0; j < num_edges; j++) {
		fv0.texcoord[0] = face->m_u[j].c_float();
		fv0.texcoord[1] = face->m_v[j].c_float();
		face->m_points[j].copy_to(&fv0.position.x);
		gsVboAddData(&face->vbo, &fv0, sizeof(faceVertex_s), 1);
	}
#else
	gsVboCreate(&face->vbo, sizeof(faceVertex_s)*(num_edges-2)*3);
	vbo_cb += sizeof(faceVertex_s)*(num_edges - 2) * 3;
	//0
	fv0.texcoord[0] = face->m_u[0].c_float();
	fv0.texcoord[1] = face->m_v[0].c_float();
	face->m_points[0].copy_to(&fv0.position.x);
	//1
	fv1.texcoord[0] = face->m_u[1].c_float();
	fv1.texcoord[1] = face->m_v[1].c_float();
	face->m_points[1].copy_to(&fv1.position.x);
	for (int j = 2; j < num_edges; j++) {
		gsVboAddData(&face->vbo, &fv0, sizeof(faceVertex_s), 1);
		gsVboAddData(&face->vbo, &fv1, sizeof(faceVertex_s), 1);
		fv2.texcoord[0] = face->m_u[j].c_float();
		fv2.texcoord[1] = face->m_v[j].c_float();
		face->m_points[j].copy_to(&fv2.position.x);
		gsVboAddData(&face->vbo, &fv2, sizeof(faceVertex_s), 1);

		fv1 = fv2;
	}
#endif
	//gsVboFlushData(&face->vbo);
}
*/

int q1Bsp::load_faces() {
	dheader_t &header = m_header;
	lump_t &lump_faces = header.lumps[LUMP_FACES];
	int len = lump_faces.filelen;
	int num = len / sizeof(dface_t);
	m_num_faces = num;
	m_faces = new(pool)q1_face[num];
	dface_t *in_faces = new dface_t[num];
	m_file->read(reinterpret_cast<char *>(in_faces), lump_faces.fileofs, len);

	lump_t &lump_vertexes = header.lumps[LUMP_VERTEXES];
	len = lump_vertexes.filelen;
	num = len / sizeof(dvertex_t);
	dvertex_t *in_vertexes = new dvertex_t[num];
	m_file->read(reinterpret_cast<char *>(in_vertexes), lump_vertexes.fileofs, len);

	lump_t &lump_edges = header.lumps[LUMP_EDGES];
	len = lump_edges.filelen;
	num = len / sizeof(dedge_t);
	dedge_t *in_edges = new dedge_t[num];
	m_file->read(reinterpret_cast<char *>(in_edges), lump_edges.fileofs, len);

	lump_t &lump_surfedges = header.lumps[LUMP_SURFEDGES];
	len = lump_surfedges.filelen;
	num = len / sizeof(int);
	int *in_surfedges = new int[num];
	m_file->read(reinterpret_cast<char *>(in_surfedges), lump_surfedges.fileofs, len);

	for (int i = 0; i < m_num_faces; i++) {
		int num_edges = in_faces[i].numedges;
		m_faces[i].m_num_points = num_edges;
		m_faces[i].m_plane = &m_planes[in_faces[i].planenum];
		m_faces[i].side = in_faces[i].side;
		m_faces[i].m_lightmaps = &m_lighting[in_faces[i].lightofs];
		for (int j = 0; j < MAXLIGHTMAPS; j++) {
			m_faces[i].m_styles[j] = in_faces[i].styles[j];
			m_faces[i].m_cached_light[j] = in_faces[i].styles[j] - 1; //we will set it different to force a rebuild
		}
		q1_texinfo *tex = m_faces[i].m_texinfo = &m_texinfos[in_faces[i].texinfo];

		vec3_fixed16 *points = m_faces[i].m_points = new(pool)vec3_fixed16[num_edges];
		m_faces[i].m_u = new(pool)fixed32p16[num_edges];
		m_faces[i].m_v = new(pool)fixed32p16[num_edges];
		m_faces[i].m_vertlights = new(pool)byte[num_edges];
		m_faces[i].m_lightmap_ofs = new(pool)short[num_edges];
		int *edges = &in_surfedges[in_faces[i].firstedge];
		for (int j = 0; j < num_edges; j++) {
			int edge = *edges++;
			int side = 0;
			if (edge < 0) {
				side = 1;
				edge = -edge;
			}

			points[j] = in_vertexes[in_edges[edge].v[side]].point;
		}

		//gsVboInit(&m_faces[i].m_vbo);
		//build_face(&m_faces[i]);
		vbo_cb += sizeof(faceVertex_s) * num_edges;

		m_faces[i].build_vertex_array();
	}
	//host.printf("vbo_cb: %d %d", vbo_cb, m_num_faces);

	delete[] in_faces;
	delete[] in_edges;
	delete[] in_surfedges;
	delete[] in_vertexes;

	return 0;
}

void q1Bsp::build_lightmaps() {
	rbp::MaxRectsBinPack *bin = 0;
	q1_lightmap *lightmap;
	byte *lightmap_data;
	int create = 1;

	for (int i = 0; i < m_num_faces; i++) {
		q1_face *face = &m_faces[i];
		face->m_lightmap_id = 0;
		//skip special faces
		if (face->m_texinfo->m_flags & 1) {
			continue;
		}
		//do we need a new bin?
		if (create) {
			//if there is a bin
			//then update the texture
			if (bin) {
				//printf("load L8\n");
				sys.load_texture_L8(lightmap->m_id, lightmap->m_width, lightmap->m_height, lightmap_data);
				delete bin;
			}
			//printf("new bin\n");
			bin = new rbp::MaxRectsBinPack;
			lightmap_data = new(pool)byte[LIGHTMAP_BIN_WIDTH * LIGHTMAP_BIN_HEIGHT + 8];
			//start checks
			*(unsigned int *)lightmap_data = 0xDEADBEEF;
			lightmap_data += 4;
			*(((unsigned int *)lightmap_data) + (LIGHTMAP_BIN_WIDTH * LIGHTMAP_BIN_HEIGHT / 4)) = 0xDEADBEEF;
			//end checks
			lightmap = new(pool) q1_lightmap;
			if (bin == 0 || lightmap == 0 || lightmap_data == 0 || create > 1) {
				printf("failed\n");
				continue;
			}
			bin->Init(LIGHTMAP_BIN_WIDTH, LIGHTMAP_BIN_HEIGHT);
			memset(lightmap->m_dirty_map, 1, (LIGHTMAP_BIN_WIDTH / 8)*(LIGHTMAP_BIN_HEIGHT / 8));
			lightmap->m_id = sys.gen_texture_id();
			lightmap->m_height = LIGHTMAP_BIN_WIDTH;
			lightmap->m_width = LIGHTMAP_BIN_HEIGHT;
			lightmap->m_data = lightmap_data;
			lightmap->m_dirty = 0;
			lightmap->m_next = m_lightmaps;
			m_lightmaps = lightmap;
			create = 0;
		}
		if (!bin) {
			create++;
			--i;
			printf("no bin\n");
			continue;
		}
		int smax = (face->m_extents[0] >> 4) + 1;
		int tmax = (face->m_extents[1] >> 4) + 1;
		rbp::Rect r = bin->Insert(smax, tmax, rbp::MaxRectsBinPack::RectBestShortSideFit);
		if (r.height > 0) {
			//printf("%4d: %d %d %d %d\n", i, r.x, r.y, r.width, r.height);
			if (r.x < 0 || (r.x + r.width) > LIGHTMAP_BIN_WIDTH ||
				r.y < 0 || (r.y + r.height) > LIGHTMAP_BIN_HEIGHT) {
				printf("BAD %4d: %d %d\n", i, smax, tmax);
				printf("BAD %4d: %d %d %d %d\n", i, r.x, r.y, r.width, r.height);
				do {
					gspWaitForVBlank();
					hidScanInput();
				} while ((hidKeysUp() & KEY_A) == 0);
			}
		}
		else {
			create++;
			--i;
			//printf("%4d: bin FULL\n", i);
			continue;
		}
		face->m_lightmap_id = lightmap;
		face->m_lightmap_rect[0] = r.x;
		face->m_lightmap_rect[1] = r.y;
		face->m_lightmap_rect[2] = r.width;
		face->m_lightmap_rect[3] = r.height;
		bool rotated = face->m_lightmap_rotated = smax != r.width;

		
		bspVertex_t *fv = (bspVertex_t *)face->m_vertex_array;
		if (fv) {
			for (int j = 0; j < face->m_num_points; j++, fv++) {
				float x = fv->lightmap[0];
				float y = fv->lightmap[1];
				if (rotated) {
					x += r.y * 16;
					x += 8;
					x /= LIGHTMAP_BIN_WIDTH * 16;
					y += r.x * 16;
					y += 8;
					y /= LIGHTMAP_BIN_HEIGHT * 16;

					//fv->texcoord[0] = y;
					//fv->texcoord[1] = x;
					fv->lightmap[0] = y;
					fv->lightmap[1] = x;
				}
				else {
					x += r.x * 16;
					x += 8;
					x /= 64 * 16;
					y += r.y * 16;
					y += 8;
					y /= 64 * 16;

					//fv->texcoord[0] = x;
					//fv->texcoord[1] = y;
					fv->lightmap[0] = x;
					fv->lightmap[1] = y;
				}
			}
		}
	}

	if (bin) {
		//printf("load L8\n");
		sys.load_texture_L8(lightmap->m_id, lightmap->m_width, lightmap->m_height, lightmap_data);
		delete bin;
	}
}

void q1_bsp_node::set_parent_r(q1_node *parent) {
	m_parent = parent;
	if (m_contents < 0) {
		return;
	}
		m_children[0]->set_parent_r(this);
		m_children[1]->set_parent_r(this);
}

int q1Bsp::load_nodes() {
	lump_t &lump = m_header.lumps[LUMP_NODES];
	int len = lump.filelen;
	int num = len / sizeof(dnode_t);
	m_num_nodes = num;
	m_nodes = new(pool)q1_bsp_node[num];
	dnode_t *in = new dnode_t[num];

	m_file->read(reinterpret_cast<char *>(in), lump.fileofs, len);
	for (int i = 0; i < num; i++) {
		m_nodes[i].m_contents = 0;
		m_nodes[i].m_plane = &m_planes[in[i].planenum];
		m_nodes[i].m_first_face = &m_faces[in[i].firstface];
		m_nodes[i].m_num_faces = in[i].numfaces;
		for (int j = 0; j < 2; j++) {
			int p = in[i].children[j];
			if (p < 0) {
				m_nodes[i].m_children[j] = (q1_bsp_node *)&m_leafs[-1 - p];
			}
			else {
				m_nodes[i].m_children[j] = &m_nodes[p];
			}
		}
		for (int j = 0; j < 3; j++) {
			m_nodes[i].m_bounds[0][j] = (int)in[i].mins[j];
			m_nodes[i].m_bounds[1][j] = (int)in[i].maxs[j];
		}
	}
	delete[] in;

	m_nodes->set_parent_r(0);

	return 0;
}

int q1Bsp::load_clipnodes() {
	lump_t &lump = m_header.lumps[LUMP_CLIPNODES];
	int len = lump.filelen;
	int num = len / sizeof(dclipnode_t);
	m_num_clipnodes = num;
	m_clipnodes = new(pool)q1_clipnode[num];
	dclipnode_t *in = new dclipnode_t[num];

	m_file->read(reinterpret_cast<char *>(in), lump.fileofs, len);
	for (int i = 0; i < num; i++) {
		m_clipnodes[i].m_children[0] = in[i].children[0];
		m_clipnodes[i].m_children[1] = in[i].children[1];
		m_clipnodes[i].m_plane = &m_planes[in[i].planenum];
	}
	delete[] in;

	return 0;
}

void q1Bsp::make_hulls() {
	q1_bsp_node *child, *in = m_nodes;
	int count = m_num_nodes;
	q1_clipnode *out = new(pool) q1_clipnode[count];

	hull_t *hull = &m_hulls[0];
	hull->clipnodes = out;
	hull->firstclipnode = m_submodels[0].m_headnode[0];
	hull->lastclipnode = count - 1;
	hull->planes = m_planes;
	hull->clip_mins[0] = 0;
	hull->clip_mins[1] = 0;
	hull->clip_mins[2] = 0;
	hull->clip_maxs[0] = 0;
	hull->clip_maxs[1] = 0;
	hull->clip_maxs[2] = 0;

	for (int i = 0; i<count; i++, out++, in++)
	{
		//out->planenum = in->plane - bloadmodel->planes;
		out->m_plane = in->m_plane;
		for (int j = 0; j<2; j++)
		{
			child = in->m_children[j];
			if (child->m_contents < 0)
				out->m_children[j] = child->m_contents;
			else
				out->m_children[j] = child - m_nodes;
		}
	}

	hull = &m_hulls[1];
	hull->clipnodes = m_clipnodes;
	hull->firstclipnode = m_submodels[0].m_headnode[1];
	hull->lastclipnode = m_num_clipnodes - 1;
	hull->planes = m_planes;
	hull->clip_mins[0] = -16;
	hull->clip_mins[1] = -16;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 16;
	hull->clip_maxs[1] = 16;
	hull->clip_maxs[2] = 32;

	hull = &m_hulls[2];
	hull->clipnodes = m_clipnodes;
	hull->firstclipnode = m_submodels[0].m_headnode[2];
	hull->lastclipnode = m_num_clipnodes - 1;
	hull->planes = m_planes;
	hull->clip_mins[0] = -32;
	hull->clip_mins[1] = -32;
	hull->clip_mins[2] = -24;
	hull->clip_maxs[0] = 32;
	hull->clip_maxs[1] = 32;
	hull->clip_maxs[2] = 64;
}

int q1Bsp::load_markfaces() {
	lump_t &lump = m_header.lumps[LUMP_MARKSURFACES];
	int len = lump.filelen;
	int num = len / sizeof(short);
	m_num_markfaces = num;
	m_markfaces = new(pool)q1_face*[num];
	short *in = new short[num];

	m_file->read(reinterpret_cast<char *>(in), lump.fileofs, len);
	for (int i = 0; i < num; i++) {
		m_markfaces[i] = m_faces + in[i];
	}
	delete[] in;

	return 0;
}

int q1Bsp::load_visibility() {
	lump_t &lump = m_header.lumps[LUMP_VISIBILITY];
	int len = lump.filelen;
	m_len_visibility = len;
	m_visibility = new(pool) unsigned char[len];

	m_file->read(reinterpret_cast<char *>(m_visibility), lump.fileofs, len);

	return 0;
}

int q1Bsp::load_lighting() {
	lump_t &lump = m_header.lumps[LUMP_LIGHTING];
	int len = lump.filelen;
	m_len_lighting = len;
	m_lighting = new(pool) unsigned char[len];

	m_file->read(reinterpret_cast<char *>(m_lighting), lump.fileofs, len);

	return 0;
}

int q1Bsp::load_leafs() {
	lump_t &lump = m_header.lumps[LUMP_LEAFS];
	int len = lump.filelen;
	int num = len / sizeof(dleaf_t);
	m_num_leafs = num;
	m_leafs = new(pool)q1_leaf_node[num];
	dleaf_t *in = new dleaf_t[num];

	m_file->read(reinterpret_cast<char *>(in), lump.fileofs, len);
	for (int i = 0; i < num; i++) {
		m_leafs[i].m_contents = in[i].contents;
		m_leafs[i].m_first_face = &m_markfaces[in[i].firstmarksurface];
		m_leafs[i].m_num_faces = in[i].nummarksurfaces;
		m_leafs[i].m_compressed_vis = &m_visibility[in[i].visofs];
		m_leafs[i].m_efrags = 0;
		m_leafs[i].m_visframe = 0;
		for (int j = 0; j < 3; j++) {
			m_leafs[i].m_bounds[0][j] = (int)in[i].mins[j];
			m_leafs[i].m_bounds[1][j] = (int)in[i].maxs[j];
		}
		for (int j = 0; j < NUM_AMBIENTS; j++) {
			m_leafs[i].m_ambient_sound_level[j] = in[i].ambient_level[j];
		}
	}
	delete[] in;

	//hack vis for in solid
	m_leafs[0].m_compressed_vis = 0;

	return 0;
}

int q1Bsp::load_submodels() {
	lump_t &lump = m_header.lumps[LUMP_MODELS];
	int len = lump.filelen;
	int num = len / sizeof(dmodel_t);
	m_num_submodels = num;
	m_submodels = new(pool) q1Model[num];
	dmodel_t *in = new dmodel_t[num];

	m_file->read(reinterpret_cast<char *>(in), lump.fileofs, len);
	for (int i = 0; i < num; i++) {
		for (int j = 0; j < 3; j++) {
			m_submodels[i].m_bounds[0][j] = in[i].mins[j] - 1;
			m_submodels[i].m_bounds[1][j] = in[i].maxs[j] + 1;
			m_submodels[i].m_origin[j] = in[i].origin[j];
		}
		for (int j = 0; j < MAX_MAP_HULLS; j++) {
			m_submodels[i].m_headnode[j] = in[i].headnode[j];
		}
		m_submodels[i].m_visleafs = in[i].visleafs;
		m_submodels[i].m_firstface = in[i].firstface;
		m_submodels[i].m_numfaces = in[i].numfaces;
	}
	delete[] in;

	return 0;
}
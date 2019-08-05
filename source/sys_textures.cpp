#include "sys.h"
#ifdef WIN32
#include <gl\gl.h>
#include <gl\glu.h>
#endif
#include "memPool.h"
#include <stdlib.h>
#include "ctr_render.h"

static unsigned int *dest32 = 0;// [512 * 512 * 4];
static unsigned char *dest256 = 0;

void SYS::pal256toRGBA(int width, int height, unsigned char *data, unsigned char *palette) {
	if ((width*height) > (512 * 512) || data == 0 || palette == 0 || dest32 == 0) {
		return;
	}
	unsigned char *c = (unsigned char *)dest32;
	for (int i = 0; i<(width*height); i++) {
		int c0 = data[i];
		c[0] = palette[c0 * 3 + 0];
		c[1] = palette[c0 * 3 + 1];
		c[2] = palette[c0 * 3 + 2];
		c[3] = 255;
		c += 4;
	}
}
void SYS::pal16toRGBA(int width, int height, unsigned char *data, unsigned char *palette) {
	if ((width*height) >(512 * 512) || data == 0 || palette == 0 || dest32 == 0) {
		return;
	}
	unsigned char *c = (unsigned char *)dest32;
	for (int i = 0; i<(width*height / 2); i++) {
		int c0 = data[i] & 0xf;
		int c1 = (data[i] >> 4) & 0xf;
		c[0] = palette[c0 * 3 + 0];
		c[1] = palette[c0 * 3 + 1];
		c[2] = palette[c0 * 3 + 2];
		c[3] = (c0 == 15 ? 0 : 255);
		c += 4;
		c[0] = palette[c1 * 3 + 0];
		c[1] = palette[c1 * 3 + 1];
		c[2] = palette[c1 * 3 + 2];
		c[3] = (c0 == 15 ? 0 : 255);
		c += 4;
	}
}
unsigned int SYS::gen_texture_id(GPU_TEXTURE_FILTER_PARAM min, GPU_TEXTURE_FILTER_PARAM mag) {
	//unsigned int id = 0;
#ifdef WIN32
	if(dest32==0) {
		dest32 = (unsigned int *)malloc(512*512*4*sizeof(int));
		dest256 = (unsigned char *)malloc(512 * 512 * 4 * sizeof(unsigned char));
	}
	glGenTextures(1, &id);
	m_last_tex_id_allocated = id;
	return id;
#endif

	tex3ds_t *ptr = new(pool) tex3ds_t;
	memset(ptr, 0, sizeof(tex3ds_t));
	m_last_tex_id_allocated = (unsigned)ptr;
	//ptr->min = min;
	//ptr->mag = mag;
	return (unsigned)ptr;
}

void SYS::bind_texture(int id, int unit) {
	if (!id) {
		return;
	}
	tex3ds_t *ptr = (tex3ds_t *)id;
	C3D_TexSetWrap(ptr, GPU_REPEAT, GPU_REPEAT);
	C3D_TexBind(unit, ptr);
	return;
	int w = ptr->width;
	int h = ptr->height;
	byte *data = (byte *)ptr->data;
	GPU_TEXCOLOR type = ptr->fmt;
	GPU_TEXUNIT tunit;
	switch (unit) {
	case 1:
		tunit = GPU_TEXUNIT1;
		break;
	case 0:
	default:
		tunit = GPU_TEXUNIT0;
		break;
	}
#ifdef CITRO3D
	ctrSetTex(tunit, type, w, h, data);
#endif
}

int SYS::texture_size(int id) {
	if (!id) {
		return 0;
	}
	tex3ds_t *ptr = (tex3ds_t *)id;
	int w = ptr->width;
	int h = ptr->height;
	
	return w * h * 2;
}

void cquant16(unsigned char *data, int width, int height, unsigned char *palette, unsigned char *dest, unsigned char *dest_palette);
void cquant16_unpacked(unsigned char *data, int width, int height, unsigned char *palette, unsigned char *dest, unsigned char *dest_palette);

void SYS::convert_texture256to16(int width, int height, byte *data_in, byte *pal_in, byte *data_out, byte *pal_out) {
	memset(pal_out, 0, 16);
	cquant16_unpacked(data_in, width, height, pal_in, data_out, pal_out);
}

bool mask_tex(int width, int height, byte *in8, byte *out) {
	bool found = false;
	for (int i = 0; i< width*height; i++) {
		unsigned int s0 = *in8++;

		if (s0 < 247 /* || s0 > 251*/) {
			out[3] = 0;
		}
		else {
			found = true;
		}
		out += 4;
	}

	return found;
}

bool mask_tex2(int width, int height, byte *in8) {
	bool found = false;

	for (int i = 0; i< width*height; i++) {
		unsigned int s0 = *in8;

		if (s0 < 240/*247  || s0 > 251*/) {
			*in8 = 0;
		}
		else {
			found = true;
		}
		in8++;
	}

	return found;
}

#define RGBA8(r,g,b,a) (\
(((r) & 0xFF) << 24)\
| (((g) & 0xFF) << 16)\
| (((b) & 0xFF) << 8)\
| ((a) & 0xFF))\

#define RGBA5551(r,g,b,a) (\
(((r>>3) & 0x1F) << 11)\
| (((g>>3) & 0x1F) << 6)\
| (((b>>3) & 0x1F) << 1)\
| (a))\

static int swiz[64];

void init_block_map() {
	for (int s = 0; s < 8; s++) {
		for (int t = 0; t < 8; t++) {
			int texel_index_within_tile = 0;
			for (int block_size_index = 0; block_size_index < 3; ++block_size_index) {
				int sub_tile_width = 1 << block_size_index;
				int sub_tile_height = 1 << block_size_index;
				int sub_tile_index = (s & sub_tile_width) << block_size_index;
				sub_tile_index += 2 * ((t & sub_tile_height) << block_size_index);
				texel_index_within_tile += sub_tile_index;
			}
			swiz[t * 8 + s] = texel_index_within_tile;
		}
	}
}

int tex_dim(int x)
{
	//if (x > 256)
	//{
	//	return 512;
	//}
	if (x > 128)
	{
		return 256;
	}
	if (x > 64)
	{
		return 128;
	}
	if (x > 32)
	{
		return 64;
	}
	if (x > 16)
	{
		return 32;
	}
	if (x > 8)
	{
		return 16;
	}
	return 8;
}

#if 0
static void waitforit(char *text) {
	printf(text);
	printf("press A...");
	do {
		scanKeys();
		gspWaitForEvent(GSPEVENT_VBlank0, false);
	} while ((keysDown() & KEY_A) == 0);
	do {
		scanKeys();
		gspWaitForEvent(GSPEVENT_VBlank0, false);
	} while ((keysDown() & KEY_A) == KEY_A);
	printf("done\n");
}
#endif

byte* _scale_texture(tex3ds_t *tx, byte *in,bool scale)
{
	int i, j;

	int inwidth = tx->width;
	int inheight = tx->height;
	int width = tex_dim(tx->width);
	int height = tex_dim(tx->height);

	if(inwidth == width && inheight == height) {
		return in;
	}

	tx->width = width;
	tx->height = height;
	//printf("alloc %d %d %d\n", width, height,width*height);
	byte *out = new byte[width*height];
	byte *outrow = out;
	byte *inrow = in;
	//printf("scale: %08x %d %d %d %d\n", out, inwidth, inheight, width, height);
	//waitforit("pre scale\n");
	memset(out, 0, width*height);
	if (true) {
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
	else {
		for (i = 0; i < inheight; i++, outrow += width, inrow += inwidth)
		{
			for (j = 0; j < inwidth; j++)
			{
				outrow[j] = inrow[j];
			}
		}
	}
	//waitforit("post scale\n");
	return out;
}

int tileOrder[] = { 0, 1, 8, 9, 2, 3, 10, 11, 16, 17, 24, 25, 18, 19, 26, 27, 4, 5, 12, 13, 6, 7, 14, 15, 20, 21, 28, 29, 22, 23, 30, 31, 32, 33, 40, 41, 34, 35, 42, 43, 48, 49, 56, 57, 50, 51, 58, 59, 36, 37, 44, 45, 38, 39, 46, 47, 52, 53, 60, 61, 54, 55, 62, 63 };
void parseTile8(u8 *tile, byte *image, int width, int height, int x, int y) {
	int i, j, k;
	byte pixel;
	for (k = 0; k < (8 * 8); k++) {
		i = tileOrder[k] % 8;
		j = (tileOrder[k] - i) / 8;
		pixel = image[(height - (y + j) - 1)*width + (x + i)];
		tile[k] = pixel;
	}
}
void parseTile16(u16 *tile, byte *image, byte *palette, int width, int height, int x, int y) {
	int i, j, k;
	byte pixel;
	for (k = 0; k < (8 * 8); k++) {
		i = tileOrder[k] % 8;
		j = (tileOrder[k] - i) / 8;
		pixel = image[(height - (y + j) - 1)*width + (x + i)];
		tile[k] = RGBA5551(palette[pixel * 3 + 0],
			palette[pixel * 3 + 1],
			palette[pixel * 3 + 2],
			1);
	}
}
void parseTileTrans16(u16 *tile, byte *image, byte *palette, int width, int height, int x, int y, byte trans) {
	int i, j, k;
	byte pixel;
	for (k = 0; k < (8 * 8); k++) {
		i = tileOrder[k] % 8;
		j = (tileOrder[k] - i) / 8;
		pixel = image[(height - (y + j) - 1)*width + (x + i)];
		tile[k] = RGBA5551(palette[pixel * 3 + 0],
			palette[pixel * 3 + 1],
			palette[pixel * 3 + 2],
			pixel == trans ? 0 : 1);
	}
}
void parseTileTrans(u32 *tile, byte *image, byte *palette, int width, int height, int x, int y, byte trans) {
	int i, j, k;
	byte pixel;
	for (k = 0; k < (8 * 8); k++) {
		i = tileOrder[k] % 8;
		j = (tileOrder[k] - i) / 8;
		pixel = image[(y + j)*width + (x + i)];
		tile[k] = RGBA8(palette[pixel * 3 + 0],
			palette[pixel * 3 + 1],
			palette[pixel * 3 + 2],
			pixel == trans ? 0 : 255);
	}
}
void parseTile(u8 *tile, byte *image, byte *palette, int width, int height, int x, int y) {
	int i, j, k;
	byte pixel;
	byte *buf = (u8*)tile;
	for (k = 0; k < (8 * 8); k++) {
		i = tileOrder[k] % 8;
		j = (tileOrder[k] - i) / 8;
		pixel = image[(y + j)*width + (x + i)];
		*buf++ = palette[pixel * 3 + 2];
		*buf++ = palette[pixel * 3 + 1];
		*buf++ = palette[pixel * 3 + 0];
	}
}
void _pal256toRGBA(tex3ds_t *tx, unsigned char *data, unsigned char *palette, int trans) {
	if ((tx->width*tx->height) > (512 * 512) || data == 0 || palette == 0) {
		return;
	}
	//printf("scaling\n");
	byte *src = _scale_texture(tx, data, trans ? false : true);
	int i, j;
	int width = tx->width;
	int height = tx->height;
	int cb = width * height * 2;// (trans ? 4 : 3);
	byte *tile8;
	
	//printf("allocing\n");
	//tx->data = (byte*)linearMemAlign(cb, 0x80); //textures need to be 0x80-byte aligned
	if (tx->data == 0) {
		if (tx->data == 0) {
			tx->data = (byte*)linear.alloc(cb, 0x80);
		}
		//bail
		if (tx->data == 0) {
			return;
		}
	}
	tile8 = (byte *)tx->data;
	byte transindex = *src;
	tx->fmt = GPU_RGBA5551;// trans ? GPU_RGBA8 : GPU_RGB8;
	
	//printf("converting\n");
	for (j = 0; j < height; j += 8) {
		for (i = 0; i < width; i += 8) {
#if 1
			if (trans) {
				parseTileTrans16((u16 *)tile8, src, palette, width, height, i, j, transindex);
			}
			else {
				parseTile16((u16 *)tile8, src, palette, width, height, i, j);
			}
			tile8 += (8 * 8 * 2);
#else
			if (trans) {
				parseTileTrans((u32 *)tile8, src, palette, width, height, i, j, transindex);
				tile8 += (8 * 8 * 4);
			}
			else {
				parseTile(tile8, src, palette, width, height, i, j);
				tile8 += (8 * 8 * 3);
			}
#endif
		}
	}
	
	//printf("freeing\n");
	if (src && src != data) {
		delete [] src;
	}
	//printf("flushing\n");
	GSPGPU_FlushDataCache(tx->data, cb);
	//printf("done\n");
}

void SYS::load_texture_L8(int id, int width, int height, unsigned char *data) {
	tex3ds_t *tex = (tex3ds_t*)id;
	if (!tex) {
		return;
	}
	if (tex->data == 0) {
		tex->data = (byte *)linear.alloc(width*height, 0x80);
		tex->width = width;
		tex->height = height;
		tex->fmt = GPU_A8;
		C3D_TexSetFilter(tex, GPU_LINEAR, GPU_LINEAR);
	}
	if (tex->data == 0) {
		return;
	}
	byte *tile = (byte *)tex->data;
	for (int j = 0; j < height; j += 8) {
		for (int i = 0; i < width; i += 8) {
			parseTile8(tile, data, width, height, i, j);
			tile += (8 * 8);
		}
	}
	GSPGPU_FlushDataCache(tex->data, width * height);
}

void SYS::load_texture256(int id, int width, int height, unsigned char *data, unsigned char *palette, int trans) {
	if ((width*height) > (512 * 512) || data == 0 || palette == 0) {
		return;
	}

#if 0
	pal256toRGBA(width, height, data, palette);

	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, dest32);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#else
	static unsigned char pal16[16 * 3];
	tex3ds_t *tex = (tex3ds_t*)id;
	if (!tex) {
		return;
	}

	//::printf("%08x %d %d\n", id, width, height);
	tex->width = width;
	tex->height = height;
	C3D_TexSetFilter(tex, GPU_LINEAR, GPU_LINEAR);
	_pal256toRGBA(tex, data, palette, trans);
	if (trans == false) {
		//this process is destructive so we need to copy the buffer
		byte *data2 = new byte[width*height];
		if (data2 == 0) {
			return;
		}
		memcpy(data2, data, width*height);
		if (mask_tex2(width, height, data2)) {
			int id2 = sys.gen_texture_id();
			tex3ds_t *tex2 = (tex3ds_t*)id2;
			tex2->width = width;
			tex2->height = height;
			_pal256toRGBA(tex2, data2, palette, 1);
		}
		if (data2) {
			delete[] data2;
		}
	}
	return;

	memset(pal16, 0, sizeof(pal16));
	cquant16(data, width, height, palette, dest256, pal16);
	load_texture16(id, width, height, dest256, pal16, trans);

	if (trans == false && mask_tex2(width, height, data)) {
		int id2 = sys.gen_texture_id();
		memset(pal16, 0, sizeof(pal16));
		cquant16(data, width, height, palette, dest256, pal16);
		load_texture16(id2, width, height, dest256, pal16, 1);
	}

#endif
}

void trans_tex(int width, int height, byte *in4, byte *out) {
	unsigned int mask = in4[0] & 0xf;
	for (int i = 0; i< width*height; i += 2) {
		unsigned int c0 = (*in4) & 0xf;
		unsigned int c1 = ((*in4) >> 4) & 0xf;

		if (c0 == mask) {
			out[3] = 0;
		}
		if (c1 == mask) {
			out[4 + 3] = 0;
		}
		out += 8;
		in4++;
	}
}

void SYS::load_texture16(int id, int width, int height, unsigned char *data, unsigned char *palette, int trans) {
	if ((width*height) > (512*512) || data == 0 || palette == 0) {
		return;
	}

	//byte *text = new(pool)byte[width * height / 2];

	pal16toRGBA(width, height, data, palette);
	if (trans) {
		trans_tex(width, height, data, (byte *)dest32);
	}

#ifdef WIN32
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, dest32);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#endif
}

void trans_tex_byte(int width, int height, byte *in, byte *out) {
	unsigned int mask = in[0];
	for (int i = 0; i< width*height; i ++) {
		unsigned int c0 = *in;

		if (c0 == mask) {
			out[3] = 0;
		}
		out += 4;
		in++;
	}
}


void SYS::load_texture_byte(int id, int width, int height, unsigned char *data, unsigned char *palette, int trans) {
	if ((width*height) > (512 * 512) || data == 0 || palette == 0) {
		return;
	}

	pal256toRGBA(width, height, data, palette);
	if (trans) {
		trans_tex_byte(width, height, data, (byte *)dest32);
	}

#ifdef WIN32
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, dest32);
#endif
}

void SYS::update_texture_byte(int id, int width, int height, unsigned char *data, unsigned char *palette, int trans) {
	if ((width*height) > (512 * 512) || data == 0 || palette == 0) {
		return;
	}
	tex3ds_t *tex = (tex3ds_t*)id;
	if (!tex) {
		return;
	}

	_pal256toRGBA(tex, data, palette, trans);
	return;

	pal256toRGBA(width, height, data, palette);
	if (trans) {
		trans_tex_byte(width, height, data, (byte *)dest32);
	}

#ifdef WIN32
	glBindTexture(GL_TEXTURE_2D, id);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, dest32);
#endif
}

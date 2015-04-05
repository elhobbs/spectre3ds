#include "sys.h"

inline void DrawPixel(byte *image, int x, int y, int c) {
	image[y * 128 + x] = c;
}

int arcColor(byte *image, int x, int y, int rad, int start, int end, int color)
{
	int result;
	int cx = 0;
	int cy = rad;
	int df = 1 - rad;
	int d_e = 3;
	int d_se = -2 * rad + 5;
	int xpcx, xmcx, xpcy, xmcy;
	int ypcy, ymcy, ypcx, ymcx;
	byte drawoct;
	int startoct, endoct, oct, stopval_start = 0, stopval_end = 0;
	double dstart, dend, temp = 0.;


	/*
	* Sanity check radius
	*/
	if (rad < 0) {
		return (-1);
	}

	/*
	* Special case for rad=0 - draw a point
	*/
	if (rad == 0) {
		DrawPixel(image, x, y, color);
		return 1;
	}

#if 0
	/*
	* Get arc's circle and clipping boundary and
	* test if bounding box of circle is visible
	*/
	x2 = x + rad;
	left = dst->clip_rect.x;
	if (x2<left) {
		return(0);
	}
	x1 = x - rad;
	right = dst->clip_rect.x + dst->clip_rect.w - 1;
	if (x1>right) {
		return(0);
	}
	y2 = y + rad;
	top = dst->clip_rect.y;
	if (y2<top) {
		return(0);
	}
	y1 = y - rad;
	bottom = dst->clip_rect.y + dst->clip_rect.h - 1;
	if (y1>bottom) {
		return(0);
	}
#endif
	// Octant labelling
	//      
	//  \ 5 | 6 /
	//   \  |  /
	//  4 \ | / 7
	//     \|/
	//------+------ +x
	//     /|\
	//  3 / | \ 0
	//   /  |  \
	//  / 2 | 1 \
	//      +y

	// Initially reset bitmask to 0x00000000
	// the set whether or not to keep drawing a given octant.
	// For example: 0x00111100 means we're drawing in octants 2-5
	drawoct = 0;

	/*
	* Fixup angles
	*/
	start %= 360;
	end %= 360;
	// 0 <= start & end < 360; note that sometimes start > end - if so, arc goes back through 0.
	while (start < 0) start += 360;
	while (end < 0) end += 360;
	start %= 360;
	end %= 360;

	// now, we find which octants we're drawing in.
	startoct = start / 45;
	endoct = end / 45;
	oct = startoct - 1; // we increment as first step in loop

	// stopval_start, stopval_end; 
	// what values of cx to stop at.
	do {
		oct = (oct + 1) % 8;

		if (oct == startoct) {
			// need to compute stopval_start for this octant.  Look at picture above if this is unclear
			dstart = (double)start;
			switch (oct)
			{
			case 0:
			case 3:
				temp = sin(dstart * M_PI / 180.);
				break;
			case 1:
			case 6:
				temp = cos(dstart * M_PI / 180.);
				break;
			case 2:
			case 5:
				temp = -cos(dstart * M_PI / 180.);
				break;
			case 4:
			case 7:
				temp = -sin(dstart * M_PI / 180.);
				break;
			}
			temp *= rad;
			stopval_start = (int)temp; // always round down.
			// This isn't arbitrary, but requires graph paper to explain well.
			// The basic idea is that we're always changing drawoct after we draw, so we
			// stop immediately after we render the last sensible pixel at x = ((int)temp).

			// and whether to draw in this octant initially
			if (oct % 2) drawoct |= (1 << oct); // this is basically like saying drawoct[oct] = true, if drawoct were a bool array
			else		 drawoct &= 255 - (1 << oct); // this is basically like saying drawoct[oct] = false
		}
		if (oct == endoct) {
			// need to compute stopval_end for this octant
			dend = (double)end;
			switch (oct)
			{
			case 0:
			case 3:
				temp = sin(dend * M_PI / 180);
				break;
			case 1:
			case 6:
				temp = cos(dend * M_PI / 180);
				break;
			case 2:
			case 5:
				temp = -cos(dend * M_PI / 180);
				break;
			case 4:
			case 7:
				temp = -sin(dend * M_PI / 180);
				break;
			}
			temp *= rad;
			stopval_end = (int)temp;

			// and whether to draw in this octant initially
			if (startoct == endoct)	{
				// note:      we start drawing, stop, then start again in this case
				// otherwise: we only draw in this octant, so initialize it to false, it will get set back to true
				if (start > end) {
					// unfortunately, if we're in the same octant and need to draw over the whole circle, 
					// we need to set the rest to true, because the while loop will end at the bottom.
					drawoct = 255;
				}
				else {
					drawoct &= 255 - (1 << oct);
				}
			}
			else if (oct % 2) drawoct &= 255 - (1 << oct);
			else			  drawoct |= (1 << oct);
		}
		else if (oct != startoct) { // already verified that it's != endoct
			drawoct |= (1 << oct); // draw this entire segment
		}
	} while (oct != endoct);

	// so now we have what octants to draw and when to draw them.  all that's left is the actual raster code.

	/*
	* Draw arc
	*/
	result = 0;

	/*
	* Alpha Check
	*/
	if (true) {//(color & 255) == 255) {

		/*
		* No Alpha - direct memory writes
		*/

		/*
		* Draw
		*/
		do {
			ypcy = y + cy;
			ymcy = y - cy;
			if (cx > 0) {
				xpcx = x + cx;
				xmcx = x - cx;
				// always check if we're drawing a certain octant before adding a pixel to that octant.
				if (drawoct & 4)  DrawPixel(image, xmcx, ypcy, color); // drawoct & 4 = 22; drawoct[2]
				if (drawoct & 2)  DrawPixel(image, xpcx, ypcy, color);
				if (drawoct & 32) DrawPixel(image, xmcx, ymcy, color);
				if (drawoct & 64) DrawPixel(image, xpcx, ymcy, color);
			}
			else {
				if (drawoct & 6)  DrawPixel(image, x, ypcy, color); // 4 + 2; drawoct[2] || drawoct[1]
				if (drawoct & 96) DrawPixel(image, x, ymcy, color); // 32 + 64
			}

			xpcy = x + cy;
			xmcy = x - cy;
			if (cx > 0 && cx != cy) {
				ypcx = y + cx;
				ymcx = y - cx;
				if (drawoct & 8)   DrawPixel(image, xmcy, ypcx, color);
				if (drawoct & 1)   DrawPixel(image, xpcy, ypcx, color);
				if (drawoct & 16)  DrawPixel(image, xmcy, ymcx, color);
				if (drawoct & 128) DrawPixel(image, xpcy, ymcx, color);
			}
			else if (cx == 0) {
				if (drawoct & 24)  DrawPixel(image, xmcy, y, color); // 8 + 16
				if (drawoct & 129) DrawPixel(image, xpcy, y, color); // 1 + 128
			}

			/*
			* Update whether we're drawing an octant
			*/
			if (stopval_start == cx) {
				// works like an on-off switch because start & end may be in the same octant.
				if (drawoct & (1 << startoct)) drawoct &= 255 - (1 << startoct);
				else drawoct |= (1 << startoct);
			}
			if (stopval_end == cx) {
				if (drawoct & (1 << endoct)) drawoct &= 255 - (1 << endoct);
				else drawoct |= (1 << endoct);
			}

			/*
			* Update pixels
			*/
			if (df < 0) {
				df += d_e;
				d_e += 2;
				d_se += 2;
			}
			else {
				df += d_se;
				d_e += 2;
				d_se += 4;
				cy--;
			}
			cx++;
		} while (cx <= cy);

	}
#if 0
	else {

		/*
		* Using Alpha - blended pixel blits
		*/

		do {
			ypcy = y + cy;
			ymcy = y - cy;
			if (cx > 0) {
				xpcx = x + cx;
				xmcx = x - cx;

				// always check if we're drawing a certain octant before adding a pixel to that octant.
				if (drawoct & 4)  result |= pixelColorNolock(dst, xmcx, ypcy, color);
				if (drawoct & 2)  result |= pixelColorNolock(dst, xpcx, ypcy, color);
				if (drawoct & 32) result |= pixelColorNolock(dst, xmcx, ymcy, color);
				if (drawoct & 64) result |= pixelColorNolock(dst, xpcx, ymcy, color);
			}
			else {
				if (drawoct & 96) result |= pixelColorNolock(dst, x, ymcy, color);
				if (drawoct & 6)  result |= pixelColorNolock(dst, x, ypcy, color);
			}

			xpcy = x + cy;
			xmcy = x - cy;
			if (cx > 0 && cx != cy) {
				ypcx = y + cx;
				ymcx = y - cx;
				if (drawoct & 8)   result |= pixelColorNolock(dst, xmcy, ypcx, color);
				if (drawoct & 1)   result |= pixelColorNolock(dst, xpcy, ypcx, color);
				if (drawoct & 16)  result |= pixelColorNolock(dst, xmcy, ymcx, color);
				if (drawoct & 128) result |= pixelColorNolock(dst, xpcy, ymcx, color);
			}
			else if (cx == 0) {
				if (drawoct & 24)  result |= pixelColorNolock(dst, xmcy, y, color);
				if (drawoct & 129) result |= pixelColorNolock(dst, xpcy, y, color);
			}

			/*
			* Update whether we're drawing an octant
			*/
			if (stopval_start == cx) {
				// works like an on-off switch.  
				// This is just in case start & end are in the same octant.
				if (drawoct & (1 << startoct)) drawoct &= 255 - (1 << startoct);
				else						   drawoct |= (1 << startoct);
			}
			if (stopval_end == cx) {
				if (drawoct & (1 << endoct)) drawoct &= 255 - (1 << endoct);
				else						 drawoct |= (1 << endoct);
			}

			/*
			* Update pixels
			*/
			if (df < 0) {
				df += d_e;
				d_e += 2;
				d_se += 2;
			}
			else {
				df += d_se;
				d_e += 2;
				d_se += 4;
				cy--;
			}
			cx++;
		} while (cx <= cy);

	}				/* Alpha check */

#endif
	return (result);
}

void CircleBorder(byte *image, int x0, int y0, int rad, int thickness, int color) {
	int i, x = rad, y = 0;
	int radiusError = 1 - x;
	while (x >= y)
	{
		for (i = 0; i < thickness; i++) {
			DrawPixel(image, x0 + x - i, y0 + y, color);
			DrawPixel(image, x0 + y, y0 + x - i, color);
			
			DrawPixel(image, x0 - x + i, y0 + y, color);
			DrawPixel(image, x0 - y, y0 + x - i, color);
			
			DrawPixel(image, x0 - x + i, y0 - y, color);
			DrawPixel(image, x0 - y, y0 - x + i, color);
			
			DrawPixel(image, x0 + x - i, y0 - y, color);
			DrawPixel(image, x0 + y, y0 - x + i, color);
		}
		y++;
		if (radiusError < 0)
		{
			radiusError += 2 * y + 1;
		}
		else
		{
			x--;
			radiusError += 2 * (y - x + 1);
		}
	}
}

inline void DrawVline(byte *image, int x, int y, int len, int c) {
	byte *p = &image[y * 128 + x];
	for (; len >= 0; len--) {
		*p = c;
		p += 128;
	}
}

inline void DrawHline(byte *image, int x, int y, int len, int c) {
	byte *p = &image[y * 128 + x];
	for (; len >= 0; len--) {
		*p++ = c;
	}
}

inline void DrawHlineThick(byte *image, int x, int y, int len, int thickness, int c) {
	for (int i = 0; i < thickness;i++) {
		DrawHline(image, x, y + i, len, c);
	}
}

inline void DrawVlineThick(byte *image, int x, int y, int len, int thickness, int c) {
	byte *p = &image[y * 128 + x];
	for (; len >= 0; len--) {
		for (int i = 0; i < thickness; i++) {
			p[i] = c;
		}
		p += 128;
	}
}

void CircleFilled(byte *image, int x0, int y0, int rad, int color) {
	int x = rad, y = 0;
	int radiusError = 1 - x;
	while (x >= y)
	{
		DrawVline(image, x0 - y, y0 - x, 2 * x, color);
		DrawVline(image, x0 + y, y0 - x, 2 * x, color);
		y++;
		if (radiusError < 0)
		{
			radiusError += 2 * y + 1;
		}
		else
		{
			DrawVline(image, x0 - x, y0 - y + 1, 2 * (y - 1), color);
			DrawVline(image, x0 + x, y0 - y + 1, 2 * (y - 1), color);
			x--;
			radiusError += 2 * (y - x + 1);
		}
	}
}

void CircleRadius(byte *image, int x0, int y0, int rad, int thickness, int color) {
	double delta = 2.0 * M_PI / 100.0;
	double angle = 2.0 * M_PI;
	double r = rad - thickness;
	int x, y;

	int health = 100 - 80;

	rad -= thickness;
	
	CircleBorder(image, x0, y0, thickness, 1, color);

	for (int i = 0; i < 100; i++) {
		x = sin(angle) * r + 0.5;
		y = -cos(angle) * r + 0.5;

		CircleFilled(image, x0 + x, y0 + y, thickness, i < health ? 2 : 1);

		angle -= delta;
	}
	CircleBorder(image, x0, y0, rad + thickness + 1, 3, 2);
	CircleBorder(image, x0, y0, rad - thickness, 3, 2);
}

void arc_percent(byte *image, int x0, int y0, int rad, int thickness, int center, int border, int percent) {
	double delta = 2.0 * M_PI / 106.0;
	double angle = 2.0 * M_PI;
	double r = rad - thickness;
	int x, y;
	//center = 1;
	//border = 2;

	int mega = percent > 100 ? percent - 100 : 0;

	if (mega) {
		percent = mega * 100 / 150;
		center = 3;
		border = 1;
	}

	if (percent < 0) {
		percent = 0;
	}
	if (percent > 100) {
		percent = 100;
	}

	int health = 100 - percent;

	rad -= thickness;

	//CircleBorder(x0, y0, thickness, 1, color);

	for (int i = 0; i < 100; i++) {
		x = sin(angle) * r;
		y = -cos(angle) * r;

		CircleFilled(image, x0 + x, y0 + y, thickness, i < health ? border : center);

		angle -= delta;
	}
	CircleBorder(image, x0, y0, rad + thickness + 0, 2, 0);
	CircleBorder(image, x0, y0, rad - thickness - 1, 2, 0);
}

void hline_percent2(byte *image, int x0, int y0, int length, int thickness, int percent, int color) {
	double delta = length / 100.0;
	int end = length * percent / 100;
	int center = color;
	int border = 2;

	for (int i = length; i > 0; i--) {
		CircleFilled(image, x0 + i, y0, thickness, i > end ? border : center);
	}
}

void hline_percent(byte *image, int x0, int y0, int length, int thickness, int percent, int color) {
	double delta = length / 100.0;
	int end = length * percent / 100;
	int center = color;
	int border = 2;

	if (percent < 100) {
		CircleFilled(image, x0 + length, y0, thickness, border);
		DrawHlineThick(image, x0+end,y0-thickness,length-end,thickness*2+1,border);
	}
	if (percent > 0) {
		CircleFilled(image, x0, y0, thickness, center);
		DrawHlineThick(image, x0 + thickness, y0 - thickness, end - thickness, thickness * 2 + 1, center);
		CircleFilled(image, x0 + end, y0, thickness, center);
	}
}

void vline_percent(byte *image, int x0, int y0, int length, int thickness, int percent, int color) {
	if (percent > 100) {
		percent = 100;
	}
	else if (percent < 0) {
		percent = 0;
	}
	double delta = length / 100.0;
	int end = length * percent / 100;
	int center = color;
	int border = 2;

	if (percent < 100) {
		CircleFilled(image, x0 + thickness, y0, thickness, border);
		DrawVlineThick(image, x0, y0, length - end, thickness * 2 + 1, border);
	}
	if (percent > 0) {
		CircleFilled(image, x0 + thickness, y0 + length - end, thickness, center);
		DrawVlineThick(image, x0, y0 + length - end, end, thickness * 2 + 1, center);
		CircleFilled(image, x0 + thickness, y0 + length, thickness, center);
	}
}

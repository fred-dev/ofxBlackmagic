#include "ColorConversion.h"

static unsigned char clamp(int value) {
	if(value > 255) return 255;
	if(value < 0)   return 0;
	return value;
}

static bool yuvReady = false;
static unsigned char yuvRed[256][256];
static unsigned char yuvGreen[256][256][256];
static unsigned char yuvBlue[256][256];
static void createYuvLookupTables(){
	if(yuvReady) {
		return;
	}
	int yy, uu, vv, ug_plus_vg, ub, vr, val;
	for (int y = 0; y < 256; y++) {
		for (int v = 0; v < 256; v++) {
			yy = y << 8;
			vv = v - 128;
			vr = vv * 359;
			val = (yy + vr) >>  8;
			yuvRed[v][y]  = clamp(val);
		}
	}
	for (int y = 0; y < 256; y++) {
		for (int u = 0; u < 256; u++) {
			for (int v = 0; v < 256; v++) {
				yy = y << 8;
				uu = u - 128;
				vv = v - 128;
				ug_plus_vg = uu * 88 + vv * 183;
				val = (yy - ug_plus_vg) >> 8;
				yuvGreen[u][v][y] = clamp(val);
			}
		}
	}
	for (int y = 0; y < 256; y++) {
		for (int u = 0; u < 256; u++) {
			yy = y << 8;
			uu = u - 128;
			ub = uu * 454;
			val = (yy + ub) >> 8;
			yuvBlue[u][y] = clamp(val);
		}
	}
	yuvReady = true;
}

void cby0cry1_to_y(unsigned char* cby0cry1, unsigned char* y, unsigned int n) {
	for(unsigned int i = 0, j = 1; i < n; i++, j += 2) {
		y[i] = cby0cry1[j];
	}
}

void cby0cry1_to_rgb(unsigned char* cby0cry1, unsigned char* rgb, unsigned int n) {
	createYuvLookupTables();
	unsigned char u, y0, v, y1;
	unsigned int cby0cry1n = 2 * n;
	for(unsigned int i=0, j=0; i<cby0cry1n; i+=4, j+=6){
		u = cby0cry1[i+0];
		y0 = cby0cry1[i+1];
		v = cby0cry1[i+2];
		y1 = cby0cry1[i+3];
		rgb[j+0] = yuvRed[v][y0];
		rgb[j+3] = yuvRed[v][y1];
		rgb[j+1] = yuvGreen[u][v][y0];
		rgb[j+4] = yuvGreen[u][v][y1];
		rgb[j+2] = yuvBlue[u][y0];
		rgb[j+5] = yuvBlue[u][y1];
	}
}

void argb_to_y(const unsigned char* argb, unsigned char* y, unsigned int n) {
	// Rec.709 luma, integer
	for(unsigned int i = 0, j = 0; i < n; i++, j += 4) {
		y[i] = (unsigned char)((54 * argb[j + 1] + 183 * argb[j + 2] + 19 * argb[j + 3]) >> 8);
	}
}

// v210: 6 pixels in 4 little endian 32 bit words, 3 x 10 bit components per word:
// w0 = Cb0 Y0 Cr0, w1 = Y1 Cb1 Y2, w2 = Cr1 Y3 Cb2, w3 = Y4 Cr2 Y5
static inline unsigned int v210Component(const unsigned char* word, int c) {
	const unsigned int w = word[0] | (word[1] << 8) | (word[2] << 16) | ((unsigned int)word[3] << 24);
	return (w >> (10 * c)) & 0x3FF;
}

void v210_to_y(const unsigned char* v210, unsigned char* y, unsigned int width) {
	// luma positions within a group of 6 pixels: (word, component)
	static const int lumaWord[6] = { 0, 1, 1, 2, 3, 3 };
	static const int lumaComponent[6] = { 1, 0, 2, 1, 0, 2 };
	for(unsigned int x = 0; x < width; x++) {
		const unsigned int group = x / 6, k = x % 6;
		const unsigned char* words = v210 + group * 16;
		y[x] = (unsigned char)(v210Component(words + 4 * lumaWord[k], lumaComponent[k]) >> 2);
	}
}

void r210_to_y(const unsigned char* r210, unsigned char* y, unsigned int n) {
	for(unsigned int i = 0, j = 0; i < n; i++, j += 4) {
		const unsigned int w = ((unsigned int)r210[j] << 24) | (r210[j + 1] << 16) | (r210[j + 2] << 8) | r210[j + 3];
		const int r = (w >> 20) & 0x3FF, g = (w >> 10) & 0x3FF, b = w & 0x3FF;
		const int l = (54 * r + 183 * g + 19 * b) >> 10; // Rec.709, 10 -> 8 bit
		y[i] = clamp(l);
	}
}

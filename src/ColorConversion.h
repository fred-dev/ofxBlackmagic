#pragma once

// 8 bit YUV 4:2:2 (cb y0 cr y1, bmdFormat8BitYUV), n = number of pixels
void cby0cry1_to_y(unsigned char* cby0cry1, unsigned char* y, unsigned int n);
void cby0cry1_to_rgb(unsigned char* cby0cry1, unsigned char* rgb, unsigned int n);
// 8 bit ARGB (bmdFormat8BitARGB, RGB 4:4:4 signals), n = number of pixels
void argb_to_y(const unsigned char* argb, unsigned char* y, unsigned int n);
// 10 bit YUV 4:2:2 (v210, bmdFormat10BitYUV): one row, width pixels -> 8 bit luma
void v210_to_y(const unsigned char* v210, unsigned char* y, unsigned int width);
// 10 bit RGB (r210, bmdFormat10BitRGB, big endian, video levels): n pixels -> 8 bit luma
void r210_to_y(const unsigned char* r210, unsigned char* y, unsigned int n);

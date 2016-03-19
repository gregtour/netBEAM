// DCT Compression //
#ifndef _DCT_COMPRESSION_H
#define _DCT_COMPRESSION_H

#include "quality.h"

typedef struct {
    unsigned char coefRed[SIG_TERMS];
    unsigned char coefGreen[SIG_TERMS];
    unsigned char coefBlue[SIG_TERMS];
} MACROBLOCK;

// DCT { 1/4 x a(u) x a(v) x cos((2x+1)u pi / 16) x cos((2y+1)v pi / 16) } x 1000.

// RGBA
extern int* DIC;// = NULL;
extern void InitCompression();
extern void ShutdownCompression();
extern MACROBLOCK* DCTcompress(const unsigned char* img, MACROBLOCK* data, const int width, const int height, int* bw, int* bh);
extern char* DCTdecompress(const MACROBLOCK* data, char* imgBuffer, const int xblocks, const int yblocks);

extern double cofactor[64][8][8];
extern int DICTable[8][8][64];

#endif // #ifndef _DCT_COMPRESSION_H

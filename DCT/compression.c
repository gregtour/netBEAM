// DCT Compression //
/* compression lib */

#include <stdlib.h>
#include "compression.h"
#include "quality.h"
#include "itables.h"
#include "dtables.h"


// Zig-zag order of (X,Y) discrete cosine transforms.
const int SIG[64] = {0,  1,  8,  2,  9,  16,  3,  10,  17,  24,  4,  11,  18,  25,  32,  5,  12,  19,  26,  33,  40,  6,  13,  20,  27,  34,  41,  48,  7,  14,  21,  28,  35,  42,  49,  56,  15,  22,  29,  36,  43,  50,  57,  23,  30,  37,  44,  51,  58,  31,  38,  45,  52,  59,  39,  46,  53,  60,  47,  54,  61,  55,  62,  63, };

int* DIC = NULL;

void InitCompression() {     
  int x, y, j;
    // Convert the table of double values to integers, for faster multiplication.
    DIC = (int*)malloc(sizeof(int)*8*8*SIG_TERMS);
    for (y = 0; y < 8; y++) {
        //DIC[y] = (int**)malloc(sizeof(int*) * 8);
        for (x = 0; x < 8; x++) {
            //DIC[y][x] = (int*)malloc(sizeof(int)*SIG_TERMS);
            for (j = 0; j < SIG_TERMS; j++) {
                //DIC[y][x][j] = cofactor[SIG[j]][x][y] * 1024;
                DIC[y*8*SIG_TERMS + x*SIG_TERMS + j] = cofactor[SIG[j]][x][y] * 1024;
            }
        }
    }
}

void ShutdownCompression() {
    free(DIC);
}

// RGBA
MACROBLOCK* DCTcompress(const unsigned char* img, MACROBLOCK* data, const int width, const int height, int* bw, int* bh) 
{ 
    int xblocks = (width - 1) / 8 + 1;
    int yblocks = (height - 1) / 8 + 1;
    int blockCount = xblocks * yblocks;
    int channel;
    int xBlkitr, yBlkitr;

    //MACROBLOCK* 
    if (data == NULL) data = (MACROBLOCK*)malloc(sizeof(MACROBLOCK) * blockCount);
    if (data == NULL) return NULL;

    *bw = xblocks;
    *bh = yblocks;

    for (yBlkitr = 0; yBlkitr < yblocks; yBlkitr++)
    {
        for (xBlkitr = 0; xBlkitr < xblocks; xBlkitr++)
        {
            MACROBLOCK* block = &(data[yBlkitr * xblocks + xBlkitr]);
            int xPixitr, yPixitr, j;
            float pix;

            for (channel = 0; channel < 3; channel++)
            {
                unsigned char* coef = NULL;
                double dct[SIG_TERMS];

                for (j = 0; j < SIG_TERMS; j++) { dct[j] = 0; }

                for (yPixitr = 0; yPixitr < 8; yPixitr++)
                {
                    for (xPixitr = 0; xPixitr < 8; xPixitr++)
                    {
                        pix = img[width * (yBlkitr * 8 + yPixitr) * 4 + (xBlkitr * 8 + xPixitr) * 4 + 3 - channel];
                        pix = (pix - 128)/2.0;

                        for (j = 0; j < SIG_TERMS; j++)
                        {
                            dct[j] += (pix * cofactor[SIG[j]][xPixitr][yPixitr]);
                        }
                    }
                }

                // quantize

                if (channel == 0) coef = (unsigned char*)block->coefRed;
                else if (channel == 1) coef = (unsigned char*)block->coefGreen;
                else coef = (unsigned char*)block->coefBlue;

                for (j = 0; j < SIG_TERMS; j++)
                {   double quantized = (dct[j] / QUANT_FACTOR + 0.5);
#ifdef LARGER_DC
                    if (j == 0) {
                        if (quantized < -128) quantized = -128;
                        if (quantized > 127) quantized = 127;
                    } else {
#endif
                        if (quantized < CLAMP_MIN) quantized = CLAMP_MIN;
                        if (quantized > CLAMP_MAX) quantized = CLAMP_MAX;
#ifdef LARGER_DC
                    }
#endif
                    coef[j] = (unsigned char)(quantized+128);
                }
            }
        }
    }

    return data;
}


int* DIC;

// RGBA
char* DCTdecompress(const MACROBLOCK* data, char* imgBuffer, const int xblocks, const int yblocks) 
{ 
    int width = xblocks * 8;
    //int height = yblocks * 8;
    //int pixelSize = 4;
    int xBlkitr, yBlkitr;

    //int imgSize = width * height * pixelSize;
    //char* img = (char*)malloc(imgSize);
    //if (img == NULL) return NULL;
    //memset(img, 255, sizeof(char) * imgSize);

    unsigned int* buffer = (unsigned int*)imgBuffer;

    int xPixitr, yPixitr, j, pixR, pixG, pixB;
    unsigned int p;
    const MACROBLOCK* block;
    for (yBlkitr = 0; yBlkitr < yblocks; yBlkitr++)
    {
        for (xBlkitr = 0; xBlkitr < xblocks; xBlkitr++)
        {
            block = &(data[yBlkitr * xblocks + xBlkitr]);

            int multipliers[SIG_TERMS * 3];

            const unsigned char* coR = (const unsigned char*)block->coefRed;
            const unsigned char* coG = (const unsigned char*)block->coefGreen;
            const unsigned char* coB = (const unsigned char*)block->coefBlue;

            for (j = 0; j < SIG_TERMS; j++)
            {
                int QF = QUANT_FACTOR;
                multipliers[j*3] = ((int)coR[j] - 128) * QF;
                multipliers[j*3+1] = ((int)coG[j] - 128) * QF;
                multipliers[j*3+2] = ((int)coB[j] - 128) * QF;
            }

            p = 0;
            for (yPixitr = 0; yPixitr < 8; yPixitr++)
            {
                for (xPixitr = 0; xPixitr < 8; xPixitr++)
                {
                    pixR = 0;
                    pixG = 0;
                    pixB = 0;
                    for (j = 0; j < SIG_TERMS; j++) {
                        int dic = DIC[p++];
                        pixR += multipliers[j*3] * dic;
                        pixG += multipliers[j*3+1] * dic;
                        pixB += multipliers[j*3+2] * dic;
                    }

                    pixR >>= 9;
                    pixG >>= 9;
                    pixB >>= 9;

                    if (pixR < -128) pixR = 0; else if (pixR > 127) pixR = 255;
                            else pixR += 128;
                    if (pixG < -128) pixG = 0; else if (pixG > 127) pixG = 255;
                            else pixG += 128;
                    if (pixB < -128) pixB = 0; else if (pixB > 127) pixB = 255;
                            else pixB += 128;

                    //*buffer = 0x000000FF | (pixB << 24) | (pixG << 16) | (pixR << 8);
                    *buffer = 0xFF000000 | (pixR << 16) | (pixG << 8) | (pixB);
                    buffer++;
                }
                buffer = buffer - 8 + width;
            }
            buffer = buffer - 8 * width + 8;

        }
        buffer = buffer + width * 7;
    }

    return imgBuffer;
}


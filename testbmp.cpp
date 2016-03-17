/*
 netBEAm - dctcompress and test
*/

#include <cstdlib>
#include <fstream>
#include <iostream>

using namespace std;

// Decompression stress tests.
#define ITERATIONS      1

// Use 4-bit values.
#define QHD
// Use 8-bit leading constant byte.
#define LARGER_DC
// Up-to 64 terms.
#define SIG_TERMS       15
//// Color saturation.
//#define SATURATION      2.12

// Diagonal frequencies X+Y
/*(j<1 ? 1 : j<3 ? 1 : j<6 ? 1 : j<10 ? 1 : j<15 ? 1 : j<21 ? 1 : j<36 ? 1 : j<43 ? 1 : j<49 ? 1 : j<54 ? 1 : j<58 ? 1 : j<61 ? 1 : j<63 ? 1 : 1)*/

#ifdef QHD
// 4-bit quantization values.
# ifdef LARGER_DC
#  define QUANT_FACTOR    (j == 0 ? 8 : j < 3 ? 8 : j < 6 ? 4 : j < 10 ? 12 : 8)
# else
#  define QUANT_FACTOR    (j == 0 ? 60 : j < 3 ? 8 : j < 6 ? 4 : j < 10 ? 12 : 8)
#endif

#else
// 8-bit quantization values.
# define QUANT_FACTOR    (j == 0 ? 8 : j < 3 ? 4 : j < 6 ? 2 : 1) 

#endif

// Zig-zag order of (X,Y) discrete cosine transforms.
const int SIG[64] = {0,  1,  8,  2,  9,  16,  3,  10,  17,  24,  4,  11,  18,  25,  32,  5,  12,  19,  26,  33,  40,  6,  13,  20,  27,  34,  41,  48,  7,  14,  21,  28,  35,  42,  49,  56,  15,  22,  29,  36,  43,  50,  57,  23,  30,  37,  44,  51,  58,  31,  38,  45,  52,  59,  39,  46,  53,  60,  47,  54,  61,  55,  62,  63, };

typedef char coffType;

// Value range for coefficients.
#ifdef QHD
# define CLAMP_MIN       -8
# define CLAMP_MAX       7
#else
# define CLAMP_MIN       -128
# define CLAMP_MAX       127
#endif

typedef struct {
    coffType coefRed[64];
    coffType coefGreen[64];
    coffType coefBlue[64];   
} MACROBLOCK;

typedef struct {
    char red[8][8];
    char green[8][8];
    char blue[8][8];
} IMAGEBLOCK;


// DCT { 1/4 x a(u) x a(v) x cos((2x+1)u pi / 16) x cos((2y+1)v pi / 16) } x 1000.
#include "tables.h"

/* save to bitmap */
// UGLY BPM CODE FOLLOWS SHORTLY //////////////////////////////////////////////////
#define XY(x,y,w)   ((x)+(y)*(w))
void SaveBMP(string filename, int width, int height, char* pixels) {
    ofstream file;
    file.open(filename, ios::out | ios::binary);

    // BMP Header
    unsigned int ui;
    signed int si;
    short s;

    s = 19778;                      file.write((char*)&s, 2);
    ui = width*height * 3 + 54;     file.write((char*)&ui, 4);
    ui = 0;                         file.write((char*)&ui, 4);
    ui = 54;                        file.write((char*)&ui, 4);
    ui = 40;                        file.write((char*)&ui, 4);
    si = width;                     file.write((char*)&si, 4);
    si = height;                    file.write((char*)&si, 4);
    s = 1;                          file.write((char*)&s, 2); // apparently this has to be 1
    s = 24;                         file.write((char*)&s, 2);

    ui = 0;                         for (int i = 0; i < 6; i++) file.write((char*)&ui, 4);

    // Color Data   
    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            for (int i = 0; i < 3; i++) 
            {
                //char byte = 255;
                file.write((char*)&pixels[XY(x * 4, y, width * 4) + 3 - i], 1);
                //file.write(&byte, 1);
            }
        }
    }

    file.close();
}

// Not just for saving.
char* LoadBMP(string filename, int* w, int* h) 
{
    ifstream file;

    int width;
    int height;
    unsigned int ui;
    signed int si;
    short s;
    char* image;
    unsigned int hdrAmt;

    file.open(filename, ios::in | ios::binary);

    /*s = 19778;                      */file.read((char*)&s, 2);
    /*ui = width*height * 3 + 54;     */file.read((char*)&ui, 4); //cout << "File Size: " << ui << " Expected " << (1920*1080*4+138) << endl;
    /*ui = 0;                         */file.read((char*)&ui, 4); //cout << "0: " << ui << endl;
    /*ui = 54;                        */file.read((char*)&ui, 4); hdrAmt = ui; //cout << "54: " << ui << endl; 
    /*ui = 40;                        */file.read((char*)&ui, 4); //cout << "40: " << ui << endl;
    /*si = width;                     */file.read((char*)&si, 4); width = si;
    /*si = height;                    */file.read((char*)&si, 4); height = si;
    /*s = 0;                          */file.read((char*)&s, 2);
    /*s = 24;                         */file.read((char*)&s, 2); hdrAmt -= 30; hdrAmt /= 4; /* hdrAmt = 6 */
    /*ui = 0;                         */for (int i = 0; i < hdrAmt; i++) file.read((char*)&ui, 4);

    cout << "Loading BMP. Width: " << width << " Height: " << height << std::endl;

    image = (char*)malloc(width * height * 4);

    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            for (int i = 0; i < 4; i++) 
            {
                file.read((char*)&image[XY(x * 4, y, width * 4) + i], 1);
            }
        }
    }

    file.close();

    *w = width;
    *h = height;

    return image;
}



// RGBA
MACROBLOCK* DCTcompress(const unsigned char* img, const int width, const int height, int* bw, int* bh) 
{ 
    int xblocks = (width - 1) / 8 + 1;
    int yblocks = (height - 1) / 8 + 1;
    int blockCount = xblocks * yblocks;
    int channel;
    int xBlkitr, yBlkitr;

    MACROBLOCK* data = (MACROBLOCK*)malloc(sizeof(MACROBLOCK) * blockCount);
    if (data == NULL) return NULL;

    *bw = xblocks;
    *bh = yblocks;

    for (yBlkitr = 0; yBlkitr < yblocks; yBlkitr++)
    {
        for (xBlkitr = 0; xBlkitr < xblocks; xBlkitr++)
        {
            MACROBLOCK* block = &(data[yBlkitr * xblocks + xBlkitr]);
            int xPixitr, yPixitr, j, pix;

            for (channel = 0; channel < 3; channel++)
            {
                coffType* coef = NULL;
                double dct[SIG_TERMS];

                for (j = 0; j < SIG_TERMS; j++) { dct[j] = 0; }

                for (yPixitr = 0; yPixitr < 8; yPixitr++)
                {
                    for (xPixitr = 0; xPixitr < 8; xPixitr++)
                    {
                        pix = img[width * (yBlkitr * 8 + yPixitr) * 4 + (xBlkitr * 8 + xPixitr) * 4 + 3 - channel];
                        pix = (pix - 128)/2;

                        for (j = 0; j < SIG_TERMS; j++)
                        {
                            dct[j] += (pix * cofactor[SIG[j]][xPixitr][yPixitr]);
                        }
                    }
                }

                // quantize

                if (channel == 0) coef = block->coefRed;
                else if (channel == 1) coef = block->coefGreen;
                else coef = block->coefBlue;

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
                    coef[j] = (coffType)quantized;
                }
            }
        }
    }

    return data;
}


int* DIC;

// RGBA
char* DCTdecompress(const MACROBLOCK* data, const int xblocks, const int yblocks) 
{ 
    int width = xblocks * 8;
    int height = yblocks * 8;
    int pixelSize = 4;
    int xBlkitr, yBlkitr;

    int imgSize = width * height * pixelSize;

    char* img = (char*)malloc(imgSize);
    if (img == NULL) return NULL;

    memset(img, 255, sizeof(char) * imgSize);

    unsigned int* buffer = (unsigned int*)img;

    int xPixitr, yPixitr, j, pixR, pixG, pixB;
    unsigned int p;
    const MACROBLOCK* block;
    for (yBlkitr = 0; yBlkitr < yblocks; yBlkitr++)
    {
        for (xBlkitr = 0; xBlkitr < xblocks; xBlkitr++)
        {
            block = &(data[yBlkitr * xblocks + xBlkitr]);

            int multipliers[SIG_TERMS * 3];

            const char* coR = block->coefRed;
            const char* coG = block->coefGreen;
            const char* coB = block->coefBlue;

            for (j = 0; j < SIG_TERMS; j++)
            {
                int QF = QUANT_FACTOR;
                multipliers[j*3] = (int)coR[j] * QF;
                multipliers[j*3+1] = (int)coG[j] * QF;
                multipliers[j*3+2] = (int)coB[j] * QF;
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

                    *buffer = 0x000000FF | (pixB << 24) | (pixG << 16) | (pixR << 8);
                    buffer++;
                }
                buffer = buffer - 8 + width;
            }
            buffer = buffer - 8 * width + 8;

        }
        buffer = buffer + width * 7;
    }

    return img;
}

// Test.
int main(int argc, char* argv[]) 
{
    int iwidth, iheight, bwidth, bheight;
    char* img = LoadBMP("test.bmp", &iwidth, &iheight);

    // Load an image and compress it.
    MACROBLOCK* blocks = DCTcompress((const unsigned char*)img, iwidth, iheight, &bwidth, &bheight);
    char* decompressedImg = NULL;

    cout << "Compressing with "
    #ifdef QHD
        << "4-bit values"
    #ifdef LARGER_DC
        << " and 8-bit DC"
    #endif
    #else
        << "8-bit values"
    #endif
        << " and " << SIG_TERMS << " terms." << endl;

    // Convert the table of double values to integers, for faster multiplication.
    DIC = (int*)malloc(sizeof(int)*8*8*SIG_TERMS);
    for (int y = 0; y < 8; y++) {
        //DIC[y] = (int**)malloc(sizeof(int*) * 8);
        for (int x = 0; x < 8; x++) {
            //DIC[y][x] = (int*)malloc(sizeof(int)*SIG_TERMS);
            for (int j = 0; j < SIG_TERMS; j++) {
                //DIC[y][x][j] = cofactor[SIG[j]][x][y] * 1024;
                DIC[y*8*SIG_TERMS + x*SIG_TERMS + j] = cofactor[SIG[j]][x][y] * 1024;
            }
        }
    }

    // Try decompressing the image as many times as possible.
    printf("\n\nStarting...\n");
    for (int i = 0; i < ITERATIONS; i++) {
        if (decompressedImg) { free(decompressedImg); }
        decompressedImg = DCTdecompress(blocks, bwidth, bheight); 
    }
    printf("\n\nDone.\n\n");

    SaveBMP("save24.bmp", iwidth, iheight, img);
    SaveBMP("decompressed.bmp", iwidth, iheight, decompressedImg);

    return 0;
}

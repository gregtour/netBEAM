/*
 netBEAm - dctcompress
*/

#include <cstdlib>
#include <fstream>
#include <iostream>

using namespace std;

#define QHD

#define WHOLE_BYTE

#define SIG_TERMS       /*64*/ 15

#define SATURATION      1.26

#ifdef QHD
#define QUANT_FACTOR    (86 / ((j+1)/2 + 1) + 5)
#else
#define QUANT_FACTOR    (j == 0 ? 40 : 40 / ((j+1)/2) + 3)
#endif
//2000000


const int SIG[64] = {0,  1,  8,  2,  9,  16,  3,  10,  17,  24,  4,  11,  18,  25,  32,  5,  12,  19,  26,  33,  40,  6,  13,  20,  27,  34,  41,  48,  7,  14,  21,  28,  35,  42,  49,  56,  15,  22,  29,  36,  43,  50,  57,  23,  30,  37,  44,  51,  58,  31,  38,  45,  52,  59,  39,  46,  53,  60,  47,  54,  61,  55,  62,  63, };

typedef char coffType;

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
    s = 1;                          file.write((char*)&s, 2);
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
    /*ui = width*height * 3 + 54;     */file.read((char*)&ui, 4); cout << "File Size: " << ui << " Expected " << (1920*1080*4+138) << endl;
    /*ui = 0;                         */file.read((char*)&ui, 4); cout << "0: " << ui << endl;
    /*ui = 54;                        */file.read((char*)&ui, 4); cout << "54: " << ui << endl; hdrAmt = ui;
    /*ui = 40;                        */file.read((char*)&ui, 4); cout << "40: " << ui << endl;
    /*si = width;                     */file.read((char*)&si, 4); width = si;
    /*si = height;                    */file.read((char*)&si, 4); height = si;
    /*s = 0;                          */file.read((char*)&s, 2);
    /*s = 24;                         */file.read((char*)&s, 2); hdrAmt -= 30; hdrAmt /= 4; /* hdrAmt = 6 */
    /*ui = 0;                         */for (int i = 0; i < hdrAmt; i++) file.read((char*)&ui, 4);

    cout << "Width: " << width << " Height: " << height << std::endl;

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

    int channelSize = 8;
    int channels = 3;
    int channel;
    //char* bufferPtr[];

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
                // dct stores two decimal places
                double dct[SIG_TERMS];

                for (j = 0; j < SIG_TERMS; j++) { dct[j] = 0; }

                for (yPixitr = 0; yPixitr < 8; yPixitr++)
                {
                    for (xPixitr = 0; xPixitr < 8; xPixitr++)
                    {
                        pix = img[width * (yBlkitr * 8 + yPixitr) * 4 + (xBlkitr * 8 + xPixitr) * 4 + 3 - channel];
                        // shift from [0, 255] to [-128, 127]
                        pix = pix - 128;

                        for (j = 0; j < SIG_TERMS; j++)
                        {
                            // cofactor stores 3 decimal places, dct stores 2
                            dct[j] += (pix * cofactor[SIG[j]][xPixitr][yPixitr]);
                        }
                    }
                }

                // quantize

                if (channel == 0) coef = block->coefRed;
                else if (channel == 1) coef = block->coefGreen;
                else coef = block->coefBlue;

                //memset(coef, 0, SIG_TERMS);

                for (j = 0; j < SIG_TERMS; j++)
                {   double quantized = (dct[j] / QUANT_FACTOR + 0.5);
                    if (quantized < CLAMP_MIN) quantized = CLAMP_MIN;
                    if (quantized > CLAMP_MAX) quantized = CLAMP_MAX;
                    coef[j] = (coffType)quantized;
                }
            }
        }
    }

    return data;
}

// RGBA
char* DCTdecompress(const MACROBLOCK* data, const int xblocks, const int yblocks) 
{ 
    int width = xblocks * 8;
    int height = yblocks * 8;
    int pixelSize = 4;
    int xBlkitr, yBlkitr;
    int channel;

    int imgSize = width * height * pixelSize;

    char* img = (char*)malloc(imgSize);
    if (img == NULL) return NULL;

    memset(img, 255, sizeof(char) * imgSize);

    for (yBlkitr = 0; yBlkitr < yblocks; yBlkitr++)
    {
        for (xBlkitr = 0; xBlkitr < xblocks; xBlkitr++)
        {
            const MACROBLOCK* block = &(data[yBlkitr * xblocks + xBlkitr]);
            int xPixitr, yPixitr, j, pix, dct;

            for (channel = 0; channel < 3; channel++)
            {
                const coffType* coef = NULL;

                if (channel == 0) coef = block->coefRed;
                else if (channel == 1) coef = block->coefGreen;
                else coef = block->coefBlue;

                for (yPixitr = 0; yPixitr < 8; yPixitr++)
                {
                    for (xPixitr = 0; xPixitr < 8; xPixitr++)
                    {
                        char writePix;

                        pix = 0;
                        for (j = 0; j < SIG_TERMS; j++)
                        {
                            double coefC = coef[j] * QUANT_FACTOR;
                            pix += SATURATION * coefC * cofactor[SIG[j]][xPixitr][yPixitr]; // cofactor has three fixed decimal points
                                                                             // keep pix with two fixed decimals
                            //dct[j] += (pix * cofactor[j][xPixitr][yPixitr]);
                        }
                        
                        // remove decimal
                        pix += 128; // adjust form -128 to 127 to 0 to 255
                        if (pix > 255) pix = 255; // clamp
                        if (pix < 0) pix = 0;
                        // cast to 8 bit
                        writePix = pix;
                        img[width * (yBlkitr * 8 + yPixitr) * 4 + (xBlkitr * 8 + xPixitr) * 4 + channel + 1] = pix;
                    } 
                }

            }
        }
    }

    return img;
}

int main(int argc, char* argv[]) 
{
    int iwidth, iheight, bwidth, bheight;
    char* img = LoadBMP("test.bmp", &iwidth, &iheight);

    MACROBLOCK* blocks = DCTcompress((const unsigned char*)img, iwidth, iheight, &bwidth, &bheight);
    char* decompressedImg = DCTdecompress(blocks, bwidth, bheight); 

    SaveBMP("save24.bmp", iwidth, iheight, img);
    SaveBMP("decompressed.bmp", iwidth, iheight, decompressedImg);

    return 0;
}




/*
 dctcompress
*/

#define SIG_TERMS		8

typedef struct {
	char coefRed[4];
	char coefGreen[4];
	char coefBlue[4];	
} MACROBLOCK;

typedef struct {
	char red[8][8];
	char green[8][8];
	char blue[8][8];
} IMAGEBLOCK;


// RGBA
MACROBLOCK* DCTcompress(const unsigned char* img, const int width, const int height) 
{ 
	int xblocks = (width - 1) / 8 + 1;
	int yblocks = (height - 1) / 8 + 1;
	int blockCount = xblocks * yblocks;

	int channelSize = 8;
	int channels = 3;
	int channel;
	char* bufferPtr;

	int xBlkitr, yBlkitr;

	MACROBLOCK* data = (MACROBLOCK*)malloc(sizeof(MACROBLOCK) * blockCount);
	if (data == NULL) return NULL;


	for (yBlkitr = 0; yBlkitr < yblocks; yBlkitr++)
	{
		for (xBlkitr = 0; xBlkitr < xblocks; xBlkitr++)
		{
			MACROBLOCK* block = &(data[yBlkitr * xblocks + xBlkitr]);
			int xPixitr, yPixitr, j, pix;

			for (channel = 0; channel < 3; channel++)
			{
				char* coef = NULL;
				// dct stores two decimal places
				int dct[8];

				for (j = 0; j < 8; j++) { dct[j] = 0; }

				for (yPixitr = 0; yPixitr < 8; yPixitr++)
				{
					for (xPixitr = 0; xPixitr < 8; xPixitr++)
					{
						pix = img[width * (yBlkitr * 8 + yPixitr) * 4 + (xBlkitr * 8 + xPixitr) * 4 + 3	- channel];
						pix = pix - 128;

						for (j = 0; j < SIG_TERMS; j++)
						{
							dct[j] += (pix * cofactor[j][xPixitr][yPixitr] / 100);
						}
					}
				}

				// quantize

				if (channel == 0) coef = &block->coefRed;
				else if (channel == 1) coef = &block->coefGreen;
				else coef = &block->coefBlue;

				memset(coef, 0, SIG_TERMS/2);

				for (j = 0; j < SIG_TERMS; j++)
				{
					char svalue;
					dct[j] = dct[j] * preMultFactor[j] / 100;
					dct[j] = dct[j] / 52; // quantized
					if (dct[j] > 15) dct[j] = 15;
					if (dct[j] < -16) dct[j] = -16;
					svalue = dct[j] + 16;

					coef[j/2] |= (svalue) << ((j%2)*4);
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
				char* coef = NULL;

				if (channel == 0) coef = &block->coefRed;
				else if (channel == 1) coef = &block->coefGreen;
				else coef = &block->coefBlue;

				for (yPixitr = 0; yPixitr < 8; yPixitr++)
				{
					for (xPixitr = 0; xPixitr < 8; xPixitr++)
					{
						char writePix;

						pix = 0;
						for (j = 0; j < SIG_TERMS; j++)
						{
							dct = (coef[j/2] >> (j%2 * 4)) & 255;
							dct -= 16;

							// undo quantization
							dct = dct * 52;
							pix += dct * cofactor[j][xPixitr][yPixitr];
							//dct[j] += (pix * cofactor[j][xPixitr][yPixitr]);
						}
						
						pix *= preMultFactor[j] / 100;

						pix += 128;
						if (pix > 255) pix = 255;
						if (pix < 0) pix = 0;
						writePix = pix;

						img[width * (yBlkitr * 8 + yPixitr) * 4 + (xBlkitr * 8 + xPixitr) * 4 + 3 - channel] = pix;
					}
				}

			}
		}
	}

	return img;
}


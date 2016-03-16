/*
 netBEAm - dctcompress
*/

#define SIG_TERMS		9

typedef struct {
	char coefRed[8];
	char coefGreen[8];
	char coefBlue[8];	
} MACROBLOCK;

typedef struct {
	char red[8][8];
	char green[8][8];
	char blue[8][8];
} IMAGEBLOCK;


// DCT { 1/4 x a(u) x a(v) x cos((2x+1)u pi / 16) x cos((2y+1)v pi / 16) } x 1000.
int cofactor[9][8][8] = {
// 0,0
  {
    { 124, 124, 124, 124, 124, 124, 124, 124, },
    { 124, 124, 124, 124, 124, 124, 124, 124, },
    { 124, 124, 124, 124, 124, 124, 124, 124, },
    { 124, 124, 124, 124, 124, 124, 124, 124, },
    { 124, 124, 124, 124, 124, 124, 124, 124, },
    { 124, 124, 124, 124, 124, 124, 124, 124, },
    { 124, 124, 124, 124, 124, 124, 124, 124, },
    { 124, 124, 124, 124, 124, 124, 124, 124, },
  },
// 0,1
  {
    { 173, 146, 98, 34, -34, -98, -146, -173, },
    { 173, 146, 98, 34, -34, -98, -146, -173, },
    { 173, 146, 98, 34, -34, -98, -146, -173, },
    { 173, 146, 98, 34, -34, -98, -146, -173, },
    { 173, 146, 98, 34, -34, -98, -146, -173, },
    { 173, 146, 98, 34, -34, -98, -146, -173, },
    { 173, 146, 98, 34, -34, -98, -146, -173, },
    { 173, 146, 98, 34, -34, -98, -146, -173, },
  },
// 0,2
  {
    { 163, 67, -67, -163, -163, -67, 67, 163, },
    { 163, 67, -67, -163, -163, -67, 67, 163, },
    { 163, 67, -67, -163, -163, -67, 67, 163, },
    { 163, 67, -67, -163, -163, -67, 67, 163, },
    { 163, 67, -67, -163, -163, -67, 67, 163, },
    { 163, 67, -67, -163, -163, -67, 67, 163, },
    { 163, 67, -67, -163, -163, -67, 67, 163, },
    { 163, 67, -67, -163, -163, -67, 67, 163, },
  },
// 1,0
  {
    { 173, 173, 173, 173, 173, 173, 173, 173, },
    { 146, 146, 146, 146, 146, 146, 146, 146, },
    { 98, 98, 98, 98, 98, 98, 98, 98, },
    { 34, 34, 34, 34, 34, 34, 34, 34, },
    { -34, -34, -34, -34, -34, -34, -34, -34, },
    { -98, -98, -98, -98, -98, -98, -98, -98, },
    { -146, -146, -146, -146, -146, -146, -146, -146, },
    { -173, -173, -173, -173, -173, -173, -173, -173, },
  },
// 1,1
  {
    { 240, 203, 136, 47, -47, -136, -203, -240, },
    { 203, 172, 115, 40, -40, -115, -172, -203, },
    { 136, 115, 77, 27, -27, -77, -115, -136, },
    { 47, 40, 27, 9, -9, -27, -40, -47, },
    { -47, -40, -27, -9, 9, 27, 40, 47, },
    { -136, -115, -77, -27, 27, 77, 115, 136, },
    { -203, -172, -115, -40, 40, 115, 172, 203, },
    { -240, -203, -136, -47, 47, 136, 203, 240, },
  },
// 1,2
  {
    { 226, 93, -93, -226, -226, -93, 93, 226, },
    { 192, 79, -79, -192, -192, -79, 79, 192, },
    { 128, 53, -53, -128, -128, -53, 53, 128, },
    { 45, 18, -18, -45, -45, -18, 18, 45, },
    { -45, -18, 18, 45, 45, 18, -18, -45, },
    { -128, -53, 53, 128, 128, 53, -53, -128, },
    { -192, -79, 79, 192, 192, 79, -79, -192, },
    { -226, -93, 93, 226, 226, 93, -93, -226, },
  },
// 2,0
  {
    { 163, 163, 163, 163, 163, 163, 163, 163, },
    { 67, 67, 67, 67, 67, 67, 67, 67, },
    { -67, -67, -67, -67, -67, -67, -67, -67, },
    { -163, -163, -163, -163, -163, -163, -163, -163, },
    { -163, -163, -163, -163, -163, -163, -163, -163, },
    { -67, -67, -67, -67, -67, -67, -67, -67, },
    { 67, 67, 67, 67, 67, 67, 67, 67, },
    { 163, 163, 163, 163, 163, 163, 163, 163, },
  },
// 2,1
  {
    { 226, 192, 128, 45, -45, -128, -192, -226, },
    { 93, 79, 53, 18, -18, -53, -79, -93, },
    { -93, -79, -53, -18, 18, 53, 79, 93, },
    { -226, -192, -128, -45, 45, 128, 192, 226, },
    { -226, -192, -128, -45, 45, 128, 192, 226, },
    { -93, -79, -53, -18, 18, 53, 79, 93, },
    { 93, 79, 53, 18, -18, -53, -79, -93, },
    { 226, 192, 128, 45, -45, -128, -192, -226, },
  },
// 2,2
  {
    { 213, 88, -88, -213, -213, -88, 88, 213, },
    { 88, 36, -36, -88, -88, -36, 36, 88, },
    { -88, -36, 36, 88, 88, 36, -36, -88, },
    { -213, -88, 88, 213, 213, 88, -88, -213, },
    { -213, -88, 88, 213, 213, 88, -88, -213, },
    { -88, -36, 36, 88, 88, 36, -36, -88, },
    { 88, 36, -36, -88, -88, -36, 36, 88, },
    { 213, 88, -88, -213, -213, -88, 88, 213, },
  },
};



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
						// shift from [0, 255] to [-128, 127]
						pix = pix - 128;

						for (j = 0; j < SIG_TERMS; j++)
						{
							// cofactor stores 3 decimal places, dct stores 2
							dct[j] += (pix * cofactor[j][xPixitr][yPixitr] / 10);
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
					dct[j] = dct[j] / 5200; // quantized by factor of 52, decimal points removed
					// clamp to 4 bit representation
					if (dct[j] > 15) dct[j] = 15;
					if (dct[j] < -16) dct[j] = -16;
					// remove sign
					svalue = dct[j] + 16;
					// pack bits
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
							// unpack bits
							dct = (coef[j/2] >> (j%2 * 4)) & 255;
							// sign result
							dct -= 16;

							// undo quantization
							dct = dct * 5200; // dct now has two fixed decimal points
							pix += dct * cofactor[j][xPixitr][yPixitr] / 10; // cofactor has three fixed decimal points
																			 // keep pix with two fixed decimals
							//dct[j] += (pix * cofactor[j][xPixitr][yPixitr]);
						}
						
						pix /= 100; // remove decimal
						pix += 128; // adjust form -128 to 127 to 0 to 255
						if (pix > 255) pix = 255; // clamp
						if (pix < 0) pix = 0;
						// cast to 8 bit
						writePix = pix;
						img[width * (yBlkitr * 8 + yPixitr) * 4 + (xBlkitr * 8 + xPixitr) * 4 + 3 - channel] = pix;
					}
				}

			}
		}
	}

	return img;
}

/*from math import pi, cos
def outputTables():
	total = 64
	xwidth = 8
	i = 0
	x = 0
	y = 0
	print "// DCT { 1/4 x a(u) x a(v) x cos((2x+1)u pi / 16) x cos((2y+1)v pi / 16) } x 1000."
	print "int cofactor[" + str(total) + "][8][8] = {"
	while i < total:
		i = i + 1
		print "// " + str(y) + "," + str(x)
		print "  {"
		for yy in range(8):
			print "    {",
			for xx in range(8):
				value = cos((2*xx+1)*x*pi/16.0) * cos((2*yy+1)*y*pi/16.0)
				value = value / 4
				if x == 0:
					value = value * 0.7071
				if y == 0:
					value = value * 0.7071
				value = int(value * 1000)
				print str(value) + ",",
			print "},"
		print "  },"
		x = x + 1
		if x >= xwidth:
			x = 0
			y = y + 1
	print "};"*/

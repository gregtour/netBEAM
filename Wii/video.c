/* **************************************** Video Module **************************************** */

#include "netbeam.h"
#include "video.h"
#include "../DCT/compression.h"
#include "../DCT/quality.h"

const int DRAW_BUFFER_WIDTH = 640;
const int DRAW_BUFFER_HEIGHT = 480;

int InitVideo(VIDEO_STATE* ref)
{
#ifndef USING_GRRLIB
  unsigned int x;
  unsigned int y;

  // get video mode
  if ((ref->rmode = VIDEO_GetPreferredMode(NULL)) == NULL)
  {
    return -1;
  }
  
  // alloc framebuffer
  if ((ref->frameBuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(ref->rmode))) == NULL)
  {
    return -1;
  }

  // fill screen a solid color
  for (y = 0; y < ref->rmode->xfbHeight; y++)
  {
    for (x = 0; x < ref->rmode->fbWidth; x++)
    {
      unsigned int* innerBuffer = (unsigned int*)ref->frameBuffer;
      innerBuffer[y * ref->rmode->fbWidth + x] = COLOR_YELLOW;
    }
  }

  // reset frame count
  ref->frame = 0;

  // start and flip the screen
  VIDEO_Configure(ref->rmode);
  VIDEO_SetNextFramebuffer(ref->frameBuffer);
  VIDEO_SetBlack(FALSE);
  VIDEO_Flush();
  VIDEO_WaitVSync();

  if (ref->rmode->viTVMode & VI_NON_INTERLACE) {
    VIDEO_WaitVSync();
  }
#else
  GRRLIB_Init();
  ref->drawBuffer = GRRLIB_CreateEmptyTexture(DRAW_BUFFER_WIDTH, DRAW_BUFFER_HEIGHT);
#endif

  return 0;
}

int ShutdownVideo(VIDEO_STATE* ref)
{
#ifndef USING_GRRLIB
  unsigned int x;
  unsigned int y;

  // fill screen a solid color
  for (y = 0; y < ref->rmode->xfbHeight; y++)
  {
    for (x = 0; x < ref->rmode->fbWidth; x++)
    {
      unsigned int* innerBuffer = (unsigned int*)ref->frameBuffer;
      innerBuffer[y * ref->rmode->fbWidth + x] = COLOR_RED;
    }
  }
#else
  GRRLIB_FreeTexture(ref->drawBuffer);
  GRRLIB_Exit();  
#endif
  return 0;
}

int RenderColorBuffer(VIDEO_STATE* ref, COLOR_BUFFER* buffer);

int DecodeAndRenderColorBuffer(VIDEO_STATE* ref, COLOR_BUFFER* buffer) 
{
  //DecodeAndRenderColorBuffer(ref, &ref->mainBuffer);
  //DCTdecompress(, ref->mainBuffer.width/8, ref->mainBuffer.height/8);
  //extern char* DCTdecompress(const MACROBLOCK* data, const int xblocks, const int yblocks);
    //const MACROBLOCK* data = (MACROBLOCK*)ref->mainBuffer.pixels;
    const unsigned char* macroData = (const unsigned char*)ref->mainBuffer.pixels;
    int xblocks = ref->mainBuffer.width / 8;
    int yblocks = ref->mainBuffer.height / 8;
    //int pixelSize = 4;
    int xBlkitr, yBlkitr;

    //int imgSize = width * height * pixelSize;
    //char* img = (char*)malloc(imgSize);
    //if (img == NULL) return NULL;
    //memset(img, 255, sizeof(char) * imgSize);

    //unsigned int* buffer = (unsigned int*)imgBuffer;
    const int blockSize = SIG_TERMS * 3;
    const int GOffs = SIG_TERMS;
    const int BOffs = 2 * SIG_TERMS;

    int xPixitr, yPixitr, j, pixR, pixG, pixB;

    int x = 0;
    int y = 0;
    for (yBlkitr = 0; yBlkitr < yblocks; yBlkitr++)
    {
        for (xBlkitr = 0; xBlkitr < xblocks; xBlkitr++)
        {
            //block = &(data[yBlkitr * xblocks + xBlkitr]);
            // char R[SIG_TERMS] G[SIG_TERMS] B[SIG_TERMS] ... block block block

            int multipliers[SIG_TERMS * 3];

            const unsigned char* coR = macroData + (yBlkitr * xblocks + xBlkitr) * blockSize;
            //block->coefRed;
            const unsigned char* coG = coR + GOffs;
            //block->coefGreen;
            const unsigned char* coB = coR + BOffs;
            //block->coefBlue;

            for (j = 0; j < SIG_TERMS; j++)
            {
                int QF = QUANT_FACTOR;
                multipliers[j*3] = ((int)coR[j] - 128) * QF;
                multipliers[j*3+1] = ((int)coG[j] - 128) * QF;
                multipliers[j*3+2] = ((int)coB[j] - 128) * QF;
            }

            for (yPixitr = 0; yPixitr < 8; yPixitr++)
            {
                for (xPixitr = 0; xPixitr < 8; xPixitr++)
                {
                    pixR = 0;
                    pixG = 0;
                    pixB = 0;
                    for (j = 0; j < SIG_TERMS; j++) {
                        int dic = DICTable[xPixitr][yPixitr][j];
                        //DIC[p++];
                        pixR += ((multipliers[j*3] * dic) >> 9);
                        pixG += ((multipliers[j*3+1] * dic) >> 9);
                        pixB += ((multipliers[j*3+2] * dic) >> 9);
                    }

                    //pixR >>= 9;
                    //pixG >>= 9;
                    //pixB >>= 9;
                    unsigned char cr;
                    unsigned char cg;
                    unsigned char cb;
                    
                    //pixR = 0;
                    //pixG = 0;
                    //pixB = 0;

                    if (pixR < -128) 
                      cr = 0; 
                    else if (pixR > 127) cr = 255;
                            else cr = pixR + 128;
                    if (pixG < -128) 
                      cg = 0; 
                    else if (pixG > 127) cg = 255;
                            else cg = pixG + 128;
                    if (pixB < -128) cb = 0; 
                    else if (pixB > 127) cb = 255;
                            else cb = pixB + 128;

                    GRRLIB_SetPixelTotexImg(x, y, ref->drawBuffer, 
                      RGBA(cr, cg, cb, 255)
                      );
                    x++;
                }
                x -= 8;
                y++;
            }
            y -= 8;
            x += 8;
        }
        x = 0;
        y += 8;
    }
  GRRLIB_FlushTex(ref->drawBuffer);
  return 0;
}

int FlipVideo(VIDEO_STATE* ref)
{
#ifndef USING_GRRLIB
  // flip framebuffer & wait vsync
  VIDEO_SetNextFramebuffer(ref->frameBuffer);
  VIDEO_Flush();    
  VIDEO_WaitVSync();
  if (ref->rmode->viTVMode & VI_NON_INTERLACE) {
    VIDEO_WaitVSync();
  }
#else
  if (!ref->decoded) {
    DecodeAndRenderColorBuffer(ref, &ref->mainBuffer);
    //DCTdecompress((MACROBLOCK*)ref->mainBuffer.pixels, ref->mainBuffer.width/8, ref->mainBuffer.height/8);
    //extern char* DCTdecompress(const MACROBLOCK* data, const int xblocks, const int yblocks);
    ref->decoded = 1;
  }

  GRRLIB_FillScreen(0xFFFFFFFF);
  float pixSizeX = 640.0f / ref->mainBuffer.width;
  float pixSizeY = 480.0f / ref->mainBuffer.height;
  GRRLIB_DrawImg(0, 0, ref->drawBuffer, 0, pixSizeX, pixSizeY, 0xFFFFFFFF);
  GRRLIB_Render();
#endif
  return 0;
}

int RenderColorBuffer(VIDEO_STATE* ref, COLOR_BUFFER* buffer)
{
  unsigned int x, y;
#ifdef USING_GRRLIB
  if (buffer->pixels == NULL) return 0;

  //unsigned int* texture = (unsigned int*)ref->drawBuffer->data;
  for (y = 0; y < DRAW_BUFFER_HEIGHT; y++)
  {
    for (x = 0; x < DRAW_BUFFER_WIDTH; x++)
    {
      /*if (y < buffer->height && x < buffer->width) {
        *texture = buffer->pixels[y * buffer->width + x];
      } else {
        *texture = RGBA(255,0,0,255);
      }
      texture++;*/
      unsigned int copyx = x / 4;
      unsigned int copyy = y / 4;
      unsigned int color = RGBA(255, 0, 0, 255);
      if (copyy < buffer->height && copyx < buffer->width) {
        color = buffer->pixels[copyy * buffer->width + copyx];
      }
      GRRLIB_SetPixelTotexImg(x, y, ref->drawBuffer, color);
    }
  }
  
  GRRLIB_FlushTex(ref->drawBuffer);
#endif
#ifndef USING_GRRLIB
  unsigned int swidth, sheight;
  unsigned int width, height;
  unsigned int xlimit, ylimit;
  unsigned int* baseframe;
  unsigned int* frame;
  unsigned int* pixelsbase;
  unsigned int* pixels;
  unsigned char yuvFormat;
  // inner loop
  unsigned int pixel;
  unsigned int r, g, b;
  unsigned char l, u, v;

  swidth = ref->rmode->fbWidth;
  sheight = ref->rmode->xfbHeight;

  baseframe = (unsigned int*)ref->frameBuffer;
  pixelsbase = buffer->pixels;
  width = buffer->width;
  height = buffer->height;
  yuvFormat = buffer->yuvFormat;

  xlimit = width < swidth ? width : swidth;
  ylimit = height < sheight ? height : sheight;
  for (y = 0; y < ylimit; y++)
  {
    frame = baseframe + y * swidth;
    pixels = pixelsbase + y * width;
    for (x = 0; x < xlimit; x++)
    {
      if (yuvFormat) {
        *frame = *pixels;
      } else {

        pixel = *pixels;
        r = pixel;
        g = pixel >> 8;
        b = pixel >> 16;

        l = (299*r + 587*g + 114*b)/1000;
        u = (-147*r + 289*g + 436*b)/1000 + 128;
        v = (651*r + 515*g + 100*b) / 1000;

        *frame = (l << 24) | (u << 16) | (l << 8) | (v);
      }
      frame++;
      pixels++;
    }
  }
#endif
  return 0;
}

/* **************************************** /Video Module **************************************** */

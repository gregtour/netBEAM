/*
  Released under Artistic License 2.0.
  Netbeam 2016.
*/
#ifndef _NETBEAM_VIDEO_H
#define _NETBEAM_VIDEO_H

#define USING_GRRLIB

#ifdef USING_GRRLIB
#include <grrlib.h>
#endif

extern const int DRAW_BUFFER_WIDTH;
extern const int DRAW_BUFFER_HEIGHT;

typedef struct {
  unsigned int * pixels;
  unsigned int width;
  unsigned int height;
  unsigned int size;
  unsigned char yuvFormat;
} COLOR_BUFFER;

typedef struct {
#ifndef USING_GRRLIB
  GXRModeObj* rmode;
  void* frameBuffer;
#else
  GRRLIB_texImg *drawBuffer;
#endif
  unsigned int frame;
  COLOR_BUFFER mainBuffer;
  unsigned int decoded;
} VIDEO_STATE;

int InitVideo(VIDEO_STATE* ref);
int ShutdownVideo(VIDEO_STATE* ref);
int RenderColorBuffer(VIDEO_STATE* ref, COLOR_BUFFER* buffer);
int DecodeAndRenderColorBuffer(VIDEO_STATE* ref, COLOR_BUFFER* buffer);
int FlipVideo(VIDEO_STATE* ref);
int RenderColorBuffer(VIDEO_STATE* ref, COLOR_BUFFER* buffer);

#endif // HEADER WRAPPER

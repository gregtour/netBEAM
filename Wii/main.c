/*
  NetBeam Game Streaming Client | Wii | main.c
  Copyright (C) 2016 Greg Tourville
  Available under the Artistic License 2.0.
*/
/* **************************************** Main Header **************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <network.h>
#include <debug.h>
#include <errno.h>
#include <wiiuse/wpad.h>
#include <grrlib.h>

typedef enum {
  ERROR = 0,
  STARTED = 1,
  STOPPING = 2
} _STATE;

/* extern */ _STATE globalState;

//#define INPUT_BUFFER_SIZE   (512*1024)
#define INPUT_BUFFER_SIZE   (1024*1024)
#define OUTPUT_BUFFER_SIZE  128

#define NETWORK_PORT      970
#define HOST_NAME         "alien-pc"
#define HOST_IP           "192.168.1.90"

typedef struct {
  unsigned int dataLength;
  unsigned int timeStamp;
  unsigned int width;
  unsigned int height;
  unsigned int size;
  unsigned char yuvFormat;
} RESPONSE_HEADER;

// macro
//! Byte swap unsigned int
unsigned int swap_uint32(unsigned int val)
{
	val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
	return (val << 16) | (val >> 16);
}


/* **************************************** /Main Header **************************************** */
/* **************************************** Video Module **************************************** */

#define USING_GRRLIB

const int DRAW_BUFFER_WIDTH = 640;
const int DRAW_BUFFER_HEIGHT = 480;

typedef struct {
#ifndef USING_GRRLIB
  GXRModeObj* rmode;
  void* frameBuffer;
#else
  GRRLIB_texImg *drawBuffer;
#endif
  unsigned int frame;
} VIDEO_STATE;

typedef struct {
  unsigned int * pixels;
  unsigned int width;
  unsigned int height;
  unsigned int size;
  unsigned char yuvFormat;
} COLOR_BUFFER;

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
  GRRLIB_FillScreen(0xFFFFFFFF);
  GRRLIB_DrawImg(0, 0, ref->drawBuffer, 0, 1, 1, 0xFFFFFFFF);
  GRRLIB_Render();
#endif
  return 0;
}

int RenderColorBuffer(VIDEO_STATE* ref, COLOR_BUFFER* buffer)
{
  unsigned int x, y;
#ifdef USING_GRRLIB
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
/* **************************************** Sound Module **************************************** */

typedef struct {
  unsigned char enabled;
} SOUND_STATE;

int InitSound(SOUND_STATE* ref)
{
  ref->enabled = 1;
  return 0;
}

int ShutdownSound(SOUND_STATE* ref)
{
  ref->enabled = 0;
  return 0;
}

int UpdateSound(SOUND_STATE* ref)
{
  // do nothing
  return 0;
}

/* **************************************** /Sound Module **************************************** */
/* **************************************** Input Module **************************************** */

typedef struct {
  unsigned char buttons[8];
} INPUT_STATE;

int InitInput(INPUT_STATE* ref)
{
  WPAD_Init();
  return 0;
}

int ShutdownInput(INPUT_STATE* ref)
{
  return 0;
}

int UpdateInput(INPUT_STATE* ref)
{
  unsigned int pressed;
  WPAD_ScanPads();
  pressed = WPAD_ButtonsDown(0);
  if (pressed & WPAD_BUTTON_HOME) {
    globalState = STOPPING;
  } 
  return 0;
}

/* **************************************** /Input Module **************************************** */
/* **************************************** Network Module **************************************** */

typedef struct {
  char localip[16];
  char gateway[16];
  char netmask[16];
  int socket; 
  struct sockaddr_in sockAddr;
  lwp_t thread_handle;
  VIDEO_STATE* pVideoState;
  SOUND_STATE* pSoundState;
  INPUT_STATE* pInputState;
  unsigned char threadTerminated;
} NET_STATE;

void* NetworkThread(void*);

int InitNetwork(NET_STATE* ref)
{
  int ret, result;
  ret = if_config(ref->localip, ref->netmask, ref->gateway, TRUE);
  if (ret < 0) {
    return -1;
  }

  ref->socket = net_socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  if (ref->socket == INVALID_SOCKET) {
    return -1;
  }

  memset(&ref->sockAddr, 0, sizeof(ref->sockAddr));
  ref->sockAddr.sin_family = AF_INET;
  ref->sockAddr.sin_port = htons(NETWORK_PORT);
  ref->sockAddr.sin_addr.s_addr = inet_addr(HOST_IP);

  /*
    getaddrinfo(HOST_NAME, STR(HOST_IP), NULL, &addrinfo);
  */

  result = net_connect(ref->socket, (struct sockaddr *)&ref->sockAddr, sizeof(ref->sockAddr));
  if (result == -1) {
    net_close(ref->socket);
    return -1;
  }

  if (LWP_CreateThread(&ref->thread_handle, NetworkThread, (void*)ref, NULL, 64*1024, 60) < 0)
  {
    ref->threadTerminated = 1;
    return -1;
  }

  return 0;
}

int ShutdownNetwork(NET_STATE* ref)
{
  if (!ref->threadTerminated) {
    void* ret_value;
    LWP_JoinThread(ref->thread_handle, &ret_value);
  }

  net_close(ref->socket);
  return 0;
}

void* NetworkThread(void* arg)
{
  char* sendData;
  unsigned int sendDataSz = OUTPUT_BUFFER_SIZE;
  char* recvData;
  unsigned int recvDataSz = INPUT_BUFFER_SIZE;
  unsigned int lastTimeStamp = 1;
  RESPONSE_HEADER responseHeader;

  NET_STATE* ref = (NET_STATE*)arg;
  if (ref == NULL) { return NULL; }

  // init buffers
  sendData = malloc(sendDataSz);
  recvData = malloc(recvDataSz);
  if (recvData == NULL || sendData == NULL) {
    return NULL;
  }

  memset(recvData, 0, recvDataSz);
  memset(sendData, 0, sendDataSz);

  /*
    Operate in lock step for now.
  */

  while (globalState == STARTED)
  {
    int sentBytes, recvBytes;
    COLOR_BUFFER renderFrame;

    // prep data to send
    memset(sendData, 0, sendDataSz);
    sprintf(sendData, "Client response; Frame number: %i.\r\n", ref->pVideoState->frame++);
    sentBytes = net_send(ref->socket, sendData, strlen(sendData)+1, 0);

    if (sentBytes < 0) { globalState = STOPPING; break; }

    // prep data to recv frame
    unsigned int bytesReceived = 0;

    recvBytes = net_recv(ref->socket, (char*)&responseHeader, sizeof(RESPONSE_HEADER), 0);
    if (recvBytes == 0) continue;
    if (recvBytes < sizeof(RESPONSE_HEADER)) {
      globalState = STOPPING;
      sprintf(sendData, "Client response; Error: partial header received, size %i.\r\n", recvBytes);
      sentBytes = net_send(ref->socket, sendData, strlen(sendData)+1, 0);
      break;
    }

    if (responseHeader.dataLength > recvDataSz || responseHeader.dataLength == 0) {
      sprintf(sendData, "Client response; Frame too big or size zero. Size: %i.\r\n", responseHeader.dataLength);
      sentBytes = net_send(ref->socket, sendData, strlen(sendData)+1, 0);
      globalState = STOPPING;
      break;
    }

    while (bytesReceived < responseHeader.dataLength && recvBytes >= 0)
    {
      recvBytes = net_recv(ref->socket, recvData + bytesReceived, responseHeader.dataLength - bytesReceived, 0);
      bytesReceived += recvBytes;
    }

    if (bytesReceived < responseHeader.dataLength) {
      globalState = STOPPING;
      sprintf(sendData, "Client response; Didn't receive complete frame. %i bytes out of %i.\r\n", 
        bytesReceived, responseHeader.dataLength);
      sentBytes = net_send(ref->socket, sendData, strlen(sendData)+1, 0);
      break;
    }

    renderFrame.pixels = (unsigned int*)recvData;
    renderFrame.width = responseHeader.width;
    renderFrame.height = responseHeader.height;
    renderFrame.size = responseHeader.size;
    renderFrame.yuvFormat = responseHeader.yuvFormat;
    
    sprintf(sendData, "Client response; Drawing frame %i.\r\n", 
      ref->pVideoState->frame-1);
    sentBytes = net_send(ref->socket, sendData, strlen(sendData)+1, 0);

    if (responseHeader.timeStamp > lastTimeStamp) {
      // blit the image to the screen
      RenderColorBuffer(ref->pVideoState, &renderFrame);
      lastTimeStamp = responseHeader.timeStamp;
    }
  }

  // free buffers
  free(sendData);
  free(recvData);
  ref->threadTerminated = 1;

  return NULL;
}

/* **************************************** /Network Module **************************************** */
/* **************************************** Main Module **************************************** */

int main(int argc, char* argv[])
{
  NET_STATE networkState;
  VIDEO_STATE videoState;
  SOUND_STATE soundState;
  INPUT_STATE inputState;

  memset(&networkState, 0, sizeof(NET_STATE));
  memset(&videoState, 0, sizeof(VIDEO_STATE));
  memset(&soundState, 0, sizeof(SOUND_STATE));
  memset(&inputState, 0, sizeof(INPUT_STATE));
  globalState = STARTED;

  if (InitVideo(&videoState) == -1) exit(1);
  if (InitSound(&soundState) == -1) exit(1);
  if (InitInput(&inputState) == -1) exit(1);

  networkState.pVideoState = &videoState;
  networkState.pSoundState = &soundState;
  networkState.pInputState = &inputState;

  if (InitNetwork(&networkState) == -1) exit(1);

  do {
    UpdateSound(&soundState);
    UpdateInput(&inputState);
    FlipVideo(&videoState);
  } while (globalState == STARTED);

  ShutdownVideo(&videoState);
  ShutdownSound(&soundState);
  ShutdownNetwork(&networkState);
  ShutdownInput(&inputState);

  exit(0);
  return 0;
}

/* **************************************** /Main Module **************************************** */

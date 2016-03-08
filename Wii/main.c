/*
  NetBeam Game Streaming Client
*/
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
//#include <aesndlib.h>
////#include "jpeg.h"

static void* frameBuffer[2] = { NULL, NULL };
static u32 frameCount = 0;
static u32 fb = 0;

static GXRModeObj *rmode = NULL;
static lwp_t thread_handle = (lwp_t)NULL;

typedef struct {
  s32 sock; 
  struct sockaddr_in sa;
  
} socket_data;

const int SCR_WIDTH = 160;
const int SCR_HEIGHT = 120;
const int PIX_SIZE = 4;

int bRunning = 1;

void UpdateFramebuffer(void* fb, char* buffer, s32 count);

void *networkThread(void* argument)
{
  //char recvData[];
  //char sendData[1026];
  u32 recvDataSz = SCR_WIDTH*SCR_HEIGHT*PIX_SIZE + 2; 
  char* recvData = (char*)malloc(recvDataSz);
  u32 sendDataSz = 1026;
  char* sendData = (char*)malloc(sendDataSz);
  s32 bytes;
  
  if (recvData == NULL || sendData == NULL) {
    free(recvData); free(sendData); return NULL;
  }
  
  memset(recvData, 0, recvDataSz);
  memset(sendData, 0, sendDataSz);
  
  socket_data handle = *((socket_data*)argument);
  
  while (bRunning)
  { // ARGB
    sendData[0] = 255;
    sendData[1] = (frameCount % 256);
    sendData[2] = 0;
    sendData[3] = 0;
    
    net_send(handle.sock, sendData, 4, 0);
    //net_send "req" + frameCount + 1;
    /* input sendData */
    // grab frame    
    // image sendData and sound sendData
    bytes = net_recv(handle.sock, recvData, 1024, 0); // echo server
    
    if (bytes <= 0) { bRunning = 0; }
    
    // update framebuffer
    UpdateFramebuffer(frameBuffer[fb^1], recvData, bytes);    
    fb ^= 1;
    frameCount++;
  }
  
  free(recvData);
  free(sendData);
  
  return NULL;
}

void UpdateFramebuffer(void* fb, char* buffer, s32 count)
{
  u16* colorBuffer = (u16*)fb;
  u32* pixBuffer = (u32*)buffer;
  u32 width = rmode->fbWidth;
  u32 height = rmode->xfbHeight;
  //u32 color = ((u32*)buffer)[0];
  u32 x, y;
  
  for (y = 0; y < height; y++)
  {
    for (x = 0; x < width; x++)
    {
      u16 dx;
      u16 dy;
      u16 p;
      u8 r; u8 g; u8 b; u8 a;
      u32 rgba;
      u16 convertedColor;
      
      dx = x * SCR_WIDTH / width;
      dy = y * SCR_HEIGHT / height;
      p = SCR_WIDTH * PIX_SIZE;
      
      rgba = pixBuffer[dy*p + dx];
      r = ((rgba >> 24)) >> 3;
      g = ((rgba >> 16) & 0xFF) >> 3;
      b = ((rgba >> 8) & 0xFF) >> 3;
      a = (rgba & 0xFF) >> 7;
      convertedColor = (a << 15) | (b << 10) | (g << 5) | (r);

      *colorBuffer = convertedColor;
      //pixBuffer[dy*p + dx]; //color;
      colorBuffer++;
    }
  }
}

int main(int argc, char** argv) 
{
    u32 ret;
    socket_data data;
    u32 error = 0;
    
    char localip[16] = {0};
    char gateway[16] = {0};
    char netmask[16] = {0};
   
    VIDEO_Init();
    WPAD_Init();
    
    rmode = VIDEO_GetPreferredMode(NULL);
    
    frameBuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    frameBuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
 
    // start jpeg
//  njInit();
    
    // start network
    ret = if_config( localip, netmask, gateway, TRUE );
    if (ret >= 0) {
      data.sock = net_socket (AF_INET, SOCK_STREAM, IPPROTO_IP);
      if (data.sock == INVALID_SOCKET) {
       // cannot create a socket
       error = 2;
      } else {
        memset(&data.sa, 0, sizeof( data.sa));
        data.sa.sin_family = AF_INET;
        data.sa.sin_port = htons(970);
        data.sa.sin_addr.s_addr = inet_addr("192.168.1.90");
        
        //res = inet_pton(AF_INET, "alien-pc", &data.sa.sin_addr);
        //struct hostent * net_gethostbyname(const char *addrString);
      
        if (-1 == net_connect(data.sock, (struct sockaddr *)&data.sa, sizeof(data.sa))) {
          net_close(data.sock);
          error = 3;
        } else {
          //net_recv();
        }
      }
    } else {
      error = 1;
    }

    // start wii systems
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(frameBuffer[fb]);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
    
    // spawn thread
    if (error == 0)
    {
      // start network thread
      LWP_CreateThread(&thread_handle, networkThread, (void*)&data, NULL, 32*1024, 50);
    }
    
    while 
      (error == 0 && bRunning == 1) 
    {
      u32 pressed;
      
      WPAD_ScanPads();
      pressed = WPAD_ButtonsDown(0);
      
      if (pressed & WPAD_BUTTON_HOME) {bRunning = 0; } //;exit(0);
      
      // per frame callback
      // update input array
      // ...
    
      VIDEO_SetNextFramebuffer(frameBuffer[fb]);
      VIDEO_Flush();    
      VIDEO_WaitVSync();
      if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
    }
    
//  njDone();

    // term
    net_close(data.sock);
    return 0;
}

/*
  Released under Artistic License 2.0.
  Netbeam 2016.
*/
#ifndef _NETBEAM_NETWORK_H
#define _NETBEAM_NETWORK_H

#include <network.h>
#include "input.h"
#include "sound.h"
#include "video.h"

#define MAX_NET_SIZE      52000

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

typedef struct {
  unsigned int dataLength;
  unsigned int timeStamp;
  unsigned int width;
  unsigned int height;
  unsigned int size;
  unsigned char yuvFormat;
} RESPONSE_HEADER;

int InitNetwork(NET_STATE* ref);
int ShutdownNetwork(NET_STATE* ref);
void* NetworkThread(void* arg);

#endif // HEADER WRAPPER

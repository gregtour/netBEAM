/*
  NetBeam Game Streaming Client | Wii | netbeam.c
  Copyright (C) 2016 Greg Tourville
  Available under the Artistic License 2.0.
*/
/* **************************************** Main Header **************************************** */

#include "netbeam.h"
#include "sound.h"
#include "video.h"
#include "input.h"
#include "network.h"

_STATE globalState;

//! Byte swap unsigned int
unsigned int swap_uint32(unsigned int val)
{
  val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
  return (val << 16) | (val >> 16);
}

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
  
  //InitCompression();
  
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
  
  //ShutdownCompression();

  exit(0);
  return 0;
}

/* **************************************** /Main Module **************************************** */

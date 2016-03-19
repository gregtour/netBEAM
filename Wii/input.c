/* **************************************** Input Module **************************************** */

#include <wiiuse/wpad.h>
#include "netbeam.h"
#include "input.h"

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
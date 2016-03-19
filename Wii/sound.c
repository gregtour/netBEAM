/* **************************************** Sound Module **************************************** */

#include "netbeam.h"
#include "sound.h"

int InitSound(SOUND_STATE* ref)
{
  // stub
  ref->enabled = 1;
  return 0;
}

int ShutdownSound(SOUND_STATE* ref)
{
  // stub
  ref->enabled = 0;
  return 0;
}

int UpdateSound(SOUND_STATE* ref)
{
  // stub
  return 0;
}

/* **************************************** /Sound Module **************************************** */


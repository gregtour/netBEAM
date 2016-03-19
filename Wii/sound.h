/*
  Released under Artistic License 2.0.
  Netbeam 2016.
*/
#ifndef _NETBEAM_SOUND_H
#define _NETBEAM_SOUND_H

typedef struct {
  unsigned char enabled;
} SOUND_STATE;

int InitSound(SOUND_STATE* ref);
int ShutdownSound(SOUND_STATE* ref);
int UpdateSound(SOUND_STATE* ref);

#endif // HEADER WRAPPER

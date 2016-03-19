/*
  Released under Artistic License 2.0.
  Netbeam 2016.
*/
#ifndef _NETBEAM_INPUT_H
#define _NETBEAM_INPUT_H

typedef struct {
  unsigned char buttons[8];
} INPUT_STATE;

int InitInput(INPUT_STATE* ref);
int ShutdownInput(INPUT_STATE* ref);
int UpdateInput(INPUT_STATE* ref);

#endif // HEADER WRAPPER

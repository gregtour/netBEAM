/*
  Released under Artistic License 2.0.
  Netbeam 2016.
*/
#ifndef _NETBEAM_GLOBAL_H
#define _NETBEAM_GLOBAL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <debug.h>
#include <errno.h>

typedef enum {
  ERROR = 0,
  STARTED = 1,
  STOPPING = 2
} _STATE;

extern _STATE globalState;

// read from config file instead //
//#define HOST_NAME           "alien-pc"
#define HOST_IP             "192.168.1.90"
#define NETWORK_PORT_A      970   /* video */
#define NETWORK_PORT_B      971   /* input */
/*#define NETWORK_PORT_C    972   // sound // */

#define INPUT_BUFFER_SIZE   (1024*1024)
#define PAGE_SIZE           (256*256)
#define OUTPUT_BUFFER_SIZE  128

unsigned int swap_uint32(unsigned int val);

#endif // HEADER WRAPPER

/* **************************************** Network Module **************************************** */

#include "netbeam.h"
#include "network.h"

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
  ref->sockAddr.sin_port = htons(NETWORK_PORT_A);
  ref->sockAddr.sin_addr.s_addr = inet_addr(HOST_IP);

  /*
    getaddrinfo(HOST_NAME, STR(HOST_IP), NULL, &addrinfo);
  */

  result = net_connect(ref->socket, (struct sockaddr *)&ref->sockAddr, sizeof(ref->sockAddr));
  if (result == -1) {
    net_close(ref->socket);
    return -1;
  }

  if (LWP_CreateThread(&ref->thread_handle, NetworkThread, (void*)ref, NULL, 256*1024, 60) < 0)
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
  recvData = (/*MEM_K0_TO_K1(*/malloc(recvDataSz)/*)*/);
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
    int sentBytes = 1, recvBytes = 0;
    COLOR_BUFFER renderFrame;

    // prep data to send
    //memset(sendData, 0, sendDataSz);
    
    //sprintf(sendData, "Client response; Frame number: %i.\r\n", ref->pVideoState->frame++);
    //sentBytes = net_send(ref->socket, sendData, strlen(sendData)+1, 0);

    if (sentBytes < 0) { globalState = STOPPING; break; }

    // prep data to recv frame
    int bytesReceived = 0;

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
      int reqAmount = (responseHeader.dataLength - bytesReceived);
      if (reqAmount > MAX_NET_SIZE) reqAmount = MAX_NET_SIZE;
      recvBytes = net_recv(ref->socket, recvData + bytesReceived, reqAmount, 0);
      bytesReceived += recvBytes;
      
      /*sprintf(sendData, "Downloading: Received: %i. %i / %i.\r\n", 
        recvBytes, bytesReceived, responseHeader.dataLength);
      sentBytes = net_send(ref->socket, sendData, strlen(sendData)+1, 0);*/
    }

    if (bytesReceived < responseHeader.dataLength || bytesReceived < 0) {
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
    
    /*sprintf(sendData, "Client response; Flipping frame %i. Received data: %i / %i\r\n",  ref->pVideoState->frame-1, bytesReceived, responseHeader.dataLength);
    sentBytes = net_send(ref->socket, sendData, strlen(sendData)+1, 0);*/

    if (responseHeader.timeStamp > lastTimeStamp) {
      // blit the image to the screen
      //RenderColorBuffer(ref->pVideoState, &renderFrame);
      /*sprintf(sendData, "Client response; Copy frame %i of size %i. Resolution is %i x %i.\r\n",  
        responseHeader.timeStamp, responseHeader.dataLength, renderFrame.width, renderFrame.height);
      sentBytes = net_send(ref->socket, sendData, strlen(sendData)+1, 0);*/
      
      ref->pVideoState->mainBuffer.width = renderFrame.width;
      ref->pVideoState->mainBuffer.height = renderFrame.height;
      ref->pVideoState->mainBuffer.size = renderFrame.size;
      ref->pVideoState->mainBuffer.yuvFormat = renderFrame.yuvFormat;
      ref->pVideoState->mainBuffer.pixels = renderFrame.pixels;
      ref->pVideoState->decoded = 0;
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

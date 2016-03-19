/*
  Released under Artistic License 2.0.
  Netbeam 2016.
  PC Server.
*/

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <io.h>
#include <fcntl.h> 
#include <winsock2.h>
#include <ws2tcpip.h>

#define USE_WINDOWS
//#define DX
#define GDI

#pragma comment(lib, "Ws2_32.lib")

#ifdef DX
  #define WINDOWS_LEAN_AND_MEAN
  #include <windows.h>
  #include <d3d9.h>
  #include <d3d9types.h>
  #include <d3d9caps.h>
  // include the Direct3D Library file
  #pragma comment (lib, "d3d9.lib")
#endif

extern "C" 
{
  #include "../DCT/compression.h"
}

// /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
// NetBeam Common Code
// /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/

const int SCR_WIDTH = 320;
const int SCR_HEIGHT = 240;

#define FRAME_RATE    24

typedef struct {
  unsigned int dataLength;
  unsigned int timeStamp;
  unsigned int width;
  unsigned int height;
  unsigned int size;
  unsigned char yuvFormat;
} RESPONSE_HEADER;

const int DEFAULT_PORT = 970;

// /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/

using namespace std;

#define printf printf2
int __cdecl printf2(const char *format, ...)
{
  char str[1024];
  va_list argptr;
  va_start(argptr, format);
  int ret = vsnprintf(str, sizeof(str), format, argptr);
  va_end(argptr);
  OutputDebugStringA(str);
  return ret;
}

//! Byte swap unsigned int
unsigned int swap_uint32(unsigned int val)
{
  val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
  return (val << 16) | (val >> 16);
}

// BMP Library //
#define XY(x,y,w)  ((x)+(y)*(w))
void SaveBMP(string filename, int width, int height, char* pixels) {
  ofstream file;
  file.open(filename, ios::out | ios::binary);

  // BMP Header
  unsigned int ui;
  signed int si;
  short s;

  s = 19778;            file.write((char*)&s, 2);
  ui = width*height * 3 + 54;    file.write((char*)&ui, 4);
  ui = 0;               file.write((char*)&ui, 4);
  ui = 54;              file.write((char*)&ui, 4);
  ui = 40;              file.write((char*)&ui, 4);
  si = width;           file.write((char*)&si, 4);
  si = height;          file.write((char*)&si, 4);
  s = 1;                file.write((char*)&s, 2);
  s = 24;               file.write((char*)&s, 2);
  ui = 0;               for (int i = 0; i < 6; i++) file.write((char*)&ui, 4);
  
  // Color Data  
  for (int y = height - 1; y >= 0; y--) {
    for (int x = 0; x < width; x++) {
      for (int i = 0; i < 3; i++) 
      {
        file.write((char*)&pixels[XY(x * 4, y, width * 4) + i], 1);
      }
    }
  }

  file.close();
}

const int PIX_SIZE = 4;
const int BUFFER_SIZE = SCR_WIDTH * SCR_HEIGHT * PIX_SIZE;

char SCREEN_BUFFER[SCR_WIDTH * SCR_HEIGHT * PIX_SIZE];
char SCREEN_BUFFER2[SCR_WIDTH * SCR_HEIGHT * PIX_SIZE];
MACROBLOCK BLOCK_BUFFER[SCR_WIDTH / 8 * SCR_HEIGHT / 8];

#ifdef DX
  // D3D9 // global declarations
  LPDIRECT3D9 d3d;              // the pointer to our Direct3D interface
  LPDIRECT3DDEVICE9 d3ddev;     // the pointer to the device class
                                // function prototypes
  void initD3D(HWND hWnd);      // sets up and initializes Direct3D
  void render_frame(void);      // renders a single frame
  void cleanD3D(void);          // closes Direct3D and releases memory
#endif

const char* g_szClassName = "NetBeam Server";
const int DEFAULT_BUFLEN = 512;
bool running = true;

#ifdef DX
// Direct3D 9
// this function initializes and prepares Direct3D for use
D3DPRESENT_PARAMETERS D3DPresentParams;
void initD3D(HWND hWnd)
{
  d3d = Direct3DCreate9(D3D_SDK_VERSION);    // create the Direct3D interface
  ZeroMemory(&D3DPresentParams, sizeof(D3DPresentParams));    // clear out the struct for use
  D3DPresentParams.Windowed = TRUE;    // program windowed, not fullscreen
  D3DPresentParams.SwapEffect = D3DSWAPEFFECT_COPY;    // discard old frames
  D3DPresentParams.hDeviceWindow = hWnd;    // set the window to be used by Direct3D
                   // create a device class using this information and information from the d3dpp stuct
  d3d->CreateDevice(D3DADAPTER_DEFAULT,
    D3DDEVTYPE_HAL,
    hWnd,
    D3DCREATE_SOFTWARE_VERTEXPROCESSING,
    &D3DPresentParams,
    &d3ddev);
}
// this is the function that cleans up Direct3D and COM
void cleanD3D(void)
{
  d3ddev->Release();    // close and release the 3D device
  d3d->Release();    // close and release Direct3D
}
// this is the function used to render a single frame
void render_frame(void)
{
}
#endif

#ifdef USE_WINDOWS
// Network thread
DWORD WINAPI NetworkThread(LPVOID lpParameter);
// Windows code:
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_CREATE:
    break;
  case WM_DESTROY:
    break;
  case WM_TIMER:
    break;
  //case WM_PAINT:
  //  break;
  case WM_CLOSE:
    running = false;
    DestroyWindow(hWnd);
    break;
  case WM_QUIT:
    running = false;
  default:
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

// WinMain
int CALLBACK WinMain(HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{
  WNDCLASSEX wincl;
  HWND hWnd;
  MSG  message;
  int x, y, width, height, windowType;

  // Create network thread.
  InitCompression();
  
  DWORD threadID;
  CreateThread(NULL, 0, NetworkThread, NULL /* argument */, 0, &threadID);

  // Window struct
  wincl.hInstance = hInstance;
  wincl.lpszClassName = g_szClassName;
  wincl.lpfnWndProc = WndProc;
  wincl.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wincl.cbSize = sizeof(WNDCLASSEX);
  wincl.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wincl.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
  wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
  wincl.lpszMenuName = NULL;
  wincl.cbClsExtra = 0;
  wincl.cbWndExtra = 0;
  wincl.hbrBackground = NULL;

  memset(SCREEN_BUFFER, 0xFF, BUFFER_SIZE);
  
  // Register with Windows
  if (!RegisterClassEx(&wincl)) return 0;

  // Screen placement
  // Create window
  windowType = /*WS_POPUPWINDOW | */WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU /*| WS_CLIPCHILDREN | WS_CLIPSIBLINGS*/;

  hWnd = CreateWindowEx(
    WS_EX_TOPMOST, g_szClassName, g_szClassName,
    windowType, 0, 0, 400, 400,
    HWND_DESKTOP, NULL, hInstance, NULL
    );

  ShowWindow(hWnd, nCmdShow);
  
#ifdef DX
  initD3D(hWnd);
#endif
  UpdateWindow(hWnd);
  
  while (running) {
    while (PeekMessage(&message, hWnd, 0, 0, PM_REMOVE))
    {
      DispatchMessage(&message);
    }
    if (message.message == WM_QUIT) {
      break;
    }

#ifdef DX
    // clear the window to a deep blue
    d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 40, 100), 1.0f, 0);
    d3ddev->BeginScene();    // begins the 3D scene
                 // do 3D rendering on the back buffer here
    d3ddev->EndScene();    // ends the 3D scene
    d3ddev->Present(NULL, NULL, NULL, NULL);    // displays the created frame
#endif

#if defined(DX) && !defined(GDI)
    // GetBackBuffer
    IDirect3DSurface9 *surface;
    HRESULT hr = d3ddev->CreateOffscreenPlainSurface(
      D3DPresentParams.BackBufferWidth,
      D3DPresentParams.BackBufferHeight,
      D3DPresentParams.BackBufferFormat,
      D3DPOOL_SYSTEMMEM,
      &surface,
      NULL);
      
    d3ddev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface);
    if (surface != NULL)
    {
      //D3DXSaveSurfaceToFile(L"screen.bmp", D3DXIFF_BMP, pSurface, NULL, NULL);
      D3DSURFACE_DESC desc;
      surface->GetDesc(&desc);
      D3DLOCKED_RECT rect;
      surface->LockRect(&rect, NULL, D3DLOCK_READONLY);
      char* pixels = (char*)rect.pBits;
#else
    HDC hdc = GetDC(NULL); // get the desktop device context
    HDC hDest = CreateCompatibleDC(hdc); // create a device context to use yourself
    // get the height and width of the screen
    int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    // create a bitmap
    HBITMAP hbDesktop = CreateCompatibleBitmap( hdc, width, height);
    // use the previously created device context with the bitmap
    SelectObject(hDest, hbDesktop);
    // copy from the desktop device context to the bitmap device context
    // call this once per 'frame'
    BitBlt(hDest, 0,0, width, height, hdc, 0, 0, SRCCOPY);
    //BITMAP bitmap;
    //GetObject(hbDesktop, sizeof(bitmap), (void*)&bitmap);
    //DIBSECTION dib;
    //GetObject(hbDesktop, sizeof(dib), (LPVOID)&dib);
    //hbDesktop
    BITMAPINFO bmInfo = { 0 }; // get info
    bmInfo.bmiHeader.biSize = sizeof(bmInfo.bmiHeader);
    if (GetDIBits(hDest, hbDesktop, 0, 0, NULL, &bmInfo, DIB_RGB_COLORS) == 0) {
      continue;
    }
    // make buffer
    char* pixelData = new char[bmInfo.bmiHeader.biSizeImage];
    bmInfo.bmiHeader.biBitCount = 32;
    bmInfo.bmiHeader.biCompression = BI_RGB;
    bmInfo.bmiHeader.biHeight = abs(bmInfo.bmiHeader.biHeight);
    if (GetDIBits(hDest, hbDesktop, 0, bmInfo.bmiHeader.biHeight, (void*)pixelData, &bmInfo, DIB_RGB_COLORS) == 0) {
      continue;
    }
    char* pixels = (char*)pixelData;//dib.dsBm.bmBits;
    //bitmap.bmBits;
    //NULL;
#endif



      if (pixels != NULL) 
      {
        int X, Y;
        for (Y = 0; Y < SCR_HEIGHT; Y++) {
          for (X = 0; X < SCR_WIDTH; X++)
          {
            //int LINE = Y * desc.Height / SCR_HEIGHT;
            //int ROW = X * 4;// / SCR_WIDTH;
#if defined(DX) && !defined(GDI)
            int POS = Y * SCR_WIDTH * 4 + X * 4;
#else
          //int POS = (height - (2*Y + 320) - 1) * width * 4 + (2*X + 160) * 4;
          int POS = (height - (Y * height / SCR_HEIGHT)-1) * width * 4 + (X * width / SCR_WIDTH)* 4;
#endif
            /*SCREEN_BUFFER[Y*SCR_WIDTH*PIX_SIZE + X*PIX_SIZE] = pixels[POS + 2];
            SCREEN_BUFFER[Y*SCR_WIDTH*PIX_SIZE + X*PIX_SIZE + 1] = pixels[POS + 1];
            SCREEN_BUFFER[Y*SCR_WIDTH*PIX_SIZE + X*PIX_SIZE + 2] = pixels[POS];
            SCREEN_BUFFER[Y*SCR_WIDTH*PIX_SIZE + X*PIX_SIZE + 3] = pixels[POS + 3];*/

            SCREEN_BUFFER[Y*SCR_WIDTH*PIX_SIZE + X*PIX_SIZE] =  pixels[POS + 3];
            SCREEN_BUFFER[Y*SCR_WIDTH*PIX_SIZE + X*PIX_SIZE + 1] =  pixels[POS + 0];
            SCREEN_BUFFER[Y*SCR_WIDTH*PIX_SIZE + X*PIX_SIZE + 2] =  pixels[POS + 1];
            SCREEN_BUFFER[Y*SCR_WIDTH*PIX_SIZE + X*PIX_SIZE + 3] = pixels[POS + 2];
          }
        }
      }


      SaveBMP("screencap.bmp", SCR_WIDTH, SCR_HEIGHT, SCREEN_BUFFER);

#if 0
      int bwidth, bheight;
      DCTcompress((const unsigned char*)SCREEN_BUFFER, BLOCK_BUFFER, SCR_WIDTH, SCR_HEIGHT, &bwidth, &bheight);
      //header.dataLength = swap_uint32(sizeof(BLOCK_BUFFER));

      DCTdecompress(BLOCK_BUFFER, SCREEN_BUFFER2, bwidth, bheight);
      SaveBMP("decompressed.bmp", SCR_WIDTH, SCR_HEIGHT, SCREEN_BUFFER2);
#endif


#if defined(DX) && !defined(GDI)
      surface->UnlockRect();
      surface->Release();
    }
#else
    DeleteObject(hbDesktop);
    // after the recording is done, release the desktop context you got..
    ReleaseDC(NULL, hdc);
    // ..and delete the context you created
    DeleteDC(hDest);
    delete[] pixelData;
#endif

    Sleep(1);
  }
#ifdef DX
  cleanD3D();
#endif
  return 0;
}

#endif

// TODO: Capture screen.
// TODO: Encode Jpeg.
// TODO: Capture Audio. Compress.
// TODO: Simulate input driver. [WinDDK & .inf]
// TODO: Send sound and video.
// TODO: Correct timing.
// TODO: Switch to UDP packets. Improve network and application performance.

//int main() 
#ifdef USE_WINDOWS
DWORD WINAPI NetworkThread(LPVOID lpParameter)
#else
int main(int argc, char* argv)
#endif
{
  WSADATA wsaData;
  int iResult;
  int i, count;
  int stamp = 2;
  struct addrinfo *result = NULL, *ptr = NULL, hints;
  
    // Initialize Winsock
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) {
    printf("WSAStartup failed: %d\n", iResult);
    return 1;
  }

  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  // Resolve the local address and port to be used by the server
  iResult = getaddrinfo(NULL, "970", &hints, &result);
  if (iResult != 0) {
    printf("getaddrinfo failed: %d\n", iResult);
    WSACleanup();
    return 1;
  }
  SOCKET ListenSocket = INVALID_SOCKET;
  // Create a SOCKET for the server to listen for client connections
  ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (ListenSocket == INVALID_SOCKET) {
    printf("Error at socket(): %ld\n", WSAGetLastError());
    freeaddrinfo(result);
    WSACleanup();
    return 1;
  }
  // Setup the TCP listening socket
  iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
  if (iResult == SOCKET_ERROR) {
    printf("bind failed with error: %d\n", WSAGetLastError());
    freeaddrinfo(result);
    closesocket(ListenSocket);
    WSACleanup();
    return 1;
  }
  freeaddrinfo(result);
  if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
    printf("Listen failed with error: %ld\n", WSAGetLastError());
    closesocket(ListenSocket);
    WSACleanup();
    return 1;
  }
  SOCKET ClientSocket;
  ClientSocket = INVALID_SOCKET;
  for (;;) {
    char recvbuf[DEFAULT_BUFLEN];
    int iSendResult;
    int recvbuflen = DEFAULT_BUFLEN;
    RESPONSE_HEADER header;
    
    printf("Waiting for client...\n");
    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
      printf("accept failed: %d\n", WSAGetLastError());
      closesocket(ListenSocket);
      WSACleanup();
      return 1;
    }
    printf("Client accepted!\n");
    // Receive until the peer shuts down the connection
    do {
      int bwidth, bheight;
      //swap_uint32(/*sizeof(pixel_data)*/ BUFFER_SIZE);
      header.timeStamp = swap_uint32(stamp++);
      header.width = swap_uint32(SCR_WIDTH);
      header.height = swap_uint32(SCR_HEIGHT);
      header.size = swap_uint32(0);
      header.yuvFormat = 0;
      // compress frame
      //extern MACROBLOCK* DCTcompress(const unsigned char* img, MACROBLOCK* data, const int width, const int height, int* bw, int* bh);
      DCTcompress((const unsigned char*)SCREEN_BUFFER, BLOCK_BUFFER, SCR_WIDTH, SCR_HEIGHT, &bwidth, &bheight);
      header.dataLength = swap_uint32(sizeof(BLOCK_BUFFER));
      //iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
      iResult = 1;
      recvbuf[0] = '\0';
      if (iResult >= 0) {
        if (iResult) {
          printf("Bytes received: %d\n", iResult);
          //recvbuf[iResult - 1] = 0;
          //printf("'%s'\n\n", recvbuf);
          //iResult = 1;
          for (int i = 0; i < iResult; i++)
          {
            printf("%c", recvbuf[i]);
          }
          printf("\n");
        }
        else {
          iResult = 1;
        }

        // Echo the buffer back to the sender
        //iSendResult = send(ClientSocket, recvbuf, iResult, 0);
        //iSendResult = send(ClientSocket, SCREEN_BUFFER, BUFFER_SIZE, 0);
        //iSendResult = send(ClientSocket, (const char*)sizeData, 4, 0);
        iSendResult = send(ClientSocket, (const char*)&header, sizeof(header), 0);
        iSendResult = send(ClientSocket, (const char*)BLOCK_BUFFER/*SCREEN_BUFFER*/, sizeof(BLOCK_BUFFER), 0);
        if (iSendResult == SOCKET_ERROR) {
          printf("send failed: %d\n", WSAGetLastError());
          closesocket(ClientSocket);
          //WSACleanup();
          //return 1;
          iResult = 0;
        }
        printf("Bytes sent: %d\n", iSendResult);
        Sleep(1000 / FRAME_RATE);
      }
      //else if (iResult == 0)
      //  printf("Connection closing...\n");
      else {
        printf("recv failed: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        //WSACleanup();
        //system("PAUSE");
        //return 1;
        iResult = 0;
      }
    } while (iResult > 0);

    // shutdown the send half of the connection since no more data will be sent
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
      printf("shutdown failed: %d\n", WSAGetLastError());
      closesocket(ClientSocket);
      WSACleanup();
      system("PAUSE");
      return 1;
    }
    // cleanup
    closesocket(ClientSocket);
    WSACleanup();
    system("PAUSE");
    return 1;
  }
  WSACleanup();
#ifndef DX
  system("PAUSE");
#endif
  return 0;
}

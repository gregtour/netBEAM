
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#include <d3d9.h>
#include <d3d9types.h>
#include <d3d9caps.h>
// include the Direct3D Library file
#pragma comment (lib, "d3d9.lib")
// WinSock
#pragma comment(lib, "Ws2_32.lib")

const int SCR_WIDTH = 160;
const int SCR_HEIGHT = 120;
const int PIX_SIZE = 4;
const int BUFFER_SIZE = SCR_WIDTH * SCR_HEIGHT * PIX_SIZE;

char SCREEN_BUFFER[SCR_WIDTH * SCR_HEIGHT * PIX_SIZE];
//int nReadBuffer = 0;
//int nWriteBuffer = 1;
//int bScrReading = 0;
//int bScrWriting = 0;


// D3D9 // global declarations
LPDIRECT3D9 d3d;    // the pointer to our Direct3D interface
LPDIRECT3DDEVICE9 d3ddev;    // the pointer to the device class
						 // function prototypes
void initD3D(HWND hWnd);    // sets up and initializes Direct3D
void render_frame(void);    // renders a single frame
void cleanD3D(void);    // closes Direct3D and releases memory


const char* g_szClassName = "NetBeam Server";

bool running = true;
const int DEFAULT_PORT = 970;
const int DEFAULT_BUFLEN = 512;


// Direct3D 9
// this function initializes and prepares Direct3D for use
void initD3D(HWND hWnd)
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);    // create the Direct3D interface
	D3DPRESENT_PARAMETERS d3dpp;    // create a struct to hold various device information
	ZeroMemory(&d3dpp, sizeof(d3dpp));    // clear out the struct for use
	d3dpp.Windowed = TRUE;    // program windowed, not fullscreen
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;    // discard old frames
	d3dpp.hDeviceWindow = hWnd;    // set the window to be used by Direct3D
								   // create a device class using this information and information from the d3dpp stuct
	d3d->CreateDevice(D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp,
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
	// clear the window to a deep blue
	d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 40, 100), 1.0f, 0);
	d3ddev->BeginScene();    // begins the 3D scene
							 // do 3D rendering on the back buffer here
	d3ddev->EndScene();    // ends the 3D scene
	d3ddev->Present(NULL, NULL, NULL, NULL);    // displays the created frame
}

// Network thread

DWORD WINAPI NetworkThread(LPVOID lpParameter);

// Windows code: for directx

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
	//	break;
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

	// Register with Windows
	memset(SCREEN_BUFFER, 0xFF, BUFFER_SIZE);
	if (!RegisterClassEx(&wincl)) return 0;

	// Screen placement
	// Create window
	windowType = WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU /*| WS_CLIPCHILDREN | WS_CLIPSIBLINGS*/;

	hWnd = CreateWindowEx(
		WS_EX_TOPMOST, g_szClassName, g_szClassName,
		windowType, 0, 0, 400, 400,
		HWND_DESKTOP, NULL, hInstance, NULL
		);

	ShowWindow(hWnd, nCmdShow);

	initD3D(hWnd);

	UpdateWindow(hWnd);


	while (running) {
		while (PeekMessage(&message, hWnd, 0, 0, PM_REMOVE))
		{
			DispatchMessage(&message);
		}
		if (message.message == WM_QUIT) {
			break;
		}


		// GetBackBuffer
		IDirect3DSurface9 *surface;

		//IDirect3DSurface9* pSurface;
		d3ddev->CreateOffscreenPlainSurface(SCR_WIDTH, SCR_HEIGHT,
			D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &surface, NULL);
		d3ddev->GetFrontBufferData(0, surface);

		//d3ddev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface);
		if (surface != NULL)
		{
			D3DSURFACE_DESC desc;
			surface->GetDesc(&desc);

			D3DLOCKED_RECT rect;
			surface->LockRect(&rect, NULL, D3DLOCK_READONLY);

			char* pixels = (char*)rect.pBits;

			if (pixels != NULL) {
				int X, Y;
				for (Y = 0; Y < SCR_HEIGHT; Y++) {
					for (X = 0; X < SCR_WIDTH; X++)
					{
						//int LINE = Y * desc.Height / SCR_HEIGHT;
						//int ROW = X * 4;// / SCR_WIDTH;
						int POS = Y * SCR_WIDTH * 4 + X * 4;
						SCREEN_BUFFER[Y*SCR_WIDTH*PIX_SIZE + X*PIX_SIZE] = pixels[POS];
						SCREEN_BUFFER[Y*SCR_WIDTH*PIX_SIZE + X*PIX_SIZE + 1] = pixels[POS + 1];
						SCREEN_BUFFER[Y*SCR_WIDTH*PIX_SIZE + X*PIX_SIZE + 2] = pixels[POS + 2];
						SCREEN_BUFFER[Y*SCR_WIDTH*PIX_SIZE + X*PIX_SIZE + 3] = pixels[POS + 3];
					}
				}
			}

			surface->UnlockRect();
			surface->Release();
		}
		//IDirect3DSurface9_LockRect(&d3drect, NULL, D3DLOCK_READONLY);
		
		// save the surface
		//D3DXSaveSurfaceToFileA("filename.png", D3DXIFF_PNG, surface, NULL, NULL);
		//SAFE_RELEASE(surface);

		render_frame();

		Sleep(1);
	}

	cleanD3D();
	return 0;
}



// TODO: Capture screen.
// TODO: Encode Jpeg.
// TODO: Capture Audio. Compress.
// TODO: Simulate input driver. [WinDDK & .inf]
// TODO: Send sound and video.
// TODO: Correct timing.
// TODO: Switch to UDP packets. Improve network and application performance.

//int main() 

DWORD WINAPI NetworkThread(LPVOID lpParameter)
{
	WSADATA wsaData;

	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	struct addrinfo *result = NULL, *ptr = NULL, hints;

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

	char recvbuf[DEFAULT_BUFLEN];
	int iSendResult;
	int recvbuflen = DEFAULT_BUFLEN;

	// Receive until the peer shuts down the connection
	do {
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);

			// Echo the buffer back to the sender
			//iSendResult = send(ClientSocket, recvbuf, iResult, 0);
			iSendResult = send(ClientSocket, SCREEN_BUFFER, BUFFER_SIZE, 0);
			if (iSendResult == SOCKET_ERROR) {
				printf("send failed: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
			printf("Bytes sent: %d\n", iSendResult);
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else {
			printf("recv failed: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}
	} while (iResult > 0);

	// shutdown the send half of the connection since no more data will be sent
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(ClientSocket);
	WSACleanup();

	return 0;
}




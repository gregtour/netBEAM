_netBEAM_: remote game casting.

The server[0] is software that streams some portion of video output[1] and optionally some portion of audio output[2] to clients that connect to the host on a specified port[3]. The server includes capabilities to receive input data[4] from the client to pass to the PC as a virtual input device.[5]

The client is software for one or more platforms that can be configured to connect to a specific host in order to receive audio and video data from a PC game while responding in turn with input data for a game controller.

Platforms for server
  * Windows with DirectX

Platforms for client, planned and implemented:
  * Nintendo Wii - Homebrew Channel - in development
  * Raspberry PI - planned

Version Features

  Wii
    - 480p video, 44800hz audio, GC controller support.

Development Notes

[0] Current methods for video capture from a Windows PC include using Direct3D to read the frontbuffer (slow), intercepting the swap chain for an application to read the backbuffer (fast), or targetting a program with GDI to read image data. DirectX seems un-cooperative right now, so I'm going to start with GDI.

[1] Development has begun with tests of quarter width raw VGA data for 16-bit color video. This has a bandwidth requirement of about 40 KB per video frame. 

The end product will probably utilize a stream of JPEG images of certain quality or resolution.

[2] Audio support is limited at this point. The initial implementation will likely use raw waveform.

[3] Port 970 TCP. UDP data might be sent on port 971.

[4] The data received will match the gamecube controller.

[5] The intention is to use vJoystick for the time being, outside of intercepting DLL's or writing a HID device driver.


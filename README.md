### _netBEAM_: remote game casting.

/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/

The server is software that streams some portion of video output and optionally some portion of audio output to clients that connect to the host on a specified port. The server includes capabilities to receive input data from the client to pass to the PC as a virtual input device.

The client is software for one or more platforms that can be configured to connect to a specific host in order to receive audio and video data from a PC game while responding in turn with input data for a game controller.

Platforms for server
  * Windows with DirectX

Platforms for client, planned and implemented:
  * Nintendo Wii - Homebrew Channel - in development
  * Raspberry PI - planned

Version Features

  Wii
    - 480p video, 44800hz audio, GC controller support.

---

## Development status.

Wii - Video 160 x 120 (QQVGA) streaming at 20 frames per second is supported.
PC - Frame grabs using GDI, supports some games.

### Proprietary Image Compression

Check out these examples:

![480 Original](/original.png)

(a) This is a lossless test image with a resolution of 640 x 480. The data size is 921,600 bytes.
The data requires 24 bits per pixel.

![480 8-bit Compressed](/d_8.png)

(b) This is an image compressed at quality 15 with 8-bit values. The data size is 216,000 bytes or 23.4% size.
The data requires 5.63 bits per pixel.

![480 8-bit + 4-bit Compressed](/d4_8.png)

(c) This is an image compressed at quality 15 with one 8-bit value and trailing 4-bit values. The data size is 115,200 bytes or 12.5%.
The data requires 3 bits per pixel.

![480 4-bit Compressed](/d4_4.png)

(d) This is an image compressed at quality 15 with 4-bit values. The data size is 108,000 bytes or 11.7%.
The data requires 2.82 bits per pixel.





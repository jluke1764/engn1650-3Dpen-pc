# engn1650-3Dpen-pc

PC code: converts MCU signals to usable variables
* input: output of MCU → usb
  * requires driver
  * IMU algorithm (separated magnetometer)
  * Mac vs PC ? 
  * C/C++
* input: camera data → usb
  * maybe requires driver? may come with its own driver
  * image processing
  * edge detection (pixel contrast tracking of light and dark)
  * ken’s algorithm for distance
  * ken’s algorithm for ellipse correction
  * how to get info from camera in real time?
* output: pen x, y, z position; speed of pen; button presses

---

### Ken's email

To compile and run the programs, you need to:
1. Install `gcc/clang` at the terminal. To do this, simply type `gcc` at the terminal and follow the instructions. You don't need Xcode as I did everything with command line tools only.
2. To compile, change to the correct directory and type `make -f macmain.c`, or `make -f nav.c`. The makefiles are embedded at the top of the respective files. `Nav.c` won't work unless you happen to have a SpaceNavigator, which you probably don't. I have one in the lab if you want to try it. Otherwise, it shouldn't be hard to modify the code to work with another HID device.

#### App 1: macmain: opens a window which you can draw stuff to, reads: keyboard/mouse/timer, and other utility functions (works but unfinished!)
`macmain/macmain.c`: The top half is the driver part, but I also have a test program (inside #ifdef STANDALONE) at the end which tests most of the functions.
`macmain/macmain.h`: header file - useful to see what functions are supported.

#### App 2: Communicates with a USB generic HID device (your pen device will either be this, or possibly a Virtual Com Port (VCP) which will require completely different code)
`usb/hidmac.c`: driver file - you don't need to touch this. Contains 4 handy functions: `rawhid_*()`.
`usb/nav.c`: test app for 3D Connection SpaceNavigator. To modify for another device, you would simply change the vid/pid (1st 2 numbers) passed to `rawhid_open()`, the code immediately after the call to process `rawhid_recv()`, and the code immediately before `rawhid_send()`, as these will be different for each device.

#### TODO:
- webcam code
- speeding up macmain by using "metal"
- example for virtual com port
- bugs / unfinished things in macmain

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

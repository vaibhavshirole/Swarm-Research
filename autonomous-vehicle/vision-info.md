# Getting the car to see

Some initial thoughts on the direction to take this project. I haven't looked outside of microcontrollers, so this is basically focused on TinyML options. Either running computer vision on device, or importing trained models to detect objects on slow camera frame rates.

The only microcontroller that I have considered outside of the ESP32 (preferred, has been used extensively) is the STM32 with the FP-AI-VISION1 module. This is likely to give better performance if using OpenCV, and supports everything that you need for this application. This is going to focus on the ESP-CAM (Which is just an ESP32 with a OV2640 or OV7670). 

## OpenCV 
Exists for object recognition and computer vision. Along with that, it can also be used for [color detection](https://how2electronics.com/color-detection-tracking-with-esp32-cam-opencv/).  
- Pretty heavy, so it doesn't run great on standard ESP32, but can run on the 8MB Flash version
- Local version is stripped down but has the most important things
- Can use webserver for full version, which is a good option if we want to use any camera

> Available as a library for ESP32 to [run on device](https://www.youtube.com/watch?v=7qPIRBY6C8c), but can also send data back, basically just acting as a camera stream. 

## OpenMV
Uses its own IDE, and trying to be the "Arduino of computer vision". Can connect it to an ESP32 over serial using their Arduino interface library if you want to connect to the internet without needing a full computer.
- Supports TensorFlow Lite through a module
- Higher resolution, higher performance
    - Higher frame rate camera (6ish FPS with lowest end, the only one in stock is 20-30 FPS) instead of 2ish FPS with ESP32
- No WiFi, no Bluetooth
- Very expensive
- Code with MicroPython
- Features:
    - April Tag detection (can be cool for checkpoints and such)
    - Color Tracking
    - Face Detection
    - Eye Tracking
    - QR code Detection
    - AprilTag Tracking
    - Line Detection
    - Image Capture
    - Video Recording

> No stock in anything but OpenMV Cam H7 Plus. It is its own standalone microcontroller, which can run TensorFlow and local object recognition, but not OpenCV. 

## TensorFlow
With the TensorFlow route, we would use TensorFlow Lite for Microcontrollers (*TFLite, which is a TinyML model*). This gives more flexibility in what we want the device to be able to understand. For instance, it can understand voice models, gestures, language, etc. 

> Trained models can be imported to ESP32, STM32, and a bunch of other things. Models are trained on a computer first, and then exported as TensorFlow Lite for Microcontrollers to be "run on device". 

## Edge Impulse
Haven't looked into it much, but it makes it easier to work with a collection of real sensor data and do live signal processing from raw data. Works alongside TensorFlow.

## ESP-CAM Dev Boards

### AI Thinker [(buy)](https://www.amazon.com/dp/B096WBMZ43?psc=1)
- [Spec rundown](https://www.arducam.com/esp32-machine-vision-learning-guide/) 
- ESP32 SoC 
- Has the most existing sample code --> Less development time
- 4MB Flash
- 520KB SRAM + 4MB PSRAM
- Bluetooth & WiFi
- UART/SPI/I2C/PWM/ADC/DAC
- This one comes with [add-on USB programmer](https://randomnerdtutorials.com/upload-code-esp32-cam-mb-usb/), but usually needs [FTDI Adapter](https://randomnerdtutorials.com/program-upload-code-esp32-cam/) to program.
 
![enter image description here](https://i0.wp.com/randomnerdtutorials.com/wp-content/uploads/2020/03/ESP32-CAM-pinout-new.png?quality=100&strip=all&ssl=1)

### ESP-EYE [(buy)](https://www.adafruit.com/product/4095)
- Comes with programmer built-in
- 4MB Flash
- 8MB PSRAM
- Less documentation than AI Thinker but should be fine
- More expensive

### FREENOVE WROVER-CAM [(buy)](https://www.amazon.com/dp/B09BC5CNHM?psc=1) 
- Based on WROVER
	- Comes with programmer built-in
	- More GPIO
- 4MB Flash
- 520KB SRAM + 4MB PSRAM
- Potentially less documentation
- More expensive

## Direction
1. Order an ESP-CAM Dev Board (and maybe FTDI if getting the standard AI Thinker board)
2. Get TFLite running on my normal ESP32 until the board gets in

## Resources
- Translating between ESP32-CAM Dev Boards: https://randomnerdtutorials.com/esp32-cam-camera-pin-gpios/#board6
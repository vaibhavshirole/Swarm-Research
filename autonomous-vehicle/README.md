# To-do

## Hardware
* Design new (smaller) car
* Create back wheel for accurate odometry

## Software
* Create controller web interface (for testing)
* Write LIDAR drivers
* Run SLAM on ESP32

## Ordering
* TB6612 to replace L298N
    * Reason: Size + Efficiency

<br>
<br>

# Completed

## Hardware
* Make testing car

## Software
* Car moving using ESP32 and ESP-IDF + RTOS
* ESP-CAM object recognition implementation with TensorFlow
* Performance testing OpenCV and TinyML

## Ordering
| 2pc ESP32-CAM                              | https://www.amazon.com/HiLetgo-ESP32-CAM-Development-Bluetooth-Raspberry/dp/B07RXPHYNM | Amazon | $18.49 | 1 | Allows for object detection by running either a TensorFlow model or OpenCV. Using the camera will let the robot do lane following, object following, object identification, etc.                                                          |
|--------------------------------------------|--------------------------------------------------------------------------------------------------------|--------|--------|---|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 4pc Servo Motors                           | https://www.amazon.com/Smraza-Helicopter-Airplane-Control-Arduino/dp/B09Y55C21K                        | Amazon | $9.99  | 1 | These servos will be used to rotate the camera in every direction so that it can track objects as the vehicle moves. This will be useful if I go with mecanum wheels, but can also be used to keep a stationary node and survey a space.  |
| (1pc) Prototyping Base                     | https://www.amazon.com/DollaTek-Battery-Chassis-Encoder-Arduino/dp/B07F73HY34#customerReviews          | Amazon | $12.99 | 1 | Will allow quick prototyping before the custom parts. This kit includes some super cheap motors.                                                                                                                                          |
| (2pc) Antenna                              | https://www.amazon.com/Diymall-Antenna-Antennas-Arduino-ESP-072pcs/dp/B00ZBJNO9O#customerReviews       | Amazon | $9.99  | 1 | Allows for a more stable connection to the internet by the ESP32-CAM. Important for getting training data for the TensorFlow Lite model running on-device.                                                                                |
| (2pc) Low-res Incremental Rotary Encoders  | https://www.amazon.com/Maxmoral-Encoder-Degrees-Compatible-Development/dp/B07M631J1Q#customerReviews   | Amazon | $7.59  | 1 | These are the cheapest rotary encoders that I could find. I wanted to get these to see if they are workable for our odometry needs, because it will drastically reduce the price of this project ($15 savings)                            |
| (1pc) High-res Incremental Rotary Encoders | https://www.amazon.com/Signswise-Incremental-Optical-Encoder-Quadrature/dp/B085ZLCYS1                  | Amazon | $18.99 | 2 | These are basically the cheapest high resolution rotary encoders that I could find. Used for getting x and y position as the robot moves.                                                                                                 |
| (2pc) H-bridge Motor Controller            | https://www.amazon.com/DAOKI-Controller-H-Bridge-Stepper-Mega2560/dp/B085XSLKFQ                        | Amazon | $8.59  | 1 | Cheap motor controllers for the brushed DC motors so that the microcontroller doesnt burn out.                                                                                                                                            |
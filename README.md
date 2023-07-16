# Swarm-Research

## Project Structure
<pre> /swarm-research/
├── autonomous-vehicle              //discovery robot
│   ├── computer-vision             //image recognition training
│   │   ├── ESP32-CAM
│   │   │   ├── image-collection    //collect training data
│   │   │   └── object-recognition  //apply trained model
│   │   └── vision-models           //trained models
│   ├── matlab                      //discovery algorithm from MATLAB
│   │   └── ...
│   ├── micro-ros                   //main project
│   │   └── ...
│   ├── motor-check                 //test PWM drivers
│   ├── odometry                    //back wheel 3D model
│   ├── playground                  //testing space
│   │   └── ...
│   └── ydlidar-drivers             //in-progress LiDAR drivers
├── chaos                           //chaotic encryption
├── mesh                            //demo mesh project
└── mqtt                            //demo mqtt project
</pre>
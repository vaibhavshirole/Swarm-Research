# Getting the car to move

## Task (delete later)
While we are checking the options for the image processing hw/sw, could you please take a look at how to make the vehicle move? The microcontroller should be connected to a drive motor board commanding the wheels. How do we make the vehicle steer?  Can you come up with a first idea for a design that includes the microcontroller, the drive board, the chassis (to be 3D printed), wheels and motors? In the near future we would like to buy and assemble all these things together. 

## Questions
* Budget?
* Powering 4 or 2 wheels?
* How large?
* How important is hardware vs autonomous driving? 

## Processing
ESP32S - Can stick with the ones that we already have. They have enough pins. Through an ESC, you need one pin per motor for a PWM signal, and 4 pins total if using a motor controller (H-bridge). 

## Motors
After looking into the options for the motors that we can get, we should focus towards finding Brushless DC (BLDC) and an ESC combo that allows for sensored operation. The biggest tradeoff here is price, and we need to decide whether to do 2 wheel or 4 wheel drive. 
### AC Motor
> Spolier: Not going to use
* Used if being powered by the wall

### DC Motor
> Spoiler: Use sensorless brushless DC motor
* Used if being powered by a battery
* Brushless
    * Energy efficient (no friction)
    * Can reach the desired speed more quickly 
    * Run faster for longer
    * Deliver more torque for the equivalent power consumption
    * Less heat & maintenance
    * Requires ESC to control, no PWM
    * Purchasing
        * [Sensorless, ESC, 2838 3600KV](https://www.amazon.com/GoolRC-Sensorless-Brushless-Electronic-Controller/dp/B083QF6Y8L) - $40 for 4 + [YouTube Guide](https://www.youtube.com/watch?v=-EjVWE8KvKE)
        * [Sensorless, no ESC S3650 4300KV](https://www.amazon.com/GoolRC-S3650-4300KV-Sensorless-Brushless/dp/B07BGWRFGX#customerReviews) - $29 for 1
        * [Sensorless, no ESC, 2212 12V, used for drones](https://www.amazon.com/Readytosky-Brushless-Motors-Phantom-Quadcopter/dp/B075DD16LK?psc=1#customerReviews) - $40 for 4
        * [Sensored, no ESC 3800KV Castle Creations](https://www.amazon.com/Motor-4-Pole-SENSORED-BRUSHLESS-1406-3800KV/dp/B0734DYWHR?ufe=app_do%3Aamzn1.fos.304cacc1-b508-45fb-a37f-a2c47c48c32f) - $72 for 1
        * [Sensored, no ESC 3800KV budget pick](https://www.amazon.com/Rocket-HI-Torque-Brushless-3800KV-Competition/dp/B083BTWJZP ) - $42 for 1
* Brushed
    * Cheaper
    * Can be used with H-bridge and PWM instead of ESC
    * Simpler design because no ESC needed

### ESC 
Quick schematic of what a basic circuit would look like: 

![bldc](https://howtomechatronics.com/wp-content/uploads/2019/02/Arduino-BLDC-Motor-Control-Circuit-Diagram-Schematic.png)

And here is what an ESC is: 

![esc-explaniner](https://howtomechatronics.com/wp-content/uploads/2019/02/How-does-an-ESC-Work-Electronic-Speed-Controller.png?ezimgfmt=ng:webp/ngcb2)

Basically, it controls the brushless motor movement or speed by activating the appropriate MOSFETs to create the rotating magnetic field so that the motor rotates. The higher the frequency or the quicker the ESC goes through the 6 intervals, the higher the speed of the motor will be. Gets which interval its in by EMF sensor or Hall-effect sensors. 

* Used for brushless
* One per motor
* Purchasing
    * Sensorless:
        * [Basic ESC](https://www.amazon.com/RC-Brushless-Electric-Controller-bullet/dp/B071GRSFBD?linkId=e1a4f6875272396432e2554f55ee0113&language=en_US)
            * [Used by the walkthrough that I pulled the above picture from](https://howtomechatronics.com/tutorials/arduino/arduino-brushless-motor-control-tutorial-esc-bldc/#comments)
        * [Affordable pack of 4](https://www.amazon.com/Readytosky-Electronic-Controller-Helicopter-Quadcopter/dp/B09G5WFXSV?psc=1)
    * Sensored:
        * [Cheapest sensored](https://www.amazon.com/HOBBYWING-QUICRUN-10BL120-SENSORED-Touring/dp/B0765Q2SVS#customerReviews)

> #### Sensored or Sensorless?
> Sensored is better becauses it allows for a smoother initial start (and low speeds) compared to a sensorless motor due to the detection of the rotor position. However for this project, it makes more sense to be sensorless for cost reasons. 



### H-bridge
Quick schematic of what a basic circuit would look like: 

![hbridge](https://i0.wp.com/dronebotworkshop.com/wp-content/uploads/2019/12/TB6612FNG-Arduino-Hookup.jpeg)

* Used for brushed
* One per two motors
* Helpful [comparison between options + how they work](https://dronebotworkshop.com/tb6612fng-h-bridge/)
* Purchasing
    * [TB6612](https://www.amazon.com/Adafruit-TB6612-Stepper-Driver-Breakout/dp/B00VY32VU4)
        * 2.5-13.5V
        * Newer, no heatsink, small
        * 91-95% efficiency
        * Has a standby mode
        * 2 motors can be controlled
        * $10
    * [L298](https://www.amazon.com/HiLetgo-Controller-Stepper-H-Bridge-Mega2560/dp/B07BK1QL5T?pd_rd_r=f8f3dfb8-ee6c-4da1-8c81-6589bbffd8c9&psc=1)
        * 4.5-46V
        * Old, heatsink, large
        * 40-70% efficiency
        * No standby mode
        * 2 motors can be controlled
        * $3

## How to have it turn
There are [three main ways](https://www.embeddedrelated.com/showarticle/906.php) to have things turn. But, I think the easiest option is to have at least two motors. I want to spend more time here to explore omniwheels, because that'd be easier to make quick movements in any direction. Downside is it will take more motors, but if we stick with sensorless BLDC it won't be too expensive.  

## Power
This can probably be any LiPo battery packs that hit the current target. A decision can be made based on what motor we get. There are plenty of options. 

## Chassis
* 2 choices 
    * 3d printed
    * kit off amazon


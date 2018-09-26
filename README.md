# Full motion flight sim
Here be the software side of this horridly dangerous and irresponsible machine.

## Hardware
### Base Controller
The base controller is the controller mounted on the sled (where the person sits) that directly interfaces with the onboard PC. 

Currently, the base controller is based off a Raspberry Pi 3 Model B running Stretch Lite.

The base controller has 3 distinct responsibilities, and 3 programs that run to independently manage each of those. They are 
#### Panel interface to the user (gpioctrl.c)
A simple panel is above the user's head that shows a series of LEDs indicating states of the machine - they include:
- Enabled: There is power to the base controller
- Link enabled: The serial controller is allowed to actively propegate instructions to the axis controllers
- Link active: The serial controller is actively receiving data from the PC
- Pitch active: The axis controller is active and actively sending acknowlegements
- Roll active: The roll controller is active and actively sending acknowlegements

In addition to the LEDs, there are two momentary pushbuttons
- Link toggle: Toggles the enabled status of the link - effectively turns movement on/off
- Position reset: Sends a signal to each axis telling them to mechanically return to zero (implementation TODO)


#### Serial signal processing from SimTools (ser2pc.c)
Simtools will send it's movement instructions over serial to the base controller. Using a common UART chip (PL2303 in my case) to go from USB to a common 4ish-wire serial, we recieve instructions. The serial processor will know if sending instructions is enabled and will propegate the instructions to the RF transmitter, as well as send a message to the panel controller letting it know the link is actively receiving data.

#### Transmitting instructions to axis controllers (rfaxiscomm.cpp)
The transmitting service will take instructions primarily from the serial processor and broadcast them to both axes. Initially, I had thought to broadcast on independent pipes for each axis, but I could get higher throughput writing both coordinates to a single pipe and have each axis figure out what it gives a shit about. Was just too expensive to switch the write pipe vs. tacking a few more bytes to the payload.

#### Setting up the Pi

TODO : THE PINS!



First we install stretch lite onto the Pi, be sure to do all the things with wpa_supplicant and the ssh file in the SD root to enable that stuff. Let's just assume you're already SSHed into it.

become root

install git with apt-get 

go into raspi-config and enable SPI

clone this repo into somewhere reasonable. 

run setup.sh in the base dir

reboot

it's all set up and will all run when the pi boots.



### Axis controller
This is explained in /src/axis_controller, I'm quite sure.

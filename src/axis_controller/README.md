Easiest way to get these on the controllers is Arduino's IDE.

Target board is a NodeMCU 1.0, which is an ESP-12E 8266 chip.

Ensure there is proper power to the board.

This is configured to use SPI interfacing with an NRF24l01+ 2.4 transciever with the following pins:
CE - D4 
SCK - D5
MISO - D6
CSN - D8
MOSI - D7

Install. Run. Should all work. Further documentation is inevitable once the generic axis script is made into separate x and y scripts and the actual interfacing with the servomotor drives is done.


Remaining stuff:   

The idea of easing, or "inertial preemption" is something I'd want to incorporate. I'll explain it more later, but it sits atop assumptions about the movement profile of the games this sim is designed for to allow the drives to implement some 'easing' countermeasures to smooth out any jitteryness and also be an additional safety mechanism against movements that approach 100% of the sim's movement capacity.

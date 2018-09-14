/*
  TMRh20 2014 - Optimized RF24 Library Fork
*/

/**
 * Example using Dynamic Payloads
 *
 * This is an example of how to use payloads of a varying (dynamic) size.
 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "./RF24.h"


using namespace std;
//
// Hardware configuration
// Configure the appropriate pins for your connections

/****************** Raspberry Pi ***********************/

// Radio CE Pin, CSN Pin, SPI Speed
// See http://www.airspayce.com/mikem/bcm2835/group__constants.html#ga63c029bd6500167152db4e57736d0939 and the related enumerations for pin information.

// Setup for GPIO 22 CE and CE0 CSN with SPI Speed @ 4Mhz
//RF24 radio(RPI_V2_GPIO_P1_22, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_4MHZ);

// NEW: Setup for RPi B+
//RF24 radio(RPI_BPLUS_GPIO_J8_15,RPI_BPLUS_GPIO_J8_24, BCM2835_SPI_SPEED_8MHZ);

// Setup for GPIO 15 CE and CE0 CSN with SPI Speed @ 8Mhz
RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ);

/*** RPi Alternate ***/
//Note: Specify SPI BUS 0 or 1 instead of CS pin number.
// See http://tmrh20.github.io/RF24/RPi.html for more information on usage

//RPi Alternate, with MRAA
//RF24 radio(15,0);

//RPi Alternate, with SPIDEV - Note: Edit RF24/arch/BBB/spi.cpp and  set 'this->device = "/dev/spidev0.0";;' or as listed in /dev
//RF24 radio(22,0);


/****************** Linux (BBB,x86,etc) ***********************/

// See http://tmrh20.github.io/RF24/pages.html for more information on usage
// See http://iotdk.intel.com/docs/master/mraa/ for more information on MRAA
// See https://www.kernel.org/doc/Documentation/spi/spidev for more information on SPIDEV

// Setup for ARM(Linux) devices like BBB using spidev (default is "/dev/spidev1.0" )
//RF24 radio(115,0);

//BBB Alternate, with mraa
// CE pin = (Header P9, Pin 13) = 59 = 13 + 46
//Note: Specify SPI BUS 0 or 1 instead of CS pin number.
//RF24 radio(59,0);

/**************************************************************/

// Radio pipe addresses comms - W, R. 
const uint64_t axispipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

static char rfcfpath[] = "/tmp/rfpath";
static char * panelcfpath = "/tmp/panelpath";



const int write_payload_size = 9;
const int read_payload_size = 4;

int resetrequested = 0;

char receive_payload[read_payload_size+1]; // +1 to allow room for a terminating NULL char

//Maybe axes this to use one read pipe and have each axes figure out what to get. 
void broadcasttocontrollers(char broadcaststr[write_payload_size]){
  uint64_t currentwriteypipe = axispipes[0];

  radio.stopListening(); //Like what my girlfriend did when I made the grave one-time mistake of saying 'calm down'
  
  char writepayload[write_payload_size];

  radio.openWritingPipe(currentwriteypipe);

  radio.write(broadcaststr, write_payload_size);

  radio.startListening();
}

void fetchandbroadcast(){
  uint32_t broadcast;
  ifstream myfile (rfcfpath);
  if (myfile.is_open()){
    myfile >> broadcast;

    uint32_t broadcast = 0x3FFFF;
      
    char outbuffer[write_payload_size];
    
    uint16_t xcoord = (broadcast & 0x1FF);
    uint16_t ycoord = (broadcast & (0x1FF << 9)) >> 9;
    int resetrequested = (broadcast & (1 << 18)) >> 18;  
    
    sprintf(outbuffer, "x%dy%dr%d", xcoord, ycoord, resetrequested);

    myfile.close();
  }
}

int main(int argc, char** argv) {

  // Print preamble:
  cout << "THE RADIO IS STARRRTING\n";

  // Setup and configure rf radio
  radio.begin();
  radio.enableDynamicPayloads();
  radio.setRetries(5, 15);
  radio.printDetails();


  printf("\n ************ THEM DETAILS IS ABOVE ***********\n");
  
  radio.openReadingPipe(1, axispipes[1]);

  radio.startListening();

// forever loop
  while (1)
  {

    if radio.available(){
      // Grab the response, compare, and send to debugging spew
      char response[read_payload_size];
      radio.read( response, read_payload_size );

      // Put a zero at the end for easy printing
      response[read_payload_size] = 0;

      // Spew it
      printf("Got response size=%i value=%s\n\r", read_payload_size, read_payload_size);

      //Send the response msg to some sort of thing. 
    }

    //Check to see if there's a command in the infile
    //If there is one, parse it and broadcast to both axes

  }
}



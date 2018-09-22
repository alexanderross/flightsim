/*

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins D4 and D8

RF24 radio(D4,D8);

// sets the role of this unit in hardware.  Connect to GND to be the 'pong' receiver
// Leave open to be the 'ping' transmitter
const int role_pin = 5;

//
// Topology
//

// Radio pipe addresses for the 2 nodes to communicate x and y share them.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

//
// Payload
//

const int read_payload_size = 10f ;

//When each is made for the individual axis, each will have a unique ACK
char ack_msg[] = "OK";

char receive_payload[read_payload_size+1]; // +1 to allow room for a terminating NULL char

void setup(void)
{

  //
  // Print preamble
  //

  Serial.begin(115200);

  //
  // Setup and configure rf radio
  //

  radio.begin();

  // enable dynamic payloads
  radio.enableDynamicPayloads();

  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);
  //
  // Start listening
  //

  radio.startListening();

  //
  // Dump the configuration of the rf unit for debugging
  //

  radio.printDetails();

  radio.stopListening();
  //Push an ack out to indicate the axis control is started and listening
  radio.write(ack_msg,2);
  radio.startListening();
}

void loop(void)
{

  while ( radio.available() )
  {

    // Fetch the payload, and see if this was the last one.
    uint8_t len = radio.getDynamicPayloadSize();
    
    // If a corrupt dynamic payload is received, it will be flushed
    if(!len){
      continue; 
    }
    
    radio.read( receive_payload, len );

    // Put a zero at the end for easy printing
    receive_payload[len] = 0;

    // Spew it
    Serial.print(F("Got response size="));
    Serial.print(len);
    Serial.print(F(" value="));
    Serial.println(receive_payload);

    //TODO - transmit to the axis drive after doing 360' translation and possible movement preemption
    //That's a big todo

    // First, stop listening so we can talk
    radio.stopListening();

    // Send the final one back.
    radio.write(ack_msg, 2 );
    Serial.println(F("Sent response."));

    // Now, resume listening so we catch the next packets.
    radio.startListening();
  }
}

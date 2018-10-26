
/*

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <SoftwareSerial.h>



//CHANGE FOR EACH AXIS DAMNIT ---------------------------
static char ACK_MSG[] = "PI";
char CMD_AXIS_FLAG = 'I';
char POS_AXIS_FLAG = 'P';
// ------------------------------------------------------

SoftwareSerial driveserial(D1, D2);

// Set up nRF24L01 radio on SPI bus plus pins D4 and D8
RF24 radio(D4,D8);

//
// Topology
//

// Radio pipe addresses for the 2 nodes to communicate x and y share them.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
static int ZERO_STOP_PIN = D0;

//
// Payload
//

const int read_payload_size = 10;
int resetrequested = 0;
char receive_payload[read_payload_size+1]; // +1 to allow room for a terminating NULL char

void setup(void)
{

  //
  // Print preamble
  //

  pinMode(ZERO_STOP_PIN, INPUT);

  Serial.begin(115200);
  
  pinMode(D2, OUTPUT);
  driveserial.begin(38400);

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
  radio.write(ACK_MSG,2);
  radio.startListening();
}

void process_message(char *message){
  char *payloaditem = message;
  char buffer[5];
  //If beginning is W, we want to write
  if(*payloaditem == 'W'){
    payloaditem++;

    if(*(payloaditem++) == CMD_AXIS_FLAG){
      int target, val;

      strncpy(buffer, payloaditem, 3);
      buffer[3] = '\0';
      sscanf(buffer, "%d", &target); 
      
      payloaditem = payloaditem + 3;
      strncpy(buffer, payloaditem, 5);
      buffer[5]= '\0';
      sscanf(buffer, "%d", &val);
      
      printf("WRITING %d to P%d\n", val, target);
      write_to_register(target, val);
    }
  //If beginning is P, it's an instruction. 
  }else if(*payloaditem == 'P'){
    //Find the relevant set (Pxxx for pitch, Rxxx for roll)
    int post_forwarding = 4;
    if(POS_AXIS_FLAG=='P'){
      payloaditem++;  
      //We need this later.
      post_forwarding = 8;
    }else{
      payloaditem = payloaditem + 5;  
    }
    
    int req_position;
    strncpy(buffer, payloaditem, 3);
    sscanf(buffer, "%d", &req_position);
    printf("Movement target is %d\n", req_position);
    //Jump to the reset value
    payloaditem = payloaditem + post_forwarding;
    
    //Check reset bytes at end (Sx) - if that is true then flip the reset flag and return
    printf("RESET is %c", *payloaditem);
    if(*payloaditem == '1'){
      resetrequested = 1;
      resetposition();
    }else{
      sendposition(req_position);
    }
  }
}

// Get the ASCII message to write value to dest_register on the drive.
static char nibble_to_hex_ascii(uint8_t nibble) {
    char c;
    if (nibble < 10) {
        c = nibble + '0';
    } else {
        c = nibble - 10 + 'A';
    }
    return c;
}

void write_to_register(int dest_register, int value){
  uint8_t message[10];
  message[0] = 1;
  message[1] = 6;
  message[2] = dest_register >> 8;
  message[3] = dest_register & 0x00ff;
  message[4] = value >> 8;
  message[5] = value & 0x00ff;

  uint8_t lrc = 0; 
  int iter = 6;

  while (iter--) {
      lrc += message[iter+1];
  }
  
  lrc = (-lrc) - 1;
  message[6] = lrc;
  
  char ascii_message[18];

  ascii_message[0] = ':';
  
  for(int k = 0; k < 7; k++){
    ascii_message[(2*k)+1] = nibble_to_hex_ascii(message[k] >> 4);
    ascii_message[(2*k)+2] = nibble_to_hex_ascii(message[k] & 0x0f);
  }
  
  ascii_message[15] = '\r';
  ascii_message[16] = '\n';
  ascii_message[17] = '\0';

  Serial.print(ascii_message);
}

void sendposition(int pos){

}

void resetposition(){
  //Or put in a speed request here.

  while(!digitalRead(ZERO_STOP_PIN)){
    //Keep inching axis drive until we hit the stop.
  }
  resetrequested = 0;
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

    //TODO - transmit to the axis drive after doing 360' translation
    //That's a big todo

    // Parse out that reset signal too

    // First, stop listening so we can talk
    radio.stopListening();

    // Send the final one back.
    radio.write(ACK_MSG, 2 );
    Serial.println(F("Sent response."));

    // Now, resume listening so we catch the next packets.
    radio.startListening();
  }
}
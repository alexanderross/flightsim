
/*

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <SoftwareSerial.h>




//CHANGE FOR EACH AXIS DAMNIT 
// PITCH---------------------------
//static char ACK_MSG[] = "P";
//static char CMD_AXIS_FLAG = 'I';
//static char POS_AXIS_FLAG = 'P';
//static char GEAR_REDUCTION = 40;
// ROLL ---------------------------
static char ACK_MSG[] = "R";
static char CMD_AXIS_FLAG = 'O';
static char POS_AXIS_FLAG = 'R';
static char GEAR_REDUCTION = 30;
// 

SoftwareSerial driveserial(D1, D2);

// Set up nRF24L01 radio on SPI bus plus pins D4 and D8
RF24 radio(D4,D8);


// Radio pipe addresses for the 2 nodes to communicate x and y share them.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
static int ZERO_STOP_PIN = D0;

const int read_payload_size = 10;
char receive_payload[read_payload_size+1]; // +1 to allow room for a terminating NULL char

//Reset flags
int resetrequested = 0;
int resetcomplete = 0;

int use_speed_jump = 1;

//We have 8 registers to write to - go through them to allow the accel smoothing for speed mode
uint16_t last_used_speed = 0;

uint16_t last_req_speed = 0;


void setup(void)
{

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

  //Push an ack out to indicate the axis control is started and listening
  ack_message();
  delay(1000);

  //Cycle the fan.
  write_to_register(75, 1);
  delay(3000);
  write_to_register(75, 0);
}

void ack_message(){
  radio.stopListening();
  radio.write(ACK_MSG,1);
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

      char *tptr;
      target = strtol(buffer, &tptr,10);
      
      payloaditem = payloaditem + 3;
      strncpy(buffer, payloaditem, 5);
      buffer[5]= '\0';

      char *pptr;
      val = strtol(buffer, &pptr, 10);
      
      Serial.printf("WRITING %d to P%d\n", val, target);
      process_cmd(target, val);
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
    char *posptr;
    req_position = strtol(buffer, &posptr, 10);

    Serial.printf("Movement target is %d\n", req_position);
    //Jump to the reset value
    payloaditem = payloaditem + post_forwarding;
    
    //Check reset bytes at end (Sx) - if that is true then flip the reset flag and return
    Serial.printf("RESET is %c", *payloaditem);

    if(*payloaditem == '1' && !resetcomplete){
      resetrequested = 1;
      resetposition();
    }else{
      sendposition(req_position);
      resetcomplete = 0;
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

void process_cmd(int destination, int value){
  if(destination >= 220){
    //This is a fancy command that we process here.
    if(destination == 220){
      //220 - Manual Reset
      resetposition();
    }else if(destination == 221){
      send_speed_command(value);
    }else if(destination == 222){
    //222 - Move to position N*2
    }else if(destination == 223){
      setdriveenabled(value);
    }else if(destination == 224){
      use_speed_jump = value;
    }

  }else{
    write_to_register(destination, value);
  }
}

void write_to_register(int dest_register, int value){
  Serial.printf("Attempt print %d - %d \n", dest_register, value);

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
      lrc += message[iter];
  }
  
  lrc = (-lrc);
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

  Serial.printf("Writing as %s \n", ascii_message);
  driveserial.print(ascii_message);
}

void send_speed_command(int requested_speed){
    if(last_req_speed != requested_speed){

      //Write the speed to ISR2
      last_used_speed++;

      if(use_speed_jump == 0 ){
        last_used_speed = 0;
      }

      if(last_used_speed == 8){
        last_used_speed = 0;
      }
      write_to_register(169 + last_used_speed, requested_speed);

      //Form the command to run ISR2
      uint32_t command = 0x01;
      //set speed to req #
      command = command | (last_used_speed << 8);
      write_to_register(69, 0);
      write_to_register(68, command);

      last_req_speed = requested_speed;
    }else{
      Serial.println("SPEED SAME");
    }
}

void setdriveenabled(int state){
  uint32_t command = state;
  write_to_register(68, command);
}

void sendposition(int pos){
  //Write the position to IP1

  int rpos = (2500/360.0) * pos * GEAR_REDUCTION;

  write_to_register(122, rpos/10000);
  write_to_register(123, rpos % 10000);

  //Form the command to run POS1
  uint32_t command = 0x01;
  //set speed to req #
  command = command | (1 << 8);
  write_to_register(68, 0);
  write_to_register(69, command);
  
}

void resetposition(){
  //Or put in a speed request here.
  send_speed_command(75);
  while(!digitalRead(ZERO_STOP_PIN)){
    //Keep waiting
  }
  resetrequested = 0;
  send_speed_command(0);

  //We check this flag above to avoid continually entering the reset position loop.
  resetcomplete = 1;
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
    receive_payload[len] = '\0';

    // Spew it
    Serial.print(F("Got response size="));
    Serial.print(len);
    Serial.print(F(" value="));
    Serial.println(receive_payload);

    //TODO - transmit to the axis drive after doing 360' translation
    //That's a big todo

    // Parse out that reset signal too
    process_message(receive_payload);

    // First, stop listening so we can talk
    radio.stopListening();

    // Send the final one back.
    ack_message();
    Serial.println(F("Sent response."));

    // Now, resume listening so we catch the next packets.
    radio.startListening();
  }
}
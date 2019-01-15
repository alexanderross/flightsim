
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
//static char MAX_MOTOR_SPEED = 1000;
//const uint8_t tx_addr[6] = "2Node";
// ROLL ---------------------------
static char ACK_MSG[] = "R";
static char CMD_AXIS_FLAG = 'O';
static char POS_AXIS_FLAG = 'R';
static char GEAR_REDUCTION = 30;
static char MAX_MOTOR_SPEED = 1250;
const uint8_t tx_addr[6] = "3Node";
// 

SoftwareSerial driveserial(D1, D2);

// Set up nRF24L01 radio on SPI bus plus pins D4 and D8
RF24 radio(D4,D8);


// Radio pipe addresses for the 2 nodes to communicate x and y share them.
const uint8_t rx_addr[6] = "1Node";
static int ZERO_STOP_PIN = D3;

static int DRIVE_MODE_SPEED = 1;
static int DRIVE_MODE_LOCATION = 2;

//Timeout for serial reads in microseconds
const int READ_TIMEOUT = 5000;

const int read_payload_size = 10;

//Reset flags
int resetrequested = 0;
int resetcomplete = 0;

int ack_interval = 140000;

//Used for determining how much to speed up depending on position deltas
int speed_coefficient = 100;


int ack_ct = 0;

//We have 8 registers to write to - go through them to allow the accel smoothing for speed mode
uint16_t last_req_speed = 0;

//The current position in degrees.
uint16_t current_position = 0;

//The current position of the drive
uint32_t current_position_steps = 0;

//We need to know this to compare against if we give another command before the previous is complete.
uint32_t last_requested_position_steps = 0;


uint8_t current_mode = DRIVE_MODE_LOCATION;



//When we start and read the initial position of the drive (or zero it), we need to be able to 'offset' to get our true zero.
//Eg. if we reset and the axis is level, but the drive indicates 11000 steps, then our offset is -11000.
uint32_t step_offset = 0;


void setup(void)
{

  pinMode(ZERO_STOP_PIN, INPUT);

  Serial.begin(115200);
  
  pinMode(D2, OUTPUT);
  driveserial.begin(38400);
  Serial.println("STARTING");

  //
  // Setup and configure rf radio
  //
  radio.begin();
  // enable dynamic payloads
  radio.enableDynamicPayloads();
  radio.setAutoAck(false);
  radio.setChannel(110);
  radio.setPALevel(RF24_PA_HIGH);



  radio.openWritingPipe(tx_addr);
  radio.openReadingPipe(1, rx_addr);
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
  write_to_register(176,0);
  send_speed_command(0);
}

void ack_message(){
  // Get the state of the drive, then send it.

  //Write the read command
  int value = read_register(386);
  //Listen to the response

  if(value < 0 ){
    Serial.println("Recieved corrupt response, not sending ack.");
  }else{

    //Ack message is 5 bytes depicting state of drive, 3 for current degree position, a utility byte and term char. (10 total.)
    char sendpayload[10];
    sprintf(sendpayload,"%05d%03dX\0", value, current_position);
    //Send the response ( 5 digits + 1 end)
    radio.stopListening();
    radio.write(sendpayload,6);
    radio.startListening();
  }
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
      //SEND AS SPEED COMMAND
      send_speed_command(value);
    }else if(destination == 222){
      //SEND POSITION
      sendposition(value);
    }else if(destination == 223){
      //ENABLE or DISABLE DRIVE
      setdriveenabled(value);
    }else if(destination == 224){
      //SET SPEED COEFFICIENT
      speed_coefficient = value;
    }else if(destination == 225){
      //ACK interval in 1k cycles
      ack_interval = value * 1000;
    }else if(destination == 226){
      //RESTART CONTROLLER
      ESP.restart();
    }else if(destination == 227){
      //Adjust position in degrees
      send_position_adjustment(value);
    }

  }else{
    write_to_register(destination, value);
  }
}

char calculate_lrc_for_message(uint8_t * message){
  uint8_t lrc = 0; 
  int iter = 6;

  while (iter--) {
      lrc += message[iter];
  }
  
  return (-lrc);
  
}

void send_modbus_ascii(int dest_register, int value, uint8_t * message){
  message[2] = dest_register >> 8;
  message[3] = dest_register & 0x00ff;
  message[4] = value >> 8;
  message[5] = value & 0x00ff;

  message[6] = calculate_lrc_for_message(message);
  
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

uint8_t response_is_valid(char * response_msg){
  if(strlen(response_msg) < 13){
    return 0;
  }

  if(response_msg[0] != ':'){
    return 0;
  }

  uint8_t newmessage[5];
      
  for(int i=0; i < 5; i++){
    char tmpasc[3];
    tmpasc[0] = response_msg[(2*i)+1];
    tmpasc[1] = response_msg[(2*i)+2];
    tmpasc[2] = '\0';
    
    uint8_t val = strtol(tmpasc,NULL,16);
    newmessage[i] = val;
  }
  
  char lrcasc[3];
  lrcasc[0] = response_msg[11];
  lrcasc[1] = response_msg[12];
  lrcasc[2] = '\0';
  
  uint8_t msg_lrc = strtol(lrcasc,NULL,16);
  
  uint8_t lrc = calculate_lrc_for_message(newmessage);
  if(msg_lrc == lrc){
    return 1;
  }else{
    return 0;
  }
  
  return 1;
}

int parse_int_from_read_response(char * response_msg){
  char tmpasc[5];
  for(int i = 7;i<11;i++){
    tmpasc[i-7] = response_msg[i];
  }
  tmpasc[4] = '\0';
  return (int)strtol(tmpasc,NULL,16);
}

int read_register(int d_register){
  if(d_register > 0 && d_register <= 389){
    driveserial.flush();

    Serial.printf("Attempt read %d  \n", d_register);

    uint8_t message[10];
    message[0] = 1;
    message[1] = 3;

    send_modbus_ascii(d_register, 1, message);

    unsigned int starttime = 0;
    starttime = micros();
    char srecbuffer[14];
    uint8_t rcv_len = 0;

    while( srecbuffer[rcv_len] != '\r'){
      if((micros() - starttime) > READ_TIMEOUT){
        Serial.println("Read timeout exceeded.");
        break;
      }

      if(driveserial.available()){
        srecbuffer[rcv_len] = driveserial.read();
        rcv_len++;
      }
      yield();
    }
    srecbuffer[rcv_len]='\0';

    Serial.printf("GOT %s\n",srecbuffer);
    Serial.println("--Validating--");

    if(response_is_valid(srecbuffer) == 1){
      return -1;
    }else{
      return parse_int_from_read_response(srecbuffer);
    }

    driveserial.flush();
  }
}

void write_to_register(int dest_register, int value){
  if(dest_register > 0 && dest_register < 190){
    Serial.printf("Attempt print %d - %d \n", dest_register, value);

    uint8_t message[10];
    message[0] = 1;
    message[1] = 6;

    send_modbus_ascii(dest_register, value, message);

  }
}

void switch_to_speed_mode(){
  if(current_mode != DRIVE_MODE_SPEED){
    Serial.println("SPEED SAME");
    write_to_register(2, DRIVE_MODE_SPEED);
    
    write_to_register(69, 0);
    write_to_register(70, 30898);
    write_to_register(71, 4095);
    current_mode = DRIVE_MODE_SPEED;
  }
}

void switch_to_location_mode(){
  if(current_mode != DRIVE_MODE_LOCATION){
    write_to_register(2, DRIVE_MODE_LOCATION);
    write_to_register(117,1);

    write_to_register(68, 1);
    write_to_register(69, 1024);
    write_to_register(70, 32691);
    write_to_register(71, 32767);
    current_mode = DRIVE_MODE_LOCATION;
  }
}

void send_speed_command(int requested_speed){
    if(last_req_speed != requested_speed){

      switch_to_speed_mode();


      write_to_register(176, requested_speed);

      uint32_t command = 0x01;
      //set speed to req #
      command = command | (8 << 8);
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

void set_zero(){
  int steps = getrotorposition;
  step_offset = -steps;
  current_position = 0;
}

//Doesn't give a shit where the motor is, just moves it n degrees at 400 speed.
void send_position_adjustment(int degrees){

  int steps = ((GEAR_REDUCTION * 10000) / 360) * degrees;
  write_to_register(128, 400);


  write_to_register(120, steps/10000);
  write_to_register(121, steps % 10000);

  //Form the command to run POS1

  write_to_register(71, 32767);
  write_to_register(71, 31743);
}

void send_degree_change(int degrees){
  //Some speed adjusting based on how far we need to be going
  int new_speed = abs(distance * speed_coefficient);

  if(new_speed > MAX_MOTOR_SPEED){
    new_speed = MAX_MOTOR_SPEED;
  }

  write_to_register(128, new_speed);


  write_to_register(120, rpos/10000);
  write_to_register(121, rpos % 10000);

  //Form the command to run POS1

  write_to_register(71, 32767);
  write_to_register(71, 31743);
}

void send_position_command(int pos){
  //Write the position to IP1
  if(pos != last_req_location){
    checkposition();

    if(current_position != pos){
      switch_to_location_mode();

      int distance = 0;

      //We assume the closest path to the pos target from the current pos
      if(current_position < 40 && pos > 320){
        distance = pos - 360 - current_position;
      }else if(current_position > 320 && pos < 40){
        distance = current_position - 360 - pos;
      }else{
        distance = pos - current_position;
      }

      send_degree_change(distance);
    }
  }
}

void resetposition(){
  //Or put in a speed request here.
  Serial.println("Reset requested");
  send_speed_command(100);
  while(digitalRead(ZERO_STOP_PIN)){
    yield();
  }
  Serial.println("Reset zero signaled");
  resetrequested = 0;
  send_speed_command(0);

  //We check this flag above to avoid continually entering the reset position loop.
  resetcomplete = 1;
  set_zero();
}

int getrotorposition(){
  int pos_high = read_register(378);
  int pos_lo = read_register(377);
  //Listen to the response

  int rotor_position = 0;

  if(pos_high >= 0){
    rotor_position = pos_high * 10000;
  }

  if(pos_lo >= 0){
    rotor_position += pos_lo;
  }

  return rotor_position;
}

void checkposition(){

  current_position_steps = getrotorposition;
  current_position = ( 360.0 / ( GEAR_REDUCTION * 10000 ) ) * ( (rotor_position + offset) % (GEAR_REDUCTION * 10000) );
}


void loop(void)
{

  while( radio.available() )
  {

    // Fetch the payload, and see if this was the last one.
    uint8_t len = radio.getDynamicPayloadSize();
    
    // If a corrupt dynamic payload is received, it will be flushed
    if(!len){
      Serial.println("Dynamic payload is corrupted.");
      continue; 
    }

    char receive_payload[read_payload_size+1]; // +1 to allow room for a terminating NULL char
    
    radio.read( receive_payload, len );

    // Put a zero at the end for easy printing
    receive_payload[len] = '\0';

    // Spew it
    Serial.printf("Got %s len is %d", receive_payload, strlen(receive_payload));
    Serial.println(receive_payload);

    //TODO - transmit to the axis drive after doing 360' translation
    //That's a big todo

    // Parse out that reset signal too
    process_message(receive_payload);

  }
  ack_ct++;

  if(ack_ct > ack_interval){
    ack_message();
    ack_ct = 0;
  }
}
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <fcntl.h>
#include <stdio.h> 
#include <RF24/RF24.h>
#include <sys/shm.h>


using namespace std;

// NEW: Setup for RPi B+
//RF24 radio(RPI_BPLUS_GPIO_J8_15,RPI_BPLUS_GPIO_J8_24, BCM2835_SPI_SPEED_8MHZ);

// Setup for GPIO 15 CE and CE0 CSN with SPI Speed @ 8Mhz
RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ);

// Radio pipe addresses comms - W, R. 
const uint64_t axispipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

static char rfcfpath[] = "/tmp/rfpath";
static char rfcmdpath[] = "/tmp/rfcmdpath"; 
static char panelcfpath[] = "/tmp/panelpath";

static char xactiveflag = 'R';
static char yactiveflag = 'P';

static uint8_t ROLLACTIVEMASK = 0x40; //01000000 64
static uint8_t PITCHACTIVEMASK = 0x20;//00100000 32

const int write_payload_size = 10;
const int read_payload_size = 2;

int resetrequested = 0;

int rf_cmd_desc = open( "/tmp/rfcmdpath", O_RDONLY,O_NONBLOCK);

char receive_payload[read_payload_size+1]; // +1 to allow room for a terminating NULL char

//Maybe axes this to use one read pipe and have each axes figure out what to get. 
void broadcasttocontrollers(char broadcaststr[write_payload_size+1]){
  uint64_t currentwriteypipe = axispipes[0];

  radio.stopListening(); //Like what my SO did when I made the grave one-time mistake of saying 'calm down'
  
  char writepayload[write_payload_size];

  radio.openWritingPipe(currentwriteypipe);

  radio.write(broadcaststr, write_payload_size);
  printf("WROTE %s \n", broadcaststr);

  radio.startListening();
}

void writetosharedmem(char * path, uint32_t contents, int do_overwrite){
  int shmid;
  key_t key;
  uint32_t *shm, *s;

  key = ftok(path, 65);

  if ((shmid = shmget(key, 16, IPC_CREAT | 0666)) < 0) {
      perror("shmget error");
  }

  if ((shm = (uint32_t *) shmat(shmid, NULL, 0)) == (uint32_t *) -1) {
      perror("shmat error");
  }

  s = shm;

  if(do_overwrite){
    *s = contents;
  }else{
    *s = *s | contents;
  }

  shmdt(shm);

}

uint32_t readfromsharedmem(char * path, int do_clear){
    int shmid;
    key_t key;
    uint32_t *shm, *s;

    uint32_t returnval = 0;

    key = ftok(path, 65);

    if ((shmid = shmget(key, 16, IPC_CREAT | 0666)) < 0) {
        perror("shmget error");
    }

    if ((shm = (uint32_t *) shmat(shmid, NULL, 0)) == (uint32_t *) -1) {
        perror("shmat error");
    }

    s = shm;

    returnval = returnval | *s;

    if(do_clear){
        *s = 0;
    }

    shmdt(shm);

    return returnval;
}

//This is dirty, but it does what it needs to do.
void readfromcmdmem(){
  char inbuffer[255];

  int bytes_read = read( rf_cmd_desc, inbuffer, 255);

  if( bytes_read > 0){
    printf("GOT %s\n",inbuffer);  
    broadcasttocontrollers(inbuffer);
  }

}

void fetchandbroadcast(){
  readfromcmdmem();
  uint32_t broadcast = readfromsharedmem(rfcfpath, 1);
  if(broadcast != 0){
    char outbuffer[write_payload_size];

    uint16_t xcoord = (broadcast & 0x1FF);
    uint16_t ycoord = (broadcast & (0x1FF << 9)) >> 9;
    int resetrequested = (broadcast & (1 << 18)) >> 18; 
    
    sprintf(outbuffer, "P%dR%dS%d", xcoord, ycoord, resetrequested);

    broadcasttocontrollers(outbuffer);
  }
}

void setgpioflags(int xactive, int yactive){
  uint32_t panelwritemask = 0;
  if(xactive == 1){
    panelwritemask = panelwritemask | ROLLACTIVEMASK;
  }

  if(yactive == 1){
    panelwritemask = panelwritemask | PITCHACTIVEMASK;
  }
  
  writetosharedmem(panelcfpath, panelwritemask, 0);

}

void handleresponse(char response[]){
  for(int i = 0; i < read_payload_size; i++){
    if(response[i] == xactiveflag){
      setgpioflags(1,0);
    }else if(response[i] == yactiveflag){
      setgpioflags(0,1);
    }
  }
}

int main(int argc, char** argv) {

  // Print preamble:
  cout << "THE RADIO IS STARRRTING\n";

  // Setup and configure rf radio
  radio.begin();
  radio.enableDynamicPayloads();
  radio.setRetries(5, 15);
  radio.setPALevel(RF24_PA_MIN);
  radio.printDetails();


  printf("\n ************ THEM DETAILS IS ABOVE ***********\n");
  
  radio.openReadingPipe(1, axispipes[1]);

  radio.startListening();

// forever aloop
  while (1)
  {

    if(radio.available()){
      // Grab the response, compare, and send to debugging spew
      char response[read_payload_size];
      radio.read( response, read_payload_size );

      handleresponse(response);


      // Spew it
      printf("Got response size=%i value=%s\n\r", read_payload_size, response);

      //Send the response msg to some sort of thing. 
    }

    //Check to see if there's a command in the shared mem
    //If there is one, parse it and broadcast to both axes
    fetchandbroadcast();

  }
}
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <fcntl.h>
#include <stdio.h> 
#include <unistd.h> 
#include <RF24/RF24.h>
#include <sys/shm.h>


using namespace std;

// NEW: Setup for RPi B+
//RF24 radio(RPI_BPLUS_GPIO_J8_15,RPI_BPLUS_GPIO_J8_24, BCM2835_SPI_SPEED_8MHZ);

// Setup for GPIO 15 CE and CE0 CSN with SPI Speed @ 8Mhz
RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ);

static uint8_t PITCH_PIPE_INDEX = 2;
static uint8_t ROLL_PIPE_INDEX = 1;

// Radio pipe addresses comms - W, R. 
const uint64_t axispipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
const uint8_t txaddr[6] = "1Node";
const uint8_t pitchrxaddr[6] = "2Node";
const uint8_t rollrxaddr[6] = "3Node";

static char rfcfpath[] = "/tmp/rfpath";
static char rfcmdpath[] = "/tmp/rfcmdpath"; 
static char panelcfpath[] = "/tmp/panelpath";

static char pitchdrivestatepath[] = "/tmp/pitch_state";
static char rolldrivestatepath[] = "/tmp/roll_state";

static char xactiveflag = 'R';
static char yactiveflag = 'P';

static char CMD_MEM_SEP = '$';

static uint8_t ROLLACTIVEMASK = 0x40; //01000000 64
static uint8_t PITCHACTIVEMASK = 0x20;//00100000 32

const int write_payload_size = 10;
const int read_payload_size = 11;

int resetrequested = 0;


//Maybe axes this to use one read pipe and have each axes figure out what to get. 
void broadcasttocontrollers(char broadcaststr[write_payload_size+1]){
  uint64_t currentwriteypipe = axispipes[0];

  radio.stopListening(); //Like what my SO did when I made the grave one-time mistake of saying 'calm down'
  
  char writepayload[write_payload_size];

  radio.openWritingPipe(currentwriteypipe);

  if(radio.write(broadcaststr, write_payload_size))
  {
    printf("WROTE %s \n", broadcaststr);
    usleep(10);
  } else {
    printf("FAILED TO TX %s", broadcaststr);
  }

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
  int rf_cmd_desc = open(rfcmdpath , O_RDONLY,O_NONBLOCK);
  char inbuffer[255];

  int bytes_read = read(rf_cmd_desc, inbuffer, 255);

  if( bytes_read > 0){
    printf("GOT %s\n",inbuffer); 

    int pos = 0;
    char cmdbuffer[10];
    char *buff_ptr;

    buff_ptr = inbuffer;
    while(true){
      if(*buff_ptr == CMD_MEM_SEP){
        cmdbuffer[pos] = '\0';
        broadcasttocontrollers(cmdbuffer);
        pos = -1;
      } else if(*buff_ptr == '\0'){
        cmdbuffer[pos] = '\0';
        broadcasttocontrollers(cmdbuffer);
        break;
      } else {
        cmdbuffer[pos] = *buff_ptr;
      }

      pos++;
      buff_ptr++;
    }
    close(rf_cmd_desc);
  }
  rf_cmd_desc = 0;
  bytes_read = 0;

}

void fetchandbroadcast(){
  readfromcmdmem();

  uint32_t broadcast = readfromsharedmem(rfcfpath, 1);
  if(broadcast != 0){
    char outbuffer[write_payload_size];

    uint16_t xcoord = (broadcast & 0x1FF);
    uint16_t ycoord = (broadcast & (0x1FF << 9)) >> 9;
    int resetrequested = (broadcast & (1 << 18)) >> 18; 
    
    sprintf(outbuffer, "P%03dR%03dS%d\0", xcoord, ycoord, resetrequested);

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

void write_drive_state(char state_str[], char write_path[]){
  ofstream outputFile;
  outputFile.open(write_path);

  outputFile << strtol(state_str, NULL, 10) << endl;

  outputFile.close();
}

void handleresponse(char response[], uint8_t pipeNumber){
  if(pipeNumber == PITCH_PIPE_INDEX){
    setgpioflags(0,1);
    write_drive_state(response, pitchdrivestatepath);
  }else if(pipeNumber == ROLL_PIPE_INDEX){
    setgpioflags(1,0);
    write_drive_state(response, rolldrivestatepath);
  }

  //At some point, write the response somewhere
  printf("Handled %s from %d", response, pipeNumber);
}

int main(int argc, char** argv) {

  // Print preamble:
  cout << "THE RADIO IS STARRRTING\n";

  // Setup and configure rf radio
  radio.begin();
  radio.enableDynamicPayloads();
  radio.setAutoAck(false);
  radio.setChannel(110);
  radio.setPALevel(RF24_PA_HIGH);

  radio.openReadingPipe(ROLL_PIPE_INDEX, rollrxaddr);
  radio.openReadingPipe(PITCH_PIPE_INDEX, pitchrxaddr);

  radio.printDetails();


  printf("\n ************ THEM DETAILS IS ABOVE ***********\n");
  

  radio.startListening();

// forever aloop
  while (1)
  {

    uint8_t pipeNum;
    while(radio.available(&pipeNum)){
      // Grab the response, compare, and send to debugging spew
      char response[read_payload_size+1];
      radio.read( response, read_payload_size );

      handleresponse(response, pipeNum);


      // Spew it
      printf("Got response size=%i value=%s\n from %d\r", read_payload_size, response, pipeNum);

      //Send the response msg to some sort of thing. 
    }

    //Check to see if there's a command in the shared mem
    //If there is one, parse it and broadcast to both axes
    fetchandbroadcast();

  }
}
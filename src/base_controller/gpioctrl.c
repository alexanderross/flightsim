#include <wiringPi.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PitchPin 4
#define RollPin 5
#define LinkEnPin 6
#define LinkActPin 1

#define ResetSwPin 2
#define LinkEnSwPin 0
#define DEBOUNCE 10000
#define ACTIVATERST 50000

static char * LINKACTIVETOKEN = "A!";
static char * ROLLACTIVETOKEN = "P!";
static char * PITCHACTIVETOKEN = "r!";
static char * ENABLEDTOKEN = "e!";

int linkenableddown = 0;
int resetdown = 0;
int activatetime = 0;
int pitchcommtime = 0;
int rollcommtime = 0;

int linkenabled = 0;
int buttondebounce = 0;

char * panelpathfifo = "/tmp/panelfifo";
char * serpathfifo = "/tmp/serpathfifo";

void writetofifo(char* path, char* msg){
  char arrrg[80];
  // Open FIFO for write only
  int fd = open(path, O_WRONLY|O_NONBLOCK);

  // Write the input arr2ing on FIFO
  // and close it
  write(fd, msg, strlen(msg)+1);
  close(fd);
}

void setlinkenabled(int state){
	digitalWrite(LinkEnPin, state);
	linkenabled = state;

	char enabledmsg[3] = {'D','!'};
	if(linkenabled){ enabledmsg[0] = 'E';}
	writetofifo(serpathfifo, enabledmsg);
}

void togglelinkenabled(void){
	setlinkenabled(!linkenabled);
	printf("TOGGLE LINK ENABLED %d \n", linkenabled);
	//Send into pipe to enable/disable
}


//For these, we set them on, and every time we get an active signal, we 'bump' the active time
//To keep the LED on. The main loop decrements these counts and when they're zero, the active led goes byebye
void showlinkactive(void){
	digitalWrite(LinkActPin, 1);
	activatetime = ACTIVATERST;
}

void showRollAxisUp(void){
	digitalWrite(RollPin, 1);
	rollcommtime = ACTIVATERST;
}

void showPitchAxisUp(void){
	digitalWrite(PitchPin, 1);
	pitchcommtime = ACTIVATERST;
}

void activatereset(void){
	//Send reset signal
	setlinkenabled(0);
	printf("ACTIVATE RESET \n");
	writetofifo(serpathfifo, "R!");
}

void checkpipestate(char* path){
	char arr1[2];
	// Incoming commands are 2 bytes
	// Open FIFO for Read only
	int fd = open(path, O_RDONLY|O_NONBLOCK);

	// Read from FIFO
	int res = read(fd, arr1, sizeof(arr1));
	if(res > 0){
			//Check for reset (reset sets enabled to false) else
			//Check for enabled signal
			// Print the read message
			printf("Read: %s\n", arr1);

			if(strcmp(arr1, LINKACTIVETOKEN)){showlinkactive();}
			if(strcmp(arr1, PITCHACTIVETOKEN)){showPitchAxisUp();}
			if(strcmp(arr1, ROLLACTIVETOKEN)){showRollAxisUp();}
			if(strcmp(arr1, ENABLEDTOKEN)){setlinkenabled(1);}
	}
	close(fd);
}

int main(void) {
		if(wiringPiSetup() == -1) { //when initialize wiringPi failed, print message to screen
			printf("setup wiringPi failed !\n");
			return -1;
		}
		
		// Creating the named file(FIFO)
		// mkfifo(<pathname>, <permission>)
		mkfifo(panelpathfifo, 0666);
		mkfifo(serpathfifo, 0666);


		int activeoutput[4] = {PitchPin, RollPin, LinkEnPin, LinkActPin};


		pinMode(ResetSwPin, INPUT);
		pinMode(LinkEnSwPin, INPUT);

		// Make sure leds are good.
		for(int i = 0; i < sizeof(activeoutput); i++){
			pinMode(activeoutput[i], OUTPUT);

			digitalWrite(activeoutput[i], HIGH);   //led on
			delay(100);                 // wait 1 sec
			digitalWrite(activeoutput[i], LOW);  //led off
			delay(100);
		}
		printf("CYCLED \n");
		int buttondebounce = 0;

		while(1) {
			checkpipestate(panelpathfifo);

			int resetvalue = digitalRead(ResetSwPin);
			int enablevalue = digitalRead(LinkEnSwPin);

			if(buttondebounce <= 0){
				if(resetvalue && resetdown == 0){
					activatereset();
					resetdown = 1;
					buttondebounce = DEBOUNCE;
				}else if(!resetvalue){
					resetdown = 0;
				}

				if(enablevalue && linkenableddown == 0){
					togglelinkenabled();
					linkenableddown = 1;
					buttondebounce = DEBOUNCE;
				}else if(!enablevalue){
					linkenableddown = 0;
				}
			}else{
				buttondebounce -= 1;
			}

			if(activatetime == 0){
				digitalWrite(LinkActPin, 0);
			}else{
				activatetime -= 1;
			}

			if(rollcommtime == 0){
				digitalWrite(RollPin, 0);
			}else{
				rollcommtime -= 1;
			}

			if(pitchcommtime == 0){
				digitalWrite(PitchPin, 0);
			}else{
				pitchcommtime -= 1;
			}

		}
	return 0;
}

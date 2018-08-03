#include <wiringPi.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PitchPin 4
#define RollPin 5
#define LinkEnPin 6
#define LinkActPin 1
#define ResetSwPin 3
#define LinkEnSwPin 0
#define DEBOUNCE 10000

int linkenableddown = 0;
int resetdown = 0;

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

	char enabledmsg[3] = {'E','0','!'};
	if(linkenabled){ enabledmsg[1] = '1';}
	writetofifo(serpathfifo, enabledmsg);
}

void togglelinkenabled(void){
	setlinkenabled(!linkenabled);
	printf("TOGGLE LINK ENABLED %d \n", linkenabled);
	//Send into pipe to enable/disable
}

void showlinkactive(void){
	if(fork() == 0){
		digitalWrite(LinkActPin, HIGH);
		sleep(1);
		digitalWrite(LinkActPin, LOW);
	}
}

void activatereset(void){
	//Send reset signal
	setlinkenabled(0);
	printf("ACTIVATE RESET \n");
	writetofifo(serpathfifo, "R!");
}

void checkpipestate(char* path){
	char arr1[80];
	// Open FIFO for Read only
	int fd = open(path, O_RDONLY|O_NONBLOCK);

	// Read from FIFO
	int res = read(fd, arr1, sizeof(arr1));
	if(res > 0){
			//Check for reset (reset sets enabled to false) else
			//Check for enabled signal
			// Print the read message
			printf("Read: %s\n", arr1);
			if(strcmp(arr1, "A!")){showlinkactive();}
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

		}
	return 0;
}

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
#include <mqueue.h>

#define PitchPin 4
#define RollPin 5
#define LinkEnPin 6
#define LinkActPin 1

#define ResetSwPin 2
#define LinkEnSwPin 0
#define DEBOUNCE 10000
#define ACTIVATERST 50000

#define MAX_MSG_SIZE 4


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

char * panelpathmq = "/panelmq";
char * serpathmq = "/serpathmq";

void writetomq(char* path, char* msg){
  mqd_t mq = mq_open(path, O_WRONLY);

  mq_send(mq, msg, MAX_SIZE, 0));
  close(fd);
}

mqd_t getpanelmq(void){
  struct mq_attr attr;
  char buffer[MAX_SIZE + 1];
  int must_stop = 0;

  /* initialize the queue attributes */
  attr.mq_flags = 0;
  attr.mq_maxmsg = 25;
  attr.mq_msgsize = MAX_MSG_SIZE;
  attr.mq_curmsgs = 0;

  /* create the message queue */
  return mq_open(panelpathmq, O_CREAT | O_RDONLY, 0644, &attr);
}

void setlinkenabled(int state){
	digitalWrite(LinkEnPin, state);
	linkenabled = state;

	char enabledmsg[3] = {'D','!'};
	if(linkenabled){ enabledmsg[0] = 'E';}
	writetomq(serpathfifo, enabledmsg);
}

void togglelinkenabled(void){
	setlinkenabled(!linkenabled);
	printf("TOGGLE LINK ENABLED %d \n", linkenabled);
	//Send into pipe to enable/disable
}


//For these, we set them on, and every time we get an active signal, we 'bump' the active time
//To keep the LED on. The main loop decrements these counts and when they're zero, the active led goes byebye
void showlinkactive(void){
	printf("SHOW LINK ACT \n");
	digitalWrite(LinkActPin, 1);
	activatetime = ACTIVATERST;
}

void showRollAxisUp(void){
	printf("SHOW AXIS UP \n");
	digitalWrite(RollPin, 1);
	rollcommtime = ACTIVATERST;
}

void showPitchAxisUp(void){
	printf("SHOW PITCH UP \n");
	digitalWrite(PitchPin, 1);
	pitchcommtime = ACTIVATERST;
}

void activatereset(void){
	//Send reset signal
	setlinkenabled(0);
	printf("ACTIVATE RESET \n");
	writetofifo(serpathfifo, "R!");
}

void checkmqtate(char* path){
	char buffer[MAX_MSG_SIZE];
	// Incoming commands are 2 bytes
	mqd_t mq = getpanelmq();

	// Read from mq while we have stuff
	while(1){
		ssize_t bytes_read = mq_receive(mq, buffer, MAX_MSG_SIZE, NULL);

		if(bytes_read > 0){
				//Check for reset (reset sets enabled to false) else
				//Check for enabled signal
				// Print the read message
				printf("Read: %s\n", buffer);

				if(strcmp(buffer, LINKACTIVETOKEN) == 0){showlinkactive();}
				if(strcmp(buffer, PITCHACTIVETOKEN) == 0){showPitchAxisUp();}
				if(strcmp(buffer, ROLLACTIVETOKEN) == 0){showRollAxisUp();}
				if(strcmp(buffer, ENABLEDTOKEN) == 0){setlinkenabled(1);}
		}else{
			break;
		}
	}
	mq_close(mq);
}

int main(int argc, char *argv[]) {

		if(wiringPiSetup() == -1) { //when initialize wiringPi failed, print message to screen
			printf("setup wiringPi failed !\n");
			return -1;
		}

		int activeoutput[4] = {PitchPin, RollPin, LinkEnPin, LinkActPin};

		pinMode(ResetSwPin, INPUT);
		pinMode(LinkEnSwPin, INPUT);

		// Make sure leds are good.
		for(int v = 0; v < 3; v++){
			for(int i = 0; i < 4; i++){
				pinMode(activeoutput[i], OUTPUT);

				digitalWrite(activeoutput[i], HIGH);   //led on
				delay(50);                 // wait 1 sec
				digitalWrite(activeoutput[i], LOW);  //led off
				delay(50);
			}
		}
		printf("STARTING WHILE LOOPY \n");
		int buttondebounce = 0;

		while(1) {
			checkmqstate(panelpathfifo);

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

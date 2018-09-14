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
#include <stdint.h>

#define PitchPin 4
#define RollPin 5
#define LinkEnPin 6
#define LinkActPin 1

#define ResetSwPin 2
#define LinkEnSwPin 0
#define DEBOUNCE 40000
#define ACTIVATERST 50000


static uint8_t LINKACTIVEMASK = 0x80; //10000000 128
static uint8_t ROLLACTIVEMASK = 0x40; //01000000 64
static uint8_t PITCHACTIVEMASK = 0x20;//00100000 32
static uint8_t ENABLEDMASK = 0x10;    //00010000 16
static uint8_t DISABLEDMASK = 0x08;    //00001000 8
static uint8_t SERENABLEMASK = 0x80;
static uint8_t SERDISABLEMASK = 0x40;
static uint8_t SERRESETMASK = 0x20;

int linkenableddown = 0;
int resetdown = 0;
int activatetime = 0;
int pitchcommtime = 0;
int rollcommtime = 0;

uint8_t serwritetmp = 0;

int linkenabled = 0;
int buttondebounce = 0;

char * panelcfpath = "/tmp/panelpath";
char * sercfpath = "/tmp/serpath";

void queueforserialwrite(uint8_t mask){
	serwritetmp = serwritetmp | mask;
	printf("Queued write for %d, results in %d\n",mask, serwritetmp);
}

void committoserial(){
	FILE *serfile;
	serfile = fopen(sercfpath,"w+");

  uint8_t inint;
  fscanf(serfile, "%d", inint);
  printf("FILE CURRENTLY CONTAINS %d", inint);
  inint = inint | serwritetmp;
  printf("WRITING %d to serial resulting in final %d \n", serwritetmp, inint);
  rewind(serfile);
  fprintf(serfile, "%d", inint);
  fflush(serfile);
  if(serfile != NULL){ fclose(serfile); }

  serwritetmp = 0;

}

void setlinkenabled(int state){
	digitalWrite(LinkEnPin, state);
	linkenabled = state;

	if(linkenabled){ 
		queueforserialwrite(SERENABLEMASK);
	}else{
		queueforserialwrite(SERDISABLEMASK);
	}
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
	activatetime = ACTIVATERST*2;
}

void showRollAxisUp(void){
	printf("SHOW ROLL AXIS UP \n");
	digitalWrite(RollPin, 1);
	rollcommtime = ACTIVATERST;
}

void showPitchAxisUp(void){
	printf("SHOW PITCH AXIS UP \n");
	digitalWrite(PitchPin, 1);
	pitchcommtime = ACTIVATERST;
}

void activatereset(void){
	//Send reset signal
	setlinkenabled(0);
	printf("ACTIVATE RESET \n");
	queueforserialwrite(SERRESETMASK);
}

void checkipcstate(){
	FILE* file;
	file = fopen(panelcfpath, "r");

	if(file == NULL){
		//Nothin new
		return;
	}else{
    uint8_t inint;
    usleep(20);
    fscanf(file, "%d", &inint);
    printf("READ '%d' \n", inint);

    printf("LINK ACTIVE IS %d \n", LINKACTIVEMASK & inint);

    if((inint & LINKACTIVEMASK) > 0){showlinkactive();};
    if((inint & PITCHACTIVEMASK) > 0){showPitchAxisUp();}
    if((inint & ROLLACTIVEMASK) > 0){showRollAxisUp();}
    if((inint & ENABLEDMASK) > 0){setlinkenabled(1);}
    if((inint & DISABLEDMASK) > 0){setlinkenabled(0);}

    if(file != NULL){
      fclose(file);
    }
    remove(panelcfpath);
	}
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
			checkipcstate();

			if(serwritetmp > 0){
				committoserial();
			}

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

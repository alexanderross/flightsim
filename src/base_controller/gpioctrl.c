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
#include <sys/ipc.h> 
#include <sys/shm.h>
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

    uint32_t returnval;

    key = ftok(path, 65);

    if ((shmid = shmget(key, 16, IPC_CREAT | 0666)) < 0) {
        perror("shmget error");
    }

    if ((shm = (uint32_t *) shmat(shmid, NULL, 0)) == (uint32_t *) -1) {
        perror("shmat error");
    }

    s = shm;

    returnval = *s;

    if(do_clear){
        *s = 0;
    }

    shmdt(shm);

    return returnval;
}

void committoserial(){
	writetosharedmem(sercfpath, serwritetmp, 0);
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
	uint32_t inint = readfromsharedmem(panelcfpath,1);

  if((inint & LINKACTIVEMASK) > 0){showlinkactive();};
  if((inint & PITCHACTIVEMASK) > 0){showPitchAxisUp();}
  if((inint & ROLLACTIVEMASK) > 0){showRollAxisUp();}
  if((inint & ENABLEDMASK) > 0){setlinkenabled(1);}
  if((inint & DISABLEDMASK) > 0){setlinkenabled(0);}

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

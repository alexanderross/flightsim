#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h> 
#include <sys/stat.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>


uint32_t writetosharedmem(char * path, uint32_t contents, int do_overwrite){
  int shmid;
  key_t key;
  uint32_t *shm, *s;

  uint32_t result;

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

  result = *s;

  shmdt(shm);

  return result;

}

int main( int argc, char *argv[] )
{
	uint32_t inInt = 0;

	int force_overwrite = 0;

	if(argc >= 3){
		sscanf(argv[2],"%d",&inInt);

		printf("Writing %d into shmem\n", inInt);
	}

	if(argc >= 4){
		force_overwrite = 1;
		printf("Forcing Write\n");
	}


	printf("%s contains %d\n", argv[1], writetosharedmem(argv[1], inInt, force_overwrite));


}
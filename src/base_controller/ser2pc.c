#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>



#define BUFFERLENGTH 100
#define PORTNAME "/dev/ttyS0"

const char STOP_CHAR = '>';
const char START_CHAR = '<';
const char MSG_DELIM = '!';

_Bool linkenabled = 1;
_Bool resetrequested = 0;

/* ser2pc
*  This takes the input from flight sim and translates into common messages to send to the socket that is 
*  shared by all 3 of the base controller's duties/scripts.
*
*/

int set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

_Bool is_reserved_char(char bchar){
  return (bchar == START_CHAR || bchar == STOP_CHAR || bchar == MSG_DELIM);
}

void set_mincount(int fd, int mcount)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error tcgetattr: %s\n", strerror(errno));
        return;
    }

    tty.c_cc[VMIN] = mcount ? 1 : 0;
    tty.c_cc[VTIME] = 5;        /* half second timer */

    if (tcsetattr(fd, TCSANOW, &tty) < 0)
        printf("Error tcsetattr: %s\n", strerror(errno));
}

void writetofifo(char* path, char* msg){
    char arrrg[80];
    // Open FIFO for write only
    int fd = open(path, O_WRONLY|O_NONBLOCK);

    // Write the input arr2ing on FIFO
    // and close it
    write(fd, arrrg, strlen(arrrg)+1);
    close(fd);
}

_Bool checklinkenabled(char* path){
    char arr1[80];
    // Open FIFO for Read only
    int fd = open(path, O_RDONLY);

    // Read from FIFO
    int res = read(fd, arr1, sizeof(arr1));
    if(res > 0){
        //Check for reset (reset sets enabled to false) else
        //Check for enabled signal
        // Print the read message
        printf("Read: %s\n", arr1);
    }
    close(fd);
    return linkenabled;
}


int main()
{

    char *portname = PORTNAME;
    int fd;
    int wlen;

    char * panelpathfifo = "/tmp/panelfifo";
    char * rfpathfifo = "/tmp/rffifo";
 
    // Creating the named file(FIFO)
    // mkfifo(<pathname>, <permission>)
    mkfifo(panelpathfifo, 0666);
    mkfifo(rfpathfifo, 0666);

    printf("STARTING");
    fd = open(portname, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        printf("Error opening %s: %s\n", portname, strerror(errno));
        return -1;
    }
    /*baudrate 115200, 8 bits, no parity, 1 stop bit (8N1) */
    set_interface_attribs(fd, B115200);
    //set_mincount(fd, 0);                /* set to pure timed read */

    /* simple output */
    wlen = write(fd, "I'm ONLINE!\n", 7);
    if (wlen != 7) {
      printf("Error from write: %d, %d\n", wlen, errno);
    }
    tcdrain(fd);    /* delay for output */

    do {
        size_t buf_idx = 0;
        char buf[BUFFLEN] = { 0 };

        //Loop reading single bytes into the buffer until we hit a significant character
        while (buf_idx < BUFFLEN && linkenabled){
            int res = read(fd, &buf[buf_idx], 1);

            if(res > 0){
                buf_idx++;
            }

            if (buf_idx > 0 && is_reserved_char(buf[buf_idx - 1])){
                break;
            }
        }  

        if (buf_idx > 0) {
            char tmp[BUFFLEN];
            strcpy(tmp,buf);
            tmp[buf_idx] = 0;

            linkenabled = checklinkenabled(panelpathfifo);

            if(resetrequested){
                //send reset command to axes
                writetofifo(rfpathfifo, "*R!");
                
                //reset resetrequested flag
                resetrequested = 0;
            }else if(linkenabled){
              // Send link active to panel
              writetofifo(panelpathfifo, "A!");
              // Forward command to 2.4
              writetofifo(rfpathfifo, tmp);
            }
            printf("Read %d: \"%s\"\n", buf_idx, tmp);
        } else if (buf_idx < -1) {
          //This will happen so let's juuuuuust ignore it.
        }  
    } while (1);
}

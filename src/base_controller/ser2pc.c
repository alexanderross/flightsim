#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>



#define BUFFERLENGTH 100
#define PORTNAME "/dev/ttyS0"

const char STOP_CHAR = '>';
const char START_CHAR = '<';
const char MSG_DELIM = '!';
const char COORD_SEPARATOR = '|';

int linkenabled = 1;
int resetrequested = 0;
int encheckbuff = 0;

static char panelcfpath[] = "/tmp/panelpath";
static char sercfpath[] = "/tmp/serpath";
static char rfcfpath[] = "/tmp/rfpath";

static int CHECK_INTERVAL_CYCLES = 1000;

static uint8_t SERENABLEMASK = 0x80;
static uint8_t SERDISABLEMASK = 0x40;
static uint8_t SERRESETMASK = 0x20;

static uint8_t PANELLINKACTIVEMASK = 0x80;

static int RF_RESET_SHIFT= 18;

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

int is_reserved_char(char bchar){
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

void sendtorf(uint8_t mask){
    FILE *fd = fopen(rfcfpath, "w+");
    fprintf(fd, "%d", mask);
    // Write the input arr2ing on FIFO
    // and close it
    //write(fd, arrrg, strlen(arrrg)+1);
    printf("SENT %d TO RF\n", mask);
    fclose(fd);
}

void sendtopanel(uint8_t mask){
    FILE *fd = fopen(panelcfpath, "w+");

    // Write the input arr2ing on FIFO
    // and close it
   fprintf(fd, "%d", mask);
   fclose(fd);
}

void sendresetsignal(){
    printf("PROCESSED RESET\n");
    linkenabled = 0;
    resetrequested = 1;

    sendtorf((1 << RF_RESET_SHIFT));    
}

void sendcoordstorf(int xcoord, int ycoord){
    uint32_t masky = 0;

    masky = masky | xcoord | (ycoord << 9) | (resetrequested << RF_RESET_SHIFT);

    sendtorf(masky);
}

void process_command(char* coord_str){
    //So we're going to reserve 9-bit slots for the coordinate system. 
    // 0-8 is X 9-17 is Y (stores position value of 0-359 representing desired position)
    // 31 (0x100) is an indicator bit representing if a reset signal should be sent. (returns RF components to mechanical 0 positions)

    // the commands come in like '<xaxis|yaxis>' This method needs to parse these, convert into integers, then creates a mask by shifting the y coord 9 bits.

    // reset requested is permanently written into this mask and the comm file being removed is the indication to return that value to 0. (RF will remove the file once it acks the reset).
    // As a reset disables the link, this shouldn't be the primary means of reset (sendresetsignal does it explicitly), but it's a failsafe.
    int x_coord, y_coord;
    int buf_position = 0;
    char xbuf[3], ybuf[3];

    char *current_buf;  

    for(int swag = 0 ; swag < sizeof(coord_str); swag++){
        if(coord_str[swag] == START_CHAR){
            current_buf = xbuf;
        }else if(coord_str[swag] == STOP_CHAR){
            printf("COORD READ X(%d) - Y(%d)\n", atoi(xbuf), atoi(ybuf));
            sendcoordstorf(atoi(xbuf), atoi(ybuf));
            buf_position = 0;
        }else if(coord_str[swag] == COORD_SEPARATOR){
            current_buf = ybuf;
        }else{
            *(current_buf + buf_position) = coord_str[swag];
            buf_position++;
        }
    }
}

void checklinkenabled(){
    FILE* file;
    file = fopen(sercfpath, "r");

    if(file == NULL){
        //Nothin new
        return;
    }else{
        uint8_t inint;
        usleep(20);
        fscanf(file, "%d", &inint);
        printf("READ '%d' \n", inint);

        if((inint & SERENABLEMASK) > 0){linkenabled = 1;};
        if((inint & SERDISABLEMASK) > 0){linkenabled = 0;}
        if((inint & SERRESETMASK) > 0){sendresetsignal();}

        fflush(file);
        if(file != NULL){
          fclose(file);
        }

        remove(sercfpath);
    }
}


int main()
{

    int fd;
    int wlen;


    printf("STARTING \n");
    fd = open(PORTNAME, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        printf("Error opening %s: %s\n", PORTNAME, strerror(errno));
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

    printf("STARTED \n");
    do {
        size_t buf_idx = 0;
        char buf[BUFFERLENGTH] = { 0 };

        //Loop reading single bytes into the buffer until we hit a significant character
        while (buf_idx < BUFFERLENGTH && linkenabled){
            int res = read(fd, &buf[buf_idx], 1);

            if(res > 0){
                buf_idx++;
            }

            if (buf_idx > 0 && is_reserved_char(buf[buf_idx - 1])){
                break;
            }

            if(encheckbuff <= 0){
                checklinkenabled();
                encheckbuff = CHECK_INTERVAL_CYCLES;
            }else{
                encheckbuff--;
            }
        }  

        if (buf_idx > 0) {
            char tmp[BUFFERLENGTH];
            strcpy(tmp,buf);
            tmp[buf_idx] = 0;

            if(linkenabled){
              // Send link active to panel
              sendtopanel(PANELLINKACTIVEMASK);
              // Forward command to 2.4
              process_command(tmp);
            }
            printf("Read %d: \"%s\"\n", buf_idx, tmp);
        } else if (buf_idx < -1) {
          //This will happen so let's juuuuuust ignore it.
        }
        if(encheckbuff <= 0){
            checklinkenabled();
            encheckbuff = CHECK_INTERVAL_CYCLES;
        }else{
            encheckbuff--;
        }
    } while (1);
}

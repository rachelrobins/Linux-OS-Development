#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include "message_slot.h"

int main(int argc, char *argv[]) {
    char buff[BUF_LEN];
    char *filePath;
    unsigned long  channelid;
    char *ptr;
    int fileDesc;
    int readbytes ;
    if (argc != 3) {
        exit(1);
    }
    filePath = argv[1];
    fileDesc = open(filePath, O_RDONLY);
    if (fileDesc < 0) {
        perror("open failed. \n");
        exit(1);
    }
    channelid = strtoul(argv[2],&ptr,10);
    if (ioctl(fileDesc, MSG_SLOT_CHANNEL, channelid)<0)
    {
        perror( "ioctl  failed. \n");


        exit(1);
    }

    readbytes = read(fileDesc, buff, BUF_LEN);

    if (readbytes < 0) {

        perror("read failed. \n");

        exit(1);
    } else if (readbytes < BUF_LEN) {
        buff[readbytes] = '\0';
    }

    close(fileDesc);
    if(write(STDOUT_FILENO,buff,readbytes)<0){
        perror("printing  failed. \n");
        exit(1);
    }
    exit(0);


}



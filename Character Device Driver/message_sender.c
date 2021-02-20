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
    char *filePath;
    char *ptr;
    int written;
    unsigned long channelid;
    int fileDesc;
    if (argc != 4) {
        exit(1);
    }
    filePath = argv[1];
    fileDesc = open(filePath, O_RDWR);
    if (fileDesc < 0) {
        perror( "open " "failed. \n");
        exit(1);
    }
    channelid = strtoul(argv[2],&ptr,10);
    if (ioctl(fileDesc, MSG_SLOT_CHANNEL, channelid)<0)
    {
        perror("ioctl  failed. \n");

        exit(1);
    }

    written = write(fileDesc,argv[3],strlen(argv[3]));

    if (written < 0) {

        perror("write failed. \n");

        exit(1);
    }
    close(fileDesc);
    exit(0);

}


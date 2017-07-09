#include "message_slot.h"    

/* ***** Example w/ minimal error handling - for ease of reading ***** */

#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main(int argc, char const *argv[])
{
    int file_desc, ret_val,ch_num;
    if(argc != 3){
        printf("Invalid parameters.\n");
        printf("Please enter 1st arg : number of ch to write in\n");
        printf("Please enter 2nd arg : message to write \n")
    }
    ch_num = atoi(argv[1]);
    if (ch_num < 0 || ch_num > 3)
    {
        printf("the input %s is invalid pls enter 0 or 1 or 2 or 3\n",argv[1] );
        exit(-1);
    }

    file_desc = open("/dev/"DEVICE_FILE_NAME, O_RDWR);


    if (file_desc < 0) {
        printf ("Can't open device file: %s errno %s\n", DEVICE_FILE_NAME,strerror(errno));
        exit(-1);
    }


    ret_val = ioctl(file_desc, IOCTL_SET_CH, ch_num);

    if (ret_val < 0) {
         printf ("ioctl_set_channel failed:%d\n", ret_val);
         exit(-1);
    }

    /////read /// 
    ret_val = write(file_desc,argv[2],strlen(argv[2]));

    if (ret_val < 0) {
         printf ("write to file failed: %d. %s\n", ret_val,strerror(errno));
         exit(-1);
    }

    close(file_desc); 
    printf("~~~~~~~~status message~~~~~~~\n");
    printf("the message : %s\n was write into : %d slot",argv[2],ch_num);
    printf("the length that wrote is : %d\n",ret_val );
    printf("~~~~end status message~~~~~~~\n");
    return 0;
}

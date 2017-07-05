#include "message_slot.h"    

/* ***** Example w/ minimal error handling - for ease of reading ***** */

#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char const *argv[])
{
    int file_desc, ret_val,ch_num;

    file_desc = open("/dev/"DEVICE_FILE_NAME, 0);
    if (file_desc < 0) {
        printf ("Can't open device file: %s\n", DEVICE_FILE_NAME);
        exit(-1);
    }
    ch_num = atoi(argv[1]);
    if (ch_num < -1 || ch_num > 4)
    {
        printf("the input %s is invalid pls enter 0 or 1 or 2 or 3\n",argv[1] );
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
         printf ("write to file failed:%d\n", ret_val);
         exit(-1);
    }

    close(file_desc); 
    printf("~~~~status message~~~~~~~\n");
    printf("the message : %s\n was write into : %d slot",argv[2],ch_num);
    printf("the length that wrote is : %d\n",ret_val );
    return 0;
}

#include "message_slot.h"    

#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFF_SIZE 128

int main(int argc, char const *argv[])
{
    int file_desc, ret_val,ch_num;
    char buff [BUFF_SIZE + 1];
    if(argc != 3){
        printf("Invalid parameters.\n");
        printf("Please enter 1st arg : number of ch to read from\n");
        return 0;
    }
    ch_num = atoi(argv[1]);
    if (ch_num < 0 || ch_num > 3)
    {
        printf("the input %s is invalid pls enter 0 or 1 or 2 or 3\n",argv[1] );        
        exit(-1);
    }

    file_desc = open(argv[2], O_RDWR);
    if (file_desc < 0) {
        printf ("Can't open device file: %s\n", DEVICE_FILE_NAME);
        exit(-1);
    }


    ret_val = ioctl(file_desc, IOCTL_SET_CH, ch_num);

    if (ret_val < 0) {
        printf ("ioctl_set_channel failed:%d\n", ret_val);
        close(file_desc);
        exit(-1);
    }

    /////read /// 
    ret_val = read(file_desc,buff,BUFF_SIZE);
    if (ret_val < 0) {
        printf ("read from file failed:%d\n", ret_val);
        close(file_desc);
        exit(-1);
    }
    buff[ret_val] = '\0';

    if (close(file_desc) < 0) {
        printf ("close file failed:%d\n", ret_val);
        close(file_desc);
        exit(-1);
    }

    printf("~~~~~~~~status message reader~~~~~~~\n");
    printf("the message : %s\n was read fron : %d slot",buff,ch_num);
    printf("the length that read is : %d\n",ret_val );
    printf("~~~~end status message reader~~~~~~~\n");
    return 0;
}

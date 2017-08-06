#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#define RANDOM_FILE "/dev/urandom"
// #define RANDOM_FILE "15A.txt"
#define BUFF_SIZE 1024
#define PORT_NUM 2233
#define MY_IP "127.0.0.1"
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))


//////////////////////////////////////////////////////////////
int open_random_file()
{
    int fdInput = open(RANDOM_FILE, O_RDONLY);
    if (fdInput < 0) {
        printf("Error opening Input file: %s\n", strerror( errno));
        return -1;
    }
    return fdInput;
}

//////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Not suitable arguments.\n");
        return -1;
    }
    
    int LEN = atoi(argv[1]); //TODO check if a number
    int fdInput  = open_random_file();
    if (0 > fdInput)
    {
        return -1;
    }
    int  sockfd     = -1;
    int  bytes_read =  0;
    char recv_buff[BUFF_SIZE];

    struct sockaddr_in serv_addr;
    struct sockaddr_in my_addr;
    struct sockaddr_in peer_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in );

    memset(recv_buff, '\0', sizeof(recv_buff));
    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Error : Could not create socket \n");
        return 1;
    }

    // print socket details
    // if(0 > getsockname(sockfd,(struct sockaddr*) &my_addr,&addrsize)){
    //     printf("Error : Could not get socket name. %s \n", strerror(errno));
    //     return 1;
    // }
    // printf("Client: socket created %s:%d\n",
    //      inet_ntoa((my_addr.sin_addr)),
    //      ntohs(my_addr.sin_port));

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_port        = htons(PORT_NUM); // Note: htons for endiannes
    serv_addr.sin_addr.s_addr = inet_addr(MY_IP); // hardcoded...

    // printf("Client: connecting...\n");
    // Note: what about the client port number?
    // connect socket to the target address
    if( connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0)
    {
        printf("\n Error : Connect Failed. %s \n", strerror(errno));
        return 1;
    }

    // print socket details again
    // if(0 > getsockname(sockfd, (struct sockaddr*) &my_addr,   &addrsize))
    // {
    //     printf("Error : Could not get socket name. %s \n", strerror(errno));
    //     return 1;
    // }
    // if(0 > getpeername(sockfd, (struct sockaddr*) &peer_addr, &addrsize))
    // {
    //     printf("Error : Could not get socket name. %s \n", strerror(errno));
    //     return 1;
    // }

    // printf("Client: Connected. \n"
    //      "\t\tSource IP: %s Source Port: %d\n"
    //      "\t\tTarget IP: %s Target Port: %d\n",
    //      inet_ntoa((my_addr.sin_addr)),    ntohs(my_addr.sin_port),
    //      inet_ntoa((peer_addr.sin_addr)),  ntohs(peer_addr.sin_port));


    ssize_t lenWR;
    char str_LEN[20]={0};
    int i = 0;

    sprintf(str_LEN,"%d",LEN);
    for (i = 0; i < 20; i+=lenWR){
        lenWR = write(sockfd, &(str_LEN[i]),20 - i);
        if (0 > lenWR) {
            printf("Error write to socket file: %s\n", strerror(errno));
            close(sockfd);
            return -1;
        }
    }

    ssize_t lenRD;
    char buffer[BUFF_SIZE];
    int j;
    for (i = 0; i < LEN; i+=lenRD)
    {
        //printf(" iteration num %d\n",i );
        lenRD = read(fdInput, buffer ,MIN(BUFF_SIZE,(LEN - i)));
        if (0 > lenRD) 
        {
            printf("Error reading from random file: %s\n", strerror(errno));
            close(sockfd);
            return -1;
        }
        for (j = 0; j < lenRD; j+=lenWR)
        {
            lenWR = write(sockfd, &(buffer[j]),lenRD - j);
            if (0 > lenWR) {
                printf("Error write to socket file: %s\n", strerror(errno));
                close(sockfd);
                return -1;
            }
        }
        //printf("iteration num %d\n",i );
    }
    close(fdInput);
    // printf("sizeof('') === %ld\n", sizeof('\0'));
    // char end = -1;
    // printf("EOF is %d\n",EOF );
    // printf("end is %c\n",end );
    // if (0 > write(sockfd,&end,4)) {
    // if (0 > write(sockfd,&end,1)) {
    //     printf("In Line %d\n",__LINE__ );
    //     printf("Error write to socket file: %s\n", strerror(errno));
    //     return -1;
    // }


    // read data from server into recv_buff
    // block until there's something to read
    // print data to screen every time
    int already_read = 0;
    while( 1 )
    {
        //printf("RKKKK1\n");
        bytes_read = read(sockfd,
                          &(recv_buff[already_read]),
                          sizeof(recv_buff) - 1);
        already_read +=bytes_read;
        if (bytes_read < 0)
        {
            printf("Error reading from socket file: %s\n", strerror(errno));
            close(sockfd);
            return -1;
        }
        if( bytes_read == 0 )
            break;
    }
    recv_buff[already_read] = '\0';
    printf("num of letters that read by the server is : %s\n",recv_buff );

    close(sockfd); // is socket really done here? yes
    //printf("Write after close returns %d\n", write(sockfd, recvBuff, 1));
    return 0;
    }



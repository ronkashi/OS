#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>

//#define NUM_THREADS  5//TODO maybe its not bounded
#define BUFF_SIZE 1024
#define PORT_NUM 2233
#define MY_IP "127.0.0.1"
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define QUEUE_LEN 10
#define NUM_OF_PRINTABLE_CHARS 95

/////////////////////////global vars//////////////////////////////
int NUM_OF_THREADS_OPENED = 0;
int GLOBAL_BYTES_READ = 0;
int GLOBAL_PRINTABLE_CHARS[NUM_OF_PRINTABLE_CHARS] = {0};
int GLOBAL_PROCESS_OPENED = 0;
int GLOBAL_PROCESS_FINISHED_SUCCESFULLY = 0;
int GLOBAL_PROCESS_FINISHED_FAIL = 0;
int listenfd  = -1;
pthread_mutex_t  lock;
// pthread_t* threads;
//////////////////////////////////////////////////////////////////
void my_signal_handler( int signum, siginfo_t* info,void* ptr)
{
    printf("Signal sent from process %lu\n",(unsigned long) info->si_pid);

    //TODO should be join to all threads here
    //TODO what is the bound to num of threads
    // int t;
    int rc;
    // void* status;
    // for( t = 0; t < GLOBAL_PROCESS_OPENED; ++t ) 
    // {
    //     rc = pthread_join(threads[t], &status);
    //     if (rc) 
    //     {
    //         printf("ERROR in pthread_join(): %s\n", strerror(rc));
    //         exit( -1 );
    //     }
    // }   
    close(listenfd);
    //sleep (3);
    // free(threads);
    while (GLOBAL_PROCESS_FINISHED_SUCCESFULLY + GLOBAL_PROCESS_FINISHED_FAIL < GLOBAL_PROCESS_OPENED){
        sleep(1);
    }
    if (GLOBAL_PROCESS_FINISHED_SUCCESFULLY < GLOBAL_PROCESS_OPENED)
    {
        printf("\nAn error been occured!!\n");
    }
    else
    {
        int i = 0;
        printf("The number of bytes that read by the server is : %d\n",GLOBAL_BYTES_READ );
        printf("we saw \n");
        for (i = 0; i < NUM_OF_PRINTABLE_CHARS; ++i)
        {
            printf("%d '%c's,\t",GLOBAL_PRINTABLE_CHARS[i],(i+32) );
        }
        printf("\n");
    }
    
    pthread_mutex_destroy(&lock);
    exit(1);
}
//////////////////////////////////////////////////////////////////
////TODO maybe return value should be other////
void* counter(void *t)
{
    int fd = *((int *) t);
    printf("my fd is : %d\n", fd);
    char buffer[BUFF_SIZE];
    int local_printable_chars[NUM_OF_PRINTABLE_CHARS] = {0};
    int local_bytes_read = 0;
    int numPrintable = 0;
    int rc;
    int lenRD;
    int i = 0;

    for (i = 0; i < 20; i+=lenRD){
        lenRD = read(fd, &(buffer[i]),20 - i);
        if (0 > lenRD) {
            printf("In Line %d\n",__LINE__ );
            printf("Error reading from socket file: %s\n", strerror(errno));
            __sync_fetch_and_add(&GLOBAL_PROCESS_FINISHED_FAIL, 1);
            return NULL;
        }
    }
    int LEN = atoi(buffer);
    printf("the len recived : %d\n",LEN );
    while (local_bytes_read < LEN)
    {
        lenRD = read(fd, buffer ,BUFF_SIZE);
        if (lenRD < 0)
        {
            printf("In Line %d\n",__LINE__ );
            printf("Error in server reading from socket file: %s\n", strerror(errno));
            __sync_fetch_and_add(&GLOBAL_PROCESS_FINISHED_FAIL, 1);
            return NULL;
        }
        if (lenRD == 0)
        {
            break;
        }
        for (i = 0; i < lenRD; ++i)
        {
            if (buffer[i] > 31 && buffer[i] < 127){
                local_printable_chars[buffer[i]-32]++;
                numPrintable++;
            }
        }
        local_bytes_read +=lenRD;
    }
    printf("local_bytes_read : %d\n",local_bytes_read );
    sprintf(buffer,"%d",numPrintable);
    ssize_t lenWR;
    for (i = 0; i < strlen(buffer); i+=lenWR)
    {
        lenWR = write(fd,&(buffer[i]),(strlen(buffer) -i));
        if (0 > lenWR) {
            printf("In Line %d\n",__LINE__ );
            printf("Error write to socket file: %s\n", strerror(errno));
            __sync_fetch_and_add(&GLOBAL_PROCESS_FINISHED_FAIL, 1);
            return NULL;
        }
    }
    //TODO close connection
    free(t);
    close(fd);

    rc = pthread_mutex_lock(&lock);
    if( 0 != rc ) 
    {
        printf("In Line %d\n",__LINE__ );
        printf( "ERROR in pthread_mutex_lock(): %s\n", strerror( rc ) );
        __sync_fetch_and_add(&GLOBAL_PROCESS_FINISHED_FAIL, 1);
        return NULL;
    }
    GLOBAL_BYTES_READ +=(local_bytes_read);
    for (i = 0; i < NUM_OF_PRINTABLE_CHARS; i++)
    {
        GLOBAL_PRINTABLE_CHARS[i] += local_printable_chars[i];
    }
    
    rc = pthread_mutex_unlock(&lock);
    if( 0 != rc ) 
    {
        printf("In Line %d\n",__LINE__ );
        printf( "ERROR in pthread_mutex_unlock(): %s\n", strerror( rc ) );
        __sync_fetch_and_add(&GLOBAL_PROCESS_FINISHED_FAIL, 1);
        return NULL;
    }
    //TODO check if the pthread_exit
    __sync_fetch_and_add(&GLOBAL_PROCESS_FINISHED_SUCCESFULLY, 1);

    pthread_exit((void *)0);
}
//////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{

    int* connfd    = NULL;
    struct sockaddr_in serv_addr;
    char data_buff[BUFF_SIZE + 1];

    int rc = pthread_mutex_init( &lock, NULL );
    if( rc ) 
    {
        printf("ERROR in pthread_mutex_init(): %s\n", strerror(rc));
        return -1;
    }

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset( &serv_addr, '0', sizeof(serv_addr));
    memset( data_buff, '0', sizeof(data_buff));

    serv_addr.sin_family = AF_INET;
    // INADDR_ANY is any local machine address
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT_NUM);

    if( 0 != bind( listenfd,
                (struct sockaddr*) &serv_addr,
                sizeof(serv_addr)))
    {
        printf("Error : Bind Failed. %s \n", strerror(errno));
        return -1;
    }

    if( 0 != listen(listenfd, QUEUE_LEN) )
    {
        printf("Error : Listen Failed. %s \n", strerror(errno));
        return -1;
    }
    // Structure to pass to the registration syscall
    struct sigaction new_action;
    memset(&new_action, 0, sizeof(new_action));
    // Assign pointer to our handler function
    new_action.sa_handler = (void *)my_signal_handler;
    // Setup the flags
    new_action.sa_flags = SA_SIGINFO;
    // Register the handler
    if( 0 != sigaction(SIGINT, &new_action, NULL) )
    {
        printf("Signal handle registration failed. %s\n", strerror(errno));
        return -1;
    }
    while(1)
    {
        // Prepare for a new connection
        //socklen_t addrsize = sizeof(struct sockaddr_in );

        // Accept a connection.
        // Can use NULL in 2nd and 3rd arguments
        // but we want to print the client socket details
        // connfd = accept( listenfd,
        //                  (struct sockaddr*) &peer_addr,
        //                  &addrsize);
        connfd = (int*) malloc(sizeof(int));
        if (NULL == connfd)
        {
            printf("ERROR mallocation fail %s\n",strerror(errno));
            return -1;
        }
        *connfd = accept( listenfd, NULL, NULL);
        if( *connfd < 0 )
        {
            printf("\n Error : Accept Failed. %s \n", strerror(errno));
            return -1;
        }
        //TODO remove this
        pthread_t thread;
        // threads = (pthread_t *) realloc(threads, (1+GLOBAL_PROCESS_OPENED) * sizeof(pthread_t));
        // rc = pthread_create( &(threads[GLOBAL_PROCESS_OPENED]), NULL, counter, (void*) (&connfd));
        rc = pthread_create( &thread, NULL, counter, (void*) (connfd));
        if (rc) 
        {
            printf("ERROR in pthread_create(): %s\n", strerror(rc));
            return -1;
        }
        else
        {
            GLOBAL_PROCESS_OPENED++;
        }

        // getsockname(connfd, (struct sockaddr*) &my_addr,   &addrsize);
        // getpeername(connfd, (struct sockaddr*) &peer_addr, &addrsize);
        // printf("Server: Client connected.\n"
        //        "\t\tClient IP: %s Client Port: %d\n"
        //        "\t\tServer IP: %s Server Port: %d\n",
        //        inet_ntoa( peer_addr.sin_addr ),
        //        ntohs(     peer_addr.sin_port ),
        //        inet_ntoa( my_addr.sin_addr   ),
        //        ntohs(     my_addr.sin_port   ) );

        // // write time
        // ticks = time(NULL);
        // snprintf(data_buff, sizeof(data_buff),
        //           "%.24s\r\n", ctime(&ticks));

        // totalsent = 0;
        // int notwritten = strlen(data_buff);

        // // keep looping until nothing left to write
        // while( notwritten > 0 )
        // {
        //     // notwritten = how much we have left to write
        //     // totalsent  = how much we've written so far
        //     // nsent = how much we've written in last write() call */
        //     nsent = write(connfd,
        //                 data_buff + totalsent,
        //                 notwritten);
        //     // check if error occured (client closed connection?)
        //     assert( nsent >= 0);
        //     printf("Server: wrote %d bytes\n", nsent);

        //     totalsent  += nsent;
        //     notwritten -= nsent;
        // }

        /* close socket  */
        //close(connfd);
    }
    // printf("In Line %d\n",__LINE__ );
    //pthread_exit(NULL);
}
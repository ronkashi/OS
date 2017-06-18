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
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>

#define BUFF_SIZE 1024
#define PORT_NUM 2233
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define QUEUE_LEN 10
#define NUM_OF_PRINTABLE_CHARS 95

/////////////////////////global vars//////////////////////////////
// int NUM_OF_THREADS_OPENED = 0;
int GLOBAL_BYTES_READ = 0;
int GLOBAL_PRINTABLE_CHARS[NUM_OF_PRINTABLE_CHARS] = {0};
int GLOBAL_PROCESS_ACTIVE = 0;
int GLOBAL_PROCESS_FINISHED_FAIL = 0;
int listenfd  = -1;
pthread_mutex_t  lock;
//////////////////////////////////////////////////////////////////
void my_signal_handler( int signum, siginfo_t* info,void* ptr)
{
    int rc;
    close(listenfd);
    int i = 0;
    while (0 < GLOBAL_PROCESS_ACTIVE && i < 5){
        sleep(1);
        i++;
    }
    if (0 < GLOBAL_PROCESS_FINISHED_FAIL)
    {
        printf("\nAn error been occured!!\n");
    }
    else
    {
        i = 0;
        printf("The number of bytes that read by the server is : %d\n",GLOBAL_BYTES_READ );
        printf("we saw \n");
       
        rc = pthread_mutex_lock(&lock);
        if( 0 != rc ) 
        {
            printf( "ERROR in pthread_mutex_lock(): %s\n", strerror( rc ) );
            exit(-1);
        }

        for (i = 0; i < NUM_OF_PRINTABLE_CHARS; ++i)
        {
            printf("%d '%c's,\t",GLOBAL_PRINTABLE_CHARS[i],(i+32) );
        }

        rc = pthread_mutex_unlock(&lock);
        if( 0 != rc ) 
        {
            printf( "ERROR in pthread_mutex_unlock(): %s\n", strerror( rc ) );
            exit(-1);
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
            printf("Error write to socket file: %s\n", strerror(errno));
            __sync_fetch_and_add(&GLOBAL_PROCESS_FINISHED_FAIL, 1);
            return NULL;
        }
    }
    free(t);
    close(fd);

    rc = pthread_mutex_lock(&lock);
    if( 0 != rc ) 
    {
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
        printf( "ERROR in pthread_mutex_unlock(): %s\n", strerror( rc ) );
        __sync_fetch_and_add(&GLOBAL_PROCESS_FINISHED_FAIL, 1);
        return NULL;
    }
    //TODO check if the pthread_exit
    __sync_fetch_and_add(&GLOBAL_PROCESS_ACTIVE, -1); // decrement

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
        connfd = (int*) malloc(sizeof(int));
        if (NULL == connfd)
        {
            printf("ERROR mallocation fail %s\n",strerror(errno));
            continue;
        }
        *connfd = accept( listenfd, NULL, NULL);
        if( *connfd < 0 )
        {
            printf("\n Error : Accept Failed. %s \n", strerror(errno));
            free(connfd);
            continue;
        }
        pthread_t thread;
        rc = pthread_create( &thread, NULL, counter, (int*) (connfd));
        if (rc) 
        {
            printf("ERROR in pthread_create(): %s\n", strerror(rc));
            close(*connfd);
            free(connfd);
            continue;
        }
        else
        {
            __sync_fetch_and_add(&GLOBAL_PROCESS_ACTIVE, 1); //increment
        }
    }
}
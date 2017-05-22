
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#define msg_invalid_cmd "Invalid input in command line\n"
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define bufSize 4096
#define PREFFIX_PIPE "/tmp/counter__"
#define FileNameLength 257 


///////////////////////////////////////////////////////////
typedef enum _RetVals
{
	RV_SUCCESS 	= 0,
	RV_INVALID_CMD 	= -1,
	RV_ERROR_I_O_FILES = -3,
	RV_MAP_FAIL = -4
} RetVals;


///////////////////////////////////////////////////////////
//////////////////////decleration//////////////////////////
// int parse_first_arg(char* arg,char* ch);
ssize_t parse_length(char* arg);
off_t parse_offset(char* arg);
int counter(int fd, off_t offset,ssize_t len_to_process, char query_ch);
long long count_char_in_array(char query_ch,char* arr ,int len);

///////////////////////////////////////////////////////////
// The flow:
// 1. Map the relevant chunk of the text file into the memory as an array.
// 2. Iterate through the array, count the characters. Obtain result R.
// 3. Determine own process ID – PID.
// 4. Create a named pipe file with the name “/tmp/counter_PID”. Open it for writing.
// 5. Determine Dispatcher process ID. Use getppid() function, 
//		since Dispatcher is the parent process.
// 6. Send USR1 signal to the Dispatcher process.
// 7. Write R into the pipe.
// 8. Sleep for 1 second.
// 9. Unmap the file, close the pipe, delete the pipe file. Exit.
///////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
	/*prasing input*/
	if (argc != 5){
		printf(msg_invalid_cmd);
		return RV_INVALID_CMD;
	}

	char query_ch;
	if (1 != strlen(argv[1])){
		printf(msg_invalid_cmd);
		return RV_INVALID_CMD;
	}
	query_ch = argv[1][0];

    int fd = open(argv[2], O_RDONLY);
    if (0 > fd) {
		printf("Error opening file for writing: %s\n", strerror(errno));
		return RV_ERROR_I_O_FILES;
    }
	off_t start_offset = parse_offset(argv[3]);
	ssize_t len_to_process = parse_length(argv[4]);
	if (0 > start_offset || 0 > len_to_process){
		printf(msg_invalid_cmd);
		return RV_INVALID_CMD;
	}
	/*prasing input*/

	/*open mmap*/
	// printf("len_to_process : %zd\n",len_to_process);
	// printf("start_offset : %zd\n", start_offset);
	// printf("fd is : %d\n", fd);

	char* arr = (char*)mmap(NULL, len_to_process, PROT_READ, MAP_SHARED, fd, start_offset);
    //printf("%s\n",arr);
    if (arr == MAP_FAILED) {
    	// printf("len_to_process : %zd\n", len_to_process);
    	// printf("start_offset : %zd\n",start_offset );
		printf("Error mmapping the file: %s\n", strerror(errno));
		return RV_MAP_FAIL;
    }
    /*open mmap*/

    close(fd);

    long long R = count_char_in_array(query_ch,arr,len_to_process);
    // printf("the counter : %d\n", R);
    // printf("len_to_process : %zd\n",len_to_process);
	if(0 > munmap(arr, len_to_process)){
		printf("Error mmUNmapping the file: %s\n", strerror(errno));
		return RV_MAP_FAIL;
	}

    char pipe_name[FileNameLength]; 
    sprintf(pipe_name,"%s%lu",PREFFIX_PIPE,(unsigned long)getpid());

    if(0 > mkfifo(pipe_name, 0666)){//TODO fix premission
    	printf("Error in making FIFO : %s\n", strerror(errno));
		return RV_ERROR_I_O_FILES;
    }

    if (0 > kill(getppid(), SIGUSR1)){
    	printf("Error in kill to parent : %s\n", strerror(errno));
    	return -1;
    } //send signal to parent


    int fdPipe = open(pipe_name, O_WRONLY);
    // printf("fd pipe is : %d\n", fdPipe);
    // printf("here is line %d\n",__LINE__ );
    if (0 > fdPipe) {
		printf("Error opening pipe file for writing: %s\n", strerror(errno));
		return RV_ERROR_I_O_FILES;
    }

    // ////5////
    // printf("here is line %d\n",__LINE__ );
    // kill(getppid(),SIGUSR1);
    // printf("here is line %d\n",__LINE__ );

    char str[FileNameLength];
    sprintf(str,"%lld",R);
    if(0 > write(fdPipe,str,sizeof(str))){
    	printf("Error writing to pipe file: %s\n", strerror(errno));
		return RV_ERROR_I_O_FILES;
    }
    /////////TODO usr2 handler my pearant already read//////////
	//sleep(1);
	/**closing all**/
	close(fdPipe);
    //printf("the process %lu finished \n",(unsigned long)getpid() );
	return RV_SUCCESS;
}

/////////////////////////////////////////////////////////

off_t parse_offset(char* arg){
	int i;
	ssize_t offset;
	for (i = 0; i < strlen(arg); ++i)
	{
		if(!(isdigit(arg[i]))){
			return RV_INVALID_CMD;
		}
	}
	sscanf(arg, "%zd",&offset);
	return offset;
}

/////////////////////////////////////////////////////////

ssize_t parse_length(char* arg){
	int i;
	ssize_t length;
	for (i = 0; i < strlen(arg); ++i)
	{
		if(!(isdigit(arg[i]))){
			return RV_INVALID_CMD;
		}
	}
	sscanf(arg, "%zd",&length);
	return length;
}

///////////////////////////////////////////////////////

long long count_char_in_array(char query_ch,char* arr ,int len){
	long long counter=0;
	long long i;
	for (i = 0; i < len; ++i)
	{
		if (query_ch == arr[i])
		{
			counter++;
		}
	}
	return counter;	
}

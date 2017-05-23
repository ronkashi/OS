#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX_NUM_FORKS 16
#define PREFFIX_PIPE "/tmp/counter__"
#define FileNameLength 256


#define msg_invalid_cmd "Invalid input in command line\n"

///////////////////////////////////////////////////////////

typedef enum _RetVals
{
	RV_SUCCESS 	= 0,
	RV_INVALID_CMD 	= -1,
	RV_CHILD_ERR = -2,
	RV_ERROR_I_O_FILES = -3,
	RV_FORK_ERROR = -4,
	RV_EXECV_ERROR = -5,
	RV_SIGACTION_ERROR = -6
} RetVals;

///////////////////////////////////////////////////////////
static long long globalCount = 0;
static int flagSigHandler = RV_SUCCESS;
//////////////////////decleration//////////////////////////
int parse_first_arg(char* arg,char* ch);
size_t get_file_curr_size(char *file_name);
int get_num_of_forks(off_t* offset_arr, size_t* len_of_chunk, size_t size_to_devide);
///////////////////////////////////////////////////////////
// The flow:
// 1. Determine the file size – N. Decide K – the size of the chunk each Counter 
//processes, and Q – the
// number of the Counter processes. Observe that N = K*Q. If N is less that 
// twice the memory page size,
// then let Q be 1. Constrain the decision by Q ≤ 16.
// 2. Register USR1 signal handler. Turn on SA_SIGINFO flag, so your handler 
//function gets more arguments.
// See Appendix A for an example.
// 3. Launch Q Counter processes. Use fork() and execv() pair, as shown in the class.
// 4. Wait for all processes to finish.
///////////////////////////////////////////////////////////
void my_signal_handler( int signum, siginfo_t* info,void* ptr)
{
	// printf("Signal sent from process %lu\n",(unsigned long) info->si_pid);
    char pipe_name[FileNameLength]; 
    sprintf(pipe_name,"%s%lu",PREFFIX_PIPE,(unsigned long)info->si_pid);
    int fdPipe = open(pipe_name, O_RDONLY);
    if (0 > fdPipe) {
		printf("Error opening pipe file for reading: %s\n", strerror(errno));
		flagSigHandler = RV_ERROR_I_O_FILES;
    }
    char str[FileNameLength];
    if(0 > read(fdPipe,str,sizeof(str))){
    	printf("Error reading from pipe file: %s\n", strerror(errno));
    	flagSigHandler = RV_ERROR_I_O_FILES;
    }
    globalCount +=atoll(str);
    close(fdPipe);
    unlink(pipe_name);
}
///////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		printf(msg_invalid_cmd);
		return RV_INVALID_CMD;
	}
	if (1 != strlen(argv[1])){
		printf(msg_invalid_cmd);
		return RV_INVALID_CMD;
	}
	if(-1 == access(argv[2], 0) )
   	{
    	printf("File %s isn't exists\n" ,argv[2]);
    	return RV_ERROR_I_O_FILES;
  	}
	size_t file_size = get_file_curr_size(argv[2]);
	if (0 > file_size)
	{
		printf("An error in getting file size:  %s\n", strerror( errno));
		return RV_ERROR_I_O_FILES;
	}

	off_t offset_arr[MAX_NUM_FORKS]    = {0};
	size_t len_of_chunk[MAX_NUM_FORKS] = {0};
	pid_t pid_children[MAX_NUM_FORKS]  = {-1};
	char offset_str[FileNameLength];
	char len_of_chunk_str[FileNameLength];
	int forks_num = get_num_of_forks(offset_arr,len_of_chunk,file_size);
	
	// Structure to pass to the registration syscall
	struct sigaction new_action;
	memset(&new_action, 0, sizeof(new_action));
	// Assign pointer to our handler function
	new_action.sa_handler = (void *)my_signal_handler;
	// Setup the flags
	new_action.sa_flags = SA_SIGINFO;
	// Register the handler
	if( 0 != sigaction(SIGUSR1, &new_action, NULL) ){
		printf("Signal handle registration failed. %s\n", strerror(errno));
		return RV_SIGACTION_ERROR;
	}
	////////fork to all////
	int i = 0;
	int RV = RV_SUCCESS;
	for (i = 0; (i < forks_num) && (RV_SUCCESS == flagSigHandler); ++i)
	{
		pid_children[i] = fork();
		if (pid_children[i] < 0){
			// fork() failed – handle error
			printf("Error in fork operation. %s\n", strerror(errno));
			RV = RV_FORK_ERROR;
			break;
		}
		if (pid_children[i] == 0){
			// son process – do son code
			sprintf(offset_str, "%lld",(long long)offset_arr[i]);
			sprintf(len_of_chunk_str, "%lld",(long long) len_of_chunk[i]);
			char *arg_for_execv[] = {"counter", argv[1], argv[2], offset_str, len_of_chunk_str, NULL};

			execv("counter", arg_for_execv);
			printf("Child error pid num : %lu\n",(unsigned long)pid_children[i]);
			RV = RV_EXECV_ERROR;
			break;
		}
		else {
			// parent process – do parent code
			sleep(1);
		}
	}
	int status;
	for (i = 0; i < forks_num; ++i)
	{
		waitpid(pid_children[i] , &status, 0);
		if (! WIFEXITED(status))
		{
			RV = RV_CHILD_ERR;
		}
	}
	if (RV_SUCCESS == RV && RV_SUCCESS == flagSigHandler){
		printf("The character '%s' appears %lld times in \"%s\"\n",argv[1],globalCount,argv[2]);
	}else{
		printf("An error has occurred! Not able to print Counter\n");
	}
	return RV;
}

///////////////////////////////////////////////////////////

size_t get_file_curr_size(char *file_name){
	struct stat file_stat;
    if(stat(file_name,&file_stat) < 0) return RV_ERROR_I_O_FILES;
	return file_stat.st_size;
}

///////////////////////////////////////////////////////////
int get_num_of_forks(off_t* offset_arr, size_t* len_of_chunk, size_t size_to_devide){
	int forks_num;
	int num_of_pages_per_fork;
	if (size_to_devide <= 2 * getpagesize())
	{
		forks_num = 1;
		//printf("%d\n", forks_num);
		num_of_pages_per_fork = 2;
	}
	else{
		if (size_to_devide < MAX_NUM_FORKS * getpagesize())
		{
			forks_num = size_to_devide/getpagesize()+1;
			num_of_pages_per_fork = 1;
		}
		else{
			forks_num = MAX_NUM_FORKS;
			num_of_pages_per_fork = size_to_devide/((MAX_NUM_FORKS-1)*getpagesize());
		}
	}
	int i;
	for ( i = 0; i < forks_num; ++i)
	{
		offset_arr[i] = i*num_of_pages_per_fork*getpagesize();
		len_of_chunk[i] = num_of_pages_per_fork*getpagesize();
		//printf("offset_arr [%d] : %zd len_of_chunk [%d]:%lu\n",i,offset_arr[i],i ,len_of_chunk[i]);
	}
	len_of_chunk[forks_num-1] = size_to_devide - (forks_num-1)*num_of_pages_per_fork*getpagesize();
	if (0 == len_of_chunk[forks_num-1])
	{
		forks_num--;
	}
	return forks_num;
}
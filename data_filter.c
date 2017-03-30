#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#define SmallBuf 1024
#define BigBuf 8192
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
// print the printable characters of text file
int main(int argc, char** argv) {
	// assert first argument filename
	if (argc != 4) {
		printf("number of command line arguments isn't fit");
	}

	char unit;
	long Creq = 0;
	long size;
	int i = 0;
    	for(i=0;i<(strlen(argv[1])-1);i++){
            	if(!(argv[1][i] >= 48 && argv[1][i] <= 57))    
            	{
					printf("Invalid Data amount (first argument)\n");
					return -1;
            	}
    	}
	sscanf(argv[1], "%li%c", &size, &unit); //still a problem size can be big
	//TODO
	switch (unit) {
	case 'B':
		Creq = size;
		break;
	case 'K':
		Creq = size * 1024;
		break;
	case 'M':
		Creq = size * 1024 * 1024;
		break;
	case 'G':
		Creq = size * 1024 * 1024 * 1024;
		break;
	default:
		printf("Invalid unit\n");
		return -1;
	}
//input file open for RDONLY
	int fdInput = open(argv[2], O_RDONLY);
	if (fdInput < 0) {
		printf("Error opening Input file: %s\n", strerror( errno));
		return errno;
	} else {
		printf("File is opened. Descriptor %d\n", fdInput);
	}

//output file open for WRONLY
	int fdOut = open(argv[3], O_WRONLY);

	if (fdOut < 0) {
		printf("Error opening Output file: %s\n", strerror( errno));
		return errno;
	} else {
		printf("File is opened. Descriptor %d\n", fdOut);
	}

	// Decide the size of the buffer
	char *bufRD, *bufWR;
	int bufSize = BigBuf;
	if (Creq < (SmallBuf * 16)) {
		bufSize = SmallBuf;
	}
	bufRD = (char*) malloc(bufSize * sizeof(*bufRD));
	bufWR = (char*) malloc(bufSize * sizeof(*bufWR));
	if (NULL == bufRD || NULL == bufWR) {
		printf("allocation failed");
		return -1;
	}
	long Cread = 0;
	long CPrintable = 0;
	ssize_t lenRD = 0;
	ssize_t lenWR = 0;
	int numReadable = 0;
	i = 0;
	for (; Cread < Creq; Cread += lenRD) {
		lenRD = read(fdInput, bufRD,MIN((Creq - Cread),bufSize));
		if (lenRD < 0) {
			printf("Error reading from file: %s\n", strerror(errno));
			free(bufRD);
			free(bufWR);
			return -1;
		}
		if (lenRD == 0) {
			if (Cread == 0) {
				//the file is empty
				printf("The file is empty");
				free(bufRD);
				free(bufWR);
				return -1;
			}
			else{//EOF
				lseek(fdInput,0,SEEK_SET);//move to the beginning of the file
			}

		}
		else{
			for (i = 0; i < lenRD; i++) {
				if (bufRD[i] > 31 && bufRD[i] < 127) {	//is printable
					bufWR[numReadable] = bufRD[i];
					numReadable++;
				}
				if(numReadable==bufSize-1){
					lenWR = write(fdOut, bufWR, numReadable);
					if (lenWR !=numReadable) {
						printf("Error writing to file: %s\n", strerror(errno));
						free(bufRD);
						free(bufWR);
						return -1;
					}
				CPrintable+=numReadable;
				numReadable=0;
				}
			}
		}
	}
	if(numReadable >0){
		lenWR = write(fdOut, bufWR, numReadable);
		if (lenWR !=numReadable) {
			printf("Error writing to file: %s\n", strerror(errno));
			free(bufRD);
			free(bufWR);
			return -1;
			}
		CPrintable+=numReadable;
	}
	printf("%li characters requested, %li characters read, %li are printable\n",Creq, Cread, CPrintable);
	close(fdInput); // close file
	close(fdOut); //close file
}

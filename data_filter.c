#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#define SmallBuf 1024
#define BigBuf 4096
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

// print the printable (ascii [32,126]) characters of text file
int main(int argc, char** argv) {
	// assert first argument filename
	if (argc != 4) {
		printf("number of command line arguments isn't fit\n");
		return -1;
	}

	char unit;
	long long Creq = 0;
	long long size;
	int i = 0;
    	for(i=0;i<(strlen(argv[1])-1);i++){
            	if(!(argv[1][i] >= 48 && argv[1][i] <= 57))    
            	{
					printf("Invalid Data amount (first argument)\n");
					return -1;
            	}
    	}
	sscanf(argv[1], "%lld%c", &size, &unit);
	unit = toupper(unit);
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
		return -1;
	}

	//output file open for WRONLY
	int fdOut = open(argv[3], O_WRONLY|O_CREAT|O_TRUNC,0644);//TODO fix the premission thing
	if (fdOut < 0) {
		printf("Error opening Output file: %s\n", strerror( errno));
		return -1;
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
		printf("allocation failed\n");
		return -1;
	}
	long long Cread = 0;
	long long CPrintable = 0;
	long long lenRD = 0;
	long long lenWR = 0;
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
				printf("The input file is empty\n");
				free(bufRD);
				free(bufWR);
				return -1;
			}
			else{//EOF
				if(lseek(fdInput,0,SEEK_SET)<0){//move to the beginning of the file
					printf("Error in lseek on the input file: %s\n", strerror(errno));
					free(bufRD);
					free(bufWR);
					return -1;
				}
			}

		}
		else{
			for (i = 0; i < lenRD; i++) {
				if (bufRD[i] > 31 && bufRD[i] < 127) {	//is printable
					bufWR[numReadable] = bufRD[i];
					numReadable++;
				}
				//printf("the ascii num %d\n",bufRD[i] );
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
	printf("%lld characters requested, %lld characters read, %lld are printable\n",Creq, Cread, CPrintable);
	free(bufRD);
	free(bufWR);
	close(fdInput); // close file
	close(fdOut); //close file
	return 0;
}

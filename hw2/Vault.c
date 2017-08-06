///////////////////////////////////////////

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h> // for time measurement
#include <assert.h>
#include <libgen.h>
#include <stdbool.h>
#define  _POSIX_C_SOURCE 200809L
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define RightDellimiter "<<<<<<<<"
#define LeftDellimiter ">>>>>>>>"
#define dellimterLen 8
#define MaxFragmentesCount 3
#define MaxCapableFiles 100 //100
#define FileNameLength 257 //257
#define Ksize 1024
#define BufferSize 4096
#define _FILE_OFFSET_BITS 64

///////////////////////////////////////////

typedef enum
{
	UNKNOWN 	= -1,
	INIT        = 0,
	LIST 		= 1,
	ADD         = 2,
	RM          = 3,
	FETCH       = 4,
	DEFRAG 		= 5,
	STATUS 		= 6
} Command;

typedef enum _RetVals
{
	RV_SUCCESS 	= 0,
	RV_INVALID_CMD 	= -1,
	RV_NOT_ENOUGH_SPACE = -2,
	RV_ERROR_I_O_FILES = -3,
	RV_FILE_NAME_ALLREADY_EXISTS = -4
} RetVals;

typedef struct _RepositoryMetadata
{ 
// 	ssize_t repository_size; //overall repository overall file size
 	time_t creation_time; 
 	time_t modification_time; 
 	short files_count;
 	ssize_t file_max_capacity;
 	
} RepositoryMetadata;

typedef struct _DataBlockInfo
{
	off_t offset;
	ssize_t size; //size of block allways with delimiters
	short file_index;
} DataBlockInfo;

typedef struct _FatRecord
{
 	char file_name[FileNameLength]; 
 	ssize_t file_real_size; //the real file size 
 	mode_t permissions; 
 	time_t insertion_date;
  	DataBlockInfo blocks[MaxFragmentesCount]; 
} FatRecord;

typedef struct _Catalog
{
 	RepositoryMetadata meta_data;
 	FatRecord fat_record[MaxCapableFiles];
}Catalog;

///////////////////////////////////////////

Command get_command(char* cmd);
void print_usage();
RetVals write_catalog_to_repository(Catalog* ctlg,char* repository);
int load_catalog_from_repository(Catalog* ctlg,char* repository);
int unitToBytes(char unit);
ssize_t whatIsTheSizeInBytes(char* a);
char* convert_bytes_to_readable_str(ssize_t bytes);
Catalog* catalog_init(Catalog* ctlg,ssize_t max_capacity_in_bytes);
int get_the_file_name_index_in_catalog(Catalog* ctlg, char* file_name);
RetVals delete_delimiters(Catalog* ctlg,int file_index,int fd);
RetVals write_file_from_repository(Catalog* ctlg,int fdVault,int fdOut,int index_of_file);
ssize_t get_total_file_sizes_with_dellimiters(Catalog* ctlg);
ssize_t get_total_file_sizes_without_dellimiters(Catalog* ctlg);
Catalog* init_file_rec(Catalog* ctlg,int file_index);
int perform_cmd(Command cmd, Catalog* ctlg,char* repository,char* arg);
int cmpfunc (const void * a, const void * b);
ssize_t get_file_curr_size(char *file_name);


static int list(Catalog* ctlg);
static int defrag(Catalog* ctlg,char* repository);
static int status(Catalog* ctlg,char* repository);
RetVals init(Catalog* ctlg,char* a);
static int add(Catalog* ctlg,char* repository, char* a);
static int rm(Catalog* ctlg,char* repository,char* a);
static RetVals fetch(Catalog* ctlg,char* repository, char* arg);


///////////////////////////////////////////

int main(int argc, char* argv[]) 
{

	struct timeval start, end;
  	long seconds, useconds;
  	double mtime;
  	int RV = RV_SUCCESS;

    // start time measurement
  	gettimeofday(&start, NULL);
    
	RetVals rv = RV_SUCCESS;
	Command cmd = UNKNOWN;

	char* repository = NULL;	

	if (argc <3)
	{
		print_usage();
		return RV_INVALID_CMD;
	}
	
	repository = argv[1];
	cmd = get_command(argv[2]);

	if (cmd == UNKNOWN)
	{
		print_usage();
		return RV_INVALID_CMD;
	}
	if(cmd == INIT || cmd == RM || cmd == FETCH || cmd == ADD){
		if(argc != 4){
			print_usage();
			return RV_INVALID_CMD;
		}
	}
	else{
		if(argc != 3){
			print_usage();
			return RV_INVALID_CMD;
		}
	}

	Catalog* ctlg = NULL;
	ctlg = (Catalog*) malloc(sizeof(Catalog));
	if (cmd == INIT)
	{
		RV = init(ctlg,argv[3]);
	}
	else{
		load_catalog_from_repository(ctlg,repository);
		RV = perform_cmd(cmd, ctlg,repository,argv[3]);
	}
	if(RV<0) return RV;


	ctlg->meta_data.modification_time = time(NULL);//TODO sys call
	if (0>ctlg->meta_data.modification_time)
	{
		printf("A system call Error in time  %s\n", strerror( errno));
		return RV_ERROR_I_O_FILES;
	}
	RV = write_catalog_to_repository(ctlg,repository);
	if(RV<0) return RV;
	free(ctlg);
	
	// end time measurement and print result
    gettimeofday(&end, NULL);

    seconds  = end.tv_sec  - start.tv_sec;
    useconds = end.tv_usec - start.tv_usec;
    
    mtime = ((seconds) * 1000 + useconds/1000.0);
    printf("Elapsed time: %.3f milliseconds\n", mtime);
	return RV_SUCCESS;	
}
////////////////////////////////////////////
int perform_cmd(Command cmd, Catalog* ctlg,char* repository,char* arg){
	int RV = RV_SUCCESS;
	switch (cmd)
	{
		case LIST: 	  return list(ctlg); 
		case ADD:
			RV = add(ctlg,repository,arg);
			if (RV<0){
				return RV;
			}
			printf("%s inserted\n",basename(arg) );
			return RV;

		case RM:      
			RV = rm(ctlg,repository,arg);
						if (RV<0){
				return RV;
			}
			printf("%s deleted\n",arg);
			return RV;
		case FETCH:   
			RV = fetch(ctlg,repository,arg);
			if (RV<0){
				return RV;
			}
			printf("%s created\n",arg);
			return RV;
		case DEFRAG:  return defrag(ctlg,repository);
		case STATUS:  return status(ctlg,repository);
	}
}
////////////////////////////////////////////
Command get_command(char* cmd)
{
	if (strcasecmp(cmd, "init") == 0)
		return INIT;
	if (strcasecmp(cmd, "list") == 0)
		return LIST;	
	if (strcasecmp(cmd, "add") == 0)
		return ADD;	
	if (strcasecmp(cmd, "rm") == 0)
		return RM;	
	if (strcasecmp(cmd, "fetch") == 0)
		return FETCH;
	if (strcasecmp(cmd,"defrag") == 0)
		return DEFRAG;
	if (strcasecmp(cmd,"status") == 0)
		return STATUS;
	return UNKNOWN;
}

///////////////////////////////////////////

void print_usage()
{
	printf("Invalid CommandLine. \n");
	printf("Usage: <command> [arg]. \n");
	printf("Commands: {list, defrag, status, init, add, rm, fetch} \n");
}

///////////////////////////////////////////
RetVals write_catalog_to_repository(Catalog* ctlg,char* repository){
	ssize_t lenWR;
	if(NULL == ctlg){printf("a bug\n");}
	int fd = open(repository,O_RDWR|O_CREAT,0644);
	if (fd < 0) {
		printf("Error opening Input file: %s\n", strerror(errno));
		return -1;
	}
	if(0>lseek(fd,0,SEEK_SET)){
		printf("Error seeking on repository: %s\n", strerror(errno));
		return RV_ERROR_I_O_FILES;
	}

	lenWR = write(fd, ctlg ,sizeof(Catalog));
	if (lenWR < 0) {
		printf("Error writing to file: %s\n", strerror(errno));
		return RV_ERROR_I_O_FILES;
	}
	close(fd);
	return RV_SUCCESS;
}
///////////////////////////////////////////
int load_catalog_from_repository(Catalog* ctlg,char* repository)
{
	int i=0;
	ssize_t lenRD;
	int fd;
	fd = open(repository,O_RDWR);//TODO chek if the flags right
	if (fd < 0) {
		printf("Error in open Repository file: %s\n", strerror(errno));
		return RV_ERROR_I_O_FILES;
	}

	lenRD = read(fd, ctlg ,sizeof(Catalog));
	if(lenRD < 0) {
		printf("Error reading from file: %s\n", strerror(errno));
		return RV_ERROR_I_O_FILES;
	}
	close(fd);
	return fd;
}
///////////////////////////////////////////
int unit_to_bytes(char unit)
{
	unit = toupper(unit);
	switch (unit)
	{
	case 'B': 	return 1;
	case 'K': 	return Ksize;
	case 'M': 	return Ksize * Ksize;
	case 'G': 	return Ksize * Ksize * Ksize;
	default: 	printf("Invalid unit\n"); return -1;
	}
}
///////////////////////////////////////////
ssize_t what_is_the_size_in_bytes(char* a)
{
	char unit;
	ssize_t size;
	int i = 0;
	if (strlen(a)<2)
	{
		printf("Invalid Data amount (second argument)\n");
		return -1;
	}
	for(i=0;i<(strlen(a)-1);i++){
    	if(!(isdigit(a[i])))    
    	{
			printf("Invalid Data amount (second argument)\n");
			return -1;
    	}
	}
	sscanf(a, "%zd%c", &size, &unit);
	return size * (ssize_t)unit_to_bytes(unit);
}
///////////////////////////////////////////
char* convert_bytes_to_readable_str(ssize_t bytes)
{
	//assert(bytes < 9999*Ksize*Ksize*Ksize);//TODO the program will work until 9999G
	char out_string[6]={0};
	if ((Ksize*Ksize*Ksize)<bytes){
		sprintf(out_string, "%zd", (bytes/(Ksize*Ksize*Ksize)));
		return strcat(out_string,"G");
	}
	if ((Ksize*Ksize)<bytes){
		sprintf(out_string, "%zd", bytes/(Ksize*Ksize));
		return strcat(out_string,"M");
	}
	if ((Ksize)<bytes){
		sprintf(out_string, "%zd", (bytes>>10));
		return strcat(out_string,"K");	
	}
	sprintf(out_string, "%zd", bytes);
	return strcat(out_string,"B");
}

///////////////////////////////////////////
Catalog* catalog_init(Catalog* ctlg,ssize_t max_capacity_in_bytes)
{
	int i;
	ctlg->meta_data.creation_time = time(NULL);//TODO maybe its a syscall
	if (0>ctlg->meta_data.creation_time)
	{
		printf("A system call Error in time  %s\n", strerror( errno));
		return NULL;
	}
	ctlg->meta_data.files_count = 0;
	ctlg->meta_data.file_max_capacity = max_capacity_in_bytes;
	for (i = 0; i < MaxCapableFiles; ++i)//TODO max capable
	{
		init_file_rec(ctlg,i);
	}
	printf("A vault created\n");
	return ctlg;
}
///////////////////////////////////////////

RetVals init(Catalog* ctlg , char* arg)
{
	ssize_t size = what_is_the_size_in_bytes(arg);
	if (size < 0)
	{
		printf("file size to init is invalid\n");
		return RV_INVALID_CMD;
	}
	if(size < sizeof(Catalog)+2*dellimterLen){
		printf("Fail the requested file size isn't big enough\n");
		return RV_NOT_ENOUGH_SPACE;
	}
	catalog_init(ctlg,size);
	return RV_SUCCESS;
}
///////////////////////////////////////////
int list(Catalog* ctlg)
{
	int i = 0;
	for (i = 0; i < MaxCapableFiles; ++i)
	{
		if(0 < strlen(ctlg->fat_record[i].file_name)){
			printf("%s \t %zdB \t %5.4o \t %9.30s\n",
			ctlg->fat_record[i].file_name,
			(get_file_curr_size(ctlg->fat_record[i].file_name)),
			0777&ctlg->fat_record[i].permissions,
			ctime(&(ctlg->fat_record[i].insertion_date)));			
		}
	}
}
///////////////////////////////////////////
ssize_t get_file_curr_size(char *file_name){
	struct stat file_stat;
    if(stat(file_name,&file_stat) < 0) return RV_ERROR_I_O_FILES;
	return file_stat.st_size;
}
///////////////////////////////////////////
mode_t get_file_curr_permissions(char *file_name){
	struct stat file_stat;
    if(stat(file_name,&file_stat) < 0) return RV_ERROR_I_O_FILES;
	return file_stat.st_mode;
}
///////////////////////////////////////////
short sort_data_blocks(Catalog* ctlg,DataBlockInfo* data_blocks){
	short block_num = 0;
	int i,j;
	for (i = 0; i < MaxCapableFiles; i++)
	{
		for (j = 0; j < MaxFragmentesCount; j++)
		{
			if(0 < ctlg->fat_record[i].blocks[j].size){
				data_blocks[block_num] = ctlg->fat_record[i].blocks[j];
				block_num++;	
			}
		}
	}
	qsort(data_blocks, block_num, sizeof(DataBlockInfo), cmpfunc);
	return block_num;
}
///////////////////////////////////////////
int cmpfunc (const void * a, const void * b)
{
   if(0 < ((DataBlockInfo*)a)->offset - ((DataBlockInfo*)b)->offset ) {
   		return 1;
   }
   return -1;
}

///////////////////////////////////////////
short find_gaps(Catalog* ctlg,short block_num,DataBlockInfo* data_blocks,DataBlockInfo* gap_blocks){
	ssize_t start = sizeof(Catalog);
	ssize_t end;
	short gaps_num = 0;
	int i;
	for (i = 0; i < block_num; ++i)
	{
		end = data_blocks[i].offset;
		if(end-start>1){
			gap_blocks[gaps_num].offset = start;
			gap_blocks[gaps_num].size = end-start;
			gaps_num++;
		}
		start = end + data_blocks[i].size;
	}
	end = ctlg->meta_data.file_max_capacity;
	if(end-start>1){
		gap_blocks[gaps_num].offset = start;
		gap_blocks[gaps_num].size = end-start;
		gaps_num++;
	}
	return gaps_num;
}
///////////////////////////////////////////
ssize_t sum_of_max_gaps(DataBlockInfo* max_gaps){
	int i =0;
	ssize_t sum=0;
	for (i = 0; i < MaxFragmentesCount; ++i)
	{
		sum += max_gaps[i].size;
	}
	return sum;
}
///////////////////////////////////////////
void swap(DataBlockInfo* max_gaps,int i,int j){
	DataBlockInfo temp;
	temp.offset = 0;
	temp.size = 0;
	temp = max_gaps[i];
	max_gaps[i] = max_gaps[j];
	max_gaps[i] = temp;
}
///////////////////////////////////////////
/*
* get the gaps in the vault ordered by their offset
* return if there is a defragmention that the file can fit in
*/
bool is_suitable_gaps(ssize_t file_size, 
						short gaps_num,DataBlockInfo* gap_blocks,DataBlockInfo* max_gaps){
	int i= 0;
	for (i = 0; i < MaxFragmentesCount; ++i)
	{
		max_gaps[i].offset = 0;
		max_gaps[i].size = 0;
	}
	///////////////////TODO////////////////
	if (0 < gaps_num){
		max_gaps[0] = gap_blocks[0];
		if (max_gaps[0].size >= file_size + 2*dellimterLen){
			return true;
		}
	}
	if (1 < gaps_num){
		max_gaps[1] = gap_blocks[1];
		if (max_gaps[0].size < max_gaps[1].size){
			swap(max_gaps,1,0);

		}
		if (sum_of_max_gaps(max_gaps) >= file_size + 4*dellimterLen){
			return true;
		}
	}

	for (i = 2; i < gaps_num; ++i)
	{
		if(max_gaps[2].size < gap_blocks[i].size){
			max_gaps[2] = gap_blocks[i];
			if(max_gaps[1].size < max_gaps[2].size)		{swap(max_gaps,1,2);}
			if(max_gaps[0].size < max_gaps[1].size)		{swap(max_gaps,0,1);}
			if (sum_of_max_gaps(max_gaps) >= file_size + 6*dellimterLen){
				return true;
			}
		}
	}
	return false; //MUST to defrag to add it
}
///////////////////////////////////////////
int get_available_space_in_fat_rec(Catalog* ctlg){
	int i =0;
	for (i = 0; i < MaxCapableFiles; ++i)
	{
		if (0 == strlen(ctlg->fat_record[i].file_name))
		{
			return i;
		}
	}
	return -1;
}
///////////////////////////////////////////
//gets the first 3 max gaps that we saw (from beging of the file) and insert to them
int insert_file_to_gaps(DataBlockInfo* max_gaps, ssize_t file_size, 
						Catalog* ctlg, int fdAdd, int fdVault, short index_of_file){
	int i,j;
	ssize_t lenRD;
	ssize_t lenWR;
	ssize_t left_to_write_from_file = file_size;

	char buffer[BufferSize];

	assert(max_gaps[0].size != 0);
	assert(max_gaps[0].offset != 0);
	for (i = 0; i < MaxFragmentesCount && max_gaps[i].size >= 2*dellimterLen; ++i) // 
	{
		if(0>lseek(fdVault ,max_gaps[i].offset,SEEK_SET)){
			printf("Error seeking on repository: %s\n", strerror(errno));
			return RV_ERROR_I_O_FILES;
		}
		
		if (dellimterLen != write(fdVault, RightDellimiter ,dellimterLen)) {
			printf("Error writing to file: %s\n", strerror(errno));
			return RV_ERROR_I_O_FILES;
		}
		for(j=0;j<MIN(left_to_write_from_file,max_gaps[i].size -2*dellimterLen) ;j+=lenRD)
		{
			lenRD = read(fdAdd, &buffer ,MIN(BufferSize,(MIN(left_to_write_from_file,max_gaps[i].size -2*dellimterLen - j))));
			if (lenRD < 0) {
				printf("Error reading from repository: %s\n", strerror(errno));
				return RV_ERROR_I_O_FILES;
			}
			lenWR = write(fdVault, &buffer ,lenRD);
			if (lenWR < 0) {
				printf("Error writing to file: %s\n", strerror(errno));
				return RV_ERROR_I_O_FILES;
			}
		}
		left_to_write_from_file -= j;

		if (dellimterLen != write(fdVault, LeftDellimiter ,dellimterLen)) {
			printf("Error writing to file: %s\n", strerror(errno));
			return RV_ERROR_I_O_FILES;
		}
		ctlg->fat_record[index_of_file].blocks[i].size += (j+2*dellimterLen);
		ctlg->fat_record[index_of_file].blocks[i].offset = max_gaps[i].offset;
		ctlg->fat_record[index_of_file].blocks[i].file_index = index_of_file;
	}
	return RV_SUCCESS;
}
///////////////////////////////////////////
RetVals add(Catalog* ctlg,char *repository, char *arg){
	char* file_name_to_add = basename(arg);
	int fdVault = open(repository,O_RDWR);
	if (fdVault<0)
	{
		printf("Error opening repository file: %s\n", strerror( errno));
		return RV_ERROR_I_O_FILES;
	}
	if(-1 != get_the_file_name_index_in_catalog(ctlg,file_name_to_add)){
		printf("The file: %s allready exists in the repository \n",arg);
		return RV_FILE_NAME_ALLREADY_EXISTS;
	}
	if(get_file_curr_size(repository) + get_file_curr_size(arg) + 6*dellimterLen > ctlg->meta_data.file_max_capacity){
		printf("There is no avaible space in the repository for file : %s\n",arg);
		return RV_NOT_ENOUGH_SPACE;
	}
	if (MaxCapableFiles <= ctlg->meta_data.files_count)
	{
		printf("you reach the file count limit. the limit is : %d\n",MaxCapableFiles);
		return RV_NOT_ENOUGH_SPACE;
	}
	int fdAdd = open(arg, O_RDONLY);
	if (fdAdd < 0) {
		printf("Error opening Add file: %s\n", strerror( errno));
		return RV_ERROR_I_O_FILES;
	}
	short index_of_file = get_available_space_in_fat_rec(ctlg);
	if(-1 == index_of_file){
		printf("Error you reach the number of capable files to store : %d\n",MaxCapableFiles);
		return RV_NOT_ENOUGH_SPACE;
	}
	DataBlockInfo data_blocks[MaxFragmentesCount*MaxCapableFiles];  //300
	DataBlockInfo gap_blocks[MaxFragmentesCount*MaxCapableFiles+1]; //301
	DataBlockInfo max_gaps[MaxFragmentesCount];						//3
	short block_num;
	short gaps_num;

	int i;
	block_num = sort_data_blocks(ctlg, data_blocks);
	gaps_num = find_gaps(ctlg,block_num, data_blocks,gap_blocks);
	if( is_suitable_gaps(get_file_curr_size(arg),gaps_num, gap_blocks, max_gaps)){
		insert_file_to_gaps(max_gaps, get_file_curr_size(arg), ctlg, fdAdd,  fdVault, index_of_file);
		sprintf(ctlg->fat_record[index_of_file].file_name,"%s",file_name_to_add);
		ctlg->fat_record[index_of_file].permissions = get_file_curr_permissions(arg);
		ctlg->fat_record[index_of_file].insertion_date = time(NULL);
		if (0>ctlg->fat_record[index_of_file].insertion_date)
		{
			printf("A system call Error in time  %s\n", strerror( errno));
			return RV_ERROR_I_O_FILES;
		}
		ctlg->fat_record[index_of_file].file_real_size = get_file_curr_size(arg);
		ctlg->meta_data.files_count++;
	}
	else
	{
		printf("is a free space but the content has to be fragmented into more than %d blocks\n", MaxFragmentesCount);
		return RV_SUCCESS;
	}
	close(fdVault);
	close(fdAdd);
	return RV_SUCCESS;
}
///////////////////////////////////////////
int get_the_file_name_index_in_catalog(Catalog* ctlg, char* file_name)
{
	int i;
	for ( i = 0; i < MaxCapableFiles; ++i)
	{
		if (0 == strcmp(ctlg->fat_record[i].file_name , file_name)){
			return i;	
		} 
	}
	return -1;//the file name isnt exists in the Catalog
}
///////////////////////////////////////////
Catalog* init_file_rec(Catalog* ctlg,int file_index)
{
	int j;
	ctlg->fat_record[file_index].file_name[0]='\0';
	for ( j = 0; j < MaxFragmentesCount; ++j)
	{
		ctlg->fat_record[file_index].blocks[j].size = 0;
		ctlg->fat_record[file_index].blocks[j].offset = 0;
		ctlg->fat_record[file_index].blocks[j].file_index = -1;
	}
	return ctlg;
}
///////////////////////////////////////////
RetVals delete_delimiters(Catalog* ctlg,int file_index,int fd)
{
	int i = 0;
	off_t offset_absulut;
	for (i= 0; (i < MaxFragmentesCount) && (ctlg->fat_record[file_index].blocks[i].offset != 0); ++i)
	{
		offset_absulut = ctlg->fat_record[file_index].blocks[i].offset;
		if(lseek(fd,offset_absulut,SEEK_SET) < 0) return RV_ERROR_I_O_FILES;
		if(write(fd,"00000000",dellimterLen) != dellimterLen) return RV_ERROR_I_O_FILES;

		offset_absulut += (ctlg->fat_record[file_index].blocks[i].size-dellimterLen);
		if(lseek(fd,offset_absulut,SEEK_SET) < 0) return RV_ERROR_I_O_FILES;
		if(write(fd,"00000000",dellimterLen) != dellimterLen) return RV_ERROR_I_O_FILES;

	}
	return RV_SUCCESS;
}
///////////////////////////////////////////
RetVals rm(Catalog* ctlg,char* repository ,char* arg)
{
	int index_of_file = get_the_file_name_index_in_catalog(ctlg,basename(arg));
	int fdVault = open(repository,O_RDWR);
	if (fdVault<0)
	{
		printf("Error opening repository file: %s\n", strerror( errno));
		return RV_ERROR_I_O_FILES;
	}
	if (index_of_file == -1)
	{
		printf("Fail the file: %s is not in the vault\n",arg);
		return RV_SUCCESS;
	}
	if (delete_delimiters(ctlg,index_of_file,fdVault) == RV_ERROR_I_O_FILES)
	{
		printf("Error opening Output file: %s\n", strerror( errno));
		return RV_ERROR_I_O_FILES;
	}
	close(fdVault);
	ctlg->meta_data.files_count--;
	init_file_rec(ctlg,index_of_file);
	return RV_SUCCESS;
}

///////////////////////////////////////////
RetVals write_file_from_repository(Catalog* ctlg,int fdVault, int fdOut,int index_of_file)
{
	int j=0,i=0;
	ssize_t lenRD;
	ssize_t lenWR;
	char fetch_buffer[BufferSize];

	for (j=0;j<MaxFragmentesCount;j++){
		if(0>lseek(fdVault ,ctlg->fat_record[index_of_file].blocks[j].offset + dellimterLen ,SEEK_SET)){
			printf("Error seeking on repository: %s\n", strerror(errno));
			return RV_ERROR_I_O_FILES;
		}
		for(i=0;i < ctlg->fat_record[index_of_file].blocks[j].size - 2*dellimterLen; i+=lenRD)
		{
			lenRD = read(fdVault, &fetch_buffer ,MIN(BufferSize,(ctlg->fat_record[index_of_file].blocks[j].size - i- 2*dellimterLen)));
			if (lenRD < 0) {
				printf("Error reading from repository: %s\n", strerror(errno));
				return RV_ERROR_I_O_FILES;
			}
			lenWR = write(fdOut, &fetch_buffer ,lenRD);
			if (lenWR < 0) {
				printf("Error writing to file: %s\n", strerror(errno));
				return RV_ERROR_I_O_FILES;
			}
		}
	}
	return RV_SUCCESS;
}
///////////////////////////////////////////
RetVals fetch(Catalog* ctlg,char* repository, char* arg)
{
	int index_of_file = get_the_file_name_index_in_catalog(ctlg,basename(arg));
	if (index_of_file == -1)
	{
		printf("Fail the file: %s is not in the vault\n",arg);
		return RV_SUCCESS;
	}
	int fdOut = open(arg,O_WRONLY|O_CREAT|O_TRUNC);
	if (fdOut < 0) {
		if(errno == EACCES){
			printf("no premission to create the file in this directory : %s\n", strerror( errno));
		}
		printf("Error opening Output file: %s\n", strerror( errno));
		return RV_ERROR_I_O_FILES;
	}
	int fdVault = open(repository,O_RDONLY);
	if (fdVault<0)
	{
		printf("Error opening vault file : %s\n", strerror( errno));
		return RV_ERROR_I_O_FILES;
	}

	RetVals RV = write_file_from_repository(ctlg,fdVault, fdOut,index_of_file);
	if(chmod(arg,ctlg->fat_record[index_of_file].permissions)<0){
		printf("Error changing premisions to Output file: %s\n", strerror( errno));
		return RV_ERROR_I_O_FILES;
	}
	close(fdOut);
	close(fdVault);
	return RV;

}
//////////////////////////////////////////////////////TODO first inserta files than deliinimiters
///////////////////////////////////////////
int defrag(Catalog* ctlg,char* repository)
{
	time_t insertion_time[MaxCapableFiles];
	short new_index;
	char file_path_arr[MaxCapableFiles][FileNameLength];
	int i;
	char template[] = "/tmp/tmpdir.XXXXXX";
    char *dir_name = mkdtemp(template);
    char temp[FileNameLength];
   	if(dir_name == NULL)
    {
        printf("mkdtemp failed: %s\n" ,strerror(errno));
        return RV_ERROR_I_O_FILES;
    }
	for ( i = 0;i<MaxCapableFiles;i++)
	{
		file_path_arr[i][0] = '\0';
		if ('\0' != (ctlg->fat_record[i].file_name[0])){
			insertion_time[i] = ctlg->fat_record[i].insertion_date;
			sprintf(file_path_arr[i],"%s/%s",(dir_name),ctlg->fat_record[i].file_name);
			fetch(ctlg,repository,file_path_arr[i]);
			sprintf(temp,"%s", ctlg->fat_record[i].file_name);
			rm(ctlg,repository ,temp);
		}
	}
	for ( i = 0;i<MaxCapableFiles;i++){
		if ('\0' != file_path_arr[i][0]){
			add(ctlg,repository,file_path_arr[i]);
			new_index = get_the_file_name_index_in_catalog(ctlg,file_path_arr[i]);
			ctlg->fat_record[new_index].insertion_date = insertion_time[i];
			if( 0 > remove(file_path_arr[i])){
				printf("Error in remove temp files: %s\n", strerror(errno));
				return RV_ERROR_I_O_FILES;
			}
		}
	}

    if(rmdir(dir_name) <0)
    {
        printf("rmdir failed: %s\n" ,strerror(errno));
        return RV_ERROR_I_O_FILES;
    }
    printf("Defragmention complete\n");
    return RV_SUCCESS;
}
///////////////////////////////////////////
ssize_t get_total_file_sizes_with_dellimiters(Catalog* ctlg)
{
	int i=0,j=0;
	ssize_t total_file_sizes_with_dellimiters =0;
	for (i=0;i<MaxCapableFiles;i++)
	{
		for (j = 0; j < MaxFragmentesCount; j++)
		{
			total_file_sizes_with_dellimiters+=ctlg->fat_record[i].blocks[j].size;
		}
	}
	return total_file_sizes_with_dellimiters;
}

///////////////////////////////////////////
ssize_t get_total_file_sizes_without_dellimiters(Catalog* ctlg)
{
	int i=0,j=0;
	ssize_t total_file_sizes =0;
	for (i=0;i<MaxCapableFiles;i++)
	{
		total_file_sizes+=ctlg->fat_record[i].file_real_size;
	}
	return total_file_sizes;
}
///////////////////////////////////////////
double get_fragmention_ratio(Catalog* ctlg,char* repository)
{
	double ratio;
	ssize_t gap_length = get_file_curr_size(repository)
						- get_total_file_sizes_with_dellimiters(ctlg)
						- sizeof(Catalog);//TODO substract from the last sign catalog to first "<"
	ratio = (double)gap_length / get_total_file_sizes_without_dellimiters(ctlg);
	return ratio;
}
///////////////////////////////////////////
int status(Catalog* ctlg,char* repository)
{
	printf("Number of files: %d\n",ctlg->meta_data.files_count);
	printf("Total size: %s\n",(convert_bytes_to_readable_str(get_total_file_sizes_without_dellimiters(ctlg))));
	printf("Fragmentation ratio: %lf\n",get_fragmention_ratio(ctlg,repository));
}
///////////////////////////////////////////

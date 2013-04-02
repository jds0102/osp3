#include "disk.h"
#include "disk-array.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

// Global variables
int verbose = 0;
int level, strip, disks;
struct disk_array * diskArray;

// Error Message for input
int errorMsg(int number){
    fprintf(stderr, "Error %d\n", number);
    exit(1);
}

//Prototypes for functions
void parseLine(char * line);
void readFromArray(int lba, int size);
void writeToArray(int lba, int size, char* value);
void failDisk(int disk);
void recoverDisk(int disk);
void endProgram();

// Main
int main( int argc, char* argv[]){
	// Declarations of variables
	int size;
    char * filename;
    char c;
    char * endptr;
    FILE * fd = 0;
  
    if (argc != 11 && argc != 12){
		errorMsg(11);
    }
    
    static struct option long_options[] =
    {
      {"level",  required_argument, 0, 'a'},
      {"strip",  required_argument, 0, 'b'},
      {"disks",  required_argument, 0, 'c'},
      {"size",   required_argument, 0, 'd'},
      {"trace",  required_argument, 0, 'e'},
      {"verbose", no_argument, 0, 'f'},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    // Need to add a detection if same arg name was passed twice
    // Need to add case if input for numbers includes a letter (may not need)
    while ((c = getopt_long_only(argc, argv, "a:b:c:d:e:f", long_options, &option_index)) != -1) {
	switch (c) {

		// LEVEL  
		case 'a':
			level = strtol(optarg, &endptr, 10);
			// Not an int
			if (endptr == optarg){
				errorMsg(1);
			}
			else if (level == 0 || level == 10 || level == 4 || level == 5){
	      
			}
			// Not valid RAID number
			else {
				errorMsg(2);
			}
			break;
		//STRIP
		case 'b':
			strip = strtol(optarg, &endptr, 10);
			// Not an int
			if (endptr == optarg){
				errorMsg(3);
			}
			// 0 or negative blocks per strip
			else if (strip < 1){
				errorMsg(4);
			}
			break;
	    
		// DISKS
		case 'c':
			disks = strtol(optarg, &endptr, 10);
			// Not an int
			if (endptr == optarg){
				errorMsg(5);
			}
			// 0 or negative disks
			else if (disks < 1){
				errorMsg(6);
			}
			break;
		// SIZE
		case 'd':
			size = strtol(optarg, &endptr, 10);
			// Not an int
			if (endptr == optarg){
				errorMsg(7);
			}
			// 0 or negative blocks per disk
			else if (size < 1){
				errorMsg(8);
			}
			break;
			// TRACE
		case 'e':
			filename = malloc(sizeof(char*)*strlen(optarg));
			strcpy(filename, optarg);
			fd = fopen(filename, "r");
			if (fd == 0){
				errorMsg(9);
			}
			break;
		// Verbose case
		case 'f':
			verbose = 1;
			break;    
		// Default
		default:
			printf("%s\n", optarg);
			errorMsg(10);
		}
	}
    
    // Create a disk array
    char * virtualDiskArray = "temp";
    diskArray = disk_array_create(virtualDiskArray, disks, size);
    
    char buf[1000];
    while (fgets(buf,1000, fd)!=NULL){
        parseLine(buf);
    }
    exit(0);
}


// Parsing function
// Splits up by command in trace file
void parseLine (char * line){
    char* token = strtok(line, " ");
    if (token == NULL){
		errorMsg(12);
    }
	printf("Parse\n %s\n", token);

    char * endPtr;
    if (strcmp(token, "READ") == 0){
		printf("Read\n");
		//Get the lba
		token = strtok(NULL, " ");
		if (token == NULL){
			errorMsg(13);
		}
		int lba = strtol(token, &endPtr, 10);
		if (token == endPtr) {
			errorMsg(22);
		}
      
		//Get the size of the read
		token = strtok(NULL, " ");
		if (token == NULL){
			errorMsg(15);
		}
		int readSize = strtol(token, &endPtr, 10);
		if (token == endPtr) {
			errorMsg(16);
		}
		if (token == NULL){
			errorMsg(24);
		}

		//The command should end here
		token = strtok(NULL, " ");
		if (token != NULL){
			errorMsg(17);
		}

		readFromArray(lba,readSize);
	}

	else if (strcmp(token, "WRITE") == 0){
		//Get the lba 
		token = strtok(NULL, " ");
		if (token == NULL){
			errorMsg(14);
		}
		int lba = strtol(token, &endPtr, 10);
		if (token == endPtr) {
			errorMsg(14);
		}

		//Get the size of the write
		token = strtok(NULL, " ");
		if (token == NULL){
			errorMsg(14);
		}
		int writeSize = strtol(token, &endPtr, 10);
		if (token == endPtr) {
			errorMsg(14);
		}

		//Get the value to write
		token = strtok(NULL, " ");
		if (token == NULL){
			errorMsg(14);
		}
		char* value = malloc(sizeof(char*)*strlen(token));
		strcpy(value, token);
      
		//The command should end here
		token = strtok(NULL, " ");
		if (token != NULL){
			errorMsg(14);
		}

		writeToArray(lba, writeSize, value);
		free(value);
    }
    else if (strcmp(token, "FAIL") == 0){
		//Get the disk that failed  
		token = strtok(NULL, " ");
		if (token == NULL){
			errorMsg(14);
		}
		int disk = strtol(token, &endPtr, 10);
		if (token == endPtr) {
			errorMsg(14);
		}
      
		//The command should end here
		token = strtok(NULL, " ");
		if (token != NULL){
			errorMsg(14);
		}
      
		failDisk(disk);
    }
    else if (strcmp(token, "RECOVER") == 0){  
		//Gert the disk to recover
		token = strtok(NULL, " ");
		if (token == NULL){
			errorMsg(14);
		}
		int disk = strtol(token, &endPtr, 10);
		if (token == endPtr) {
			errorMsg(14);
		}
      
		//The command should end here
		token = strtok(NULL, " ");
		if (token != NULL){
			errorMsg(14);
		}
      
		recoverDisk(disk);
    }
    else if (strcmp(token, "END") == 0){ 
		//There should be no other arguments with end
		token = strtok(NULL, " ");
		if (token != NULL){
			errorMsg(14);
		} 

		endProgram();
    }
    else{
		errorMsg(104);
    }
}

// Read LBA size
void readFromArray(int lba, int size){
	int diskToStartRead = lba;
	if (level == 0) {
		int currentDisk = (lba/strip) % disks;

		int currentBlock = (lba%strip)+(lba/strip)/disks;

		while (size > 0) {
			printf("Read this block, %i, on this disk %i\n", currentBlock, currentDisk);

			//I think this is how this is supposed to work
			//char buffer[size]
			//disk_array_read(diskArray, currentDisk, currentBlock, buffer);

			//print current block
			lba ++;
			currentDisk = (lba/strip) % disks;
			currentBlock = (lba%strip)+(lba/strip)/disks;
			size --;
		}
	}
	else if(level == 10){

	}
	else if(level == 4){

	}
	else if(level == 5){

	}
	else{
		errorMsg(15);
	}
}

// Write LBA size
void writeToArray(int lba, int size, char * value){

}

// Fail disk
void failDisk(int disk){

}

// Recover disk
void recoverDisk(int disk){

}

// End of trace file
void endProgram(){
  printf("I am going to succesfully end\n");
exit(0);
}





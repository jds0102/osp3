#include "disk.h"
#include "disk-array.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

// Global variables
int verbose = 0;
int level, strip, disks, blockSize;
struct disk_array * diskArray;

// Error Message for input
int errorMsg(int number){
    fprintf(stderr, "Error %d\n", number);
    exit(1);
}

//Prototypes for functions
void parseLine(char * line);
void readFromArray(int lba, int size);
void writeToArray(int lba, int size, char* buff);
void failDisk(int disk);
void recoverDisk(int disk);
void endProgram();
int getPhysicalBlock(int numberOfDisks, int lba, int* diskToAccess, int* blockToAccess);
int calculateXOR(int diskToSkip, int blockToRead, char* xorBlock);

// Main
int main( int argc, char* argv[]){
	// Declarations of variables
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
			blockSize = strtol(optarg, &endptr, 10);
			// Not an int
			if (endptr == optarg){
				errorMsg(7);
			}
			// 0 or negative blocks per disk
			else if (blockSize < 1){
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
    
    // Input checking for number of disks vs level
    // Check that if it is RAID 10 there are an even # of disks
    if ((level == 10) && ((disks % 2) != 0)){
      errorMsg(0);
    }
    // Check that both levels 4 and 5 have more than 2 disks
    if (level == 4 && disks < 3){
      errorMsg(0);
    }
    if (level == 5 && disks < 3){
      errorMsg(0);
    }
    
    // Create a disk array
    char * virtualDiskArray = "temp";
    diskArray = disk_array_create(virtualDiskArray, disks, blockSize);
    
    char buf[1000];
    while (fgets(buf,1000, fd)!=NULL){
	buf[strlen(buf) - 1] = '\0'; 
        parseLine(buf);
    }
    exit(0);
}


// Parsing function
// Splits up by command in trace file
void parseLine (char * line){
    fprintf(stdout, "%s\n", line);
    char* token = strtok(line, " ");
    if (token == NULL){
		errorMsg(12);
    }

    char * endPtr;
    if (strcmp(token, "READ") == 0){
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
		int value = atoi(token);
                char buff[BLOCK_SIZE];
		int i;
		for(i = 0; i < 256; i++){
		 ((int *)buff)[i] = value; 
		}

		//put value (an int) into buffer and then make a bigger buffer of it repeating
               
		//int i;
		//for (i = 0; i < 256; i++){
		//	((int*)buff)[i] = ((int*)value)[0];
		//}
		//The command should end here
		token = strtok(NULL, " ");
		if (token != NULL){
			errorMsg(14);
		}

		writeToArray(lba, writeSize, buff);
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
		printf("%s\n", token);
		errorMsg(104);
    }
}

// Read LBA size
void readFromArray(int lba, int size){
  // DOES HANDLE A FAILED DISK
  if (level == 0) {
    int currentDisk, currentBlock;
    getPhysicalBlock(disks, lba, &currentDisk, &currentBlock);
    //int currentDisk = (lba/strip) % disks;
    //int currentBlock = (lba%strip)+((lba/strip)/disks)*strip;
    while (size > 0) {
      // OUR PRINT STATEMENTS
      //printf("Read this block, %i, on this disk %i\n", currentBlock, currentDisk);
      //print current block
      //I think this is how this is supposed to work
      char buffer[BLOCK_SIZE];
      if(disk_array_read(diskArray, currentDisk, currentBlock, buffer) == 0){
	int* firstValue = (int*)buffer;
	fprintf(stdout, "%i ", *firstValue);
      }else{
	fprintf(stdout, "ERROR ");
      }
      lba ++;
      getPhysicalBlock(disks, lba, &currentDisk, &currentBlock);
      //currentDisk = (lba/strip) % disks;
      //currentBlock = (lba%strip)+((lba/strip)/disks)*strip;
      size --;
    }
  }
  // DOES HANDLE A FAILED DISK
  else if(level == 10){
    // Will first look to see if first disk is available to read from
    int currentDisk, currentBlock;
    getPhysicalBlock(disks, lba, &currentDisk, &currentBlock);
    currentDisk *= 2;
    
    while (size > 0){
      //OUR PRINT STATEMENTS
      //printf("Read this block, %i, on this disk %i\n", currentBlock, currentDisk);
      char buffer[BLOCK_SIZE];
      if (disk_array_read(diskArray, currentDisk, currentBlock, buffer) == 0){
	int* firstValue = (int *)buffer;
	fprintf(stdout, "%i ", *firstValue);
      } else if (disk_array_read(diskArray, currentDisk + 1, currentBlock, buffer) == 0){
	int* firstValue = (int *)buffer;
	fprintf(stdout, "%i ", *firstValue);
      } else {
	fprintf(stdout, "ERROR ");
      }
      lba++;
      getPhysicalBlock(disks, lba, &currentDisk, &currentBlock);
      currentDisk *= 2;
      size --;
    }
  }
  // DOES NOT INCLUDE PARITY DISK
  else if(level == 4){
    int currentDisk, currentBlock;
    getPhysicalBlock(disks, lba, &currentDisk, &currentBlock);
    
    while (size > 0) {
      // OUR PRINT STATEMENTS
      //printf("Read this block, %i, on this disk %i\n", currentBlock, currentDisk);
      //print current block
      //I think this is how this is supposed to work
      char buffer[BLOCK_SIZE];
      if(disk_array_read(diskArray, currentDisk, currentBlock, buffer) == 0){
	int* firstValue = (int *)buffer;
	printf("%i ", *firstValue);
      }else{ //Try to recover disk on read fail using parity
	if (calculateXOR(currentDisk,currentBlock,buffer) ==  -1){
	  printf("ERROR ");
	}
	else{
	  int* firstValue = (int *)buffer;
	  printf("%i ", *firstValue);
	}
      }
      lba ++;
      getPhysicalBlock(disks, lba, &currentDisk, &currentBlock);
      size --;
    }
  }
  else if(level == 5){

  }
  else{
    errorMsg(15);
  }
}

// Write LBA size
void writeToArray(int lba, int size, char * buff){
  //DOES HANDLE A FAILED DISK
  if (level == 0) {
    int currentDisk, currentBlock;
    getPhysicalBlock(disks, lba, &currentDisk, &currentBlock);
    //int currentDisk = (lba/strip) % disks;
    //int currentBlock = (lba%strip)+((lba/strip)/disks)*strip;
    
    //successfulWrite should be 0 after all writing is complete 
    int successfulWrite = 0;
    while (size > 0) {
      // OUR PRINT STATEMENTS
      //printf("Write this block, %i, on this disk %i\n", currentBlock, currentDisk);
      //memset(buff,(currentDisk+1)*(currentBlock+1),sizeof(buffer));
      //this should work
      successfulWrite += disk_array_write(diskArray, currentDisk, currentBlock, buff);
      lba ++;
      getPhysicalBlock(disks, lba, &currentDisk, &currentBlock);
      //currentDisk = (lba/strip) % disks;
      //currentBlock = (lba%strip)+((lba/strip)/disks)*strip;
      size --;
    }
    if (successfulWrite != 0) {
      fprintf(stdout, "ERROR "); 
    }
  }
  //DOES HANDLE A FAILED DISK
  else if(level == 10){
    int currentDisk, currentBlock;
    getPhysicalBlock(disks, lba, &currentDisk, &currentBlock);
    currentDisk *= 2;
    
    //successfulWrite should be 0 after all writing is complete 
    int successfulWrite = 0;
    
    while (size > 0) {
      // OUR PRINT STATEMENTS
      //printf("Write this block, %i, on this disk %i\n", currentBlock, currentDisk);
      
      // Status keeps track of if write to disk succeded
      // If we fail both, status will equal -2
      int status = disk_array_write(diskArray, currentDisk, currentBlock, buff);
      status += disk_array_write(diskArray, currentDisk + 1, currentBlock, buff);
      if (status == -2){
	successfulWrite = -1;
      }
      lba ++;
      getPhysicalBlock(disks, lba, &currentDisk, &currentBlock);
      currentDisk *= 2;
      size--;
    }
    if (successfulWrite != 0) {
      fprintf(stdout, "ERROR "); 
    }
  }
  //FIRST WITHOUT PARITY DISK
  else if(level == 4){
    int successfulWrite = 0;
    int currentDisk, currentBlock;
    getPhysicalBlock(disks, lba, &currentDisk, &currentBlock);
    int stripLength =(disks - 1)*strip;
    int i;

    while (size > 0) {      
      //check if it is the start of a strip
      successfulWrite += disk_array_write(diskArray, currentDisk, currentBlock, buff);
      
      if((lba%stripLength) == stripLength - 1  || size == 1){
        char parity[BLOCK_SIZE];
	int firstBlockInStripe = currentBlock - (currentBlock % strip);
	
	//calculate how many parity blocks to update and replace strip below with it
	int x = strip;
	for(i = 0; i < x; i ++){
	    calculateXOR(disks-1, i+firstBlockInStripe, parity);
	    disk_array_write(diskArray, disks-1, i+firstBlockInStripe, parity);
	}
      }
      lba ++;
      getPhysicalBlock(disks, lba, &currentDisk, &currentBlock);
      size --;
    }
  }
  else if(level == 5){

  }
  else{
    errorMsg(15);
  }
}

// Fail disk
void failDisk(int disk){
	disk_array_fail_disk(diskArray, disk);
}

// Recover disk
// Possibly add a return value because if recover fails on a RAID 4/5
// it will act as if the disk has been recovered when in reality it
void recoverDisk(int disk){
    disk_array_recover_disk(diskArray, disk);
    if (level == 10){
     int diskToCopyFrom;
     if (disk % 2){
      diskToCopyFrom = disk - 1; 
     } else{
      diskToCopyFrom = disk + 1; 
     }
     int i;
     for (i = 0; i < blockSize; i++){
      char buffer[BLOCK_SIZE];
      disk_array_read(diskArray, diskToCopyFrom, i, buffer);
      disk_array_write(diskArray, disk, i, buffer);
     }
  }else if (level == 4  || level == 5){
    int i;
    char missingData [BLOCK_SIZE];
    for (i = 0; i < blockSize; i++){
      if(calculateXOR(disk,i, missingData) == -1){
	return;
      }
      disk_array_write(diskArray, disk, i, missingData);
    }
  }
}

// End of trace file
void endProgram(){
  //disk_array_print_stats(diskArray);
  exit(0);
}

// Return -1 if LBA is outside the realm of disk space
// Handles per level
int getPhysicalBlock(int numberOfDisks, int lba, int* diskToAccess, int* blockToAccess){
  if (level == 0){  
    *diskToAccess = (lba/strip) % numberOfDisks;
    *blockToAccess = (lba%strip)+((lba/strip)/numberOfDisks)*strip;
    return 0;
  } else if (level == 10){ 
    numberOfDisks = numberOfDisks / 2;
    *diskToAccess = (lba/strip) % numberOfDisks;
    *blockToAccess = (lba%strip)+((lba/strip)/numberOfDisks)*strip;
    return 0;
  } else if (level == 4){
    numberOfDisks -= 1;
    *diskToAccess = (lba/strip) % numberOfDisks;
    *blockToAccess = (lba%strip)+((lba/strip)/numberOfDisks)*strip;
    return 0;
  }  else {
    return 0;
  }
  return 0;
}

// Calculate the XOR of all the disks besides the one passed at the block given
// Return -1 if one of the disks required to calculate the xor is failed or if any invalid arg is passed
// the XOR'ed value is returned in the xorBlock, which is assumed to be of size BLOCK_SIZE
// As a note, this method uses additive method to calculate the parity bit

int calculateXOR(int diskToSkip, int blockToRead, char* xorBlock) {
  int i;
  for (i = 0; i < BLOCK_SIZE; i++) {
      xorBlock[i] = 0;
  }
  //The disk to skip does not exist
  if (diskToSkip >= disks) {
   return -1; 
  }
  
  //Check to make sure blockToRead is within range
  
  char temp[BLOCK_SIZE];
  
  for(i=0; i < disks; i++) {
    
    //Skip this diskToSkip
    if (i == diskToSkip) {
      continue; 
    }
    
    if (disk_array_read(diskArray, i, blockToRead, temp) != 0) {
     return -1; 
    }
    int j;
    for (j = 0; j < BLOCK_SIZE; j++) {
      xorBlock[j] = xorBlock[j] ^ temp[j];
    }
  }
  
  return 0;
}

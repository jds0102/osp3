#include "disk.h"
#include "disk-array.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

// Global variables
int verbose = 0;
int level, strip, disks, blockSize; //strip is strip size
struct disk_array * diskArray;

// Error Message for input
void errorMsgInput(){
    fprintf(stderr, "ERROR IN INPUT\n");
    exit(1);
}

// Error Message for parsing
void errorMsgParse(){
  fprintf(stderr, "ERROR IN PARSING\n");
  exit(1);
}


//Prototypes for functions
void parseLine(char * line);
void readFromArray(int lba, int size);
void writeToArray(int lba, int size, char* buff);
void failDisk(int disk);
void recoverDisk(int disk);
void endProgram();
void getPhysicalBlock(int numberOfDisks, int lba, int* diskToAccess, int* blockToAccess);
int calculateXOR(int diskToSkip, int blockToRead, char* xorBlock);


// Main
int main( int argc, char* argv[]){
  // Declarations of variables
  char * filename;
  char c;
  char * endptr;
  FILE * fd = 0;
  
  // Make sure number of args is 11 or 12 (verbose optional)
  if (argc != 11 && argc != 12){
    errorMsgInput();
  }
    
  // Structure to keep all command args  
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
  
  
  while ((c = getopt_long_only(argc, argv, "a:b:c:d:e:f", long_options, &option_index)) != -1) {
    
    switch (c) {
      // LEVEL  
      case 'a':
	level = strtol(optarg, &endptr, 10);
	// Not an int
	if (endptr == optarg){
	  errorMsgInput();
	} else if (level == 0 || level == 10 || level == 4 || level == 5){    
	  //Do not need to do anything
	}else { // Not valid RAID number
	  errorMsgInput();
	}
	break;
	
      //STRIP
      case 'b':
	strip = strtol(optarg, &endptr, 10);
	// Not an int
	if (endptr == optarg){
	  errorMsgInput();
	} else if (strip < 1){ // 0 or negative blocks per strip
	  errorMsgInput();
	}
	break;
	    
      // DISKS
      case 'c':
	disks = strtol(optarg, &endptr, 10);
	// Not an int
	if (endptr == optarg){
	  errorMsgInput();
	} else if (disks < 1){ // 0 or negative disks
	  errorMsgInput();
	}
	break;
	
	// SIZE
	case 'd':
	  blockSize = strtol(optarg, &endptr, 10);
	  // Not an int
	  if (endptr == optarg){
	    errorMsgInput();
	  } else if (blockSize < 1){// 0 or negative blocks per disk
	    errorMsgInput();
	  }
	  break;
	  
       // TRACE
       case 'e':
	 filename = malloc(sizeof(char*)*strlen(optarg));
	 strcpy(filename, optarg);
	 fd = fopen(filename, "r");
	 if (fd == 0){
	   errorMsgInput();
	 }
	 break;
	 
       // Verbose case
       case 'f':
	 verbose = 1;
	 break;
	 
       // Default
       default:
	 errorMsgInput();
     }
  }
    
  // Input checking for number of disks vs level
  // Check that if it is RAID 10 there are an even # of disks
  if ((level == 10) && ((disks % 2) != 0)){
    errorMsgInput();
  }
  // Check that both levels 4 and 5 have more than 2 disks
  if (level == 4 && disks < 3){
    errorMsgInput();
  }
  if (level == 5 && disks < 3){
    errorMsgInput();
  }
    
  // Create a disk array
  char * virtualDiskArray = "OurVirutalDisk";
  diskArray = disk_array_create(virtualDiskArray, disks, blockSize);
  
  // Gets the line from the trace file
  char buf[1000];
  while (fgets(buf,1000, fd)!=NULL){
    buf[strlen(buf) - 1] = '\0'; 
    parseLine(buf);
  }
  exit(0);
}


/*
  This function will parse the input from the trace file and call the proper function
  @param char* line - a line that will be parsed
  @return void 
*/
void parseLine (char * line){
  fprintf(stdout, "%s\n", line);
  char* token = strtok(line, " ");
  if (token == NULL){
    errorMsgParse();
  }

  char * endPtr;
  // Read Command: READ LBA SIZE
  if (strcmp(token, "READ") == 0){
    //Get the lba
    token = strtok(NULL, " ");
    if (token == NULL){
      errorMsgParse();
    }
    int lba = strtol(token, &endPtr, 10);
    if (token == endPtr) {
      errorMsgParse();
    }
    
    //Get the size of the read
    token = strtok(NULL, " ");
    if (token == NULL){
      errorMsgParse();
    }
    int readSize = strtol(token, &endPtr, 10);
    if (token == endPtr) {
      errorMsgParse();
    }
    if (token == NULL){
      errorMsgParse();
    }
    
    //The command should end here
    token = strtok(NULL, " ");
    if (token != NULL){
      errorMsgParse();
    }
    readFromArray(lba,readSize);
  }
  
  // Write command: WRITE LBA SIZE VALUE
  else if (strcmp(token, "WRITE") == 0){
    //Get the lba 
    token = strtok(NULL, " ");
    if (token == NULL){
      errorMsgParse();
    }
    int lba = strtol(token, &endPtr, 10);
    if (token == endPtr) {
      errorMsgParse(14);
    }
    
    //Get the size of the write
    token = strtok(NULL, " ");
    if (token == NULL){
      errorMsgParse();
    }
    int writeSize = strtol(token, &endPtr, 10);
    if (token == endPtr) {
      errorMsgParse();
    }
    
    //Get the value to write
    token = strtok(NULL, " ");
    if (token == NULL){
      errorMsgParse();
    }
    int value = atoi(token);
    char buff[BLOCK_SIZE];
    int i;
    for(i = 0; i < 256; i++){
      ((int *)buff)[i] = value; 
    }
    
    //The command should end here
    token = strtok(NULL, " ");
    if (token != NULL){
      errorMsgParse();
    }
    writeToArray(lba, writeSize, buff);
  }
  
  //Fail command: FAIL DISK#
  else if (strcmp(token, "FAIL") == 0){
    //Get the disk that failed  
    token = strtok(NULL, " ");
    if (token == NULL){
      errorMsgParse();
    }
    int disk = strtol(token, &endPtr, 10);
    if (token == endPtr) {
      errorMsgParse();
    }
    
    //The command should end here
    token = strtok(NULL, " ");
    if (token != NULL){
      errorMsgParse();
    }
    failDisk(disk);
  }
  
  //Recover command: RECOVER DISK#
  else if (strcmp(token, "RECOVER") == 0){
    //Gert the disk to recover
    token = strtok(NULL, " ");
    if (token == NULL){
      errorMsgParse();
    }
    int disk = strtol(token, &endPtr, 10);
    if (token == endPtr) {
      errorMsgParse();
    }
    
    //The command should end here
    token = strtok(NULL, " ");
    if (token != NULL){
      errorMsgParse();
    }
    recoverDisk(disk);
  }
  
  // End command: END
  else if (strcmp(token, "END") == 0){
    //There should be no other arguments with end
    token = strtok(NULL, " ");
    if (token != NULL){
      errorMsgParse();
    }
    endProgram();
  }
  
  else{
    errorMsgParse();
  }
}

/*
  This function will read from the disk array starting at lba for size
  @param int lba - logical block address
  @param int size - size of the read
  @return void 
*/
void readFromArray(int lba, int size){
  //Level 0
  if (level == 0) {
    int currentDisk, currentBlock;
    getPhysicalBlock(disks, lba, &currentDisk, &currentBlock);
    while (size > 0) {
     char buffer[BLOCK_SIZE];
     // If read succeeds print out the value
     if(disk_array_read(diskArray, currentDisk, currentBlock, buffer) == 0){
	int* firstValue = (int*)buffer;
	fprintf(stdout, "%i ", *firstValue);
     }else{
	fprintf(stdout, "ERROR ");
     }
     lba ++;
     getPhysicalBlock(disks, lba, &currentDisk, &currentBlock);
     size --;
    }
  }
  
  //Level 10
  else if(level == 10){
    int currentDisk, currentBlock;
    getPhysicalBlock(disks, lba, &currentDisk, &currentBlock);
    currentDisk *= 2;
    
    while (size > 0){
      char buffer[BLOCK_SIZE];
      // Need to check if 1st disk in mirrored is valid
      if (disk_array_read(diskArray, currentDisk, currentBlock, buffer) == 0){
	int* firstValue = (int *)buffer;
	fprintf(stdout, "%i ", *firstValue);
      } 
      // Need to check if 2nd disk in mirrored is valid
      else if (disk_array_read(diskArray, currentDisk + 1, currentBlock, buffer) == 0){
	int* firstValue = (int *)buffer;
	fprintf(stdout, "%i ", *firstValue);
      } 
      // Otherwise value read
      else {
	fprintf(stdout, "ERROR ");
      }
      lba++;
      getPhysicalBlock(disks, lba, &currentDisk, &currentBlock);
      currentDisk *= 2;
      size --;
    }
  }
  
  //Level 4
  else if(level == 4){
    int currentDisk, currentBlock;
    getPhysicalBlock(disks, lba, &currentDisk, &currentBlock);
    
    while (size > 0) {
      char buffer[BLOCK_SIZE];
      // If read succeeds print out value
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
  
  //Level 5
  else if(level == 5){
    int currentDisk, currentBlock;
    getPhysicalBlock(disks, lba, &currentDisk, &currentBlock);
    int currentParityDisk = (currentBlock/strip) % disks;;
    if (currentDisk >= currentParityDisk) {
      currentDisk ++;
    }
    while (size > 0) {
      char buffer[BLOCK_SIZE];
      // If read succeeds print out value
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
      currentParityDisk = (currentBlock/strip) % disks;;
      if (currentDisk >= currentParityDisk) {
        currentDisk ++;
      }
      size --;
    }
  }
  
  //Error
  else{
    errorMsgParse();
  }
}


/*
  This function will write to the disk array starting at lba, for size, and will write 
  the pattern in the buff passed into the block
  @param int lba - logical block address
  @param int size - size of the write
  @param char *buff - buffer to write to each block of the disk array
  @return void 
*/ 
void writeToArray(int lba, int size, char * buff){
  //Level 0 
  if (level == 0) {
    int currentDisk, currentBlock;
    getPhysicalBlock(disks, lba, &currentDisk, &currentBlock);
    
    //successfulWrite should be 0 after all writing is complete, otherwise print error
    //Only want to print error once
    int successfulWrite = 0;
    while (size > 0) {
      successfulWrite += disk_array_write(diskArray, currentDisk, currentBlock, buff);
      lba ++;
      getPhysicalBlock(disks, lba, &currentDisk, &currentBlock);
      size --;
    }
    if (successfulWrite != 0) {
      fprintf(stdout, "ERROR "); 
    }
  }
  
  //Level 10
  else if(level == 10){
    int currentDisk, currentBlock;
    getPhysicalBlock(disks, lba, &currentDisk, &currentBlock);
    currentDisk *= 2;
    
    //successfulWrite should be 0 after all writing is complete 
    // Only want to print error once
    int successfulWrite = 0;
    
    while (size > 0) {
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
  
  //Level 4
  else if(level == 4){
    	// successfulWrite should be 0 after all writing is complete
    	// Only want to print error once
    	int successfulWrite = 0;
    	int currentDisk, currentBlock;
    	int stripeLength =(disks - 1)*strip;
    	int i, j, k;

	char temp[BLOCK_SIZE];
	char parityChange[BLOCK_SIZE];

	while (size > 0) {
		int whereInStripe = lba % stripeLength;
		int startOfThisStripe = lba - whereInStripe;
		int blockOffsetOfThisStripe = (startOfThisStripe)/(disks-1);
		int coverTo = whereInStripe + size;
		int tempNum;

		for(i = 0; i < strip; i ++){
			for (j = 0; j < BLOCK_SIZE; j++) {
     				parityChange[j] = 0;
			}
			tempNum = i;
			//printf("i: %i\n", i);
			currentBlock = i + blockOffsetOfThisStripe;
			while(tempNum < stripeLength){
			//printf("temp: %i\n", tempNum);
				currentDisk = tempNum / strip;
				if(tempNum >= whereInStripe && tempNum < coverTo){					
					if (disk_array_write(diskArray, currentDisk, currentBlock, buff)){
						successfulWrite --; //write failed
					}
					for (k = 0; k < BLOCK_SIZE; k++) {
						parityChange[k] = parityChange[k] ^ buff[k];
					}
				}
				else{
					if (disk_array_read(diskArray, currentDisk, currentBlock, temp) != 0) {
					//error reading old data, which means we cannot update parity for this block
					}
					else{
						for (k = 0; k < BLOCK_SIZE; k++) {
							parityChange[k] = parityChange[k] ^ temp[k];
						}
					}
				}
				tempNum += strip;
			}
			disk_array_write(diskArray, disks-1, currentBlock, parityChange);
			//printf("parity %i is %i\n", i, *((int*)parityChange));
		}
		int next = startOfThisStripe + stripeLength;
		size -= next - lba;
		lba = next;
	}
    	if (successfulWrite != 0) {
      		fprintf(stdout, "ERROR "); 
    	}
  }
  
  //Level 5
  else if(level == 5){
    // successfulWrite should be 0 after all writing is complete
    	// Only want to print error once
    	int successfulWrite = 0;
    	int currentDisk, currentBlock;
    	int stripeLength =(disks - 1)*strip;
    	int i, j, k;
	int currentParityDisk;

	char temp[BLOCK_SIZE];
	char parityChange[BLOCK_SIZE];

	while (size > 0) {
		int whereInStripe = lba % stripeLength;
		int startOfThisStripe = lba - whereInStripe;
		int blockOffsetOfThisStripe = (startOfThisStripe)/(disks-1);
		int coverTo = whereInStripe + size;
		int tempNum;

		for(i = 0; i < strip; i ++){
			for (j = 0; j < BLOCK_SIZE; j++) {
     				parityChange[j] = 0;
			}
			tempNum = i;
			//printf("i: %i\n", i);
			currentBlock = i + blockOffsetOfThisStripe;
			currentParityDisk = (currentBlock/strip) % disks;
			while(tempNum < stripeLength){
			//printf("temp: %i\n", tempNum);
				currentDisk = tempNum / strip;
				if (currentDisk >= currentParityDisk) {
				  currentDisk ++;
				}
				
				if(tempNum >= whereInStripe && tempNum < coverTo){					
					if (disk_array_write(diskArray, currentDisk, currentBlock, buff)){
						successfulWrite --; //write failed
					}
					for (k = 0; k < BLOCK_SIZE; k++) {
						parityChange[k] = parityChange[k] ^ buff[k];
					}
				}
				else{
					if (disk_array_read(diskArray, currentDisk, currentBlock, temp) != 0) {
					//error reading old data, which means we cannot update parity for this block
					}
					else{
						for (k = 0; k < BLOCK_SIZE; k++) {
							parityChange[k] = parityChange[k] ^ temp[k];
						}
					}
				}
				tempNum += strip;
			}
			disk_array_write(diskArray, currentParityDisk, currentBlock, parityChange);
			//printf("parity %i is %i\n", i, *((int*)parityChange));
		}
		int next = startOfThisStripe + stripeLength;
		size -= next - lba;
		lba = next;
	}
    	if (successfulWrite != 0) {
      		fprintf(stdout, "ERROR "); 
    	}
  }
  
  //Error
  else{
    errorMsgParse();
  }
}

/*
  This function calls provided disk_array_fail_disk method to mark disk as failed
  @param int disk - disk to fail
  @return void 
*/
void failDisk(int disk){
	disk_array_fail_disk(diskArray, disk);
}

/*
  This function will recover the disk and then attempt to put any recovered data on that
  recovered disk
  @param int disk - disk to recover 
  @return void 
*/
void recoverDisk(int disk){
    // If RAID 0 do nothing besides calling the recover function
    disk_array_recover_disk(diskArray, disk);
    //Level 10
    if (level == 10){
     int diskToCopyFrom;
     
     // To determine if recovered disk is first or second in a pair of mirrored disks
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
  }
  
  //Level 4 or 5 do the same attempts at recovery
  else if (level == 4  || level == 5){
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

/*
  Exit program and print the stats
  @params no parameters
  @return void 
*/
void endProgram(){
  disk_array_print_stats(diskArray);
  exit(0);
}

/*
  Will convert an lba and a number of disks to a disk to access and block to access when
  reading and writing
  @param int numberOfDisk - number of disks (argument)
  @param int lba - logical block address
  @param int* diskToAccess - disk to access will be passed by reference
  @param int* blockToAccess - block to access will be passed by reference
  @return void 
*/
void getPhysicalBlock(int numberOfDisks, int lba, int* diskToAccess, int* blockToAccess){
  // Level 0
  if (level == 0){  
    *diskToAccess = (lba/strip) % numberOfDisks;
    *blockToAccess = (lba%strip)+((lba/strip)/numberOfDisks)*strip;
  } 
  // Level 10
  else if (level == 10){ 
    numberOfDisks = numberOfDisks / 2;
    *diskToAccess = (lba/strip) % numberOfDisks;
    *blockToAccess = (lba%strip)+((lba/strip)/numberOfDisks)*strip;
  } 
  // Level 4
  else if (level == 4 || level == 5){
    numberOfDisks -= 1;
    *diskToAccess = (lba/strip) % numberOfDisks;
    *blockToAccess = (lba%strip)+((lba/strip)/numberOfDisks)*strip;
  } 
}

/*
  Calculate the XOR of all the disks besides the one passed at the block given
  Return -1 if one of the disks required to calculate the xor is failed or if any invalid arg is passed
  the XOR'ed value is returned in the xorBlock, which is assumed to be of size BLOCK_SIZE
  As a note, this method uses additive method to calculate the parity bit
  @param int diskToSkip - either parity disk or failed disk
  @param int blockToRead - which block we want to calculate parity for
  @param char *xorBlock - pass by reference the xorBlock that will be used
  @return int - 0 on success, -1 on failure
*/
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

#include "disk.h"
#include "disk-array.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

// Global variables
int verbose = 0;

// Error Message for input
int errorMsg(int number){
    fprintf(stderr, "Error %d\n", number);
    exit(1);
}

// Read LBA size


// Write LBA size


// Fail disk


// Recover disk


// End of trace file


// Parsing function
// Splits up by command in trace file
char ** parseLine (char * line){
    char* token = strtok(line, " ");
    if (token == NULL){
	errorMsg(14);
    }
    if (strcmp(token, "READ")){
      token = strtok(NULL, " ");
      if (token == NULL){
	errorMsg(14);
      }
      char* temp1 = malloc(sizeof(char*)*strlen(token));
      strcpy(temp1, token);
      token = strtok(NULL, " ");
      if (token == NULL){
	errorMsg(14);
      }
      char* temp2 = malloc(sizeof(char*)*strlen(token));
      strcpy(temp2, token);
      if (token != NULL){
	errorMsg(14);
      }
      ReadFromArray(temp1,temp2);
    }
    else if (strcmp(token, "WRITE")){  
    }
    else if (strcmp(token, "FAIL")){  
    }
    else if (strcmp(token, "RECOVER")){  
    }
    else if (strcmp(token, "END")){  
    }
    else{
	errorMsg(14);
    }
}




// Main
int main( int argc, char* argv[])
{
    // Declarations of variables
    int level, strip, disks, size;
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
    struct disk_array * disk_array_t = disk_array_create(virtualDiskArray, disks, size);
    
    char buf[1000];
    while (fgets(buf,1000, fd)!=NULL){
        parseLine(buf);
    }
    
    
  exit(0);
}

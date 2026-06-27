#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


// CONSTANTS
#define FS_SIZE (10 * 1024 * 1024) // 10 MB
#define BLOCK_SIZE 512
#define MAX_INODES 128
#define MAX_BLOCKS (FS_SIZE / BLOCK_SIZE)
#define MAX_FILENAME 255
#define MAX_DIRECT 100


// FILE TYPES
#define TYPE_FREE 0
#define TYPE_FILE 1
#define TYPE_DIR 2


// STRUCTS
typedef struct{
	int total_blocks;
	int num_free;
	int inodes_start;
	int data_start;
} Superblock;

typedef struct {
	int used;
	int type;
	int size;
	int direct[MAX_DIRECT];
	int indirect;
} Inode;

typedef struct {
	int inode;
	char name[MAX_FILENAME];
	int valid;
} DirEntry;

typedef struct {
 int block;
 struct FreeBlock* next;
} FreeBlock;


// GLOBALS
FILE* fs = NULL;
Superblock* sb;
FreeBlock* free_blocks;
int* bitmap;
Inode* inodes; 


// Error protocol
void die(const char* msg) {
	printf("ERROR: %s\n", msg);
	exit(1);
}


// create a new filesystem
void format_fs() {
	// Initialize SuperBlock member functions
	
	free_block->block = 1;

	
	// TEST
	die("in new_fs()");
	return 0;
}


// Load existing file system
void load_filesys() {
	
	// Read Superblock data
	sb = malloc(sizeof(Superblock));
	fread(sb->total_blocks, sizeof(sb->total_blocks), 1, fs);
	fread(sb->num_free, sizeof(sb->num-free, 1, fs);
	fread(sb->inode_start, sizeof(sb->inode_start), 1, fs);
	fread(sb->data_start, sizeof(sb->data_start), 1, fs);
	// Read Free Block data
	FreeBlock* cur = malloc(sizeof(FreeBlock));
	for (int i = 0; i < sb->num_free; i++) {
	}
 	
	// Read inode data



	die("In loadfilesys()");
	return 0;
}

// add file
void add_file(char* path) {
	
	// TEST
	die("in add_file()");
}

// remove file
void remove_file(char* path) {

	// TEST
	die("in remove_file()");
}

// Extract file
void extract_file(char* path) {

	// TEST
	die("in extract_file");
}
// list directory
void list_dir() {

	// TEST
	die("in list_dir()");
}
int main(int argc, char* argv[]) {
//	const int MAX_BLOCKS = (FS_SIZE - 
	char* cmd = NULL;
	char* path = NULL;
	char* fsfile = NULL;
	if (argc == 4) {
		cmd = strdup(argv[1]);
		fsfile = strdup(argv[3]); 
	} 
	else if (argc == 5) {
		cmd = strdup(argv[1]);
		path = strdup(argv[2]);
		fsfile = strdup(argv[4]);
	} 
	else {
		die("./filefs -cmd -f -fsfile OR ./filefs -cmd path -f filesys");
	}
	// TEST
//	printf("cmd = %s\t path = %s\t fsfile = %s\n", cmd, path, fsfile);
	
	// Try to open existing file system
	if(!(fs == fopen(fsfile, "rb+"))) {
		// DOES NOT EXIST: open new file system
		fs = fopen(fsfile, "wb+");
		ftruncate(fileno(fs), FS_SIZE);
		format_fs();	
	}
	else {
		load_filesys();	
	}

	load_filesys();
	
	if (strcmp(cmd, "-l") == 0) {
		list_dir(path);
	}
	else if (strcmp(cmd, "-a") == 0) {
		add_file(path);
	} 
	else if (strcmp(cmd, "-r") == 0) {
		remove_file(path);
	}
	else if (strcmp(cmd, "-e") == 0) {
		extract_file(path);
	}
	else {
		die("Command unknown");
	}

	
	return 0;
}

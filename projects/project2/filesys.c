#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define FS_SIZE (10 * 1024 * 1024) // 10MB
#define BLOCK_SIZE 512
#define MAX_BLOCKS (FS_SIZE / BLOCK_SIZE)
#define MAX_INODES 100
#define MAX_FILENAME 255

// file types
#define TYPE_FREE 0
#define TYPE_FILE 1
#define TYPE_DIR 2


//--------------------
// Structures 
//--------------------

// Struct for Superblock
typedef struct { 
	int total_blocks;
	int inode_start;
	int data_start;
} Superblock;

// Struct for Inode
typedef struct { 
	int used;
	int type;
	int size;
	int direct[100];
	int indirect;
} Inode;

// Struct for Directory Entry
typedef struct { 
	int inode;
	char name[MAX_FILENAME];
}DirEntry;


//--------------------
// Globals
//--------------------
FILE *fs; // Filesystem
Superblock sb; // Superblock
int* bitmap; // Free blocks
Inode *inodes; //Inodes


//--------------------
// Helpers
//--------------------

// Error handler: send message and exit
void die (const char *msg) {
	printf("Error: %s\n", msg);
	exit(1);
}

// read from filesystem
void fs_read (void* buf, int block) {
	// Set file pointer. block * BLOCK_SIZE = offset
	fseek(fs, block * BLOCK_SIZE, SEEK_SET);
	fread(buf, BLOCK_SIZE, 1, fs); // Read to buf
}

// write to file system
void fs_write (void* buf, int block) {
	// Set file pointer 
	fseek(fs, block * BLOCK_SIZE, SEEK_SET);
	fwrite(buf, BLOCK_SIZE, 1, fs); // Write to location
}

int alloc_block() {

}

void free_block (int b) {

}

int alloc_inode() {
	for (int i = 0; i < MAX_INODES; i++) {
		if (!inodes[i].used) {
			inodes[i].used = 1;
			return i;
		}
	}

	// No inodes found
	die ("No free inodes");
	return -1;
}


//--------------------
// Formatting
//--------------------
// Used when initializing the file system
void format_fs() {
	// Set values of superblock members
	sb.total_blocks = MAX_BLOCKS;
	sb.inode_start = 1;
	sb.data_start = 1 + (MAX_INODES * sizeof(Inode))/BLOCKSIZE + 1;
	
	// Reserve memmory for Free Blocks and Inodes
	bitmap = calloc(sb.total_blocks, 1);
	inodes = calloc(MAX_INODES, sizeof(Inode);

	// Root directory
	int root = alloc_inode(); // Allocate ROOT INODE
	inodes[root].type = TYPE_DIR;
	inodes[root].size = 0;

	// Write structures
	fs_write(&sb, 0); // Write SUPERBLOCK at start
	fs_write(bitmap, 1); // Write BITMAP
	fseek(fs, sb.inode_start * BLOCK_SIZE, SEEK_SET); // File pointer -> IDONE start
	fwrite(inodes, sizeof(Inode), MAX_INODES, fs); // Write Inode table
}


//--------------------
// Main
//--------------------

int main (int argc, char* argv[]) {
	// FINISH: variable length input code
	//--------------------
	if (argc < 4) die ("Use: ./filsys -x path -f file\n");
	
	if (argc == 4) {
		char* 
	}
	// Grab command, path, and file
	char* cmd = argv[1];
	char* path = argv[2];
	char *file = argv[4];
	
	//--------------------
	// Atempt to open existing file for read/write in binary
	if (!(fs = fopen(file, "rb+"))) { 
		// File DOES NOTE exist. Create file.
		fs = open(file, "wb+");
		ftruncate(fileno(fs), FS_SIZE);
		format_fs(); // Formating function call			// FINISH 
	}



	return 0;
}

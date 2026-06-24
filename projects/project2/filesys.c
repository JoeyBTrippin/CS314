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
typedef struct {
	int total_blocks;
	int inode_start;
	int data_start;
} Superblock;

typedef struct {
	int used;
	int type;
	int size;
	int direct[100];
	int indirect;
} Inode;

typedef struct {
	int inode;
	char name[MAX_FILENAME];
}DirEntry;


//--------------------
// Globals
//--------------------
FILE *fs;
Superblock sb;
int* bitmap;
Inode *inodes;


//--------------------
// Helpers
//--------------------

// Used to exit if an error in encountered
void die (const char *msg) {
	printf("Error: %s\n"), msg);
	exit(1);
}

void fs_read (void* buf, int block) {

}

void fs_write (void* buf, int block) {

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

void format_fs() {
	// Initialize Superblock
	sb.total_blocks = MAX_BLOCKS;
	sb.inode_start = 1;
	sb.data_start = 1 + (MAX_INODES * sizeof(Inode))/BLOCKSIZE + 1;
	
	// Allocate memmory
	bitmap = calloc(sb.total_blocks, 1);
	inodes = calloc(MAX_INODES, sizeof(Inode);

	// Root directory
	int root = alloc_inode(); // Allocate ROOT INODE
	inodes[root].type = TYPE_DIR;
	inodes[root].size = 0;

	// Write structures
	fs_write(&sb, 0); // Write SUPERBLOCK at start		FINISH fs_write()
	fs_write(bitmap, 1); // Write BITMAP
	fseek(fs, sb.inode_start * BLOCK_SIZE, SEEK_SET); // File pointer -> IDONE start
	fwrite(inodes, sizeof(Inode), MAX_INODES, fs); // Write Inode table
}


//--------------------
// Main
//--------------------

int main (int argc, char* argv[]) {
	if (argc < 4) die ("Use: ./filsys -x path -f file\n");
	
	// Grab command, path, and file
	char* cmd = argv[1];
	char* path = argv[2];
	char *file = argv[3];
	
	// Atempt to open existing file for read/write in binary
	if (!(fs = fopen(file, "rb+"))) { 
		// File DOES NOTE exist. Create file.
		fs = open(file, "wb+");
		ftruncate(fileno(fs), FS_SIZE);
		format_fs(); // Formating function call			// FINISH 
	}

	return 0;
}

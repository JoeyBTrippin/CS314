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

}


int main (int argc, char* argv[]) {
	
	return 0;
}

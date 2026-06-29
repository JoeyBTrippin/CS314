#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

// CONSTANTS A
#define FS_SIZE (10 * 1024 * 1024) // 10 MB
#define BLOCK_SIZE 512
#define MAX_INODES 128
#define MAX_BLOCKS (FS_SIZE / BLOCK_SIZE)
#define MAX_FILENAME 255
#define MAX_DIRECT 100
#define MAX_FREEBLOCKS 250

// FILE TYPES
#define TYPE_FREE 0
#define TYPE_FILE 1
#define TYPE_DIR 2

#define FREE 0
#define USED 1


// STRUCTS
typedef struct {
	char data[BLOCK_SIZE];
} Block;

typedef struct{
	unsigned int total_blocks;
	int num_free;
	int size;
	int inode_start;
} Superblock;

typedef struct {
	char name[MAX_FILENAME];
	char free;
	char type;
	char size;
   char direct[MAX_DIRECT];
	int indirect;
} Inode;

typedef struct {
	char name[MAX_FILENAME];
	unsigned int size;
	unsigned int firstblock;
	unsigned int type;
} DirEntry;

// GLOBALS
FILE* fs = NULL;
Superblock sb;
Inode inodes[MAX_INODES];
char bitmap[MAX_BLOCKS];

// CONSTANTS B 
//#define BMAP_BLOCKS (sizeof(bitmap) / BLOCK_SIZE)
//#define MAX_DATA_BLOCKS (MAX_BLOCKS - (1 + BMAP_BLOCKS + MAX_INODES))


//----------------------------------------
// ERROR TESTING
//----------------------------------------
// Error protocol
void die(const char* msg) {
	fprintf(stderr, "%s\n", msg);
	exit(1);
}


//----------------------------------------
// HELPER FUNCTIONS
//----------------------------------------
// set file pointer given block number
void set_file_pointer (int block_index) {
	if (fseek(fs, block_index * BLOCK_SIZE, SEEK_SET) !=0)
		die("Failure in setting file pointer");
}


// Check file bounds
void check_fs_bounds () {
	if ( (FS_SIZE - ftell(fs)) > BLOCK_SIZE )
		die("Block limit reached");
}


// find free Inode
int find_free_inode() {
	for (int i = 0; i < MAX_INODES; i++) {
		if (inodes[i].free == FREE)
			return i;
	}
	die("No more Inodes available");
	return -1;		
}


// find an inode given file name
int find_inode (char* file_name) {
	for (int i = 0; i < MAX_INODES; i++) {
		if ((int*)inodes[i].name == (int*)file_name)
			return i;
	}
	return -1;
}


//
int  allocate_blocks() {
	for (int i = 0; i < MAX_BLOCKS; i++) {
		if (bitmap[i] == FREE) {
			bitmap[i] = USED;
			printf("block found: %d\n", i);
			return i;
		}
	}
	die("No free blocks");
	return -1;
}

// print block
void print_block(int block) {
	char* buffer = calloc(1, BLOCK_SIZE);
	set_file_pointer(block);
	fread(buffer, 1, BLOCK_SIZE, fs);
	printf("%s", buffer);
}

// Search through directory
int search_dir(char* filename, int node) {
	int cur_inode = -1;
	for (int i = 0; i < 100; i++) {
		cur_inode = inodes[i].direct[i];
		if (strcmp(inodes[cur_inode].name, filename)) 
			return i;
	}

	return 0;

}

// Write external file to filesystem
int write_file(char* filename, int node) {
	
	// find inode
	if ( node < 0  )
		node = find_free_inode();

	// test
	printf("filename = %s   inode = %d\n", filename, node);
	
	// set node files
	printf("read_file() enter\n");
	strncpy(inodes[node].name, filename, MAX_FILENAME);
	inodes[node].free = USED;
	inodes[node].type = TYPE_FILE;
	
	// open file
	FILE* file;
	if (!(file = fopen(filename, "rb"))) {
		die("file could not be found");
	}
	// incrementals
	int size = 0; 
	int direct_index = 0;
	// block location tracer
	int block_index = 0;
	// buffer for reading and writing
	char* buffer = calloc(1, BLOCK_SIZE);
	// read from external file
	//int read = 0;
	while (fread(buffer, 1, BLOCK_SIZE, file) > 0) {
		// find empty block
		block_index = allocate_blocks();
		// TEST
		printf("buffer = %s\n", buffer);
		// write to empty block
		set_file_pointer(block_index);
		//fseek(fs, block_index * BLOCK_SIZE, SEEK_SET);
		fwrite(buffer, BLOCK_SIZE, 1, fs);
		print_block(block_index);
		// set direct block location
		inodes[node].direct[direct_index] = block_index;
		// increment direct block
		direct_index++;
	}
	inodes[node].size = size;
	free(buffer);
	
	fclose(file);
	
	return node;
	printf("read_file() exit\n");
}


//----------------------------------------
// PRIMARY FUNCTIONS
//----------------------------------------

// Initialize new file system
void new_filesys() {
	// TEST
	printf("new_filesys() enter\n");
	sb.total_blocks = MAX_BLOCKS;
	
	int bitmap_blocks = (sizeof(bitmap) + BLOCK_SIZE - 1) / BLOCK_SIZE; // protects from overflow
	// int inode_blocks = (sizeof(inodes) + BLOCK_SIZE - 1) / BLOCK_SIZE; 
	int blocks_used = 1 + bitmap_blocks + MAX_INODES;
	
	// Init Super block values
	sb.num_free = MAX_BLOCKS - blocks_used;
	sb.size = blocks_used;
	sb.inode_start = 1 + bitmap_blocks;

	// Write root inode
	strcpy(inodes[1].name, "root");
//	inodes[1].name = "root";
	inodes[1].free = USED;
	inodes[1].type = TYPE_DIR;
	inodes[1].size = 0;
	// Set all other inodes ot free
	for (int i = 1; i < MAX_INODES; i++)
		inodes[i].free = FREE;

	// Set everything in bitmap to free
	for (int i = 0; i < MAX_BLOCKS; i++)
		bitmap[i] = FREE;
	// Set used bitmap
	for (int i = 0; i < blocks_used; i++)
		bitmap[i] = USED;
	printf("new_filesys(): %d blocks used, %d free\n", blocks_used, sb.num_free);
	
	// TEST
	printf("sb.max blocks = %d = %d = MAX_BLOCKS\n", sb.total_blocks, MAX_BLOCKS);	
	printf("bitmap blocks = %d\n", bitmap_blocks);
	printf("blocks used %d = %d \n", blocks_used, 1 + MAX_INODES + bitmap_blocks);
	printf("new_filesys() exit\n");
}


// Load existing file system
void load_filesys() {
	
	// Read Superblock data
	if (fread(&sb, BLOCK_SIZE, 1, fs) != 1)
		die("reading Superblock in load_filsys()");
	
	// Initialize bitmap
	if (fread(bitmap, MAX_BLOCKS, 1, fs) != 1)
		die("reading bitmap in load_filesys()");

	// read inodes
	for (int i = 0; i < MAX_INODES; i++) {
		set_file_pointer(1 + sizeof(bitmap) + i);
		//inodes[i] = calloc(1, sizeof(inodes[i]));
		
	}
} 	


// add file
void add_file(int parent_inode, char* path) {
	printf("add_file() enter: path = %s\n", path); // TEST
	if (path[0] != '/')
		die("/path\n");
	char* token = strtok(path, "/");
	
	while (token != NULL) {
		int cur_inode = 0;
		char* next = strtok(NULL, "/"); // check for last file
		if (cur_inode = search_dir(token, parent_inode)) { // Found in directory
				// TEST
				printf("Found in directory\n");
			if ( next == NULL ) { // last of path, must be file
				// TEST
				printf("Last of path\n");

				if (inodes[cur_inode].free == FREE) { // check if file free
					// TEST
					printf("File is free\n");

					write_file(token, cur_inode);
					return;
				}
				
				// file wasn't free
				printf("Cannot override file\n");
				return;
			
			}

			// TEST
			printf("progressed through path\n");
			
			// more objects on path, progess along path
			parent_inode = cur_inode;
		
		} else { // NOT found in directory
			//TEST
			printf("Not foudn in directory\n");

			if (next == NULL) { // last of path, add inode to direct and write file
				// TEST
				printf("Last of path\n");
				
				// find open direct map
				int direct = 0;
				if ( (direct = inodes[parent_inode].size) > 100 )
					die( strcat(inodes[parent_inode].name, " full") );
				
				inodes[parent_inode].direct[direct] = write_file(token, -1);
				

				//cur_inode = find_free_inode();
			}	
					

		}

		return;
	}
			

	
	/*
	while (token != NULL) {
//	printf("%s\n", token);
		char* next = strtok(NULL, "/");
		// search for file 
		if (next == NULL) { // last file on path 
			int node_index = find_inode(token);
			if (node_index < 0) { // does not have a node
				node_index = find_free_inode();
					
				write_file(token, node_index);
					return;
			} else {
				// current file is a directory								FINISH
				int node_index = find_inode(token);
				if (node_index < 0) { // directory not in file
			
					return;
				}
			}	

		} else{


		}
		token = next;
	}

	// TEST
	die("in add_file()"); 
	*/

	printf("add_file() exit\n"); // TEST
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
void list_dir(char* path) {
	
	// TEST
	die("in list_dir()");
}




int main(int argc, char* argv[]) {
//	const int MAX_BLOCKS = (FS_SIZE - 
	
	if ( !(fs = fopen(argv[argc-1], "rb+")) ) {
		fs = fopen(argv[argc-1], "wb+");
		ftruncate(fileno(fs), FS_SIZE);
		new_filesys();
	} else {
		load_filesys();
	}

	char* cmd = argv[1];
	if (strcmp(cmd, "-a") == 0 ){
		printf("case 'a' selected\n");
		add_file(0 ,argv[2]);
	}
	

	printf("\nReturn to main\n");
	printf("size of sb = %zu\n", sizeof(sb));
	printf("Size of one bitmap = %zu\n", sizeof(bitmap[1]));
	printf("size of bitmap = %zu\n", sizeof(bitmap));
	printf("Size of one inode = %zu\n", sizeof(inodes[1]));
	printf("sb.inode_start = %d\n", sb.inode_start);
	printf("size of inodes = %zu\n", sizeof(inodes));

	// Write Superblock, bitmap, and Inode to file system
		set_file_pointer(0);
		fwrite(&sb,sizeof(sb), 1, fs);
		printf("Super block written\n");
		set_file_pointer(1);
//		for (int i =0; i < MAX_BLOCKS; i++) 
			fwrite(bitmap, sizeof(bitmap), 1, fs);
		printf("bitmap written\n");
		//set_file_pointer(sb.inode_start);
		for (int i = 0; i < MAX_INODES; i++) {
			set_file_pointer(sb.inode_start + i);
			fwrite(&inodes[i], sizeof(inodes[i]), 1, fs);
			printf(" inodes # %d written ", i);
			//set_file_pointer(sb.inode_start + i);
		}

	// TEST
	for (int i = 0; i < 10; i++) {
		set_file_pointer(0);	
		fread(stdout, 1, BLOCK_SIZE, fs);
	}

	fclose(fs);
	return 0;
}

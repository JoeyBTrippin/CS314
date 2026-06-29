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
	int data_start;
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

void check_sb () {
	printf("CHECK SUPERBLOCK:\n");
	printf("total blocks = %d   num_free = %d   ", sb.total_blocks, sb.num_free);
	printf("size = %d   inode_start = %d   ", sb.size, sb.inode_start);
	printf("data start = %d\n", sb.data_start);

}

void check_bitmap(int start, int stop) {
	printf("CHECK BITMAP\n");
	for (int i = start; i < stop; i++)
		printf(" %d ", bitmap[i]);
	printf("\n");
}

void check_inodes(int start, int stop, int d) {
	printf("INODE CHECK\n");
	for (int i = start; i < stop; i++) {
		printf("#%d: name = %s   free = %d   ", i, inodes[i].name, inodes[i].free);
		printf("type = %d   size = %d\n", inodes[i].type, inodes[i].size);
		if (d > 0 ) {
		printf("direct:");
			for (int j = 0; j < 100; j++)
				printf(" %d ", inodes[i].direct[j]);
			printf("indirect = %d", inodes[i].indirect);
		}
	}
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

/* INODE HELPER */

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


/* BLOCK HELPERS */

// allocate a free block	
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


/* DIRECTORY HELPERS */

// Search through directory for name
int search_dir(char* filename, int parent) {
	int cur = -1;
	for (int i = 0; i < 100; i++) {
		cur = inodes[parent].direct[i];
		if (strcmp(inodes[cur].name, filename) == 0) 
			return cur;
	}

	return -1;

}


/* FILE HELPERS */

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
//	sb.num_free = MAX_BLOCKS - blocks_used;
//	sb.size = blocks_used;
//	sb.inode_start = 1 + bitmap_blocks;
//	sb.data_start = 1 + bitmap_blocks + MAX_INODES;

	// Write root inode
	strcpy(inodes[0].name, "root");
//	inodes[1].name = "root";
	inodes[0].free = USED;
	inodes[0].type = TYPE_DIR;
	inodes[0].size = 0;
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

	check_sb();
	check_inodes(0, 10, 0);
	check_bitmap(0, 190);
	// TEST
//	printf("sb.max blocks = %d = %d = MAX_BLOCKS\n", sb.total_blocks, MAX_BLOCKS);	
//	printf("bitmap blocks = %d\n", bitmap_blocks);
//	printf("blocks used %d = %d \n", blocks_used, 1 + MAX_INODES + bitmap_blocks);
//	printf("inode start %d = %d\n", sb.inode_start, blocks_used - MAX_INODES);
//	printf("data start %d = %d\n", sb.data_start, blocks_used);
//	for (int i =0; i < blocks_used + 1; i++)
//		printf(" %d ", bitmap[i]);
//	printf("\nnew_filesys() exit\n");
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
	int cur_inode = 0;
	int dir = -1;
	while (token != NULL) {
		char* next = strtok(NULL, "/"); // check for last file
		if ( (dir = search_dir(token, parent_inode)) > -1) { // Found in directory
				// test
				printf("Found in directory\n");
				printf("dir = %d\n", dir);
				cur_inode = inodes[parent_inode].direct[dir];
				// TEST
			if ( next == NULL ) { // last of path, must be file
				// TEST
				printf("Last of path\n");
					
				if (inodes[cur_inode].type  == TYPE_DIR)  // check file is directory
					die("Cant over write a directory with a file\n");
				
				if (inodes[cur_inode].free == USED) // check file is free
					die("Cannot over write file.");
					
				// TEST
				printf("File is free\n");
				// TYPE = file and FREE. Can write to file
				write_file(token, cur_inode);
				// update superblock
				sb.num_free--;
				sb.size++;
				// update parent node
				inodes[parent_inode].size++;
				return;
			
			} else{ // not last in path, must be a directory

			}

			// TEST
			die("Nothing done");	
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
				// Update Superblock
				sb.num_free--;
				sb.size++;
				// Update parent Inode
				inodes[parent_inode].size++;
				return;
			}	
					

		}
		die("NO PATH TAKEN in add_file()\n");
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
	check_sb();
	check_bitmap(0, 190);
	check_inodes(0,3,1);
	// Write Superblock, bitmap, and Inode to file system
		set_file_pointer(0);
		fwrite(&sb,sizeof(sb), 1, fs);
//		printf("Super block written\n");
		set_file_pointer(1);
//		for (int i =0; i < MAX_BLOCKS; i++) 
			fwrite(bitmap, sizeof(bitmap), 1, fs);
//		printf("bitmap written\n");
		//set_file_pointer(sb.inode_start);
		for (int i = 0; i < MAX_INODES; i++) {
			set_file_pointer(sb.inode_start + i);
			fwrite(&inodes[i], sizeof(inodes[i]), 1, fs);
//			printf(" inodes # %d written ", i);
			//set_file_pointer(sb.inode_start + i);
		}

	// TEST
	char buffer[BLOCK_SIZE];
	set_file_pointer(sb.inode_start + MAX_INODES);
	for (int i = 0; i < 10; i++) {
		printf("\npointer at %ld\n", ftell(fs));
		fread(&buffer, 1, BLOCK_SIZE, fs);
		printf("%s", buffer);
	}

	fclose(fs);
	return 0;
}

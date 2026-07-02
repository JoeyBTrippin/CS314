#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// #include <math.h>

// CONSTANTS
#define FS_SIZE (10 * 1024 * 1024) // 10 MB
#define BLOCK_SIZE 512
#define MAX_INODES 128
#define MAX_BLOCKS (FS_SIZE / BLOCK_SIZE)
#define MAX_FILENAME 255
#define MAX_DIRECT 60
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
	char type;
	char size;
   int direct[MAX_DIRECT];
	int indirect;
} Inode;


// GLOBALS
FILE* fs = NULL;
Superblock sb;
Inode inodes[MAX_INODES];
char bitmap[MAX_BLOCKS];


// PROTOTYPES NEEDED
void free_file(int node); 


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
		printf("#%d: name = %s   ", i, inodes[i].name);
		printf("type = %d   size = %d\n", inodes[i].type, inodes[i].size);
		if (d > 0 ) {
		printf("direct:");
			for (int j = 0; j < MAX_DIRECT; j++)
				printf(" %d ", inodes[i].direct[j]);
			printf("indirect = %d", inodes[i].indirect);
		}
		printf("\n");
	}
}


//----------------------------------------
// HELPER FUNCTIONS
//----------------------------------------

/* FILE SYSTEM */
// set file pointer given block number
void set_file_pointer (int block_index) {
	if (fseek(fs, block_index * BLOCK_SIZE, SEEK_SET) !=0)
		die("Failure in setting file pointer");
}



/* INODE HELPER */

// find free Inode
int find_free_inode() {
	for (int i = 0; i < MAX_INODES; i++) {
		if (inodes[i].type == TYPE_FREE)
			return i;
	}
	die("No more Inodes available");
	return -1;		
}


// find an inode given file name
int find_inode (char* file_name) {
	for (int i = 0; i < MAX_INODES; i++) {
		if (strcmp(inodes[i].name, file_name) == 0)
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
//			printf("block found: %d\n", i);
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
	fread(buffer, BLOCK_SIZE, 1, fs);
	fwrite(buffer, 1, BLOCK_SIZE, stdout);
	//printf("%s", buffer);
}

void free_block(int block) {
	// locate file
	set_file_pointer(block);
	fwrite("", 1, BLOCK_SIZE, fs);

	bitmap[block] = FREE;
}


/* DIRECTORY HELPERS */


// Search through directory for name
int search_dir(char* filename, int parent) {
	int cur = 0;
	for (int i = 0; i < MAX_DIRECT; i++) {
		cur = inodes[parent].direct[i];
		// protections check
		if (cur <= 0) continue;
		if (cur >= MAX_INODES) continue;
		if (inodes[cur].type == TYPE_FREE) continue;
		
		// If we have found this file, return dir location
		if (strcmp(inodes[cur].name, filename) == 0) 
			return i;
	}
	// Not found
	return -1;

}

int search_open_dir(int parent) {
	for (int i = 0; i < MAX_DIRECT; i++) {
		if (inodes[parent].direct[i] == 0)
			return i;
		
	}
	die( strcat(inodes[parent].name, " full") );
	return -1;
}


int add_dir(char* name, int parent) {
//	printf("Enter: add_dir()\n");	
	
	for (int i = 0 ; i < MAX_DIRECT; i++) {
		if (inodes[parent].direct[i] == 0) {
//			printf("direct spot found %d\n", i);
			// set up new Inode
			int new_node = find_free_inode();
			strcpy(inodes[new_node].name, name);
			inodes[new_node].type = TYPE_DIR;
			
			// update parent
			inodes[parent].size++;
			inodes[parent].direct[i] = new_node;
			return new_node;
		}
	}

	die(strcat(inodes[parent].name, " directory is full"));
	 return -1;
}


int is_empty_dir(int node) {
// printf("Enter: is_empty_dir()\n");	
	if (inodes[node].type != TYPE_DIR)
		return 0;
	
	for (int i = 0; i < MAX_DIRECT; i++) {
		int child = inodes[node].direct[i];
		if ( (child > 0) && (child < MAX_INODES) && (inodes[child].type != TYPE_FREE) ) {		
			// Found a child 
			return 0;
		}
	}

	// NO child found
	return 1;
}


void free_directory(int node) {
	// Clear direct links
	for (int i = 0; i < MAX_DIRECT; i++) {
		int child = inodes[node].direct[i];
		
		// check bound
		if ( (child <= 0) || (child >= MAX_INODES) )
			continue;
		
		// if free, skip
		if (inodes[child].type == TYPE_FREE)
			continue;
		// if directory, recursion
		if (inodes[child].type == TYPE_DIR)
			free_directory(child);
		else
			free_file(child);

		inodes[node].direct[i] = 0;
	}

	// free the directory
	strcpy(inodes[node].name, "");
	inodes[node].type = TYPE_FREE;
	inodes[node].size = 0;
}

/* FILE HELPERS */

// Write external file to filesystem
int write_file(char* filename, int node) {
//	printf("Enter write_file\n");	
	
	// find inode
	if ( node < 0  )
		node = find_free_inode();

//	printf("filename = %s   inode = %d\n", filename, node);
	
	// set node files
	strncpy(inodes[node].name, filename, MAX_FILENAME);
	inodes[node].type = TYPE_FILE;
	
	// open file
	FILE* file;

	if (!(file = fopen(filename, "rb"))) {
//		printf("%s\n", filename);
		die(strcat(filename, " could not be found"));
	}
	// incrementals
	int direct_index = 0;
	// block location tracer
	int block_index = 0;
	// buffer for reading and writing
	char* buffer = calloc(1, BLOCK_SIZE);
	// read from external file
	while (fread(buffer, 1, BLOCK_SIZE, file) > 0) {
		// all indirect blocks have been used
		if (direct_index > 99) {
			inodes[node].indirect = write_file(NULL, -1);
			return node;
		}
		// find empty block
		block_index = allocate_blocks();
		
//		printf("buffer = %s\n", buffer);
	
		// write to empty block
		set_file_pointer(block_index);
		fwrite(buffer, BLOCK_SIZE, 1, fs);
//		print_block(block_index);
		
		//set direct block location
		inodes[node].direct[direct_index] = block_index;
		// Increment size of node.
		inodes[node].size++;
		// increment direct block
		direct_index++;
	}
	free(buffer);
	
	fclose(file);
	
	return node;

//	printf("read_file() exit\n");
}


void free_file(int node) {
//	printf("free_file() enter\n");	
	for (int i = 0; i < MAX_DIRECT; i++) {
		// block mapped to direct map
		int d = inodes[node].direct[i];
		// block was mapped 
		if (d > 0) {
			// free current block
			free_block(d);

			// Update Inode size and direct map
			inodes[node].size--;
			inodes[node].direct[i] = FREE;

		}
	}
		// indirect map
		int id = inodes[node].indirect;
		if (id != FREE) {
			free_file(id);
			inodes[node].indirect = FREE;
		}

	// Clean Inode
	strcpy(inodes[node].name, "");
	inodes[node].type = TYPE_FREE;
	inodes[node].size = 0;	

//	printf("free_file() Exit\n");	
//	check_sb();
//	check_inodes(0, 10, 0);
//	check_bitmap(0, 190);
	
	return;
}

//----------------------------------------
// PRIMARY FUNCTIONS
//----------------------------------------

// Initialize new file system
void new_filesys() {
//	printf("new_filesys() enter\n");
	
	int bitmap_blocks = (sizeof(bitmap) + BLOCK_SIZE - 1) / BLOCK_SIZE; // protects from overflow
	int blocks_used = 1 + bitmap_blocks + MAX_INODES;
	
	// Init Super block values
	sb.total_blocks = MAX_BLOCKS;
	sb.num_free = MAX_BLOCKS - blocks_used;
	sb.size = blocks_used;
	sb.inode_start = 1 + bitmap_blocks;
	sb.data_start = 1 + bitmap_blocks + MAX_INODES;

	// Write root inode
	strcpy(inodes[0].name, "root");
	inodes[0].type = TYPE_DIR;
	inodes[0].size = 0;
	// Set all other inodes ot free
	for (int i = 1; i < MAX_INODES; i++)
		inodes[i].type = TYPE_FREE;

	// Set everything in bitmap to free
	for (int i = 0; i < MAX_BLOCKS; i++)
		bitmap[i] = FREE;
	// Set used bitmap
	for (int i = 0; i < blocks_used; i++)
		bitmap[i] = USED;

//	printf("new_filesys(): %d blocks used, %d free\n", blocks_used, sb.num_free);

//	check_sb();
//	check_inodes(0, 10, 0);
//	check_bitmap(0, 190);
}


// Load existing file system
void load_filesys() {
//	printf("Enter load_filesys()\n");	
	
	// Read Superblock data
	if (fread(&sb, sizeof(sb), 1, fs) != 1)
		die("reading Superblock in load_filsys()");
	
	// Initialize bitmap
	set_file_pointer(1);
	if (fread(bitmap, sizeof(bitmap), 1, fs) != 1)
		die("reading bitmap in load_filesys()");

	// read inodes
	set_file_pointer(sb.inode_start);
	for (int i = 0; i < MAX_INODES; i++) {
		set_file_pointer(sb.inode_start + i);
		if (fread(&inodes[i], sizeof(inodes[i]), 1, fs) !=1)
			die("Failed to read inpde\n");
	}
	
//	check_sb();
//	check_bitmap(0,190);
//	check_inodes(0,5,1);
//	printf("Exit load_filesys()\n");
} 	


// add file
void add_file(int parent_inode, char* path) {
//	printf("add_file() enter: path = %s\n", path); // TEST
	if (path[0] != '/')
		die("/path\n");
	char* token = strtok(path, "/");
	int cur_inode = 0;
	int dir = -1;
	while (token != NULL) {
		char* next = strtok(NULL, "/"); 
		if ( (dir = search_dir(token, parent_inode)) > -1) { // Found in directory
//			printf("Found in directory\n");
//			printf("dir = %d\n", dir);
			
			// Grab Inode
			cur_inode = inodes[parent_inode].direct[dir];

//				printf("cur_inode = %d\n", cur_inode);	
			
			if ( next == NULL ) { 
//				printf("Found & Last in path\n");
				
				// Found and last in path, CANNOT OVERWRITE
				die("File already exist: cannot overwrite");

			} else { // not last in path, must be a directory
//				printf("entering file %s", token);
			
				// only step into directories
				if (inodes[cur_inode].type != TYPE_DIR)
					die("Path object is not a directory");

				parent_inode = cur_inode;
				token = next;
				continue;
			}

//			die("Nothing done");	
//			continue;
		
		} else { // NOT found in directory
//			printf("Not found in directory\n");

			if (next == NULL) { // last of path, add inode to direct and write file
//				printf("Last in path\n");
						
				// find open direct map
				int direct = search_open_dir(parent_inode);
						
				inodes[parent_inode].direct[direct] = write_file(token, -1);
				
				// Update Superblock
				sb.num_free--;
				sb.size++;
				// Update parent Inode
				inodes[parent_inode].size++;
				
				
				return;
			} else { // NOT found. NOT last in path. Must be DIRECTORY
				// MAP INODE TO DIRECT ARRAY OF CURRENT
				parent_inode = add_dir(token,parent_inode);
				token = next;
				continue;
					
			}
		}

//		die("NO PATH TAKEN in add_file()\n");
			return;
	}
	
}

// remove file
void remove_file(char* path) { 
//	printf("remove_file enter: %s\n", path);

	// tracking for deletion
	int path_nodes[MAX_INODES];
	int path_dirs[MAX_INODES];
	int depth = 0;

//	if (path[0] != '/')
//		die("/path\n");
	
	// first item off path
	char* token = strtok(path, "/");
	// start at root Inode
	int cur_node = 0;
	
	// Following path 
	while(token != NULL) {
		// store current node
 		path_nodes[depth] = cur_node;

		
		// search directory for current token
		int dir = search_dir (token, cur_node);
		if (dir < 0)  
			die( strcat(token, "could not be located") );
	
		// Update tracker
		path_dirs[depth] = dir;
		depth++;
		
		// Move to child
		cur_node = inodes[cur_node].direct[dir];
		
		// Grab next on path
		token = strtok(NULL, "/");
	}
 	// found file/directory to be removed
	int target = cur_node;
	int parent = path_nodes[depth -1];
	int parent_dir_index = path_dirs[depth -1];
	
	// Delete target
	if (inodes[target].type == TYPE_DIR)
		free_directory(target);
	else
		free_file(target);

	// Remove target entry from parent directory
	inodes[parent].direct[parent_dir_index] = 0;
	inodes[parent].size--;
	
	// Run back up path to delete empty directories
	for (int i = depth-1; i >=0; i--) {
		int node = path_nodes[i];

		if (node == 0) // never delete root
			break;
		
		// A non-empty directory is found. STOP
		if (!is_empty_dir(node))
			break;
		
		// Directory is empty and not root. DELETE
		free_directory(node);

		// Remove from parent
		int parent_node = path_nodes[i-1];
		for (int j = 0; j < MAX_DIRECT; j++) {
			if (inodes[parent_node].direct[j] == node) {
				inodes[parent_node].direct[j] = 0;
				inodes[parent_node].size--;
				break;
			}
		}
	}

//	printf("remove_file exit\n");
}







// Extract file
void extract_file(char* path) {
//	printf("Extract_file() enter\n");
	
	if (path[0] != '/')
		die("/path\n");
	// read first object on path
	char* token = strtok(path, "/");
	int cur = 0;

	while (token != NULL) {
	int dir = search_dir(token, cur);
	if(dir < 0)
		die(strcat(token, " not found"));
	
	cur = inodes[cur].direct[dir];
	token = strtok(NULL, "/");

	}

	// Inode found
	if (inodes[cur].type != TYPE_FILE)
		die("extract_file: not found");
	
	// Read blocks
	for (int i = 0; i < inodes[cur].size; i++) {
		int block = inodes[cur].direct[i];
		if (block <= 0)
			break;
		
		char buffer[BLOCK_SIZE];
		set_file_pointer(block);
		int n = fread(buffer, 1, BLOCK_SIZE, fs);

		// Write nonNULL bytes
		int j = 0;
		while ( (j < n) && (buffer[j] != '\0') )
			j++;
		fwrite(&buffer, 1, j, stdout);
		
	}

//	printf("extract_file() exit\n");
}
// list directory
void list_dir(int node, int depth) {
	if (inodes[node].type == TYPE_FREE) // if free, return 
		return;

	int cur = -1;	
	for ( int i = 0; i < MAX_DIRECT; i++){
		cur = inodes[node].direct[i];
		// empty so skip
		if (cur <= 0)
			continue;
		// garbage check
		if (cur >= MAX_INODES)
			continue;
		// check current inode exist
		if (inodes[cur].type == TYPE_FREE)
			continue;
		
		// Identation
		for(int j = 0; j < depth; j++)
			printf("\t");

		// print name
		printf("%s\n", inodes[cur].name);

		// if is directory, recursive call
		if (inodes[cur].type == TYPE_DIR) { // step in
			list_dir(cur, depth + 1);
		}

	}

	return;
// TEST
//	die("in list_dir()");
}




int main(int argc, char* argv[]) {

	// argument too short
	if(argc < 3) {
		die("Usage:\n"
			"\tfilefs -a /path/to/file -f filesystem\n"
			"\tfilefs -r /path/to/file -f filesystem\n"
			"\tfilefs -e /path/to/file -f filesystem\n"
			"\tfilefs -l -f filesystem");
	}

	// Grab and check command
	char* cmd = argv[1];
	if ( (strcmp(cmd, "-a") != 0) &&
		  (strcmp(cmd, "-r") != 0) &&
		  (strcmp(cmd, "-e") != 0) &&
		  (strcmp(cmd, "-l") != 0) ){
		die("Invalid command. Valid: -a, -r, -e, -l");
	}

	// Validate -f
	if (strcmp(argv[argc-2], "-f") != 0)
		die("Missing -f flag before filesystem name");

	// grab filesystem name
	char* fs_name = argv[argc-1];
	if ( (fs_name == NULL) || strlen(fs_name) ==  0)
		die("Invalid filesystem name");

	// Open file system
	if ( !(fs = fopen(fs_name, "rb+")) ) {
		fs = fopen(fs_name, "wb+");
		ftruncate(fileno(fs), FS_SIZE);
		new_filesys();
	} else {
		load_filesys();
	}
	
	// Set path, when needed
	char* path = NULL;
	if (strcmp(cmd, "-l") != 0) {
		path = argv[2];
		
		// Validate path
		if (path == NULL)
			die("Missing path argument");
		if (path[0] != '/')
			die("Path must begin with '/'");
	}

	// Execute command
	if (strcmp(cmd, "-a") == 0 ){
//		printf("case 'a' selected\n");
		add_file(0 ,path);
	} else if(strcmp(argv[1], "-r") == 0 ) {
//		printf("case 'r' selected\n");
		remove_file(path);
	} else if(strcmp(argv[1], "-e") == 0) {
//		printf("case 'e' selected\n");
		extract_file(path);
	} else if(strcmp(argv[1], "-l") == 0) {
//		printf("case 'l' selected\n");
		list_dir(0, 0);
	}
	
// TEST CODE
//	clean_inodes();
//	printf("\nReturn to main\n");
//	check_sb();
//	check_bitmap(0, 190);
//	check_inodes(0,5,0);


	// Write Superblock, bitmap, and Inode to file system
	set_file_pointer(0);
	fwrite(&sb,sizeof(sb), 1, fs);
//	printf("Super block written\n");
	set_file_pointer(1);
	fwrite(bitmap, sizeof(bitmap), 1, fs);
//	printf("bitmap written\n");
	for (int i = 0; i < MAX_INODES; i++) {
		set_file_pointer(sb.inode_start + i);
		fwrite(&inodes[i], sizeof(inodes[i]), 1, fs);
//		printf(" inodes # %d written ", i);
		}

/*
	// TEST
	char buffer[BLOCK_SIZE];
	set_file_pointer(sb.data_start);
	for (int i = 0; i < 5; i++) {
		printf("\npointer at %ld\n", ftell(fs) / BLOCK_SIZE);
		print_block(sb.data_start + i);
		printf("%s", buffer);
	}
*/
	fclose(fs);
	return 0;
}

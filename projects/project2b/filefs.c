#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


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

#define USED 0
#define FREE 1


// STRUCTS
typedef struct {
	char data[BLOCK_SIZE];
} Block;

typedef struct{
	unsigned int total_blocks;
	int num_free;
	int size;
} Superblock;

typedef struct {
	char name[MAX_FILENAME];
	int free;
	int type;
	int size;
	int direct[MAX_DIRECT];
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
unsigned char bitmap[MAX_BLOCKS];

// CONSTANTS B 
//#define BMAP_BLOCKS (sizeof(bitmap) / BLOCK_SIZE)
//#define MAX_DATA_BLOCKS (MAX_BLOCKS - (1 + BMAP_BLOCKS + MAX_INODES))


// Error protocol
void die(const char* msg) {
	fprintf(stderr, "%s\n", msg);
	exit(1);
}


// set file pointer given block number
void set_file_pointer (int block_index) {
	if (fseek(fs, block_index * BLOCK_SIZE, SEEK_SET) !=0)
		die("Failure in setting file pointer");
}


// Initialize new file system
void new_filesys() {
	// Initialize Superblcok
	sb.total_blocks = MAX_BLOCKS;
	sb.num_free = MAX_BLOCKS - 1;
	sb.size = 1;

	// initialize Inodes
	for (int i = 0; i < MAX_INODES; i++)
		inodes[i].free = FREE;
	
	// Initialize bitmap
	memset(bitmap, FREE, sizeof(bitmap));
	int blocks_used = 1 + MAX_INODES + sizeof(bitmap); 
	for (int i = 0; i < blocks_used; i++)
		bitmap[i] = USED;
	for (int i = blocks_used; i < MAX_BLOCKS; i++)
		bitmap[i] = FREE;
	printf("new_filesys()\n"); // TEST
}


// Load existing file system
void load_filesys() {
	
	// Read Superblock data
	
	if (fread(&sb, sizeof(BLOCK_SIZE), 1, fs) != 1)
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
			return i;
		}
	}
	die("No free blocks");
	return -1;
}


// Read file
void read_file(char* filename, int node) {
	// set node files
	printf("in read_file()\n");
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
	while (fread(buffer, BLOCK_SIZE, 1, file)) {
		// find empty block
		block_index = allocate_blocks();
		// write to empty block
		fseek(fs, block_index * BLOCK_SIZE, SEEK_SET);
		fwrite(buffer, BLOCK_SIZE, 1, fs);
		// set direct block location
		inodes[node].direct[direct_index] = block_index;
		// increment direct block
		direct_index++;
	}
	inodes[node].size = size;
	free(buffer);

	fclose(file);	
}

// Write directory
void write_directory () {

}

// add file
void add_file(char* path) {
	printf("in add_file()\n"); // TEST
	char* token = strtok(path, "/");
	
	while (token != NULL) {
//	printf("%s\n", token);
		char* next = strtok(NULL, "/");
		// search for file 
		if (next == NULL) { // last file on path 
			int node_index = find_inode(token);
			if (node_index < 0) { // does not have a node
				node_index = find_free_inode();
					
				read_file(token, node_index);
					return;
			} else {
				// current file is a directory								FINISH
				int node_index = find_inode(token);
				if (node_index < 0) { // directory not in file
			

				}
			}	

		}
		token = next;
	}

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
		add_file(argv[argc-1]);
	}
	/*
	long opt
	int  opt =(int) argv[1];
	// while((opt = getopt(argc, argv, "a:l:e"))!= -1) { };  
	switch (argv[1]) {
		case 'a':
			printf("case 'a' selected\n");
			add_file(argv[argc+1]);
			break;
		case 'l':
			//listFiles(optarg, argv[argc-1]);
			break;
		case 'e':
			//extractFile(optarg, argv[argc-1]);
			break;
		case 'r':
			//removeFile(optarg, argv[argc-1]);
			break;
		default:
			die("Usage: [-a] [-l] [-e] [-r] <archive> <file>");
			break;
		}
	*/
	


/*
	char* cmd = NULL;
	char* path = NULL;
	char* fsfile = NULL;
	if (argc == 4) {
		cmd = argv[1];
		fsfile = argv[3]; 
	} 
	else if (argc == 5) {
		cmd = argv[1];
		path = argv[2];
		fsfile = argv[4];
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
		new_filesys();	
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
	
*/	
	return 0;
}

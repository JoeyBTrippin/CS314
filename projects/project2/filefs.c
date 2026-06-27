#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Constant Sizes
#define FS_SIZE (10 * 1024 * 1024) // 10MB
#define BLOCK_SIZE 512
#define MAX_BLOCKS (FS_SIZE / BLOCK_SIZE)
#define MAX_INODES 100
#define MAX_FILENAME 255

// File Types
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

// Read from filesystem
void fs_read (void* buf, int block) {
	// Set file pointer: block * BLOCK_SIZE = offset
	fseek(fs, block * BLOCK_SIZE, SEEK_SET);
	fread(buf, BLOCK_SIZE, 1, fs); // Read to buf
}

// write to file system
void fs_write (void* buf, int block) {
	// Set file pointer: block * BLOCK_SIZE = offset 
	fseek(fs, block * BLOCK_SIZE, SEEK_SET);
	fwrite(buf, BLOCK_SIZE, 1, fs); // Write to location
}

// Allocate a free block from free_block
int alloc_block() {
	for (int i = sb.data_start; i < sb.total_blocks; i++) {
		if (bitmap[i] == 0) {
			bitmap[i] = 1;
			return i;
		}
	}

	// No free blocks found
	die("No free blocks");
	return -1;
}

// Free a block
void free_block (int b) {		// FINISH	
	if (b < 0 || b >= sb.total_blocks) return;
	bitmap[b] = 0;
}

// Allocate a free inode
int alloc_inode() {
	for (int i = 0; i < MAX_INODES; i++) {
		if (!inodes[i].used) {
			inodes[i].used = 1;
			inodes[i].type = TYPE_FREE;
			inodes[i].size = 0;
			for (int j = 0; j < 100; j++)
				inodes[i].direct[j] = -1;
			inodes[i].indirect = -1;
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
	sb.inode_start = 2; // block 0: superblock, block 1: free_block
	// int inodes_blocks = (MAX_INODES * (int)sizeof(Inode) + BLOCK_SIZE - 1) / BLOCK_SIZE;
	// sb.data_start = sb.inode_start + inode_blocks; 
	int inode_bytes = MAX_INODES * (int)sizeof(Inode);
	int inode_blocks = (inode_bytes + BLOCK_SIZE -1) / BLOCK_SIZE;
	sb.data_start = sb.inode_start + inode_blocks;

	// Reserve memmory for Free Blocks and Inodes
	bitmap = calloc(sb.total_blocks, 1);
	inodes = calloc(MAX_INODES, sizeof(Inode));
	
	// Mark superblock and bitmap as used, otherwise unused
	bitmap[0] = 1;
	bitmap[1] = 1;
	for (int i = sb.inode_start; i < sb.data_start; i++)
		bitmap[i] = 1;

	// Create root directory inode	
	int root = alloc_inode(); // Allocate ROOT INODE
	inodes[root].type = TYPE_DIR;
	inodes[root].size = 0;
	for (int j = 0; j < 100; j++)
		inodes[root].direct[j] = -1;

	// Write structures to file system
	fs_write(&sb, 0); // Write SUPERBLOCK at start
	fs_write(bitmap, 1); // Write FREE_BLOCKS
	fseek(fs, sb.inode_start * BLOCK_SIZE, SEEK_SET); // File pointer -> IDONE start
	fwrite(inodes, sizeof(Inode), MAX_INODES, fs); // Write INODE
}


//--------------------
// Directory operations
//--------------------
// Searches directories for a file with a matching name and returns its inode index'
int find_in_dir(int dir_inode, const char* name) {
	Inode *d = &inodes[dir_inode];
	DirEntry entries[BLOCK_SIZE / sizeof(DirEntry)];

	for (int i = 0; i < 100; i++) {
		if (d->direct[i] == -1) continue;
		fs_read(entries, d->direct[i]);

		for (int j = 0; j < BLOCK_SIZE /sizeof(DirEntry); j++) {
			if ( (entries[j].inode != -1) && (strcmp(entries[j].name, name) == 0) )
				return entries[j].inode;
		}
	}
	return -1;
}

// Add a new entry to a directory
void add_to_dir(int dir_inode, int child_inode, const char* name) {
	Inode* d = &inodes[dir_inode];
	DirEntry entries[BLOCK_SIZE / sizeof(DirEntry)];
	
	for (int i = 0; i < 100; i++) {
		// Allocate a new block if necessary
		if (d->direct[i] == -1) {
			d->direct[i] = alloc_block();
			// initialize entries
			for (int j = 0; j < BLOCK_SIZE / sizeof(DirEntry); j++) {
				entries[j].inode = -1;
				entries[j].name[0] = '\0';
			}
			// memset(entries, -1, sizeof(entries));
			strcpy(entries[0].name, name);
			entries[0].inode = child_inode;
			fs_write(entries, d->direct[i]);
			return;
		}

		// Reuse empty slot
		fs_read(entries, d->direct[i]);
		for (int j = 0; j < BLOCK_SIZE / sizeof(DirEntry); j++) {
			if (entries[j].inode == -1) {
				strcpy(entries[j].name, name);
				entries[j].inode = child_inode;
				fs_write(entries, d->direct[i]);
				return;
			}
		}
	}
	die ("Directory full");
}


//--------------------
// Path resolution
//--------------------
// Resolve a given path to an inode index
int resolve_path(const char* path, int create_dirs) {
	char temp[MAX_FILENAME];
	strcpy(temp, path);
	
	// Grab first directory on path
	char* token = strtok(temp, "/");
	int cur = 0; // root inode
	
	while (token) {
		int next = find_in_dir(cur, token);
		
		if (next == -1) {
			// are we
			if (!create_dirs)
				return -1;
			
			// Create directory
			int newinode = alloc_inode();
			inodes[newinode].type = TYPE_DIR;
			//memset(inodes[newinode].direct, -1, sizeof(inodes[newinode].direct));
			for (int j = 0; j < 100; j++) // Initialize
				inodes[newinode].direct[j] = -1;
			add_to_dir(cur, newinode, token);
			next = newinode;
		}

		cur = next;
		token = strtok(NULL, "/");
	}

	return cur;
}


//--------------------
// List contents of a directory
//--------------------
void list_dir(int inode, int depth) {
	Inode* d = &inodes[inode]; // first inode
	DirEntry entries[BLOCK_SIZE / sizeof(DirEntry)]; // array to hold directories
	
	// loop through directories
	for (int i = 0; i < 100; i++) {
		if (d->direct[i] == -1) continue; // doesn't exist, skip
		// read current directory
		fs_read(entries, d->direct[i]);
		
		// loop through current directory
		for (int j = 0; j < BLOCK_SIZE / sizeof(DirEntry); j++) {
			if (entries[j].inode == -1) continue; // doesn't exist, skip
			// formating depth spacing
			for (int k = 0; k < depth; k++) 
				printf("\t");
			printf("%s\n", entries[j].name); // print entry
			
			// if entry is anothe directory
			if (inodes[entries[j].inode].type == TYPE_DIR)
				// recurrsivly call to list this directory
				list_dir(entries[j].inode, depth + 1);
		}
	}
}


//--------------------
// Add File
//--------------------
void add_file(const char* host_path, const char* fs_path) {
	// Open file to be added
	FILE* f = fopen(host_path, "rb");
	if (!f) die("Cannot open input file");
	
	// Split path into directory + filename 
	char dirpath[MAX_FILENAME];
	strcpy(dirpath, fs_path);
	char* filename = strrchr(dirpath, '/');
	if (!filename) die("Invalid paht");
	*filename = 0;
	filename++;

	// Resolve parent directory
	int dir_inode = resolve_path(dirpath, 1);	
	
	// Create new inode
	int newinode = alloc_inode();
	Inode* node = &inodes[newinode];
	node->type = TYPE_FILE;
	node->size = 0;
	//memset(node->direct, -1, sizeof(node->direct));
	for (int j = 0; j < 100; j++){ // initialize
		node->direct[j] = -1;
	}

	// Copy file contents block by block
	char buf[BLOCK_SIZE];
	int block_index = 0;
	while (1) {
		int r = fread(buf, 1, BLOCK_SIZE, f);

		if (r <= 0) break; // too small
		if (block_index >= 100) // too large
			die("File too large for 100 direct blocks");

		int b = alloc_block();
		fs_write(buf, b);
		node->direct[block_index++] = b;
		node->size += r;
	}

	fclose(f);
	
	// Add to directory
	add_to_dir(dir_inode, newinode, filename);
}


//--------------------
// Extract File
//--------------------
void extract_file(const char* fs_path) {			
	// Search file path
	int inode = resolve_path(fs_path, 0);

	// Was the file found?
	if (inode < 0) die("File not found");
	
	// Access file and check type
	Inode* n = &inodes[inode];
	if(n->type != TYPE_FILE) die("Not a file");
	
	// Read full file block by block
	char buf[BLOCK_SIZE];
	int remaining = n->size;
	for (int i = 0; (i < 100) && (remaining > 0); i++) {
		if (n->direct[i] == -1) break; // file is unused
		
		fs_read(buf, n->direct[i]);

		int to_write = remaining > BLOCK_SIZE ? BLOCK_SIZE : remaining;
		fwrite(buf, 1, to_write, stdout);
		remaining -= to_write;
	}
}



//--------------------
// Remove file
//--------------------
void remove_file(const char* fs_path) {		
	// Seach file path
	int inode = resolve_path(fs_path, 0);
	// Was file found?
	if (inode < 0) die("File not found");
	
	// Free all blocks
	Inode* n = &inodes[inode];
	for (int i = 0; i < 100; i++) {
		if (n->direct[i] != -1)
		free_block(n->direct[i]);
	}

	// Zero contents of file
	memset(n, 0, sizeof(Inode));
}


//--------------------
// Main
//--------------------
int main (int argc, char* argv[]) {
	// At leadt 4 arguments will be required
	if (argc < 4) die ("Use: ./filefs -x path -f file\n");
		
	// Read arguments
	
	char* cmd = argv[1];
	char* path = argv[2];
	char* fsfile = argv[3];
	if (argc > 4)
		fsfile = argv[4];
	
	// Atempt to open existing file for read/write in binary
	if (!(fs = fopen(fsfile, "rb+"))) { 
		// File DOES NOTE exist. Create file.
		fs = fopen(fsfile, "wb+");
		ftruncate(fileno(fs), FS_SIZE);
		format_fs(); // Formating file 
	}
	
	// allocate space for file system data
	bitmap = calloc(MAX_BLOCKS, 1);
	inodes = calloc(MAX_INODES, sizeof(Inode));

	// Load file system data and set file pointer
	fs_read(&sb, 0);
	fs_read(bitmap, 1);
	fseek(fs, sb.inode_start * BLOCK_SIZE, SEEK_SET);
	fread(inodes, sizeof(Inode), MAX_INODES, fs);
	
	// String compare command to determine action
	if (strcmp(cmd, "-l") == 0) {
		list_dir(0, 0);
	}
	else if (strcmp(cmd, "-a") == 0) {
		add_file(path, path);
	}
	else if (strcmp(cmd, "-e") == 0) {
		extract_file(path);
	}
	else if (strcmp(cmd, "-r") == 0) {
		remove_file(path);
	}

	// Save file data
	fs_write(&sb, 0);
	fs_write(bitmap, 1);
	fseek(fs, sb.inode_start * BLOCK_SIZE, SEEK_SET);
	fwrite(inodes, sizeof(Inode), MAX_INODES, fs);

	// close file system
	fclose(fs);

	return 0;
}

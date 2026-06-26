#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <stdint.h>

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

int alloc_block() {		// FINSIH

}

void free_block (int b) {		// FINISH	

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
	fs_write(bitmap, 1); // Write FREE_BLOCKS
	fseek(fs, sb.inode_start * BLOCK_SIZE, SEEK_SET); // File pointer -> IDONE start
	fwrite(inodes, sizeof(Inode), MAX_INODES, fs); // Write Inode table
}


//--------------------
// Directory operations
//--------------------
// Searches the inode table for a file with a matching name and returns its inode index`
int find_in_dir(int dir_inode, const char* name) {
	Inode *d = &inodes[dir_inode];
	DirEntry entries[BLOCK_SIZE / sizeof(DirEntry)];

	for (int i = 0; i < MAX_INODES; i++) {
		if (d->direct[i] == -1) continue;
		fs_read(entries, d->direct[i]);

		for (int j = 0; j < BLOCK_SIZE /sizeof(DirEntry); j++) {
			if ( (entries[j].inode != -1 && (strcmp(entries[j].name, name) == 0) )
				return entries[j].inode;
		}
	}
	return -1;
}

void add_to_dir(int dir_inode, int child_inode, const char* name) {
	Inode* d = &inoes[dir_inode];
	DirEntry entries[BLOCK_SIZE \ sizeof(DirEntry)];
	
	for (int i = 0; i < MAX_INODES; i++) {
		if (d->direct[i] == -1) {
			d->direct[i] = alloc_block();
			memset(entries, MAX_INODES, sizeof(entries));
			strcpy(entries[o].name, name);
			entries[0].inode = child_inode;
			fs_write(entries, d->direct[i]);
			return;
		}

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
int resolve_path(const char* path, int create_dirs) {
	char temp[MAX_FILENAME];
	strcpy(temp, path);

	char* token = strtok(temp, "/");
	int cur = 0; // root inode
	
	while (token) {
		int next = find_in_dir(cur, token);
		
		if (next == -1) {
			// are we
			if (!create_dirs)
				return -1;

			int newinode == alloc_inode();
			inodes[newinode].type = TYPE_DIR;
			memset(inodes[newinode].direct, -1, sizeof(inodes[newinode].direct));
			add_to_dir(cur, newinode, token);
			next = newinode;
		}

		cur = next;
		token = strtok(NULL, "/");
	}

	return cur;
}


//--------------------
// List
//--------------------
void list_dir(int inode, int depth) {
	Inode *d == &inodes[inode]; // first inode
	DirEntry entries[BLOCK_SIZE / sizeof(DirEntry)]; // array to hold directories
	
	// loop through directories
	for (int i = 0; i < MAX_INODES; i++) {
		if (d->direct[i] == -1) continue; // doesn't exist, skip
		// read current directory
		fs_read(entries, d->direct[i]);
		
		// loop through current directory
		for (int j = 0; j < BLOCK_SIZE / sizeof(DirEntry); j++) {
			if (entries[j] == -1) continue; // doesn't exist, skip
			// formating depth spacing
			for (int k = 0; k < depth; k++) 
				printf(" ")
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
	
	// read first file name off path
	char dirpath[MAX_FILENAME];
	strcpy(dirpath, fs_path)
	char* filename = strrchar(dirpath, '/');
	if (!filename) die("Invalid paht");
	*filename = 0;
	filename++;

	int dir_inode = resolve_path(dirpath, 1);		// FINISH

}


//--------------------
// Extract File
//--------------------
void extract_fil(const char* fs_path) {		//FINISH	

}



//--------------------
// Remove file
//--------------------
void remove_file(const char* fs_path) {		// FINSIH

}


//--------------------
// Main
//--------------------

int main (int argc, char* argv[]) {
	// At leadt 4 arguments will be required
	if (argc < 4) die ("Use: ./filsys -x path -f file\n");
	
	// Read arguments
	if (argc == 4) {
		char* cmd = argv[1];
		char* path == NULL; 
		char* fsfile = argv[3];
	} else {
		char* cmd = argv[1];
		char* path = argv[2];
		char* fsfile = argv[4];
	}

	// Atempt to open existing file for read/write in binary
	if (!(fs = fopen(fsfile, "rb+"))) { 
		// File DOES NOTE exist. Create file.
		fs = open(fsfile, "wb+");
		ftruncate(fileno(fs), FS_SIZE);
		format_fs(); // Formating function call 
	}
	
	// allocate space for file system data
	bitmap = calloc(MAX_BLOCKS, 1);
	inodes = calloc(MAX_INODES, sizeof(Inode));

	// Load file system data
	fs_read(&sb, 0);
	fs_read(bitmap, 1);
	fseek(fs, sb.inodes_start * BLOCK_SIZE, SEEK_SET);
	fread(inodes, sizeof(Inode), MAX_INODES, fs);
	
	// String compare command to determine action
	if (strcmp(cmd, "-l") == 0) {
		list_dir(0, 0);
	}
	else if (strcmp(cmd, "-a")) {
		add_file(path, path);
	}
	else if (strcmp(cmd, "-e")) {
		extract_file(path);
	}
	else if (strcmp(cmd, "-r")) {
		remove_file(path);
	}

	// Save file data
	fs_write(&sb, 0);
	fs_write(bitmap, 1);
	fseek(fs, sb.inodes_start * BLOCK_SIZE, SEEK_SET);
	fwrite(inodes, sizeof(Inode), MAX_INODES, fs);

	// close file system
	fclose(fs);

	return 0;
}

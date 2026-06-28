#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
//#include <errno.h>

#define FS_SIZE (10 * 1024 * 1024)
#define BLOCK_SIZE 512
#define NUM_INODES 128
#define DIRECT_PTRS 100
#define NAME_MAX_LEN 255
#define MAGIC 0x46534631
#define TYPE_FREE 0
#define TYPE_FILE 1
#define TYPE_DIR 2

typedef struct {
    uint32_t magic;
    uint32_t total_size;
    uint32_t block_size;
    uint32_t num_blocks;
    uint32_t num_inodes;
    uint32_t inode_start_block;
    uint32_t bitmap_start_block;
    uint32_t data_start_block;
    uint32_t root_inode;
} Superblock;

typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t direct[DIRECT_PTRS];
    uint32_t indirect;
} Inode;

typedef struct {
    uint32_t inode;
    uint8_t valid;
    char name[256];
} DirEntry;

static FILE *fs = NULL;
static Superblock sb;
static Inode inodes[NUM_INODES];
static uint8_t *bitmap = NULL;
static uint32_t bitmap_bytes = 0;
static uint32_t bitmap_blocks = 0;
static uint32_t inode_blocks = 0;
static uint32_t data_start = 0;
static uint32_t total_blocks = 0;

static void die(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

static void seek_block(uint32_t block) {
    if (fseek(fs, (long)block * BLOCK_SIZE, SEEK_SET) != 0) die("seek failed");
}

static void read_block(uint32_t block, void *buf) {
    seek_block(block);
    if (fread(buf, 1, BLOCK_SIZE, fs) != BLOCK_SIZE) die("read failed");
}

static void write_block(uint32_t block, const void *buf) {
    seek_block(block);
    if (fwrite(buf, 1, BLOCK_SIZE, fs) != BLOCK_SIZE) die("write failed");
}

static int is_block_used(uint32_t b) {
    return (bitmap[b / 8] >> (b % 8)) & 1;
}

static void set_block_used(uint32_t b, int used) {
    if (used) bitmap[b / 8] |= (1 << (b % 8));
    else bitmap[b / 8] &= ~(1 << (b % 8));
}

static void sync_bitmap(void) {
    uint8_t block[BLOCK_SIZE];
    memset(block, 0, sizeof(block));
    memcpy(block, bitmap, bitmap_bytes);
    for (uint32_t i = 0; i < bitmap_blocks; i++) write_block(sb.bitmap_start_block + i, block);
}

static void sync_inodes(void) {
    uint8_t block[BLOCK_SIZE];
    uint32_t idx = 0;
    for (uint32_t b = 0; b < inode_blocks; b++) {
        memset(block, 0, sizeof(block));
        memcpy(block, ((uint8_t*)inodes) + idx, BLOCK_SIZE);
        write_block(sb.inode_start_block + b, block);
        idx += BLOCK_SIZE;
    }
}

static void load_inodes(void) {
    uint8_t block[BLOCK_SIZE];
    uint32_t idx = 0;
    for (uint32_t b = 0; b < inode_blocks; b++) {
        read_block(sb.inode_start_block + b, block);
        memcpy(((uint8_t*)inodes) + idx, block, BLOCK_SIZE);
        idx += BLOCK_SIZE;
    }
}

static void load_bitmap(void) {
    bitmap = calloc(1, bitmap_blocks * BLOCK_SIZE);
    if (!bitmap) die("calloc failed");
    uint8_t block[BLOCK_SIZE];
    uint32_t idx = 0;
    for (uint32_t b = 0; b < bitmap_blocks; b++) {
        read_block(sb.bitmap_start_block + b, block);
        memcpy(bitmap + idx, block, BLOCK_SIZE);
        idx += BLOCK_SIZE;
    }
}

static void format_fs(void) {
    memset(&sb, 0, sizeof(sb));
    sb.magic = MAGIC;
    sb.total_size = FS_SIZE;
    sb.block_size = BLOCK_SIZE;
    total_blocks = FS_SIZE / BLOCK_SIZE;
    sb.num_blocks = total_blocks;
    sb.num_inodes = NUM_INODES;

    inode_blocks = (NUM_INODES * sizeof(Inode) + BLOCK_SIZE - 1) / BLOCK_SIZE;
    bitmap_bytes = (total_blocks + 7) / 8;
    bitmap_blocks = (bitmap_bytes + BLOCK_SIZE - 1) / BLOCK_SIZE;

    sb.inode_start_block = 1;
    sb.bitmap_start_block = 1 + inode_blocks;
    sb.data_start_block = sb.bitmap_start_block + bitmap_blocks;
    sb.root_inode = 0;
    data_start = sb.data_start_block;

    if (ftruncate(fileno(fs), FS_SIZE) != 0) die("ftruncate failed");

    uint8_t zero[BLOCK_SIZE];
    memset(zero, 0, sizeof(zero));

    seek_block(0);
    fwrite(&sb, 1, sizeof(sb), fs);

    for (uint32_t i = 1; i < total_blocks; i++) write_block(i, zero);

    memset(inodes, 0, sizeof(inodes));
    bitmap = calloc(1, bitmap_blocks * BLOCK_SIZE);
    if (!bitmap) die("calloc failed");

    for (uint32_t i = 0; i < sb.data_start_block; i++) set_block_used(i, 1);

    inodes[0].type = TYPE_DIR;
    inodes[0].size = 0;
    for (int i = 0; i < DIRECT_PTRS; i++) inodes[0].direct[i] = 0;
    inodes[0].indirect = 0;

    sync_bitmap();
    sync_inodes();
    seek_block(0);
    fwrite(&sb, 1, sizeof(sb), fs);
    fflush(fs);
}

static void open_fs(const char *name) {
    fs = fopen(name, "r+b");
    if (!fs) {
        fs = fopen(name, "w+b");
        if (!fs) die("cannot open fs file");
        format_fs();
        return;
    }
    if (fread(&sb, 1, sizeof(sb), fs) != sizeof(sb)) die("bad superblock");
    if (sb.magic != MAGIC) format_fs();
    total_blocks = sb.num_blocks;
    inode_blocks = (NUM_INODES * sizeof(Inode) + BLOCK_SIZE - 1) / BLOCK_SIZE;
    bitmap_bytes = (total_blocks + 7) / 8;
    bitmap_blocks = (bitmap_bytes + BLOCK_SIZE - 1) / BLOCK_SIZE;
    data_start = sb.data_start_block;
    load_bitmap();
    load_inodes();
}

static uint32_t alloc_block(void) {
    for (uint32_t b = sb.data_start_block; b < total_blocks; b++) {
        if (!is_block_used(b)) {
            set_block_used(b, 1);
            memset((char[BLOCK_SIZE]){0}, 0, 0);
            return b;
        }
    }
    die("out of blocks");
    return 0;
}

static void free_block(uint32_t b) {
    if (b < total_blocks) set_block_used(b, 0);
}

static int alloc_inode(void) {
    for (int i = 0; i < (int)NUM_INODES; i++) {
        if (inodes[i].type == TYPE_FREE) return i;
    }
    return -1;
}

static uint32_t inode_block_for(uint32_t inode_idx, uint32_t logical) {
    Inode *ino = &inodes[inode_idx];
    if (logical < DIRECT_PTRS) return ino->direct[logical];
    logical -= DIRECT_PTRS;
    if (!ino->indirect) return 0;
    uint32_t *tbl = malloc(BLOCK_SIZE);
    if (!tbl) die("malloc failed");
    read_block(ino->indirect, tbl);
    uint32_t r = tbl[logical];
    free(tbl);
    return r;
}

static void inode_set_block(uint32_t inode_idx, uint32_t logical, uint32_t block) {
    Inode *ino = &inodes[inode_idx];
    if (logical < DIRECT_PTRS) {
        ino->direct[logical] = block;
        return;
    }
    logical -= DIRECT_PTRS;
    if (!ino->indirect) {
        ino->indirect = alloc_block();
        uint32_t zero[BLOCK_SIZE / 4];
        memset(zero, 0, sizeof(zero));
        write_block(ino->indirect, zero);
    }
    uint32_t *tbl = malloc(BLOCK_SIZE);
    if (!tbl) die("malloc failed");
    read_block(ino->indirect, tbl);
    tbl[logical] = block;
    write_block(ino->indirect, tbl);
    free(tbl);
}

static void inode_free_blocks(uint32_t inode_idx) {
    Inode *ino = &inodes[inode_idx];
    uint32_t count = (ino->size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    for (uint32_t i = 0; i < count && i < DIRECT_PTRS; i++) {
        if (ino->direct[i]) free_block(ino->direct[i]);
        ino->direct[i] = 0;
    }
    if (ino->indirect) {
        uint32_t *tbl = malloc(BLOCK_SIZE);
        if (!tbl) die("malloc failed");
        read_block(ino->indirect, tbl);
        for (uint32_t i = DIRECT_PTRS; i < count; i++) {
            if (tbl[i - DIRECT_PTRS]) free_block(tbl[i - DIRECT_PTRS]);
        }
        free_block(ino->indirect);
        free(ino->indirect);
        ino->indirect = 0;
        free(tbl);
    }
    ino->size = 0;
}

static int dir_read_entries(uint32_t inode_idx, DirEntry **entries_out, size_t *count_out) {
    Inode *ino = &inodes[inode_idx];
    size_t blocks = (ino->size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    size_t cap = 8, count = 0;
    DirEntry *arr = malloc(cap * sizeof(DirEntry));
    if (!arr) die("malloc failed");
    uint8_t buf[BLOCK_SIZE];
    for (size_t i = 0; i < blocks; i++) {
        uint32_t blk = inode_block_for(inode_idx, i);
        if (!blk) continue;
        read_block(blk, buf);
        size_t off = 0;
        while (off + sizeof(DirEntry) <= BLOCK_SIZE) {
            DirEntry e;
            memcpy(&e, buf + off, sizeof(DirEntry));
            if (e.valid && e.name[0]) {
                if (count == cap) {
                    cap *= 2;
                    arr = realloc(arr, cap * sizeof(DirEntry));
                    if (!arr) die("realloc failed");
                }
                arr[count++] = e;
            }
            off += sizeof(DirEntry);
        }
    }
    *entries_out = arr;
    *count_out = count;
    return 0;
}

static void dir_write_entries(uint32_t inode_idx, DirEntry *entries, size_t count) {
    Inode *ino = &inodes[inode_idx];
    size_t bytes = count * sizeof(DirEntry);
    size_t blocks = (bytes + BLOCK_SIZE - 1) / BLOCK_SIZE;
    inode_free_blocks(inode_idx);
    ino->type = TYPE_DIR;
    ino->size = bytes;
    for (size_t i = 0; i < blocks; i++) {
        uint32_t blk = alloc_block();
        inode_set_block(inode_idx, i, blk);
        uint8_t buf[BLOCK_SIZE];
        memset(buf, 0, sizeof(buf));
        size_t start = i * BLOCK_SIZE;
        size_t chunk = bytes - start;
        if (chunk > BLOCK_SIZE) chunk = BLOCK_SIZE;
        memcpy(buf, ((uint8_t*)entries) + start, chunk);
        write_block(blk, buf);
    }
}

static int dir_find_child(uint32_t dir_inode, const char *name, uint32_t *child_out) {
    DirEntry *entries = NULL;
    size_t count = 0;
    dir_read_entries(dir_inode, &entries, &count);
    for (size_t i = 0; i < count; i++) {
        if (strcmp(entries[i].name, name) == 0) {
            *child_out = entries[i].inode;
            free(entries);
            return 1;
        }
    }
    free(entries);
    return 0;
}

static uint32_t ensure_dir_child(uint32_t dir_inode, const char *name) {
    uint32_t child;
    if (dir_find_child(dir_inode, name, &child)) return child;
    int idx = alloc_inode();
    if (idx < 0) die("out of inodes");
    inodes[idx].type = TYPE_DIR;
    inodes[idx].size = 0;
    DirEntry *entries = NULL;
    size_t count = 0;
    dir_read_entries(dir_inode, &entries, &count);
    entries = realloc(entries, (count + 1) * sizeof(DirEntry));
    if (!entries) die("realloc failed");
    entries[count].inode = idx;
    entries[count].valid = 1;
    memset(entries[count].name, 0, sizeof(entries[count].name));
    strncpy(entries[count].name, name, 255);
    dir_write_entries(dir_inode, entries, count + 1);
    free(entries);
    return (uint32_t)idx;
}

static void split_path(const char *path, char **parts, int *n) {
    char *tmp = strdup(path);
    if (!tmp) die("strdup failed");
    *n = 0;
    char *tok = strtok(tmp + 1, "/");
    while (tok) {
        parts[(*n)++] = strdup(tok);
        tok = strtok(NULL, "/");
    }
    free(tmp);
}

static uint32_t resolve_parent(const char *path, char *leaf, int create_dirs) {
    if (path[0] != '/') die("path must be absolute");
    char *copy = strdup(path);
    if (!copy) die("strdup failed");
    char *last = strrchr(copy, '/');
    if (!last || last == copy) {
        strcpy(leaf, last ? last + 1 : copy + 1);
        free(copy);
        return 0;
    }
    strcpy(leaf, last + 1);
    *last = '\0';
    uint32_t cur = 0;
    char *p = copy + 1;
    char *tok = strtok(p, "/");
    while (tok) {
        uint32_t next;
        if (!dir_find_child(cur, tok, &next)) {
            if (!create_dirs) {
                free(copy);
                return UINT32_MAX;
            }
            next = ensure_dir_child(cur, tok);
        }
        cur = next;
        tok = strtok(NULL, "/");
    }
    free(copy);
    return cur;
}

static void cmd_add(const char *src, const char *dstpath) {
    FILE *in = fopen(src, "rb");
    if (!in) die("cannot open source file");
    char leaf[256];
    uint32_t parent = resolve_parent(dstpath, leaf, 1);
    if (parent == UINT32_MAX) die("path error");
    int idx = alloc_inode();
    if (idx < 0) die("out of inodes");
    inodes[idx].type = TYPE_FILE;
    inodes[idx].size = 0;
    uint8_t buf[BLOCK_SIZE];
    size_t n, blockno = 0, total = 0;
    while ((n = fread(buf, 1, BLOCK_SIZE, in)) > 0) {
        uint32_t blk = alloc_block();
        write_block(blk, buf);
        inode_set_block(idx, blockno++, blk);
        total += n;
    }
    inodes[idx].size = total;
    DirEntry *entries = NULL;
    size_t count = 0;
    dir_read_entries(parent, &entries, &count);
    entries = realloc(entries, (count + 1) * sizeof(DirEntry));
    if (!entries) die("realloc failed");
    entries[count].inode = idx;
    entries[count].valid = 1;
    memset(entries[count].name, 0, sizeof(entries[count].name));
    strncpy(entries[count].name, leaf, 255);
    dir_write_entries(parent, entries, count + 1);
    free(entries);
    fclose(in);
    sync_bitmap();
    sync_inodes();
}

static void cmd_extract(const char *path) {
    char leaf[256];
    uint32_t parent = resolve_parent(path, leaf, 0);
    if (parent == UINT32_MAX) die("not found");
    uint32_t inode;
    if (!dir_find_child(parent, leaf, &inode)) die("not found");
    Inode *ino = &inodes[inode];
    uint8_t buf[BLOCK_SIZE];
    size_t remaining = ino->size;
    size_t blocks = (ino->size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    for (size_t i = 0; i < blocks; i++) {
        uint32_t blk = inode_block_for(inode, i);
        read_block(blk, buf);
        size_t n = remaining > BLOCK_SIZE ? BLOCK_SIZE : remaining;
        fwrite(buf, 1, n, stdout);
        remaining -= n;
    }
}

static void remove_entry_from_dir(uint32_t dir_inode, const char *name) {
    DirEntry *entries = NULL;
    size_t count = 0;
    dir_read_entries(dir_inode, &entries, &count);
    size_t j = 0;
    for (size_t i = 0; i < count; i++) {
        if (strcmp(entries[i].name, name) != 0) entries[j++] = entries[i];
    }
    dir_write_entries(dir_inode, entries, j);
    free(entries);
}

static int dir_empty(uint32_t inode_idx) {
    DirEntry *entries = NULL;
    size_t count = 0;
    dir_read_entries(inode_idx, &entries, &count);
    free(entries);
    return count == 0;
}

static void recursive_remove(uint32_t parent, const char *leaf) {
    uint32_t inode;
    if (!dir_find_child(parent, leaf, &inode)) return;
    inode_free_blocks(inode);
    inodes[inode].type = TYPE_FREE;
    inodes[inode].size = 0;
    remove_entry_from_dir(parent, leaf);
    while (parent != 0 && dir_empty(parent)) {
        char temp[256] = {0};
        break;
    }
}

static void cmd_remove(const char *path) {
    char leaf[256];
    uint32_t parent = resolve_parent(path, leaf, 0);
    if (parent == UINT32_MAX) return;
    recursive_remove(parent, leaf);
    sync_bitmap();
    sync_inodes();
}

static void list_inode(uint32_t inode_idx, int depth) {
    DirEntry *entries = NULL;
    size_t count = 0;
    dir_read_entries(inode_idx, &entries, &count);
    for (size_t i = 0; i < count; i++) {
        for (int t = 0; t < depth; t++) printf("\t");
        printf("%s\n", entries[i].name);
        if (inodes[entries[i].inode].type == TYPE_DIR) list_inode(entries[i].inode, depth + 1);
    }
    free(entries);
}

static void cmd_list(void) {
    list_inode(0, 0);
}

int main(int argc, char* argv) {
	char* cmd = argv[1];
	char* path;
	char* fsfile;
	
	if (argc == 4) {
		path = NULL;
		fsfile = argv[3];
	} else if (argc == 5) {
		path = argv[2];
		fsfile = argv[4];
	} else {
		die("Use: ./filefs -x /a/b/c -f filesys OR ./filefs -l -f filesys");
	}

    open_fs(fsfile);

	if (strcmp(cmd, "-a") == 0) {
		cmd_add(path, path);
	} else if (strcmp(cmd, "-l") == 0) {
		cmd_list();
	} else if (strcmp(cmd, "-r") == 0) {
		cmd_remove(path);
	} else if (strcmp(cmd, "-e") == 0) {
		cmd_extract(path);
	} else {
		die("Commands: -a, -l, -r, -e.");
	}
	/*
    if (addpath && srcfile) cmd_add(srcfile, addpath);
    else if (rempath) cmd_remove(rempath);
    else if (expath) cmd_extract(expath);
    else if (list) cmd_list();
    else die("no operation");
	*/

    fclose(fs);
    free(bitmap);
    
	 return 0;
}

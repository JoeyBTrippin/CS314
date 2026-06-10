#ifndef MYMALLOC_H
#define MYMALLOC_H

#include <stddef.h>

#define NUM_FASTBINS 4
#define NUM_REGULAR_BINS 4

void* my_malloc (size_t size);
void my_free (void* ptr);

void dump_bin_statistics(void);

#endif

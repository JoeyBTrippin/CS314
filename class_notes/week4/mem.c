#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>

#define SIZE 10


#define MEMSIZE 1024*1024

char *mymem = NULL;

typedef struct BlockHeader{
  int size;
  int is_free;
}BlockHeader;

BlockHeader* heapstart = NULL;


/*
mymem        mymalloc returns                                      sentinel header
v            v                                                     v
|header:40,0|----|header:size,1|----------------------------------|header:0,0|
                                                                             ^
									    end of region (1024*1024-1)
 */
void* mymalloc(int bytes){
  void* ret;
  BlockHeader* headertmp = heapstart;

  //init new region
  if(mymem==NULL){
    mymem = mmap(NULL, MEMSIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    heapstart = (BlockHeader*)mymem;
    heapstart->size = MEMSIZE - sizeof(BlockHeader)*2;
    heapstart->is_free = 1;

    headertmp = heapstart;
    //initialize sentinel
    headertmp = (BlockHeader*)((char*)(headertmp)+sizeof(BlockHeader)+headertmp->size);
    headertmp->size = 0;
    headertmp->is_free = 0;
  }

  //allocate a chunk
  headertmp = heapstart;
  
  while(!headertmp->is_free){
    headertmp = (void*)headertmp+sizeof(BlockHeader)+headertmp->size;
  }

  ret = (void*)(headertmp)+sizeof(BlockHeader);

  headertmp->size = bytes;
  headertmp->is_free = 0;

  headertmp = (void*)headertmp+sizeof(BlockHeader)+headertmp->size;
  headertmp->is_free = 1;
  
  return ret;
}

void free(void* mem){


}

int main(){
  int* ints = mymalloc(sizeof(int)*SIZE);
  int* ints2 = mymalloc(sizeof(int)*SIZE);

  
  for(int i = 0; i < SIZE; i++){
    ints[i] = i*2;
    ints2[i] = i*3;
  }


  for(int i = 0; i < SIZE; i++){
    printf("%d (%p) | ", ints[i], &(ints[i]));
  }
  printf("\n");
  
  for(int i = 0; i < SIZE; i++){
    printf("%d (%p) | ", ints2[i], &(ints2[i]));
  }
  printf("\n");
  
  //  free(ints);
  
  return 0;
}

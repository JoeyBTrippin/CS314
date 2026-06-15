AUTHOR: Joseph Blecha
DATE: 6/16/2026
CLASS: SIUE CS314-001
Description: Project 1 
	
	NOTE: after running, view 'output.txt' to see 
	test/demonstration file per rubric.

	This project implements my_malloc() and my_free() per 
	specifications of project handout. The implementation code
	is in mymalloc.c and is layed out as thus.
	
	HELPER FUNCITONS:
	index_fastbin(): takes a given size and returns the correct
	fast bin index or -1 if non found.

	index_regbin(): takes a given size and returns the correct
	regualr bin index.

	remove_from_bin: removes a block from regular bin's 
	double-linked list. clearing blocks pointer, updating the
	bin head if needed, and removing dangling pointers.

	insert_sorted(): inserts a free block into regular bin
	in required sorted size. 3 cases are used, insert head,
	middle, tail. Maintains prev/next pointers.

	request_from_os(): uses mmap to request memory from operating
	system. Rounds to page size (4096 bytes). Creates allocated
	block at head and sentinal block at tail. If leftover >= 16
	bytes, create a free block and insert into appropriate bin.
	return allocated block.
	
	PRIMARY FUNCTIONS:
	my_malloc(): computes total_size before checking fastbin, if
	block exist, pop and return. Then checks regular bin, if 
	notempty, find smallest fit, split if leftover >= 16 return
	pointer. Final, if not blocks exsist, request from OS.

	my_free(): Mark block as free. Then check if size can fit in 
	fastbin, if so push into fast bin and return. If not 
	coalesc if possible. Insert into correct regular bin.

	dump_bin_statistics(): Used for debugging. Show contents of
	fast bins and regular bins. Each blocks address, size, and
	free/allocated status.
	
	DEMONSTRATIONS	
	test(): runs a series of demonstration tests along with 
	explenations. stdout is rerouted to out.txt file
	
	RUNNING PROGRAM:
	make: combiles all necessary files

	./mymalloc: runs all necessary files


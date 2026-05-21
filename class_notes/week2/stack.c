#include <stdio.h>

void function() {
	char buffer1[4];
	char *ret = buffer1+28;
	(*ret) += 7;
}

void main(int argc, char* argv[]) {
	int x;
	x=0;
	function();
	x=1;
	printf("%d\n", x);

}

/*
------"top" of stack-------
frame for main

- - - - - - - - - - - - -
x (rdp-4)

------------------------
frame for function

- - - - - - - - - -
buffer1 (rdp-4)
ret	(rdp-12)

*/

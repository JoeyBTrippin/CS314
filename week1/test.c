#include <stdio.h>

int global = 1092;

void fun( int var_in) {
	int funlocal = var_in;
}

int main(int argc, char* argv[]){
	static int staticlocal = 111;	
	int local = 10;

	printf("hell!");

	fun(local);

	return 0;
}

all:
	gcc coordsread.c -o  coordsread
	gcc coordswrite.c -o coordswrite

remakeall:
	rm coordsread
	rm coordswrite
	gcc coordsread.c -o coordsread
	gcc coordswrite.c -o coordswrite

shell:
	gcc shell.c -o shell
	gcc child.c -o child
stack:
	gcc stack.c -o stack

threads:
	gcc threads.c -o threads
clean:
	rm shell
	rm child
	rm coordsread
	rm coordswrite
	rm outfile
	rm stack
	rm threads

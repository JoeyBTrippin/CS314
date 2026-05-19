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
clean:
	rm shell
	rm child
	rm coordsread
	rm coordswrite
	rm outfile

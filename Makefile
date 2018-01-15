all: u4

u4: u4.c
	gcc -m32 -g -o u4 u4.c

clean:
	rm -f u4
	rm -rf u4.dSYM

all: u4

test: u4-test

u4: u4.c
	gcc -m32 -g -o u4 u4.c

u4-test: u4.c
	gcc -DINNER_TEST -m32 -g -o u4-test u4.c

clean:
	rm -f u4
	rm -f u4-test
	rm -rf u4.dSYM
	rm -rf u4-test.dSYM

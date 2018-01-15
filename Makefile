all: u4

u4: u4.h u4.c
	gcc -m32 -g -DU4_MAIN -o u4 u4.c

u4-test: u4.h u4.c u4-test.c
	gcc -m32 -g u4.c u4-test.c -o u4-test

test: u4-test

clean:
	rm -f u4
	rm -rf u4.dSYM
	rm -f u4-test
	rm -rf u4-test.dSYM

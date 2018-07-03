CFLAGS += -g

EXE = u4 u6 u7 eforth

all: $(EXE)

eforth: eforth.c
	gcc -Wall -Wunused -o eforth eforth.c

u7: u7.c
	gcc -Wall -Wunused -o u7 u7.c

u6: u6.c
	gcc -Wall -Wunused -o u6 u6.c

u4: u4.h u4.c
	gcc -m32 -g -DU4_MAIN -o u4 u4.c

u4-test: u4.h u4.c u4-test.c
	gcc -m32 -g u4.c u4-test.c -o u4-test

test: u4-test

clean:
	rm -f $(EXE)

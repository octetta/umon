all: u4 # u0 u1 u2 u3

u0: u0.c
	gcc -m32 -g -o u0 u0.c

u1: u1.c
	gcc -m32 -g -o u1 u1.c

u2: u2.c
	gcc -m32 -g -o u2 u2.c

u3: u3.c
	gcc -m32 -g -o u3 u3.c

u4: u4.c
	gcc -m32 -g -o u4 u4.c

u4-test: u4.c
	gcc -DINNER_TEST -m32 -g -o u4-test u4.c

clean:
	rm -f u[01234]
	rm -rf *.dSYM

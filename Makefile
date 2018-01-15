all: u0 u1 u2 u3

u0: u0.c
	gcc -m32 -g -o u0 u0.c

u1: u1.c
	gcc -m32 -g -o u1 u1.c

u2: u2.c
	gcc -m32 -g -o u2 u2.c

u3: u3.c
	gcc -m32 -g -o u3 u3.c

clean:
	rm -f u[0123]

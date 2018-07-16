/*

# eforth as defined by ting

forth-run-flag
ip interpreter pointer
sp data stack pointer
rp return stack pointer
wp word or work pointer
up user area pointer

## system interface

bye : return to os (set forth-run-flag to false)

!io : setup i/o device

?rx : return boolean for availbility of character, if true also the character

tx! : send character

## inner interpreter

dolit

dolist

$next : load next word into WP

next

?branch

branch

execute : pop value jump to address

exit

## memory access

!

@

c!

c@

## return stack

rp@

rp!

r>

r@

r>

## data stack

sp@

sp!

drop

dup

over

## logic

0<

and

or

xor

## arithmetic

um+

## joe

name
link -^
type
func
exit

*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint8_t CELL;
typedef int32_t WORD;

WORD ip = -1;
WORD sp = 0;
WORD rp = 0;
WORD wp = 0;
WORD up = 0;

#define TRUE (-1)
#define FALSE (0)

#define DICT_MAX (256 * 1024)
CELL dict[DICT_MAX] = {
};

#define STACK_MAX (64)

WORD stack[STACK_MAX];

void push(WORD n) {
    ip++;
    stack[ip] = n;
}

WORD pop(void) {
    WORD n;
    if (ip >= 0) {
        n = stack[ip];
        ip--;
        return n;
    }
    return 0;
}

static uint8_t bye_flag = 0;

void bye(void) {
    bye_flag = 1;
}

#if 1 // for unix-y systems

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

static struct termios io_old, io_new;

void unbang_io(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &io_old);
}

void bang_io(void) {
    static char first = 1;
    if (first) {
        tcgetattr(STDIN_FILENO, &io_old);
        tcgetattr(STDIN_FILENO, &io_new);
        io_new.c_lflag &= ~(ICANON | ECHO);
        io_new.c_cc[VMIN] = 1;
        io_new.c_cc[VTIME] = 0;
        first = 0;
        atexit(unbang_io);
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &io_new);
}

void is_rx(void) {
    int n;
    //if (ioctl(STDIN_FILENO, I_NREAD, &n) == 0 && n > 0) {
    if (ioctl(STDIN_FILENO, FIONREAD, &n) == 0 && n > 0) {
        char c;
        n = read(STDIN_FILENO, &c, 1);
        if (n > 0) {
            push(c);
            push(TRUE);
            return;
        }
    }
    push(FALSE);
}

void tx_store(void) {
    int n;
    char c = (char)pop();
    n = write(STDOUT_FILENO, &c, 1);
}

void key(void) {
    int n;
    char c;
    n = read(STDIN_FILENO, &c, 1);
    if (n > 0) push(c);
}

void emit(void) {
    tx_store();
}

void next(void) {

}

void execute(void) {
    
}

void dolit(void) {

}

#endif

int main(int argc, char *argv[]) {
    int n = 0;
    bang_io();
    while (1) {
#if 1
        key();
        emit();
        n++;
        if (n > 5) break;
#else
        is_rx();
        if (pop() == TRUE) {
            tx_store();
            break;
        } else {
            printf("FALSE\n");
        }
        sleep(1);
#endif
    }
    return 0;
}

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

typedef long cell_t;

#define DICT_MAX (64 * 1024)

cell_t dict[DICT_MAX] = {
};

int main(int argc, char *argv[]) {
    return 0;
}

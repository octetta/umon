#include <stdio.h>
#include "u4.h"

void one(void) {
    printf("I am one!\n");
}

void two(void) {
    printf("I am two!\n");
}

void three(void) {
    printf("I am three!\n");
}

void four(void) {
    printf("I am four!\n");
}

void inner_test(void) {
    static int first = 1;
   
    if (first) {
        first = 0;

        reg(one);
        reg(two);
        reg(three);
        reg(four);

        create("one");
        comma((int)one);
        
        create("two");
        comma((int)two);
        
        create("three");
        comma((int)three);
        
        create("four");
        comma((int)four);
        
        create("testit");
        comma((int)docolon);
        comma(tick("one"));
        comma(tick("pushlit"));
        comma(999);
        comma(tick("."));
        comma(tick("pushlit"));
        comma(666);
        comma(tick("."));
        comma(NEXT);

        create("2nd-level");
        comma((int)docolon);
        comma(tick("two"));
        comma(tick("testit"));
        comma(NEXT);

        create("3rd-level");
        comma((int)docolon);
        comma(tick("three"));
        comma(tick("2nd-level"));
        comma(NEXT);

        create("branchtest");
        comma((int)docolon);
        comma(tick("?branch"));
        comma(3);
        comma(tick("four"));
        comma(tick("branch"));
        comma(1);
        comma(tick("three"));
        comma(NEXT);
    }

    printf("\n* test 001 *\n");
    execute(tick("one"));

    printf("\n* test 002 *\n");
    execute(tick("two"));

    printf("\n* test 003 *\n");
    execute(tick("testit"));

    printf("\n* test 004 *\n");
    execute(tick("2nd-level"));

    printf("\n* test 005 *\n");
    execute(tick("3rd-level"));

    push(0);
    printf("\n* test 006 *\n");
    execute(tick("branchtest"));

    push(1);
    printf("\n* test 006 *\n");
    execute(tick("branchtest"));
}

int main(int argc, char *argv[]) {
    u4_init();
    inner_test();
    u4_start();
    return 0;
}

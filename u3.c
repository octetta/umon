#include <stdio.h>
#include <string.h>

#define SMAX 1000
#define RMAX 1000
#define NMAX 1000
#define DMAX 1000

int stack[SMAX];
int rstack[RMAX];
int sp = 0;
int rs = 0;

void rpush(int n) {
    if (rs < SMAX) rs++;
    rstack[rs] = n;
}

int rpop(void) {
    int n = rstack[rs];
    if (rs >= 0) rs--;
    return n;
}

void push(int n) {
    if (sp < SMAX) sp++;
    stack[sp] = n;
}

int pop(void) {
    int n = stack[sp];
    if (sp >= 0) sp--;
    return n;
}

void nil(void);
void one(void);
void two(void);
void three(void);
void dolist(void);

#define FEXIT (-1)

int ip = 0;


char name[NMAX] = 
   //0123
    "one\0";
   //4  free <-----+
   //              |
int nptr = 4; // --+

typedef union {
    int data;
    void (*code)(void);
    int link;
    int name;
} cell_t;


cell_t dict[DMAX] = {
    {.link = -1},      // 0 <------+
    {.name =  0},      // 1        |
    {.code = one},     // 2*       |
};                     // 3  free  |
                       //          |
int link = 0; // ------------------+
int dptr = 3;

void nil(void) {
    printf("nil\n");
}

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

void dot(void) {
    printf("%d\n", pop());
}

void pushlit(void) {
    ip++;
    push(dict[ip].data);
}

void branch(void) {
    ip++;
    ip += dict[ip].data;
}

void branchnotzero(void) {
    ip++;
    int addr = dict[ip].data;
    if (pop() == 0) {
        printf("zero\n");
    } else {
        printf("not zero\n");
        ip += addr;
    }
}

void *reg_lo = (void *)0xffffffff;
void *reg_hi = (void *)0;
void reg(void *code) {
    if (code < reg_lo) reg_lo = code;
    if (code > reg_hi) reg_hi = code;
}

int valid(void *code) {
    if (code >= reg_lo && code <= reg_hi) return 1;
    return 0;
}

void execute(int xt) {
    //printf("xt = %d\n", xt);
    if (xt == FEXIT) {
        printf("invalid xt\n");
        return;
    }
    ip = xt;
    void (*code)(void) = dict[ip].code;
    if (valid(code)) {
        code();
    } else printf("invalid code\n");
}

void dolist(void) {
    rpush(ip);
    while (1) {
        ip++;
        if (dict[ip].data == FEXIT) break;
#if 1
        dict[dict[ip].data].code();
#else
        execute(ip);
#endif
    }
    ip = rpop();
}

void create(char *s) {
    int n = strlen(s);
    strcpy(&name[nptr], s);
    dict[dptr].link = link;
    link = dptr;
    dptr++;
    dict[dptr].name = nptr;
    dptr++;
    nptr += n;
    nptr++;
}

void comma(int c) {
    dict[dptr++].data = c;
}

void walk(int start, int (*fn)(int, int *, void *), int *e, void *v) {
    int c = start;
    while (1) {
        int n = fn(c, e, v);
        if (n < 0) break;
        if (dict[c].link < 0) break;
        c = dict[c].link;
    }
}

int _dump(int c, int *e, void *v) {
    int *pc = (int *)v;
    int i;
    printf("\n");
    printf("LINK [%d] %d\n",   c+0, dict[c+0].link);
    printf("NAME [%d] \"%s\"\n", c+1, &name[dict[c+1].name]);
    printf("CODE [%d] %d\n",   c+2, dict[c+2].data);
    for (i=c+3; i<*pc; i++) {
        printf("     [%d] %d\n", i, dict[i].data);
    }
    *pc = c;
    return 0;
}
void dump(void) {
    int lc = dptr;
    walk(link, _dump, NULL, &lc);
    printf("\n");
}

int _words(int c, int *e, void *v) {
    printf("<%s> ", &name[dict[c+1].name]);
    return 0;
}
void words(void) {
    walk(link, _words, NULL, NULL);
    printf("\n");
}

int _find(int c, int *e, void *v) {
    if (strcmp((char *)v, &name[dict[c+1].name]) == 0) {
        *e = c+2;
        return -1;
    }
    return 0;
}
int find(char *name) {
    int e = -1;
    walk(link, _find, &e, name);
    return e;
}

#define REG(c) {printf("%16s = %d\n", #c, (int)c); reg(c);}

int main(int argc, char *argv[]) {
    int i;
    int e = 0;

#if 1
    for (i=dptr; i<DMAX; i++) {
        dict[i].data = FEXIT;
    }
#endif
    
    REG(nil);
    REG(one);
    REG(two);
    REG(three);
    REG(four);
    REG(dolist);
    REG(dot);
    REG(branch);
    REG(branchnotzero);

    create("two");
    comma((int)&two);
    
    create("three");
    comma((int)&three);
    
    create("four");
    comma((int)&four);
    
    create("pushlit");
    comma((int)&pushlit);
    
    create("dot");
    comma((int)&dot);
    
    create("branch");
    comma((int)&branch);

    create("branchnotzero");
    comma((int)&branchnotzero);

    create("testit");
    comma((int)&dolist);
    comma(find("one"));
    comma(find("branch"));
    comma(0);
    //comma(2);
    comma(find("pushlit"));
    comma(999);
    comma(find("dot"));
    comma(find("pushlit"));
    comma(666);
    comma(find("dot"));
    comma(FEXIT);

    create("2nd-level");
    comma((int)&dolist);
    comma(find("two"));
    comma(find("testit"));
    comma(FEXIT);

    create("3rd-level");
    comma((int)&dolist);
    comma(find("three"));
    comma(find("2nd-level"));
    comma(FEXIT);

    create("branchtest");
    comma((int)&dolist);
    comma(find("pushlit"));
    comma(1);
    comma(find("branchnotzero"));
    comma(3);
    comma(find("four"));
    comma(find("branch"));
    comma(1);
    comma(find("three"));
    comma(FEXIT);

    words();
    dump();

    printf("\n* test 001 *\n");
    execute(find("one"));

    printf("\n* test 002 *\n");
    execute(find("two"));

    printf("\n* test 003 *\n");
    execute(find("testit"));

    printf("\n* test 004 *\n");
    execute(find("2nd-level"));

    printf("\n* test 005 *\n");
    execute(find("3rd-level"));

    printf("\n* test 006 *\n");
    execute(find("branchtest"));

    return 0;
}

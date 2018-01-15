#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <string.h>
#include <unistd.h>

// INNER STUFF { --------
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

#define NEXT (-1)

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
    int prev;
    int name;
} cell_t;


cell_t dict[DMAX] = {
    {.prev = -1},      // 0 <------+
    {.name =  0},      // 1        |
    {.code = one},     // 2*       |
};                     // 3  free  |
                       //          |
int prev = 0; // ------------------+
int here = 3;

void nil(void) {
    // printf("nil\n");
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
    if (xt == NEXT) {
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
        if (dict[ip].data == NEXT) break;
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
    dict[here].prev = prev;
    prev = here;
    here++;
    dict[here].name = nptr;
    here++;
    nptr += n;
    nptr++;
}

void comma(int c) {
    dict[here++].data = c;
}

void walk(int start, int (*fn)(int, int *, void *), int *e, void *v) {
    int c = start;
    while (1) {
        int n = fn(c, e, v);
        if (n < 0) break;
        if (dict[c].prev < 0) break;
        c = dict[c].prev;
    }
}

int _dump(int c, int *e, void *v) {
    int *pc = (int *)v;
    int i;
    printf("\n");
    printf("PREV [%d] %d\n",   c+0, dict[c+0].prev);
    printf("NAME [%d] \"%s\"\n", c+1, &name[dict[c+1].name]);
    printf("CODE [%d] %d\n",   c+2, dict[c+2].data);
    for (i=c+3; i<*pc; i++) {
        printf("     [%d] %d\n", i, dict[i].data);
    }
    *pc = c;
    return 0;
}
void dump(void) {
    int lc = here;
    walk(prev, _dump, NULL, &lc);
    printf("\n");
}

int _words(int c, int *e, void *v) {
    printf("%s ", &name[dict[c+1].name]);
    return 0;
}
void words(void) {
    walk(prev, _words, NULL, NULL);
    printf("\n");
}

int _tick(int c, int *e, void *v) {
    if (strcasecmp((char *)v, &name[dict[c+1].name]) == 0) {
        *e = c+2;
        return -1;
    }
    return 0;
}
int tick(char *name) {
    int e = -1;
    walk(prev, _tick, &e, name);
    return e;
}

#define REG(c) {printf("%16s = %d\n", #c, (int)c); reg(c);}

void inner_init(void) {
    int i;
    for (i=here; i<DMAX; i++) {
        dict[i].data = NEXT;
    }
    REG(nil);
    REG(dolist);
    REG(dot);
    REG(branch);
    REG(branchnotzero);

    create("pushlit");
    comma((int)&pushlit);
    
    create(".");
    comma((int)&dot);
    
    create("branch");
    comma((int)&branch);

    create("branchnotzero");
    comma((int)&branchnotzero);
}

void inner_test(void) {
    REG(one);
    REG(two);
    REG(three);
    REG(four);

    create("two");
    comma((int)&two);
    
    create("three");
    comma((int)&three);
    
    create("four");
    comma((int)&four);
    
    create("testit");
    comma((int)&dolist);
    comma(tick("one"));
    comma(tick("branch"));
    comma(0);
    //comma(2);
    comma(tick("pushlit"));
    comma(999);
    comma(tick("."));
    comma(tick("pushlit"));
    comma(666);
    comma(tick("."));
    comma(NEXT);

    create("2nd-level");
    comma((int)&dolist);
    comma(tick("two"));
    comma(tick("testit"));
    comma(NEXT);

    create("3rd-level");
    comma((int)&dolist);
    comma(tick("three"));
    comma(tick("2nd-level"));
    comma(NEXT);

    create("branchtest");
    comma((int)&dolist);
    comma(tick("pushlit"));
    comma(1);
    comma(tick("branchnotzero"));
    comma(3);
    comma(tick("four"));
    comma(tick("branch"));
    comma(1);
    comma(tick("three"));
    comma(NEXT);

    words();
    dump();

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

    printf("\n* test 006 *\n");
    execute(tick("branchtest"));
}

// INNER STUFF } --------

// OUTER STUFF { --------

#define TMAX (80)
char tokenb[TMAX+1]; // terminal input buffer

int input(void) {
    char *line;
    line = fgets(tokenb, TMAX, stdin);
    if (line == NULL) {
        tokenb[0] = '\0';
        return -1;
    }
    return 0;
}

unsigned char islexeme(char c) {
    switch (c) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '\0':
            return 0;
    }
    return 1;
}

char _lexeme[TMAX+1];
char _ilexeme[TMAX+1];
char _jlexeme[TMAX+1];

/* todo make token accept a buffer addr and len */
int tokenp = 0;
void token(char *lexeme) {
    unsigned char collected = 0;
    //printf("->token(%p)\n", lexeme);
    if (tokenb[tokenp] == '\0') {
        tokenp = -1;
        return;
    }
    while (tokenp < TMAX) {
        if (islexeme(tokenb[tokenp])) {
            lexeme[collected] = tokenb[tokenp];
            collected++;
        } else {
            if (collected) {
                lexeme[collected] = '\0';
                return;
            } else {
                if (tokenb[tokenp] == '\0') {
                    tokenp = -1;
                    return;
                }
            }
        }
        tokenp++;
    }
    tokenp = -1;
    return;
}

int _base = 10;
int _running = 1;

int outer(void) {
    int addr;
    while (_running) {
        token(_ilexeme);
        if (tokenp < 0 || tokenp >= TMAX) {
            //printf("invalid tokenp:%d\n", tokenp);
            tokenp = 0;
            break;
        }
        // try to find lexeme in dictionary
        addr = tick(_ilexeme);
        //printf("_ilexeme<%s>/%d\n", _ilexeme, addr);
        if (addr < 0) {
            // not found, try to interpret as a number
            char *notnumber;
            int n;
            if (_base == 10) {
                if (_ilexeme[0] == '0') {
                    n = strtoul(_ilexeme, &notnumber, 0);
                } else {
                    n = strtol(_ilexeme, &notnumber, 0);
                }
            } else {
                n = strtoul(_ilexeme, &notnumber, _base);
            }
            if (notnumber == _ilexeme) {
                printf("? ");
            } else if (n == UINT32_MAX && (errno == ERANGE)) {
                //printf("? out of range ");
                push(UINT32_MAX);
            } else {
                push(n);
            }
        } else {
            // invoke the function in the dictionary
            execute(addr);

        }
        tokenp++;
    }
    return _running;
}

void mass(void) {
    printf("undefined\n");
}

void bye(void) {
    _running = 0;
}

void u4_create(void) {
    _jlexeme[0] = '\0';
    token(_jlexeme);
    if (_jlexeme[0]) create(_jlexeme);
}

void u4_start(void) {
    REG(words);
    create("words");
    comma((int)&words);
    
    REG(dump);
    create("dump");
    comma((int)&dump);

    REG(bye);
    create("bye");
    comma((int)&bye);

    REG(u4_create);
    create("create");
    comma((int)&u4_create);

    if (isatty(STDIN_FILENO)) {
        while (1) {
            if (input() < 0) break;
            if (outer() == 0) break;
        }
    } else {
        mass();
    }
}

// OUTER STUFF } --------

int main(int argc, char *argv[]) {
    inner_init();
    //inner_test();
    u4_start();
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <string.h>
#include <unistd.h>

/*

s h a m e l e s s . . . 

I stole this ASCII art while watching CoolerVoid compile raptor_waf...

               boing         boing         boing
 e-e           . - .         . - .         . - . 
(\_/)\      ,'       `.   ,'       `.   ,'       `.
 `-'\ `--._._,         . .           . .           .
    '\( ,_.-'                                       
       \                "             "             "
       ^\'              .             .          compile!

*/

// INNER STUFF { --------
#define SMAX (1000)
#define RMAX (1000)
#define NMAX (1000)
#define DMAX (1000)

int stack[SMAX];
int rstack[RMAX];
int sp = 0;
int rs = 0;

void push(int n) {
    if (sp < SMAX) sp++;
    stack[sp] = n;
}

int pop(void) {
    int n = stack[sp];
    if (sp >= 0) sp--;
    return n;
}

void rpush(int n) {
    if (rs < SMAX) rs++;
    rstack[rs] = n;
}

int rpop(void) {
    int n = rstack[rs];
    if (rs >= 0) rs--;
    return n;
}

void u4_rpush(void) {
    rpush(pop());
}

void u4_rpop(void) {
    push(rpop());
}

#define NEXT (-1)
#define NONE (-1)

typedef union {
    int data;
    void (*code)(void);
    int link;
    int name;
} cell_t;

void nil(void);

#define BASE_OFF (0)
#define LOOP_OFF (1)
#define HERE_OFF (2)
#define HEAD_OFF (3)
#define NAME_OFF (4)
#define VMIP_OFF (5)
//
#define WIDE_OFF (6)

#define BASE (dict[BASE_OFF].data)
#define LOOP (dict[LOOP_OFF].data)
#define HERE (dict[HERE_OFF].data)
#define HEAD (dict[HEAD_OFF].data)
#define NAME (dict[NAME_OFF].data)
#define VMIP (dict[VMIP_OFF].data)
//
#define WIDE (dict[WIDE_OFF].data)

cell_t dict[DMAX] = {
    {.data = 10},      // 0 BASE variable
    {.data = 1},       // 1 LOOP running variable
    {.data = 9},       // 2 HERE free dictionary index ----------+
    {.data = 6},       // 3 HEAD index to head of dictionary --+ |
    {.data = 4},       // 4 NAME free string index --+         | |
    {.data = 0},       // 5 VMIP instruction pointer |         | |
    // first "real" dictionary entry (hard-coded)    |         | |
    {.link = NONE},    // 6 <------------------------|---------+ |
    {.name =  0},      // 7 ---------+               |           |
    {.code = nil},     // 8* --------|--+            |           |
};                     // 9 free <---|--|------------|-----------+
//                                   |  |            |
char name[NMAX] = //                 |  |            |
//                                   |  |            |
//   +-------------------------------+  |            |
//   |                                  |            |
//   v                                  |            |
//   0123                               |            |
    "nil\0"; //                         |            |
//   4  free <--------------------------|------------+
//                                      |
void nil(void) { // <-------------------+
    printf("nil\n");
}

char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";

void dot(int np) {
    unsigned int number[64];
    unsigned int n = (unsigned int)np;
    int index = 0;
    if (np < 0 && BASE == 10) {
        putchar('-');
        n = -n;
    }
    if (!n) {
        putchar('0');
    } else {
        while (n) {
            number[index] = n % BASE;
            n /= BASE;
            index++;
        }
        index--;
        for(; index >= 0; index--) {
            putchar(digits[number[index]]);
        }
    }
    putchar(' ');
}

void u4_dot(void) {
    dot(pop());
}

void pushlit(void) {
    VMIP++;
    push(dict[VMIP].data);
}

void branch(void) {
    VMIP++;
    VMIP += dict[VMIP].data;
}

void branchnotzero(void) {
    VMIP++;
    int addr = dict[VMIP].data;
    if (pop() == 0) {
        printf("zero\n");
    } else {
        printf("not zero\n");
        VMIP += addr;
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
    VMIP = xt;
    void (*code)(void) = dict[VMIP].code;
    if (valid(code)) {
        code();
    } else printf("invalid code\n");
}

void u4_execute(void) {
    int xt = pop();
    execute(xt);
}

void dolist(void) {
    rpush(VMIP);
    while (1) {
        VMIP++;
        if (dict[VMIP].data == NEXT) break;
#if 1
        dict[dict[VMIP].data].code();
#else
        execute(VMIP);
#endif
    }
    VMIP = rpop();
}

void create(char *s) {
    int n = strlen(s);
    strcpy(&name[NAME], s);
    dict[HERE].link = HEAD;
    HEAD = HERE;
    HERE++;
    dict[HERE].name = NAME;
    HERE++;
    NAME += n;
    NAME++;
}

void comma(int c) {
    dict[HERE++].data = c;
}

void u4_comma(void) {
    comma(pop());
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

int walk_dump(int c, int *e, void *v) {
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
    int lc = HERE;
    walk(HEAD, walk_dump, NULL, &lc);
    printf("\n");
}

int walk_words(int c, int *e, void *v) {
    printf("%s ", &name[dict[c+1].name]);
    return 0;
}
void words(void) {
    walk(HEAD, walk_words, NULL, NULL);
    printf("\n");
}

int walk_tick(int c, int *e, void *v) {
    if (strcasecmp((char *)v, &name[dict[c+1].name]) == 0) {
        *e = c+2;
        return NONE;
    }
    return 0;
}
int tick(char *name) {
    int e = NONE;
    walk(HEAD, walk_tick, &e, name);
    return e;
}

//#ifdef INNER_TEST

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
        comma((int)dolist);
        comma(tick("one"));
        comma(tick("pushlit"));
        comma(999);
        comma(tick("."));
        comma(tick("pushlit"));
        comma(666);
        comma(tick("."));
        comma(NEXT);

        create("2nd-level");
        comma((int)dolist);
        comma(tick("two"));
        comma(tick("testit"));
        comma(NEXT);

        create("3rd-level");
        comma((int)dolist);
        comma(tick("three"));
        comma(tick("2nd-level"));
        comma(NEXT);

        create("branchtest");
        comma((int)dolist);
        comma(tick("jnz"));
        comma(3);
        comma(tick("four"));
        comma(tick("jmp"));
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

//#endif

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

char token_lexeme[TMAX+1];
char token_ilexeme[TMAX+1];
char token_jlexeme[TMAX+1];

/* todo make token accept a buffer addr and len */
int tokenp = 0;
void token(char *lexeme) {
    unsigned char collected = 0;
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

void u4_create(void) {
    token_jlexeme[0] = '\0';
    token(token_jlexeme);
    if (token_jlexeme[0]) create(token_jlexeme);
}

int outer(void) {
    int addr;
    while (LOOP) {
        token(token_ilexeme);
        if (tokenp < 0 || tokenp >= TMAX) {
            //printf("invalid tokenp:%d\n", tokenp);
            tokenp = 0;
            break;
        }
        // try to find lexeme in dictionary
        addr = tick(token_ilexeme);
        //printf("_ilexeme<%s>/%d\n", token_ilexeme, addr);
        if (addr < 0) {
            // not found, try to interpret as a number
            char *notnumber;
            int n;
            if (BASE == 10) {
                if (token_ilexeme[0] == '0') {
                    n = strtoul(token_ilexeme, &notnumber, 0);
                } else {
                    n = strtol(token_ilexeme, &notnumber, 0);
                }
            } else {
                n = strtoul(token_ilexeme, &notnumber, BASE);
            }
            if (notnumber == token_ilexeme) {
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
    return LOOP;
}

void mass(void) {
    printf("undefined\n");
}

void bye(void) {
    LOOP = 0;
}

void u4_tick(void) {
    token_jlexeme[0] = '\0';
    token(token_jlexeme);
    if (token_jlexeme[0]) push(tick(token_jlexeme));
}

void fetch(void) {
    int addr = pop();
    push(dict[addr].data);
}

void store(void) {
    int addr = pop();
    int value = pop();
    dict[addr].data = value;
}

void add(void) {
    int b = pop();
    int a = pop();
    push(a+b);
}

void subtract(void) {
    int b = pop();
    int a = pop();
    push(a-b);
}

void multiply(void) {
    int b = pop();
    int a = pop();
    push(a*b);
}

void divide(void) {
    int b = pop();
    int a = pop();
    if (b == 0) push(0);
    else push(a/b);
}

void modulus(void) {
    int b = pop();
    int a = pop();
    if (b == 0) push(0);
    else push(a%b);
}

void wfetch(void) {
    unsigned int n = pop();
    if ((n & 0xfffffffc) == n) {
        unsigned int *addr = (unsigned int *)n;
        push(*addr);
    } else {
        printf("unaligned read not allowed\n");
        push(0);
    }
}

void wstore(void) {
    unsigned int n = pop();
    if ((n & 0xfffffffc) == n) {
        unsigned int *addr = (unsigned int *)n;
        int value = pop();
        *addr = value;
    } else {
        printf("unaligned write not allowed\n");
    }
}

void dots(void) {
    int i;
    printf("<%d(%d)> ", sp, BASE);
    for (i=0; i<=sp; i++) {
        dot(stack[i+1]);
    }
}

void decimal(void) {
    BASE = 10;
}

void hex(void) {
    BASE = 16;
}

void binary(void) {
    BASE = 2;
}

typedef struct {
    char *name;
    void (*code)(void);
} bulk_t;

bulk_t vocab[] = {
    {">r",      u4_rpush},
    {"r>",      u4_rpop},
    {"bye",     bye},
    {"pushlit", pushlit},
    {"jmp",     branch},
    {"jnz",     branchnotzero},
    {"execute", u4_execute},
    {"dolist",  dolist},
    {"create",  u4_create},
    {",",       u4_comma},
    {".",       u4_dot},
    {"words",   words},
    {"dump",    dump},
    {"'",       u4_tick},
    {"@",       fetch},
    {"!",       store},
    {"w@",      wfetch},
    {"w!",      wstore},
    {"+",       add},
    {"-",       subtract},
    {"*",       multiply},
    {"/",       divide},
    {"%",       modulus},
    {".s",      dots},
    {"decimal", decimal},
    {"hex",     hex},
    {"binary",  binary},
    {NULL,      NULL}
};

void makecode(char *name, void (*code)(void)) {
    reg(code);
    create(name);
    comma((int)code);
}

void makevar(char *name, int n) {
    create(name);
    comma((int)pushlit);
    comma(n);
}

void u4_init(void) {
    int i;
    bulk_t *bulk = vocab;

    reg(nil);

    for (i=HERE; i<DMAX; i++) {
        dict[i].data = NEXT;
    }

    makevar("base",    BASE_OFF);
    makevar("running", LOOP_OFF);
    makevar("here",    HERE_OFF);
    makevar("head",    HEAD_OFF);
    makevar("name",    NAME_OFF);
    makevar("ip",      VMIP_OFF);

    while (bulk->name != NULL) {
        makecode(bulk->name, bulk->code);
        bulk++;
    }

#if 0
    // example
    create("decimal");
    comma((int)dolist);
    comma(tick("pushlit"));
    comma(10);
    comma(tick("pushlit"));
    comma(BASE_OFF);
    comma(tick("store"));
    comma(NEXT);
#endif

    makecode("test", inner_test);
}

void u4_start(void) {
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
    u4_init();
    u4_start();
    return 0;
}

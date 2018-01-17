#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/errno.h>
#include <string.h>
#include <strings.h>
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

#include "u4.h"

// INNER STUFF { --------
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

void cold(void);

#if 0
#define DPTR_OFF
#define DSTK_OFF
#define DPTR
#define DSTR

#define RPTR_OFF
#define RSTK_OFF
#define DPTR
#define DSTR
#endif

// vm registers in dictionary
#define BASE_OFF (0)
#define LOOP_OFF (1)
#define HERE_OFF (2)
#define HEAD_OFF (3)
#define NAME_OFF (4)
#define VMIP_OFF (5)
#define COLS_OFF (6)

// first dictionary entry
#define LINK0 (7)
#define NAME0 (8)
#define FLAG0 (9)
#define CODE0 (10)
#define FREE0 (11)
#define TEXT0 "cold"
#define TLEN0 sizeof(TEXT0)

#define BASE (dict[BASE_OFF].data)
#define LOOP (dict[LOOP_OFF].data)
#define HERE (dict[HERE_OFF].data)
#define HEAD (dict[HEAD_OFF].data)
#define NAME (dict[NAME_OFF].data)
#define VMIP (dict[VMIP_OFF].data)
#define COLS (dict[COLS_OFF].data)

cell_t dict[DMAX] = {
    [BASE_OFF] = {.data = 10},    // 0 BASE variable
    [LOOP_OFF] = {.data = 1},     // 1 LOOP running variable
    [HERE_OFF] = {.data = FREE0}, // 2 HERE free dictionary index -----------+
    [HEAD_OFF] = {.data = LINK0}, // 3 HEAD index to head of dictionary --+  |
    [NAME_OFF] = {.data = TLEN0}, // 4 NAME free string index             |  |
    [VMIP_OFF] = {.data = 0},     // 5 VMIP instruction pointer           |  |
    [COLS_OFF] = {.data = 80},    // 6 COLS instruction pointer           |  |
    // first "real" dictionary entry (hard-coded)                         |  |
    [LINK0] = {.link = NONE},     // 7 <----------------------------------+  |
    [NAME0] = {.name = 0},        // 8 ------------+                         |
    [FLAG0] = {.flag = 0},        // 9             |                         |
    [CODE0] = {.code = cold},     // 10  -------+  |                         |
    [FREE0 ... DMAX-1] = {.data = NEXT}, // <---|--|-- (FREE0) --------------+
};                                //            |  |
char name[NMAX] = TEXT0; // <-------------------|--+
//                                              |
void cold(void) { // <--------------------------+
    printf("cold\n");
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

void branchzero(void) {
    VMIP++;
    int addr = dict[VMIP].data;
    if (pop()) {
        //printf("not zero\n");
    } else {
        //printf("zero\n");
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

void docolon(void) {
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
    dict[HERE].flag = 0;
    HERE++;
    dict[HERE].code = pushlit;
    dict[HERE+1].data = HERE+1;
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

#define DLINK (0)
#define DNAME (1)
#define DFLAG (2)
#define DCODE (3)
#define DDATA (4)

void show_header(int c) {
    printf("LINK [%d] %d\n",     c+DLINK, dict[c+DLINK].link);
    printf("NAME [%d] [%d]%s\n", c+DNAME, dict[c+DNAME].name, &name[dict[c+DNAME].name]);
    printf("FLAG [%d] %d\n",     c+DFLAG, dict[c+DFLAG].flag);
    printf("CODE [%d] %d\n",     c+DCODE, dict[c+DCODE].data);
}

int walk_dump(int c, int *e, void *v) {
    int *pc = (int *)v;
    int i;
    printf("\n");
    show_header(c);
    for (i=c+DDATA; i<*pc; i++) {
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

int words_arg = 0;
int walk_words(int c, int *e, void *v) {
    int n = strlen(&name[dict[c+DNAME].name]);
    if (n + words_arg > COLS) {
        printf("\n");
        words_arg = 0;
    } words_arg += n;
    printf("%s ", &name[dict[c+DNAME].name]);
    return 0;
}
void words(void) {
    words_arg = 0;
    walk(HEAD, walk_words, NULL, NULL);
    if (words_arg) printf("\n");
}

int walk_tick(int c, int *e, void *v) {
    if (strcasecmp((char *)v, &name[dict[c+DNAME].name]) == 0) {
        *e = c+DCODE;
        return NONE;
    }
    return 0;
}
int tick(char *name) {
    int e = NONE;
    walk(HEAD, walk_tick, &e, name);
    return e;
}


// INNER STUFF } --------

// OUTER STUFF { --------

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

int walk_forget(int c, int *e, void *v) {
    if (strcasecmp((char *)v, &name[dict[c+DNAME].name]) == 0) {
        HEAD = dict[c+DLINK].link;
        HERE = c;
        NAME = dict[c+DNAME].name;
        return NONE;
    }
    return 0;
}
void forget(char *name) {
    walk(HEAD, walk_forget, NULL, name);
}
void u4_forget(void) {
    token_jlexeme[0] = '\0';
    token(token_jlexeme);
    if (token_jlexeme[0]) forget(token_jlexeme);
}

int see_arg = 0;
int walk_see(int c, int *e, void *v) {
    if (strcasecmp((char *)v, &name[dict[c+DNAME].name]) == 0) {
        int i;
        show_header(c);
        for (i=c+DDATA; i<see_arg; i++) {
            printf("     [%d] %d\n", i, dict[i].data);
        }
        return NONE;
    }
    see_arg = c;
    return 0;
}
void see(char *name) {
    see_arg = HERE;
    walk(HEAD, walk_see, NULL, name);
}
void u4_see(void) {
    token_jlexeme[0] = '\0';
    token(token_jlexeme);
    if (token_jlexeme[0]) see(token_jlexeme);
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

int isaligned(int n) {
    if (n & 3) return 0;
    return 1;
}

void wfetch(void) {
    unsigned int n = pop();
    if (isaligned(n)) {
        unsigned int *addr = (unsigned int *)n;
        push(*addr);
    } else {
        printf("unaligned read not allowed\n");
        push(0);
    }
}

void wstore(void) {
    unsigned int n = pop();
    if (isaligned(n)) {
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

void cr(void) {
    printf("\n");
    fflush(stdout);
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

void allot(void) {
    int n = pop();
    HERE += n;
}

void callot(void) {
    int n = pop();
    n /= 4;
    HERE += (((n + 3) / 4) * 4);
}

#define DLEN 16
void cdump(char *a, int len) {
    int i;
    int n = 0;
    char l[DLEN+1];
    for (i=0; i<len; i++) {
        char c = a[i];
        if (n == 0) {
            printf("%04x ", i);
        }
        if (i % 8 == 0) printf(" ");
        printf("%02x ", a[i]);
        if (c < 32 || c > 126) c = '_';
        l[n] = c;
        n++;
        l[n] = '\0';
        if (n >= DLEN) {
            printf(" %s\n", l);
            n = 0;
        }
    }
    if (n > 0) {
        printf("%*s", (DLEN-n)*3, " ");
        printf(" %s\n", l);
    }
}

void u4_cdump(void) {
    int len = pop();
    char *addr = (char *)pop();
    cdump(addr, len);
}

void u4_name(void) {
    push((int)&name);
}

bulk_t vocab[] = {
    {">r",       u4_rpush},
    {"r>",       u4_rpop},
    {"bye",      bye},
    {"pushlit",  pushlit},
    {"branch",   branch},
    {"?branch",  branchzero},
    {"execute",  u4_execute},
    {"docolon",  docolon},
    {"create",   u4_create},
    {",",        u4_comma},
    {".",        u4_dot},
    {"cr",       cr},
    {"words",    words},
    {"dump",     dump},
    {"'",        u4_tick},
    {"@",        fetch},
    {"!",        store},
    {"w@",       wfetch},
    {"w!",       wstore},
    {"+",        add},
    {"-",        subtract},
    {"*",        multiply},
    {"/",        divide},
    {"%",        modulus},
    {".s",       dots},
    {"decimal",  decimal},
    {"hex",      hex},
    {"binary",   binary},
    {"see",      u4_see},
    {"allot",    allot},
    {"callot",   callot},
    {"cdump",    u4_cdump},
    // use this to impede forgetting previous words
    {"$name",    u4_name},
    {"forget",   u4_forget},
    {NULL,       NULL}
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
    bulk_t *bulk = vocab;

    reg(cold);

    makevar("base",    BASE_OFF);
    makevar("running", LOOP_OFF);
    makevar("here",    HERE_OFF);
    makevar("head",    HEAD_OFF);
    makevar("name",    NAME_OFF);
    makevar("ip",      VMIP_OFF);
    makevar("cols",    COLS_OFF);

    while (bulk->name != NULL) {
        makecode(bulk->name, bulk->code);
        bulk++;
    }

#if 0
    // example
    create("decimal");
    comma((int)docolon);
    comma(tick("pushlit"));
    comma(10);
    comma(tick("pushlit"));
    comma(BASE_OFF);
    comma(tick("store"));
    comma(NEXT);
#endif
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

// bridge functions for umon replacement { ----

void start_umon(void) {
    u4_init();
    u4_start();
}

void native(char *name, void (*fn)(void)) {
    reg(fn);
    create(name);
    comma((int)fn);
}

int depth(void) {
    return sp+1;
}

// } ----

#ifdef U4_MAIN
int main(int argc, char *argv[]) {
    u4_init();
    u4_start();
    return 0;
}
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/errno.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

/*
        ___ 
 _   __/   |   ( -- ) : u4 117 emit 52 emit 33 emit cr ; u4
| | | | /| |_  http://octetta.com
| |_| |__   _| A C-based Forth-inspired ball of code.
|__/|_|  |_|   (c) 1997-2018 joseph.stewart@gmail.com

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

void drop(void) {
    pop();
}

int top(void) {
    if (sp >= 0) return stack[sp];
    return 0;
}

void u4_dup(void) {
    push(top());
}

void swap(void) {
    if (sp >= 1) {
        int n = stack[sp];
        stack[sp] = stack[sp-1];
        stack[sp-1] = n;
    }
}

int depth(void) {
    return sp+1;
}

void u4_depth(void) {
    push(depth());
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

void rdrop(void) {
    rpop();
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
#define MODE_OFF (7)

// first dictionary entry
#define LINK0 (8)
#define NAME0 (9)
#define FLAG0 (10)
#define CODE0 (11)
#define FREE0 (12)
#define TEXT0 "ABCD"
#define TLEN0 sizeof(TEXT0)

#define BASE (dict[BASE_OFF].data)
#define LOOP (dict[LOOP_OFF].data)
#define HERE (dict[HERE_OFF].data)
#define HEAD (dict[HEAD_OFF].data)
#define NAME (dict[NAME_OFF].data)
#define VMIP (dict[VMIP_OFF].data)
#define COLS (dict[COLS_OFF].data)
#define MODE (dict[MODE_OFF].data)

cell_t dict[DMAX] = {
    [BASE_OFF] = {.data = 10},       // output radix variable
    [LOOP_OFF] = {.data = 1},        // running variable
    [HERE_OFF] = {.data = FREE0},    // free dictionary index -----------+
    [HEAD_OFF] = {.data = LINK0},    // index to head of dictionary --+  |
    [NAME_OFF] = {.data = TLEN0},    // free string index             |  |
    [VMIP_OFF] = {.data = 0},        // instruction pointer           |  |
    [COLS_OFF] = {.data = 80},       // instruction pointer           |  |
    [MODE_OFF] = {.data = 0},        // current interpret mode        |  |
    // first "real" dictionary entry (hard-coded)                     |  |
    [LINK0] = {.link = NONE},        // <-----------------------------+  |
    [NAME0] = {.name = 0},           // -----------+                     |
    [FLAG0] = {.flag = FLAG_HIDDEN}, //            |                     |
    [CODE0] = {.code = cold},        // --------+  |                     |
    [FREE0 ... DMAX-1] = {.data = NEXT}, // <---|--|-- (FREE0) ----------+
};                                   //         |  |
char name[NMAX] = TEXT0; // <-------------------|--+
//                                              |
void cold(void) { // <--------------------------+
    printf("cold\n");
}

void name_base(void) {
    push(NAME);
}

void name_fetch(void) {
    int n = pop();
    if (n >= 0 && n < NMAX) {
        push((int)name[n]);
    }
}

void name_store(void) {
    int n = pop();
    int c = pop();
    if (n >= 0 && n < NMAX) {
        name[n] = (char)c;
    }
}

char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";

void dot(int np, int pad, int gap) {
    unsigned int number[64];
    unsigned int n = (unsigned int)np;
    int index = 0;
    int out = 0;
    int i;
    if (np < 0 && BASE == 10) {
        putchar('-');
        n = -n;
    }
    if (!n) {
        putchar('0');
        out++;
    } else {
        while (n) {
            number[index] = n % BASE;
            n /= BASE;
            index++;
        }
        index--;
        for(; index >= 0; index--) {
            putchar(digits[number[index]]);
            out++;
        }
    }
    if (out < pad) {
        for (i=0; i<pad-out; i++) {
            putchar(' ');
        }
    }
    for (i=0; i<gap; i++) {
        putchar(' ');
    }
}

void u4_dot(void) {
    dot(pop(), 0, 1);
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
    if (xt > DMAX) {
        printf("invalid xt\n");
        return;
    }
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

void comma(int addr) {
    dict[HERE++].data = addr;
}

void u4_comma(void) {
    comma(pop());
}

void walk(int start, int (*fn)(int, int *, void *), int *e, void *v) {
    int link = start;
    while (1) {
        int n = fn(link, e, v);
        if (n < 0) break;
        if (dict[link].link < 0) break;
        link = dict[link].link;
    }
}

// offsets from LINK to field
#define L2LINK (0)
#define L2NAME (1)
#define L2FLAG (2)
#define L2CODE (3)
#define L2DATA (4)

// offsets from CODE to field
#define C2LINK (-3)
#define C2NAME (-2)
#define C2FLAG (-1)
#define C2CODE (0)
#define C2DATA (1)

char show_str[80];
char *show_flags(int n) {
    show_str[0] = '\0';
    if (n & FLAG_IMMEDIATE) { strcat(show_str, " IMMEDIATE"); } else { strcat(show_str, " USEMODE"); }
    if (n & FLAG_HIDDEN) { strcat(show_str, " HIDDEN"); } else { strcat(show_str, " VISIBLE"); }
    return show_str;
}

void show_header(int link) {
    printf("LINK [%d] %d\n",     link + L2LINK, dict[link + L2LINK].link);
    printf("NAME [%d] [%d]%s\n", link + L2NAME, dict[link + L2NAME].name, &name[dict[link+L2NAME].name]);
    printf("FLAG [%d] (%d)%s\n",   link + L2FLAG, dict[link + L2FLAG].flag, show_flags(dict[link + L2FLAG].flag));
    printf("CODE [%d] %d\n",     link + L2CODE, dict[link + L2CODE].data);
}

int walk_dump(int link, int *e, void *v) {
    int *plink = (int *)v;
    int i;
    printf("\n");
    show_header(link);
    for (i=link + L2DATA; i<*plink; i++) {
        printf("     [%d] %d\n", i, dict[i].data);
    }
    *plink = link;
    return 0;
}
void dump(void) {
    int dlink = HERE;
    walk(HEAD, walk_dump, NULL, &dlink);
    printf("\n");
}

int words_arg = 0;
int walk_words(int c, int *e, void *v) {
    //if ((dict[c+L2FLAG].data & MASK_HIDE) == FLAG_HIDDEN) return 0;
    char *str = &name[dict[c+L2NAME].name];
    int n = strlen(str) + 1;
    if ((n + words_arg) > COLS) {
        printf("\n");
        words_arg = n;
    } else {
        words_arg += n;
    }
    printf("%s ", str);
    return 0;
}
void words(void) {
    words_arg = 0;
    walk(HEAD, walk_words, NULL, NULL);
    if (words_arg) printf("\n");
}

int walk_tick(int c, int *e, void *v) {
    char *str = &name[dict[c+L2NAME].name];
    if (strcasecmp((char *)v, str) == 0) {
        *e = c+L2CODE;
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
    if (strcasecmp((char *)v, &name[dict[c+L2NAME].name]) == 0) {
        HEAD = dict[c+L2LINK].link;
        HERE = c;
        NAME = dict[c+L2NAME].name;
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
    if (strcasecmp((char *)v, &name[dict[c+L2NAME].name]) == 0) {
        int i;
        show_header(c);
        for (i=c+L2DATA; i<see_arg; i++) {
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
                n = UINT32_MAX;
            }
            if (MODE == FLAG_COMPILE) {
                // stuff token and number to dictionary
                comma(tick("pushlit"));
                comma(n);
            } else {
                push(n);
            }
        } else {
            if (MODE == FLAG_COMPILE) {
                if ((dict[addr + C2FLAG].data & MASK_MODE) == FLAG_IMMEDIATE) {
                    // invoke the function in the dictionary
                    execute(addr);
                } else {
                    // stuff token to dictionary
                    comma(addr);
                }
            } else {
                // invoke the function in the dictionary
                execute(addr);
            }

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

void band(void) {
    int b = pop();
    int a = pop();
    push(a&b);
}

void bor(void) {
    int b = pop();
    int a = pop();
    push(a|b);
}

void bxor(void) {
    int b = pop();
    int a = pop();
    push(a^b);
}

void blsh(void) {
    int b = pop();
    int a = pop();
    push(a<<b);
}

void brsh(void) {
    int b = pop();
    int a = pop();
    push(a>>b);
}

void clt(void) {
    int b = pop();
    int a = pop();
    push(a<b);
}

void cgt(void) {
    int b = pop();
    int a = pop();
    push(a>b);
}

void ceq(void) {
    int b = pop();
    int a = pop();
    push(a==b);
}

void cand(void) {
    int b = pop();
    int a = pop();
    push(a&&b);
}

void cor(void) {
    int b = pop();
    int a = pop();
    push(a||b);
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
    printf("<%d(%d)> ", sp+1, BASE);
    for (i=0; i<=sp; i++) {
        dot(stack[i], 0, 1);
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

void compile(void) {
    MODE = FLAG_COMPILE;
}

void colon(void) {
    if (MODE == FLAG_COMPILE) return;
    u4_create();
    comma((int)docolon);
    compile();
}

void immediate(void) {
    MODE = FLAG_IMMEDIATE;
}

void semi(void) {
    if (MODE == FLAG_IMMEDIATE) return;
    comma(NEXT);
    immediate();
}

void emit(void) {
    putchar(pop());
    fflush(stdout);
}

bulk_t vocab[] = {
    {"$$",       name_base},
    {"$@",       name_fetch},
    {"$!",       name_store},
    {"depth",    u4_depth},
    {"drop",     drop},
    {"dup",      u4_dup},
    {"swap",     swap},
    {"rdrop",    rdrop},
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
    //
    {"+",        add},
    {"-",        subtract},
    {"*",        multiply},
    {"/",        divide},
    {"%",        modulus},
    //
    {"&",        band},
    {"|",        bor},
    {"^",        bxor},
    {"<<",       blsh},
    {">>",       brsh},
    //
    {"&&",       cand},
    {"||",       cor},
    {"<",        clt},
    {">",        cgt},
    {"==",       ceq},
    //
    {".s",       dots},
    {"decimal",  decimal},
    {"hex",      hex},
    {"binary",   binary},
    {"see",      u4_see},
    {"allot",    allot},
    {"callot",   callot},
    {"cdump",    u4_cdump},
    {"$name",    u4_name},
    {"cr",       cr},
    {"immediate", immediate},
    {"compile",   compile},
    {":",         colon},
    {";",         semi},
    {"emit",      emit},
    // ****
    {"forget",   u4_forget}, // place here to impede forgetting previous words
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
    int n;
    sp = -1;
    rs = -1;

    reg(cold);

    makevar("base",    BASE_OFF);
    makevar("running", LOOP_OFF);
    makevar("here",    HERE_OFF);
    makevar("head",    HEAD_OFF);
    makevar("name",    NAME_OFF);
    makevar("ip",      VMIP_OFF);
    makevar("cols",    COLS_OFF);
    makevar("mode",    MODE_OFF);

    while (bulk->name != NULL) {
        makecode(bulk->name, bulk->code);
        bulk++;
    }

#if 1
    // example
    create("hhg");
    comma((int)docolon);
    comma(tick("pushlit"));
    comma(47);
    comma(tick("."));
    comma(NEXT);
#endif

    n = tick(";");
    if (n >= 0) {
        dict[n + C2FLAG].data |= FLAG_IMMEDIATE;
    } else {
        printf("something went wrong. contact joe\n");
    }

    MODE = FLAG_IMMEDIATE;
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

// } ----

#ifdef U4_MAIN
int main(int argc, char *argv[]) {
    u4_init();
    u4_start();
    return 0;
}
#endif

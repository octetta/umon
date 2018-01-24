#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/errno.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

/*
        ___ 
 _   __/   |    ( -- ) : u4 117 emit 52 emit 33 emit cr ; u4
| | | | /| |_   http://octetta.com
| |_| |__   _|  u4(tm) A C-based Forth-inspired ball of code.
|__/|_|  |_|tm  (c) 1996-2018 joseph.stewart@gmail.com
 
In it's current form, this is used by:

 _   _  __  __  ___  _  __
| | | |/_ |/_ |/ _ \| |/_ |
| |_| | | | | | |_| | | | |
|__/|_| |_| |_|\___/|_| |_| (umon)

aka
    __        __
   / /_,__ __/ /__
  / //_/ // / / _ \
 / ,< / // / / // /
/_/\_|\_, /_/\___/ (kylo)
     /___/         

aka
   _
  |_|___  ___ _ __  ___ _ _
  | / _ \/ +_| `  \/ _ \ ` \
 _/ \___/\___|_|_|_\___/_|_| (joemon)
|__/

...but has roots stretching back to 1994-ish in C-form and 1980 in "others".

*/

#include "u4.h"

char *splash =
"         ___    \n"
"  _   __/   |   \n"
" | | | | /| |_  \n"
" | |_| |__   _| \n"
" |__/|_|  |_|tm \n";

// stack manipulation functions {

int stack[SMAX];
int rstack[RMAX];
int sp = 0;
int rs = 0;


void push(int n) {
    printf("push(%d)\n", n);
    if (sp < SMAX) sp++;
    stack[sp] = n;
}

int pop(void) {
    int n = stack[sp];
    printf("pop()=%d\n", n);
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
    printf("rpush(%d)\n", n);
    if (rs < SMAX) rs++;
    rstack[rs] = n;
}

int rpop(void) {
    int n = rstack[rs];
    printf("rpop()=%d\n", n);
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

// }


// Setup dictionary {

// Forward reference to the first C-fn in dictionary.
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
#define PMPT_OFF (7)
#define MODE_OFF (8)

// first dictionary entry
#define LINK0 (9)
#define NAME0 (10)
#define FLAG0 (11)
#define CODE0 (12)
#define FREE0 (13)
#define _U4_0 "_u4_\0"
#define PMPT0 "ok \0"
#define TEXT0 _U4_0 PMPT0
#define NLEN0 (sizeof(_U4_0) + sizeof(PMPT0))

#define BASE (dict[BASE_OFF].data)
#define LOOP (dict[LOOP_OFF].data)
#define HERE (dict[HERE_OFF].data)
#define HEAD (dict[HEAD_OFF].data)
#define NAME (dict[NAME_OFF].data)
#define VMIP (dict[VMIP_OFF].data)
#define COLS (dict[COLS_OFF].data)
#define PMPT (dict[PMPT_OFF].data)
#define MODE (dict[MODE_OFF].data)

cell_t dict[DMAX] = {
    [BASE_OFF] = {.data = 10},       // output radix variable
    [LOOP_OFF] = {.data = 1},        // running variable
    [HERE_OFF] = {.data = FREE0},    // free dictionary index -----------+
    [HEAD_OFF] = {.data = LINK0},    // index to head of dictionary --+  |
    [NAME_OFF] = {.data = NLEN0},    // free string index             |  |
    [VMIP_OFF] = {.data = 0},        // instruction pointer           |  |
    [COLS_OFF] = {.data = 80},       // terminal column width         |  |
    [PMPT_OFF] = {.data = sizeof(_U4_0)-1}, // interpreter prompt     |  |
    [MODE_OFF] = {.data = F_IMDT},   // current interpret mode        |  |
    // first "real" dictionary entry (hard-coded)                     |  |
    [LINK0] = {.link = NONE},        // <-----------------------------+  |
    [NAME0] = {.name = 0},           // -----------+                     |
    [FLAG0] = {.flag = F_HIDE | F_PRIM}, //        |                     |
    [CODE0] = {.code = cold},        // --------+  |                     |
    [FREE0 ... DMAX-1] = {.data = NEXT}, // <---|--|-- (FREE0) ----------+
};                                   //         |  |
char name[NMAX] = TEXT0; // <-------------------|--+
//                                              |
void cold(void) { // <--------------------------+
    printf("%s\n", splash);
}

// }

// Helper functions to access the name array {
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

// }

// Magic Forth-ish number printing function. Supports output base >= 2 and <= 36
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

// }

void pushlit(void) {
    VMIP++;
    push(dict[VMIP].data);
}

void branch(void) {
    VMIP++;
    VMIP += dict[VMIP].data;
}

void zerobranch(void) {
    VMIP++;
    int addr = dict[VMIP].data;
    if (pop()) {
        //printf("not zero\n");
    } else {
        //printf("zero\n");
        VMIP += addr;
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


// Use the provided index that points to a function and call it as a C-fn.
void execute(int xt) {
    printf("execute(%d)\n", xt);
    if (xt == NEXT) return;
    if (xt < 0 || xt > DMAX) {
        printf("invalid xt\n");
        return;
    }
    if (dict[xt + C2FLAG].data & F_PRIM) {
        VMIP = xt;
        void (*code)(void) = dict[VMIP].code;
        code();
    } else printf("invalid code\n");
}

// Provide a version that uses the stack.
void u4_execute(void) {
    int xt = pop();
    execute(xt);
}

void docolon(void) {
    // VMIP contains XT for me followed by the current list of tokens
    int IP = VMIP;
    printf("when starting docolon, VMIP = %d\n", VMIP);
    while (1) {
        IP++;
        rpush(IP);
        IP = dict[IP].data;
        printf("dict[%d].data : %d\n", IP, dict[IP].data);
        if (dict[IP].data == NEXT) break;
        execute(dict[IP].data);
    }
    VMIP = rpop();
    printf("when starting docolon, VMIP = %d\n", VMIP);
}

void __docolon(void) {
    rpush(VMIP);
    //printf("+VMIP:%d\n", VMIP);
    while (1) {
        VMIP++;
        //printf(" VMIP:%d\n", VMIP);
        if (dict[VMIP].data == NEXT) break;
        dict[dict[VMIP].data].code();
    }
    //printf(" EXIT\n");
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
    printf("dict[%d].data = %d\n", HERE+1, addr);
    dict[HERE++].data = addr;
}

void u4_comma(void) {
    comma(pop());
}

/*

     : a . ;

     +------+
 +-->| LINK | --> index of previous link in dictionary array
 |   +------+
 |   | NAME | --> index of "a" in name array
 |   +------+
 |   | FLAG |
 |   +------+
 |   | CODE | --> function pointer to c-code of "docolon"
 |   +------+
 |   | DATA | --+ index to "." in dictionary array
 |   +------+   |
 |   | EXIT |   |
 |   +------+   |
 |              |
 |   +------+   |
 +---| LINK |   | 
     +------+   |
     | NAME |   | --> index to "." in name array
     +------+   |
     | FLAG |   |
     +------+   |
     | CODE |<--+ --> function pointer to c-code of "."
     +------+

*/

void walk(int start, int (*fn)(int, int *, void *), int *e, void *arg) {
    int link = start;
    while (1) {
        int n = fn(link, e, arg);
        if (n < 0) break;
        if (dict[link].link < 0) break;
        link = dict[link].link;
    }
}

char show_str[80];
char *show_flags(int n) {
    show_str[0] = '\0';
    if (n & F_IMDT) { strcat(show_str, " IMDT"); } else { strcat(show_str, " CURR"); }
    if (n & F_HIDE) { strcat(show_str, " HIDE"); } else { strcat(show_str, " SHOW"); }
    if (n & F_PRIM) { strcat(show_str, " PRIM"); }
    return show_str;
}

void see_header(int link) {
    printf("LINK [%d] %d\n",      link + L2LINK, dict[link + L2LINK].link);
    printf("NAME [%d] $%d{%s}\n", link + L2NAME, dict[link + L2NAME].name, &name[dict[link+L2NAME].name]);
    printf("FLAG [%d] (%d)%s\n",  link + L2FLAG, dict[link + L2FLAG].flag, show_flags(dict[link + L2FLAG].flag));
    printf("CODE [%d] %d\n",      link + L2CODE, dict[link + L2CODE].data);
}

void u4__see(void) {
    see_header(pop() + C2LINK);
}

int walk_dump(int link, int *ret, void *arg) {
    int *plink = (int *)arg;
    int i;
    printf("\n");
    see_header(link);
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

int walk_words(int link, int *ret, void *arg) {
    int *pwords_arg = (int *)arg;
    if ((dict[link + L2FLAG].data & M_HIDE) == F_HIDE) return 0;
    char *str = &name[dict[link + L2NAME].name];
    int n = strlen(str) + 1;
    if ((n + *pwords_arg) > COLS) {
        printf("\n");
        *pwords_arg = n;
    } else {
        *pwords_arg += n;
    }
    printf("%s ", str);
    return 0;
}
void words(void) {
    int words_arg = 0;
    walk(HEAD, walk_words, NULL, &words_arg);
    if (words_arg) printf("\n");
}

int walk_tick(int link, int *ret, void *arg) {
    char *str = &name[dict[link + L2NAME].name];
    if (strcasecmp((char *)arg, str) == 0) {
        *ret = link + L2CODE;
        return NONE;
    }
    return 0;
}
int tick(char *name) {
    int ret = NONE;
    walk(HEAD, walk_tick, &ret, name);
    return ret;
}

char tokenb[TMAX+1]; // terminal input buffer

int input(void) {
    char *line;
    if (PMPT) {
        printf("%s", &name[PMPT]);
        fflush(stdout);
    }
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
        see_header(c);
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
            if (MODE == F_COMP) {
                // stuff token and number to dictionary
                comma(tick("pushlit"));
                comma(n);
            } else {
                push(n);
            }
        } else {
            if (MODE == F_COMP) {
                if ((dict[addr + C2FLAG].data & M_MODE) == F_IMDT) {
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

void bnot(void) {
    push(~pop());
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

void cnot(void) {
    push(!pop());
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

void dotr(void) {
    int i;
    printf("<%d(%d)> ", rs+1, BASE);
    for (i=0; i<=rs; i++) {
        dot(rstack[i], 0, 1);
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
            printf("%08x ", i);
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
    dict[dict[HEAD_OFF].data + L2FLAG].data &= (~F_IMDT);
    //MODE = F_COMP;
}

void colon(void) {
    if (MODE == F_COMP) return;
    u4_create();
    comma((int)docolon);
    MODE = F_COMP;
}

void immediate(void) {
    dict[dict[HEAD_OFF].data + L2FLAG].data |= F_IMDT;
    //MODE = F_IMDT;
}

void hide(void) {
    dict[dict[HEAD_OFF].data + L2FLAG].data |= F_HIDE;
}

void show(void) {
    dict[dict[HEAD_OFF].data + L2FLAG].data &= (~F_HIDE);
}

void semi(void) {
    if (MODE == F_IMDT) return;
    comma(NEXT);
    MODE = F_IMDT;
}

void emit(void) {
    putchar(pop());
    fflush(stdout);
}

void here(void) {
    push(HERE);
}

void c2l(void) {
    stack[sp] += C2LINK;
}

void l2c(void) {
    stack[sp] += L2CODE;
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
    {"0branch",  zerobranch},
    {"execute",  u4_execute},
    {"docolon",  docolon},
    {"create",   u4_create},
    {"here",     here},
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
    {"~",        bnot},
    //
    {"&&",       cand},
    {"||",       cor},
    {"<",        clt},
    {">",        cgt},
    {"==",       ceq},
    {"not",      cnot},
    //
    {".s",       dots},
    {".r",       dotr},
    {"decimal",  decimal},
    {"hex",      hex},
    {"binary",   binary},
    {"see",      u4_see},
    {"(see)",    u4__see},
    {"allot",    allot},
    {"callot",   callot},
    {"cdump",    u4_cdump},
    {"$name",    u4_name},
    {"cr",       cr},
    //{"immediate", immediate},
    //{";",         semi},
    //{"compile",   compile},
    {":",         colon},
    {"hide",      hide},
    {"show",      show},
    {"emit",      emit},
    {">link",     c2l},
    {">code",     l2c},
    // ****
    {"forget",   u4_forget}, // place here to impede forgetting previous words
    {NULL,       NULL}
};

void primitive(void) {
    dict[dict[HEAD_OFF].data + L2FLAG].data |= F_PRIM;
}

void makecode(char *name, void (*code)(void)) {
    create(name);
    primitive();
    comma((int)code);
}

void makevar(char *name, int n) {
    create(name);
    comma((int)pushlit);
    comma(n);
}

void u4_init(void) {
    bulk_t *bulk = vocab;
    sp = -1;
    rs = -1;

    makevar("base",    BASE_OFF);
    makevar("cols",    COLS_OFF);
    makevar("prompt",  PMPT_OFF);
#if 0
    makevar("(running)", LOOP_OFF);
    makevar("(here)",    HERE_OFF);
    makevar("(head)",    HEAD_OFF);
    makevar("(name)",    NAME_OFF);
    makevar("(ip)",      VMIP_OFF);
    makevar("(mode)",    MODE_OFF);
#endif

    makevar("F_IMDT", F_IMDT);
    makevar("F_HIDE", F_HIDE);

    while (bulk->name != NULL) {
        makecode(bulk->name, bulk->code);
        bulk++;
    }

    makecode("compile", compile);
    immediate();
    makecode("immediate", immediate);
    immediate();
    makecode(";", semi);
    immediate();

#if 0
    // example
    create("hhg");
    comma((int)docolon);
    comma(tick("pushlit"));
    comma(47);
    comma(tick("."));
    comma(NEXT);
#endif
    cold();
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

// bridge functions for umon replacement { ----

void start_umon(void) {
    u4_init();
    u4_start();
}

void native(char *name, void (*fn)(void)) {
    makecode(name, fn);
}

// } ----

#ifdef U4_MAIN
int main(int argc, char *argv[]) {
    u4_init();
    u4_start();
    return 0;
}
#endif


#if 0

 _ _  _ _  _ _  _ _ 
| | || | || | || | |
\___|\___|\___|\___|


#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

static char *splash =
"         ___    \n"
" ,_  ,__/   |   \n"
" | | | | /|_|_  \n"
" | |_| |__:_:_| \n"
" |__/|_|  |_|tm \n";

/*
: u4 117 emit 52 emit 33 emit cr ; u4
http://octetta.com
u4(tm) A C-based Forth-inspired ball of code.
(c) 1996-2018 joseph.stewart@gmail.com
*/

#define DATA long
#define FNUM double

typedef union {
    void (*func)(void);
    DATA data;
    FNUM fnum;
} cell_t;

#define DICT_MAX (16384)
#define NAME_MAX (4096)
#define LIFO_MAX (256)
#define RSTK_MAX (256)
#define TOKB_MAX (256)

#if 1
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
static struct termios key_old, key_new;
static void key_reset(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &key_old);
}
static void key_use(void) {
    static char first = 1;
    if (first) {
        tcgetattr(STDIN_FILENO, &key_old);
        tcgetattr(STDIN_FILENO, &key_new);
        key_new.c_lflag &= ~(ICANON | ECHO);
        key_new.c_cc[VMIN] = 1;
        key_new.c_cc[VTIME] = 0;
        first = 0;
        atexit(key_reset);
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &key_new);
}
static int key_get(void) {
    int c;
    if (!isatty(STDIN_FILENO)) return getchar();
    key_use();
    c = getchar();
    key_reset();
    return c;
}
static int key_get_timeout(int timeout) {
    int c = -1;
    struct timeval tv;
    fd_set fs;
    if (!isatty(STDIN_FILENO)) return getchar();
    key_use();
    tv.tv_usec = 0;
    tv.tv_sec = timeout;
    FD_ZERO(&fs);
    FD_SET(STDIN_FILENO, &fs);
    select(STDIN_FILENO + 1, &fs, NULL, NULL, &tv);
    if (FD_ISSET(STDIN_FILENO, &fs)) {
        c = getchar();
    }
    key_reset();
    return c;
}
#endif

void u4_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void one(void) {
    u4_printf("#one#\n");
}

void two(void) {
    u4_printf("#two#\n");
}

// NAME LINK TYPE DATA

#define QUIT (-2)
#define EXIT (-1)
#define NOOP (0)
#define PRIM (1)
#define LIST (2)
#define PUSH (3)
#define GOTO (4)
#define COND (5)

#define LIFO_OFF (0) // data stack pointer
#define RSTK_OFF (1) // return stack pointer
#define BASE_OFF (2) // input / output number radix
#define LOOP_OFF (3) // outer loop flag
#define HERE_OFF (4) // free dictionary entry
#define HEAD_OFF (5) // link to last work in dictionary
#define NAME_OFF (6) // free string index
#define TOKB_OFF (7) // token buffer index
#define COLS_OFF (8) // column wrap number
#define PMPT_OFF (9) // offset of prompt in string table
#define MODE_OFF (10) // interpret / compile mode
#define DBUG_OFF (11) // debug enable flag

#define WORD_OFF (DBUG_OFF + 1)

#define NAME0 (WORD_OFF + 0)
#define LINK0 (WORD_OFF + 1)
#define TYPE0 (WORD_OFF + 2)
#define FUNC0 (WORD_OFF + 3)
#define FREE0 (WORD_OFF + 4)

#define COLD0 "cold\0"
#define PMPT0 "ok \0"
#define TEXT0 COLD0 PMPT0
#define NLEN0 (sizeof(COLD0) + sizeof(PMPT0))

#define TOP (FREE0)

#define BASE (dict[BASE_OFF].data)
#define LOOP (dict[LOOP_OFF].data)
#define HERE (dict[HERE_OFF].data)
#define NAME (dict[NAME_OFF].data)
#define TOKB (dict[TOKB_OFF].data)
#define COLS (dict[COLS_OFF].data)
#define LIFO (dict[LIFO_OFF].data)
#define RSTK (dict[RSTK_OFF].data)
#define HEAD (dict[HEAD_OFF].data)
#define PMPT (dict[PMPT_OFF].data)
#define MODE (dict[MODE_OFF].data)
#define DBUG (dict[DBUG_OFF].data)

#define F_IMME (1<<0)
#define F_COMP (1<<1)
#define F_HIDE (1<<2)
#define F_PRIM (1<<3)
#define F_PUSH (1<<4)
#define F_LIST (1<<5)
#define F_BRAC (1<<6)

#define F_SHOW (~F_HIDE)

void cold(void);

cell_t dict[DICT_MAX] = {
    [LIFO_OFF] = {.data = 0},        // data stack pointer
    [BASE_OFF] = {.data = 10},       // output radix variable
    [LOOP_OFF] = {.data = 1},        // running variable
    [HERE_OFF] = {.data = FREE0},    // free dictionary index -----------+
    [HEAD_OFF] = {.data = LINK0},    // index to head of dictionary --+  |
    [NAME_OFF] = {.data = NLEN0},    // free string index             |  |
    [TOKB_OFF] = {.data = 0},        // token input buffer index      |  |
    [COLS_OFF] = {.data = 80},       // terminal column width         |  |
    [PMPT_OFF] = {.data = sizeof(COLD0)-1}, // interpreter prompt     |  |
    [MODE_OFF] = {.data = F_IMME},          // current interpret mode |  |
    [DBUG_OFF] = {.data = 0},               // current debug mode     |  |
    // first "real" dictionary entry (hard-coded)                     |  |
    [NAME0] = {.data = 0},           // -----------+                  |  |
    [LINK0] = {.data = EXIT},        // <----------|------------------+  |
    [TYPE0] = {.data = F_PRIM | F_HIDE}, //        |                     |
    [FUNC0] = {.func = cold},        // --------+  |                     |
    [FREE0] = {.data = EXIT},        // <-------|--|---------------------+
};                                   //         |  |   
char name[NAME_MAX] = TEXT0; // <---------------|--+   
char tokb[TOKB_MAX] = ""; // token input buffer |
//                                              |
void cold(void) { // <--------------------------+
    u4_printf("%s\n", splash);
}

void debug(const char *format, ...) {
    if (DBUG) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}

char name[NAME_MAX];
cell_t lifo[LIFO_MAX];
cell_t rstk[RSTK_MAX];

void push(DATA n) {
    if (LIFO < LIFO_MAX) LIFO++;
    lifo[LIFO].data = n;
}

DATA pop(void) {
    DATA n = lifo[LIFO].data;
    if (LIFO > 0) LIFO--;
    return n;
}

void drop(void) {
    pop();
}

DATA top(void) {
    if (LIFO >= 0) return lifo[LIFO].data;
    return 0;
}

void duplicate(void) {
    push(top());
}

void swap(void) {
    if (LIFO >= 1) {
        DATA n = top();
        lifo[LIFO] = lifo[LIFO-1];
        lifo[LIFO-1].data = n;
    }
}

void depth(void) {
    push(LIFO);
}

void rpush(DATA n) {
    if (RSTK < RSTK_MAX) RSTK++;
    rstk[RSTK].data = n;
}

void u4_rpush(void) {
    rpush(pop());
}

DATA rpop(void) {
    DATA n = rstk[RSTK].data;
    if (RSTK > 0) RSTK--;
    return n;
}

void u4_rpop(void) {
    push(rpop());
}

void rdrop(void) {
    rpop();
}

DATA dolist(DATA addr);

void bye(void) {
    LOOP = 0;
}

DATA fetch(DATA addr) {
    return dict[addr].data;
}

void u4_fetch(void) {
    push(fetch(pop()));
}

void store(DATA addr, DATA data) {
    dict[addr].data = data;
}

void u4_store(void) {
    DATA addr = pop();
    DATA data = pop();
    store(addr, data);
}

DATA isaligned(DATA n) {
    if (n & (sizeof(DATA)-1)) return 0;
    return 1;
}

void wfetch(void) {
    DATA n = pop();
    if (isaligned(n)) {
        DATA *addr = (DATA *)n;
        push(*addr);
    }
}

void wstore(void) {
    DATA n = pop();
    if (isaligned(n)) {
        DATA *addr = (DATA *)n;
        DATA value = pop();
        *addr = value;
    }
}

void cfetch(void) {
    unsigned char *addr = (unsigned char *)pop();
    push(*addr);
}

void cstore(void) {
    unsigned char *addr = (unsigned char *)pop();
    unsigned char value = (unsigned char )pop();
    *addr = value;
}

// {

void add(void) {
    DATA b = pop();
    DATA a = pop();
    push(a+b);
}

void subtract(void) {
    DATA b = pop();
    DATA a = pop();
    push(a-b);
}

void multiply(void) {
    DATA b = pop();
    DATA a = pop();
    push(a*b);
}

void divide(void) {
    DATA b = pop();
    DATA a = pop();
    if (b == 0) push(0);
    else push(a/b);
}

void modulus(void) {
    DATA b = pop();
    DATA a = pop();
    if (b == 0) push(0);
    else push(a%b);
}

void band(void) {
    DATA b = pop();
    DATA a = pop();
    push(a&b);
}

void bor(void) {
    DATA b = pop();
    DATA a = pop();
    push(a|b);
}

void bxor(void) {
    DATA b = pop();
    DATA a = pop();
    push(a^b);
}

void blsh(void) {
    DATA b = pop();
    DATA a = pop();
    push(a<<b);
}

void brsh(void) {
    DATA b = pop();
    DATA a = pop();
    push(a>>b);
}

void bnot(void) {
    push(~pop());
}

void clt(void) {
    DATA b = pop();
    DATA a = pop();
    push(a<b);
}

void cgt(void) {
    DATA b = pop();
    DATA a = pop();
    push(a>b);
}

void ceq(void) {
    DATA b = pop();
    DATA a = pop();
    push(a==b);
}

void cand(void) {
    DATA b = pop();
    DATA a = pop();
    push(a&&b);
}

void cor(void) {
    DATA b = pop();
    DATA a = pop();
    push(a||b);
}

void cnot(void) {
    push(!pop());
}

// }

#if 0

#define PRIM_MIN 0
#define PRIM_MAX 1

static cell_t prim_limit[2] = {
    [PRIM_MIN] = {.data = -1},
    [PRIM_MAX] = {.data = -1},
};

#endif

DATA doprim(DATA addr) {
    dict[addr].func();
    return addr + 1;
}

char *type2name(DATA addr) {
    return &name[dict[addr - 2].data];
}

#define execute_return(n) execute_depth--; debug("execute_leave(%d) depth:%ld\n", n, execute_depth); return n

static DATA execute_depth = 0;

DATA execute(DATA addr) {
    execute_depth++;
    DATA code = fetch(addr);
    debug("execute(%ld) depth:%ld\n", addr, execute_depth);
    if (code & F_PRIM) {
        debug("[%ld] PRIM \"%s\"\n", addr, type2name(addr));
        doprim(addr+1);
        execute_return(PRIM);
    }
    if (code & F_LIST) {
        debug("[%ld] LIST\n", addr);
        dolist(addr+1);
        execute_return(LIST);
    }
    if (code & F_PUSH) {
        debug("[%ld] PUSH\n", addr);
        push(fetch(addr+1));
        execute_return(PRIM);
    }
    if (code == EXIT) {
        debug("[%ld] EXIT\n", addr);
        execute_return(EXIT);
    }
    execute_return(QUIT);
}

void u4_execute(void) {
    execute(pop());
}

DATA dolist(DATA addr) {
    debug("dolist(%ld)\n", addr);
    while (addr != EXIT) {
        DATA code = fetch(addr);
        DATA target;
        switch (code) {
            default:
                debug("[%ld] DATA code=%ld\n", addr, code);
                switch (execute(code)) {
                    case QUIT:
                        debug("QUIT\n");
                        addr = EXIT;
                    default:
                        addr++;
                        break;
                }
                break;
            case NOOP:
                debug("[%ld] NOOP\n", addr);
                addr++;
                break;
            case EXIT:
                debug("[%ld] EXIT\n", addr);
                addr = EXIT;
                break;
            case PUSH:
                debug("[%ld] PUSH %ld\n", addr, fetch(addr+1));
                addr++;
                push(fetch(addr++));
                break;
            case GOTO:
                target = fetch(addr+1);
                debug("[%ld] GOTO jmp %ld\n", addr, addr+1+target);
                addr++;
                addr += target;
                break;
            case COND:
                target = fetch(addr+1);
                DATA arg = pop();
                debug("[%ld] COND pop()=%ld jmp %ld\n", addr, arg, addr+1+target);
                addr++;
                if (arg == 0) {
                    addr += target;
                    debug("---- =0 -> jmp %ld\n", addr);
                } else {
                    addr++;
                    debug("---- !0 -> jmp %ld\n", addr);
                }
                break;
        }
    }
    return addr;
}

void comma(DATA d) {
    store(HERE, d);
    HERE++;
}

void u4_comma(void) {
    comma(pop());
}

void fcomma(void *f) {
    dict[HERE].func = f;
    HERE++;
}

void npush(char c) {
    name[NAME] = c;
    NAME++;
}

void u4_link(void) {
    dict[HERE].data = HEAD;
    HEAD = HERE;
    HERE++;
}

DATA link2name(DATA ptr) {
    return ptr - 1;
}

DATA link2flag(DATA ptr) {
    return ptr + 1;
}

void words(void) {
    DATA next = HEAD;
    DATA t = 0;
    DATA w = 0;
    while (next >= 0) {
        DATA n = link2name(next);
        if (fetch(link2flag(next)) & F_HIDE) {
        } else {
            char *match = &name[fetch(n)];
            w = strlen(match) + 1;
            if ((t + w) > COLS) {
                u4_printf("\n");
                t = w;
            } else {
                t += w;
            }
            u4_printf("%s ", match);
        }
        next = fetch(next);
    }
    u4_printf("\n");
}

void see(char *what) {
    DATA next = HEAD;
    DATA last = EXIT;
    while (next >= 0) {
        DATA n = link2name(next);
        char *match = &name[fetch(n)];
        if (strcasecmp(what, match) == 0) {
            DATA i;
            DATA n = 0;
            char *s = "";
            if (last == EXIT) last = HERE+1;
            next--;
            last--;
            u4_printf("\"%s\" @ dict[%ld-%ld]\n", what, next, last-1);
            for (i = next; i < last; i++) {
                switch (n) {
                    case 0: s = "NAME "; break;
                    case 1: s = "LINK "; break;
                    case 2: s = "TYPE "; break;
                    default: s = ""; break;
                }
                u4_printf("[%ld] %s%ld\n", i, s, fetch(i));
                n++;
            }
            break;
        }
        last = next;
        next = fetch(next);
    }
}

void walk(void) {
    DATA next = HEAD;
    DATA stop = HERE;
    char *line = "   ";
    char *plus = "  _";
    char *diag = "|  ";
    while (next >= 0) {
        DATA i;
        DATA n = link2name(next);
        DATA f = link2flag(next);
        char *match = &name[fetch(n)];
        u4_printf("%s \n", line);
        u4_printf("%s\"%s\" @ dict[%ld-%ld]\n", line, match, n, stop-1);
        u4_printf("%s [%ld] NAME %ld\n", line, n, fetch(n));
        u4_printf("%s [%ld] LINK %ld\n", plus, next, fetch(next));
        if (fetch(next) >= 0) {
            line = "|  ";
            plus = "+->";
            diag = " / ";
        } else {
            line = "   ";
            plus = "   ";
            diag = "   ";
        }
        u4_printf("%s [%ld] TYPE %ld\n", diag, f, fetch(f));
        for (i=next+2; i<stop; i++) {
            u4_printf("%s [%ld] %ld\n", line, i, fetch(i));
        }
        stop = link2name(next);
        next = fetch(next);
    }
}

DATA tick(char *what) {
    DATA next = HEAD;
    while (next >= 0) {
        char *match = &name[fetch(link2name(next))];
        if (strcasecmp(what, match) == 0) {
            return link2flag(next);
        }
        next = fetch(next);
    }
    return EXIT;
}

void forget(char *what) {
    DATA next = HEAD;
    while (next >= 0) {
        char *match = &name[fetch(link2name(next))];
        if (strcasecmp(what, match) == 0) {
            HEAD = fetch(next);
            HERE = next;
            NAME = fetch(next-1);
            return;
        }
        next = fetch(next);
    }
}

void create(char *s) {
    comma(NAME);
    while (1) {
        char c = *s;
        npush(c);
        if (c == '\0') break;
        s++;
    }
    u4_link();
}

void here(void) {
    push(HERE);
}

void decimal(void) {
    BASE = 10;
}

void binary(void) {
    BASE = 2;
}

void hex(void) {
    BASE = 16;
}

void allot(void) {
    DATA n = pop();
    HERE += n;
}
    
void callot(void) {
    DATA n = pop();
    n /= 4;
    HERE += (((n + 3) / 4) * 4);
}

void u4_prim(char *name, void (*func)(void)) {
    create(name);
    comma(F_PRIM);
    fcomma(func);
}

void constant(char *name, DATA data) {
    create(name);
    comma(F_PUSH);
    comma(data);
}

void variable(char *name) {
    create(name);
    comma(F_LIST);
    comma(PUSH);
    comma(HERE);
    comma(EXIT);
}

// INNER {

#define TMAX (256)

char tokenb[TMAX+1]; // terminal input buffer

char *u4_gets(void) {
    return fgets(tokenb, TMAX, stdin);
}

DATA input(void) {
    char *line;
    if (MODE != F_COMP && PMPT) {
        u4_printf("%s", &name[PMPT]);
        fflush(stdout);
    }
    line = u4_gets();
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

/* todo make token accept a buffer addr and len */
DATA tokenp = 0;
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

void u4_token(void) {
    tokb[TOKB] = '\0';
    token(tokb);
    if (tokb[TOKB] != '\0') push(1);
    else push(0);
}

void u4_tick(void) {
    u4_token();
    if (pop()) push(tick(tokb));
}

void u4_create(void) {
    u4_token();
    if (pop()) {
        create(tokb);
        comma(F_PUSH);
        comma(HERE+1);
    }
}

void u4_constant(void) {
    u4_token();
    if (pop()) constant(tokb, pop());
}

void u4_see(void) {
    u4_token();
    if (pop()) see(tokb);
}

void u4_forget(void) {
    u4_token();
    if (pop()) forget(tokb);
}

void colon(void) {
    if (MODE == F_COMP) return;
    u4_token();
    if (pop()) {
        create(tokb);
        comma(F_LIST);
        MODE = F_COMP;
    }
}

void openbracket(void) {
    MODE = F_COMP | F_BRAC;
}

void semi(void) {
    if (MODE == F_IMME) {
        return;
    }
    comma(EXIT);
    MODE = F_IMME;
}

void closebracket(void) {
    MODE = F_IMME;
}

typedef struct {
    char *name;
    void (*func)(void);
} bulk_t;

// }

// -----
/*

     : a . ;

     +------+
     | NAME |---> index of "a" in name array
     +------+
 +-->| LINK |---> index of previous link in dictionary array
 |   +------+
 |   | TYPE |---> F_LIST
 |   +------+
 |   | DATA |---+ index to "." in dictionary array
 |   +------+   |
 |   | EXIT |   |
 |   +------+   |
 |              |
 |              |
 |              |
 |   +------+   |
 |   | NAME |-------> index to "." in name array
 |   +------+   |
 +---| LINK |   | 
     +------+   v
     | TYPE |---+---> F_PRIM
     +------+    
     | DATA |-------> function pointer to c-code of "."
     +------+

*/
// -----

// OUTER {

#define U4_MAX_UINT (0-1)

DATA outer(void) {
    DATA addr;
    DATA base = BASE;
    char token_ilexeme[TMAX+1];
    if (base < 2 || base > 60) base = 10;
    while (LOOP) {
        token(token_ilexeme);
        if (tokenp < 0 || tokenp >= TMAX) {
            tokenp = 0;
            break;
        }
        // try to find lexeme in dictionary
        addr = tick(token_ilexeme);
        if (addr < 0) {
            // not found, try to interpret as a number
            char *notnumber;
            DATA n;
            if (base == 10) {
                if (token_ilexeme[0] == '0') {
                    n = strtoul(token_ilexeme, &notnumber, 0);
                } else {
                    n = strtol(token_ilexeme, &notnumber, 0);
                }
            } else {
                n = strtoul(token_ilexeme, &notnumber, base);
            }
            if (notnumber == token_ilexeme) {
                u4_printf("? ");
            } else if (n == U4_MAX_UINT && (errno == ERANGE)) {
                u4_printf("? out of range ");
                n = U4_MAX_UINT;
            }
            if (MODE & F_COMP) {
                // stuff token and number to dictionary
                if ((MODE & F_BRAC) == 0) {
                    debug("PUSH -> [%ld]\n", HERE);
                    comma(PUSH);
                }
                // stuff number to dictionary
                debug("number %ld -> [%ld]\n", n, HERE);
                comma(n);
            } else {
                push(n);
            }
        } else {
            if (MODE == F_COMP) {
                //if (dict[addr].data & F_IMME) {
                if (fetch(addr) & F_IMME) {
                    // invoke the function in the dictionary
                    execute(addr);
                } else {
                    // stuff token to dictionary
                    debug("token %ld -> [%ld]\n", addr, HERE);
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

// }

// -----

// STUFF {

// Magic Forth-ish number printing function. Supports output base >= 2 and <= 60
char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz.-:+=^!/*?&<>()[]{}@%$#~";

#define DOT_SPACE (sizeof(DATA)*8)

void u4_putchar(char c) {
    putchar(c);
}

void dot(DATA np, DATA pad, DATA gap) {
    unsigned DATA number[DOT_SPACE];
    unsigned DATA n = (unsigned DATA)np;
    DATA index = 0;
    DATA out = 0;
    DATA i;
    DATA base = BASE;
    if (base < 2 || base > 60) base = 10;
    if (np < 0 && base == 10) {
        u4_putchar('-');
        n = -n;
    }
    if (!n) {
        u4_putchar('0');
        out++;
    } else {
        while (n) {
            number[index] = n % base;
            n /= base;
            index++;
        }
        index--;
        for(; index >= 0; index--) {
            u4_putchar(digits[number[index]]);
            out++;
        }
    }
    if (out < pad) {
        for (i=0; i<pad-out; i++) {
            u4_putchar(' ');
        }
    }
    for (i=0; i<gap; i++) {
        u4_putchar(' ');
    }
}

#define DPAD (0)
#define DGAP (1)

void u4_dot(void) {
    dot(pop(), DPAD, DGAP);
}

void emit(void) {
    u4_putchar(pop());
}

void key(void) {
    push(key_get());
}
void key_timeout(void) {
    push(key_get_timeout(pop()));
}

void dots(void) {
    DATA i;
    DATA base = BASE;
    if (base < 2 || base > 60) base = 10;
    u4_printf("<%ld(%ld)> ", LIFO, base);
    for (i=1; i<=LIFO; i++) {
        dot(lifo[i].data, 0, 1);
    }
}

void dotr(void) {
    DATA i;
    u4_printf("<%ld(%ld)> ", RSTK, BASE);
    for (i=1; i<=RSTK; i++) {
        dot(rstk[i].data, 0, 1);
    }
}

// }

#define DLEN 16
void cdump(char *a, DATA len) {
    DATA i;
    DATA n = 0;
    char l[DLEN+1];
    for (i=0; i<len; i++) {
        char c = a[i];
        u4_printf("%02x ", a[i]);
        if (c < 32 || c > 126) c = '.';
        l[n] = c;
        n++;
        l[n] = '\0';
        if (n >= DLEN) {
            u4_printf(" %s\n", l);
            n = 0;
        }
    }
    if (n > 0) {
        if (n < 8) u4_printf(" ");
        u4_printf("%*s", (int)(DLEN-n)*3, " ");
        u4_printf(" %s\n", l);
    }
}

void u4_cdump(void) {
    DATA len = pop();
    char *addr = (char *)pop();
    cdump(addr, len);
}

void name_dump(void) {
    cdump((char *)&name[0], NAME);
}

void immediate(void) {
    //dict[link2flag(dict[HEAD_OFF].data)].data |= F_IMME;
    dict[link2flag(HEAD)].data |= F_IMME;
}

void hide(void) {
    //dict[link2flag(dict[HEAD_OFF].data)].data |= F_HIDE;
    dict[link2flag(HEAD)].data |= F_HIDE;
}

void name_base(void) {
    push(NAME);
}

void name_addr(void) {
    push((DATA)name);
}

void name_fetch(void) {
    DATA n = pop();
    if (n >= 0 && n < NAME_MAX) {
        push(name[n]);
    }
}

void name_store(void) {
    DATA n = pop();
    DATA c = pop();
    if (n >= 0 && n < NAME_MAX) {
        name[n] = (char)c;
    }
}

void name_dot(void) {
    DATA n = pop();
    if (n >= 0 && n < NAME_MAX) {
        u4_printf("%s", &name[n]);
    }
}

void name_push(void) {
    DATA c = pop();
    npush((char)c);
}

void tron(void) {
    DBUG = 1;
}

void troff(void) {
    DBUG = 0;
}

void u4_vm(void) {
    u4_printf("name cur   max   free\n");
    u4_printf("---- ----- ----- ----\n");
    u4_printf("dict %-5ld %-5ld %3d%%\n", HERE, DICT_MAX, HERE*100/DICT_MAX);
    u4_printf("name %-5ld %-5ld %3d%%\n", NAME, NAME_MAX, NAME*100/NAME_MAX);
    u4_printf("lifo %-5ld %-5ld %3d%%\n", LIFO, LIFO_MAX, LIFO*100/LIFO_MAX);
    u4_printf("rstk %-5ld %-5ld %3d%%\n", RSTK, RSTK_MAX, RSTK*100/RSTK_MAX);
}

bulk_t vocab[] = {
    //
    {"vm",        u4_vm},
    {"tron",      tron},
    {"troff",     troff},
    //
    {",",         u4_comma},
    {"immediate", immediate},
    {"bye",       bye},
    {"'",         u4_tick},
    //{"execute",   u4_execute},
    {"see",       u4_see},
    {"forget",    u4_forget},
    //
    {"@",         u4_fetch},
    {"!",         u4_store},
    {"w@",        wfetch},
    {"w!",        wstore},
    {"c@",        cfetch},
    {"c!",        cstore},
    {">r",        u4_rpush},
    {"<r",        u4_rpop},
    {"rdrop",     rdrop},
    //
    {"+",         add},
    {"-",         subtract},
    {"*",         multiply},
    {"/",         divide},
    {"%",         modulus},
    //
    {"&",         band},
    {"|",         bor},
    {"^",         bxor},
    {"<<",        blsh},
    {">>",        brsh},
    {"~",         bnot},
    //
    {"&&",        cand},
    {"||",        cor},
    {"<",         clt},
    {">",         cgt},
    {"==",        ceq},
    {"not",       cnot},
    //
    {"emit",      emit},
    {"key",       key},
    {"kwait",     key_timeout},
    {".",         u4_dot},
    {".s",        dots},
    {".r",        dotr},
    {"$name",     name_addr},
    {"$$",        name_base},
    {"$@",        name_fetch},
    {"$!",        name_store},
    {"$.",        name_dot},
    {">$",        name_push},
    {"$dump",     name_dump},
    {"depth",     depth},
    {"drop",      drop},
    {"dup",       duplicate},
    {"swap",      swap},
    {":",         colon},
    {"[",         openbracket},
    {"create",    u4_create},
    {"constant",  u4_constant},
    {"here",      here},
    {"decimal",   decimal},
    {"binary",    binary},
    {"hex",       hex},
    {"allot",     allot},
    {"callot",    callot},
    {"walk",      walk},
    {"words",     words},
    {NULL,        NULL},
};

void u4_init(void) {
    bulk_t *bulk = vocab;

    u4_prim("execute", u4_execute);
    u4_prim(";", semi); immediate();
    u4_prim("]", closebracket); immediate();

    while (bulk->name != NULL) {
        u4_prim(bulk->name, bulk->func);
        bulk++;
    }

    cold();
}

void u4_start(void) {
    while (1) {
        if (input() < 0) break;
        if (outer() == 0) break;
    }
}

int main(int argc, char *argv[]) {
    u4_init();
    constant("base",     BASE_OFF);
    constant("(loop)",   LOOP_OFF);
    constant("(here)",   HERE_OFF);
    constant("(head)",   HEAD_OFF);
    constant("(name)",   NAME_OFF);
    constant("(cols)",   COLS_OFF);
    constant("(prompt)", PMPT_OFF);
    constant("(mode)",   MODE_OFF);
    constant("(dbug)",   DBUG_OFF);

    constant("F_IMME", F_IMME);
    constant("F_COMP", F_COMP);
    constant("F_HIDE", F_HIDE);
    constant("F_PRIM", F_PRIM);
    constant("F_PUSH", F_PRIM);
    constant("F_LIST", F_LIST);
    constant("F_BRAC", F_BRAC);
    
    constant("(quit)", QUIT);
    constant("(exit)", EXIT);
    constant("(noop)", NOOP);
    constant("(prim)", PRIM);
    constant("(list)", LIST);
    constant("(push)", PUSH);
    constant("(goto)", GOTO);
    constant("(cond)", COND);

    create("cr");
    comma(F_LIST);
    comma(PUSH);
    comma('\n');
    comma(tick("emit"));
    comma(EXIT);

    u4_prim("p0", one);
    u4_prim("p1", two);

    create("t0");
    comma(F_LIST);
    comma(PUSH);
    comma(355);
    comma(PUSH);
    comma(113);
    comma(GOTO);
    comma(1);
    comma(NOOP);
    comma(NOOP);
    comma(tick("p0"));
    comma(NOOP);
    comma(NOOP);
    comma(EXIT);
    
    create("t1");
    comma(F_LIST);
    comma(COND);
    comma(2);
    comma(tick("p0"));
    comma(GOTO);
    comma(3);
    comma(NOOP);
    comma(tick("p1"));
    comma(EXIT);

    u4_start();

    return 0;
}
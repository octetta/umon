#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#ifdef USE_LN
#include "linenoise/linenoise.h"
#endif

#define i8 char
#define u8 unsigned char
#define i16 short
#define u16 unsigned short
#define i32 int
#define u32 unsigned int

#define TMAX (80)
char tokenb[TMAX+1]; // terminal input buffer
char _lexeme[TMAX+1];
char _ilexeme[TMAX+1];
char _jlexeme[TMAX+1];

#define NMAX (64 * 1024)
#define EVE "eve"
char name[NMAX] = EVE;
i32 pname = sizeof(EVE);

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

u32 fn_reg_min = 0xffffffff;
u32 fn_reg_max = 0x0;

#define FMAX(n) (((fn_reg_max)>(n))?(fn_reg_max):(n))
#define FMIN(n) (((fn_reg_min)<(n))?(fn_reg_min):(n))

void fn_reg(void *x) {
    u32 n = (u32)x;
    fn_reg_min = FMIN(n);
    fn_reg_max = FMAX(n);
}

void eve(void);

#define DMAX (64 * 1024)
i32 dict[64 * 1024] = {0, -1, (i32)eve};

i32 _head = 1;
i32 _here = 2;


/*
indirect threaded code model (ITC)
==================================
index into name[] = name of entry (todo? positive = RAM, negative = ROM)
index into dict[] = link to previous entry
function pointer = code field (docolon or enter if this is a list of addresses)
# parameter fields

*/

#define SMAX (256)
i32 stack[SMAX];   // data stack
#define RMAX (256)
i32 rstack[RMAX];  // return stack

i32 sp = 0;
i32 rs = 0;

#define DLEN 16
void cdump(char *a, i32 len) {
    i32 i;
    i32 n = 0;
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

void _mdump(i32 i) {
    char *extra = "";
    if (i == _head) extra = "<";
    else if (i == _here) extra = "+";
    else extra = " ";
    printf("%04x %08x%s  ", i, dict[i], extra);
}
void mdump(i32 offset, i32 len) {
    i32 i;
    for (i=offset; i<offset+len; i+=4) {
        _mdump(i);
        if (i+1 < len) _mdump(i+1);
        if (i+2 < len) _mdump(i+2);
        if (i+3 < len) _mdump(i+3);
        printf("\n");
    }
}

void _comma(i32 val) {
    _here++;
    //printf("%08x -> dict[%5d]\n", val, _here);
    dict[_here] = val;
}

/*
Structure of dictionary entry held in i32 dict[]

NFA -1 = Name Field Address : index into string array of names
LFA  0 = Link Field Address : index of previous dictionary entry
CFA +1 = Code Field Address : address of native code
PFA +2 = Parameter Field Address = beginning of parameters for Code

Pointer to last dictionary entry is held in i32 _head
Pointer to last i32 in dictionary is held in i32 _here

Pointer to free area of char name[] is held in pname

*/

#define NFA(n) ((n)-1)
#define LFA(n) (n)
#define CFA(n) ((n)+1)
#define PFA(n) ((n)+2)

void _create(char *str) {
    i32 len = strlen(str) + 1;
    strcpy(&name[pname], str);
    //printf("%s -> name[%d]\n", str, pname);
    _comma(pname);
    pname += len;
    _comma(_head);
    _head = _here;
}

i32 _rtop(void) {
    return rstack[rs];
}

void _rpush(i32 n) {
    rs++;
    rstack[rs] = n;
}

i32 _rpop(void) {
    i32 n = rstack[rs];
    rs--;
    return n;
}

void push(i32 n) {
    if (sp < SMAX) sp++;
    //printf("%d -> [%d]\n", n, sp);
    stack[sp] = n;
}

i32 pop(void) {
    i32 n = stack[sp];
    //printf("%d <- [%d]\n", n, sp);
    if (sp >= 0) sp--;
    return n;
}

i32 input(void) {
    char *line;
#ifdef USE_LN
    line = linenoise("ok ");
    memset(tokenb, '\0', TMAX);
    if (line == NULL) {
        tokenb[0] = '\0';
        return -1;
    } else {
        strcpy(tokenb, line);
        tokenb[strlen(line)] = '\0';
        linenoiseFree(line);
    }
#else
    line = fgets(tokenb, TMAX, stdin);
    if (line == NULL) {
        tokenb[0] = '\0';
        return -1;
    }
#endif
    return 0;
}

void mass(void) {
}

/*

+-----+-----+---------+-----+
|eve\0|bye\0|decimal\0|hex\0|
+-----+-----+---------+-----+
 ^     ^     ^         ^
 |     |     |         |
 |     |     |         +--(C)
 |     |     |
 |     |     +--(B)
 |     |
 |     +--(A)
 |
+----+----+----+----------+
|name|link|code|parameters|
+----+----+----+----------+

*/

u8 islexeme(char c) {
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
i32 tokenp = 0;
void token(char *lexeme) {
    u8 collected = 0;
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

void dwords(void) {
    i32 addr = LFA(_head);
    while (addr >= 0) {
        if (addr >= 0) {
            printf("%-8s ", &name[dict[NFA(addr)]]);
            printf("NFA:%-5d ", dict[NFA(addr)]);
            printf("LFA:%-5d ", dict[LFA(addr)]);
            printf("CFA:%08x ", dict[CFA(addr)]);
            //printf("PFA:%d ", dict[PFA(addr)]);
            printf("\n");
        }
        addr = dict[LFA(addr)];
    }
}

void words(void) {
    i32 addr = LFA(_head);
    while (addr >= 0) {
        if (addr >= 0) {
            printf("%s ", &name[dict[NFA(addr)]]);
        }
        addr = dict[LFA(addr)];
    }
    printf("\n");
}

// looks for an entry in the dictionary. if found returns the index
// of the code otherwise -1
i32 _tick(char *what) {
    i32 addr = LFA(_head);
    while (addr >= 0) {
        char *match = &name[dict[NFA(addr)]];
        if (strcasecmp(what, match) == 0) {
            return CFA(addr);
        }
        addr = dict[LFA(addr)];
    }
    return -1;
}

#define DECIMAL "%d "
#define HEX "%x "
#define OCTAL "%o "

i32 _base = 10;
i32 _running = 1;

void binary(void)  { _base = 2; }
void octal(void)   { _base = 8; }
void decimal(void) { _base = 10; }
void hex(void)     { _base = 16; }
void base12(void)  { _base = 12; }
void base24(void)  { _base = 24; }
void base32(void)  { _base = 32; }
void base36(void)  { _base = 36; }

char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";

void _dot(i32 np) {
    u32 number[64];
    u32 n = (u32)np;
    int index = 0;
    if (np < 0 && _base == 10) {
        putchar('-');
        n = -n;
    }
    if (!n) {
        putchar('0');
    } else {
        while (n) {
            number[index] = n % _base;
            n /= _base;
            index++;
        }
        index--;
        for(; index >= 0; index--) {
            putchar(digits[number[index]]);
        }
    }
    putchar(' ');
}

void dot(void) {
    _dot(pop());
}

void add(void) {
    i32 b = pop();
    i32 a = pop();
    push(a+b);
}

void subtract(void) {
    i32 b = pop();
    i32 a = pop();
    push(a-b);
}

void multiply(void) {
    i32 b = pop();
    i32 a = pop();
    push(a*b);
}

void divide(void) {
    i32 b = pop();
    i32 a = pop();
    if (b == 0) push(0);
    else push(a/b);
}

void modulus(void) {
    i32 b = pop();
    i32 a = pop();
    if (b == 0) push(0);
    else push(a%b);
}

void wfetch(void) {
    u32 n = pop();
    if ((n & 0xfffffffc) == n) {
        u32 *addr = (u32 *)n;
        push(*addr);
    } else {
        printf("unaligned read not allowed\n");
        push(0);
    }
}

void wstore(void) {
    u32 n = pop();
    if ((n & 0xfffffffc) == n) {
        u32 *addr = (u32 *)n;
        i32 value = pop();
        *addr = value;
    } else {
        printf("unaligned write not allowed\n");
    }
}

void fetch(void) {
    i32 addr = pop();
    push(dict[addr]);
}

void store(void) {
    i32 addr = pop();
    i32 value = pop();
    dict[addr] = value;
}

void dots(void) {
    i32 i;
    printf("<%d> ", sp);
    for (i=0; i<=sp; i++) {
        _dot(stack[i]);
    }
}

i32 valid(i32 addr) {
#if 0 // does this work on Linux?
    extern char _etext;
    return (addr != NULL && (char *)addr > &_etext);
#else
#if 0
    return ((char *)addr != NULL && (char *)addr >= (char *)&eve);
#else
    u32 n = (u32)addr;
    //printf("min:%x fn:%x max:%x\n", fn_reg_min, addr, fn_reg_max);
    return (n <= fn_reg_max && n >= fn_reg_min);
#endif
#endif
}

void bye(void) {
    _running = 0;
}

void _execute(i32 addr) {
    i32 code = dict[addr];
    if (valid(code)) {
        void (*fn)(void) = (void (*)(void))code;
        fn();
    } else {
        printf("%x is not a registered function\n", code);
    }
}

i32 outer(void) {
    i32 addr;
    while (_running) {
        token(_ilexeme);
        if (tokenp < 0 || tokenp >= TMAX) {
            //printf("invalid tokenp:%d\n", tokenp);
            tokenp = 0;
            break;
        }
        // try to find lexeme in dictionary
        addr = _tick(_ilexeme);
        //printf("_ilexeme<%s>/%d\n", _ilexeme, addr);
        if (addr < 0) {
            // not found, try to interpret as a number
            char *notnumber;
            i32 n;
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
            _execute(addr);

        }
        tokenp++;
    }
    return _running;
}

void eve(void) {
    printf("fn_reg_min:%x\n", fn_reg_min);
    printf("fn_reg_max:%x\n", fn_reg_max);
    printf("name @ %p\n", &name);
    cdump(name, pname);
    printf("_head:%x  _here:%x\n", _head, _here);
    printf("dict @ %p\n", &dict);
    mdump(0, _here+1);
}

void empty(void) {
    printf("empty\n");
}

void create(void) {
    _jlexeme[0] = '\0';
    token(_jlexeme);
    if (_jlexeme[0]) _create(_jlexeme);
    //printf("create :: tokenp:%d (%s)\n", tokenp, _jlexeme);
    _comma((i32)empty);
}

void comma(void) { _comma(pop()); }

void here(void) { push(_here); }

void tick(void) {
    _jlexeme[0] = '\0';
    token(_jlexeme);
    if (_jlexeme[0]) push(_tick(_jlexeme));
}

void rpush(void) { _rpush(pop()); }
void rpop(void) { push(_rpop()); }

// ip points to me, next address is int, push that to stack
void pushlit(void) {
}

/*

[docolon][addr][addr][addr][exit]

precondition: ip is set to my cfa

put ip -> w

*/


void docolon(void) {

}

void execute(void) { _execute(pop()); }

typedef struct {
    char *name;
    void (*fn)(void);
} filler_t;

void hello(void) {
    printf("eve version 0.0\n");
}

void emit(void) {
    char c = pop();
    putchar(c);
}

void key(void) {
    push(getchar());
}

void joe(void) {
    i32 ip = 0;
    printf("joe was called from %x\n", ip);
}

filler_t vocab[] = {
    {"hello",   &hello},
    {"bye",     &bye},
    {"hex",     &hex},
    {"decimal", &decimal},
    {"octal",   &octal},
    {"binary",  &binary},
    {"base12",  &base12},
    {"base24",  &base24},
    {"base32",  &base32},
    {"base36",  &base36},
    {".",       &dot},
    {".s",      &dots},
    {"dwords",  &dwords},
    {"words",   &words},
    {"+",       &add},
    {"-",       &subtract},
    {"*",       &multiply},
    {"/",       &divide},
    {"%",       &modulus},
    {"w@",      &wfetch},
    {"w!",      &wstore},
    {"@",       &fetch},
    {"!",       &store},
    {"empty",   &empty},
    {"create",  &create},
    {",",       &comma},
    {"here",    &here},
    {"'",       &tick},
    {">r",      &rpush},
    {"r>",      &rpop},
    {"execute", &execute},
    {"emit",    &emit},
    {"key",     &key},
    {NULL,      NULL}
};

void native(char *name, void (*fn)(void)) {
    fn_reg(fn);
    _create(name);
    _comma((i32)fn);
}

void start_eve(void) {
    filler_t *fillit = vocab;

    start:
        setbuf(stdout, NULL);
    restart:
        tokenb[TMAX] = '\0';
        sp = -1;
        rs = -1;

    fn_reg(eve);

    while (fillit->name != NULL) {
        //printf("name:%s\n", fillit->name);
        native(fillit->name, fillit->fn);
        fillit++;
    }

    native("joe", joe);
    _comma(100);
    _comma(200);
    _comma(300);

    if (isatty(STDIN_FILENO)) {
        hello();
        while (1) {
            if (input() < 0) break;
            //cdump(tokenb, TMAX);
            //printf("tokenb<%s>\n", tokenb);
            if (outer() == 0) break;
        }
    } else {
        mass();
    }
}

int main(int argc, char *argv[]) {
    start_eve();
    return 0;
}

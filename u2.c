#include <stdio.h>
#include <string.h>

int ip = 0;

char name[1000] = 
   //0123
    "one\0";
void one(void) {
    printf("I am one (ip=%d)!\n", ip);
}
void two(void) {
    printf("I am two (ip=%d)!\n", ip);
}
typedef union {
    int data;
    void (*code)(void);
    int link;
    int name;
} cell_t;
void dolist(void);
#define FEXIT (-1)
cell_t dict[1000] = {
    {.link = -1},      // 0 <------+
    {.name =  0},      // 1        |
    {.code = one},     // 2*       |
};                     // 3  free  |
int link = 0; // ------------------+
int nptr = 4;
int dptr = 3;

void dolist(void) {
    printf("I am dolist (ip=%d)\n", ip);
    while (1) {
        ip++;
        printf("in dolist (ip=%d)\n", ip);
        if (dict[ip].data == FEXIT) break;
        dict[dict[ip].data].code();
    }
    printf("leaving dolist (ip=%d)\n", ip);
}

void nil(void) {
    printf("nil (ip:%d)\n", ip);
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
    printf("LINK : [%d+0] = %d\n",   c, dict[c+0].link);
    printf("NAME : [%d+1] = <%s>\n", c, &name[dict[c+1].name]);
    printf("CODE : [%d+2] = %p\n",   c, dict[c+2].code);
    return 0;
}

int _words(int c, int *e, void *v) {
    printf("<%s> ", &name[dict[c+1].name]);
    return 0;
}

int _find(int c, int *e, void *v) {
    if (strcmp((char *)v, &name[dict[c+1].name]) == 0) {
        *e = c;
        return -1;
    }
    return 0;
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

void run(int link) {
    if (link < 0) {
        printf("invalid link\n");
        return;
    }
    ip = link+2;
    void (*code)(void) = dict[ip].code;
    if (valid(code)) code();
    else printf("invalid code\n");
}

void execute(int ip) {

}

void words(void) {
    walk(link, _words, NULL, NULL);
    printf("\n");
}

void dump(void) {
    walk(link, _dump, NULL, NULL);
}

int find(char *name) {
    int e = -1;
    walk(link, _find, &e, name);
    return e;
}

int main(int argc, char *argv[]) {
    int i;
    int e = 0;
    
    reg(nil);
    reg(one);
    reg(two);
    reg(dolist);

    create("two");
    comma((int)&two);
    create("testit");
    comma((int)&dolist);
    comma(find("one")+2);
    comma(find("two")+2);
    comma(find("one")+2);
    comma(find("two")+2);
    comma(FEXIT);

    create("bob"); comma((int)&nil);
    create("cad"); comma((int)&nil);
    create("doe"); comma((int)&nil);
    create("eve"); comma((int)&nil);
    create("fan"); comma((int)&nil);
    create("gal"); comma((int)&nil);
    create("hal"); comma((int)&nil);
    create("iva"); comma((int)&nil);
    create("joe"); comma((int)&nil);

    words();
    dump();

    printf("find one = %d\n", find("one"));
    printf("find two = %d\n", find("two"));

    run(find("one"));
    run(find("two"));

    for (i=0; i<dptr; i++) {
        printf("%12d : %12d 0x%08x\n", i, dict[i].data, dict[i].data);
    }
    ip = find("testit");
    printf("find testit = %d\n", ip);
    run(ip);

    run(find("iva"));

    return 0;
}

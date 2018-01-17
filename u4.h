#ifndef _U4_DEF_
#define _U4_DEF_

typedef union {
    int data;
    void (*code)(void);
    int flag;
    int link;
    int name;
} cell_t;

void reg(void *code);
void create(char *s);
void comma(int c);
void docolon(void);
void pushlit(void);

int tick(char *name);

#define NEXT (-1)
#define NONE (-1)

void execute(int xt);
void push(int n);

#define SMAX (128)
#define RMAX (128)
#define NMAX (4096)
#define DMAX (16384)
#define TMAX (80)

typedef struct {
    char *name;
    void (*code)(void);
} bulk_t;

void makecode(char *name, void (*code)(void));
void makevar(char *name, int n);
void u4_init(void);
void u4_start(void);

#endif

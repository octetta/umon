/*
        ___ 
 _   __/   |   ( u4 -- ) : hello ." Hello World" ;
| | | | /| |_  http://octetta.com
| |_| |__   _| A C-based Forth-inspired ball of code.
|__/|_|  |_|   (c) 1997-2018 joseph.stewart@gmail.com

*/

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

#define FLAG_IMMEDIATE (1)
#define MASK_MODE      (FLAG_IMMEDIATE)
#define FLAG_USEMODE   (~FLAG_IMMEDIATE)
#define FLAG_COMPILE   (0)

#define FLAG_HIDDEN    (1<<1)
#define MASK_HIDE      (FLAG_HIDDEN)
#define FLAG_VISIBLE   (~FLAG_HIDDEN)

#endif

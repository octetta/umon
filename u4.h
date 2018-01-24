/*
        ___ 
 _   __/   |   ( u4 -- ) : hello ." Hello World" ;
| | | | /| |_  http://octetta.com
| |_| |__   _| u4(tm) A C-based Forth-inspired ball of code.
|__/|_|  |_|tm (c) 1996-2018 joseph.stewart@gmail.com

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

#define F_IMDT (1<<0)
#define F_HIDE (1<<1)
#define F_PRIM (1<<2)

#define M_MODE (F_IMDT)
#define F_CURR (~F_IMDT)
#define F_COMP (0)

#define M_HIDE (F_HIDE)
#define F_SHOW (~F_HIDE)

#endif

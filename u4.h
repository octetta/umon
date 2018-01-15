#ifndef _U4_DEF_
#define _U4_DEF_

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

#define SMAX (1000)
#define RMAX (1000)
#define NMAX (1000)
#define DMAX (1000)

typedef struct {
    char *name;
    void (*code)(void);
} bulk_t;

void makecode(char *name, void (*code)(void));
void makevar(char *name, int n);
void u4_init(void);
void u4_start(void);

#endif

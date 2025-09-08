// a.c
#include <stdio.h>

/* Definitions with external linkage */
int foo = 10;
int shared = 1;

/* Module-local; should NOT be reported as external linkage */
static int file_local = 99;

/* Defined only here; b.c declares and uses it via extern */
int only_here = 5;

/* Extern owned by b.c */
extern int bar;

/* Uses globals from both TUs and a file-local */
int inc_all(void) {
    foo += 1;        /* global use */
    shared += 1;     /* global use */
    file_local += 1; /* not external */
    return foo + shared + file_local + bar; /* bar is extern from b.c */
}

int main(void) {
    printf("%d\n", inc_all());
    return 0;
}

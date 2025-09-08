// b.c
#include <stdio.h>

/* Definition with external linkage */
int bar = 7;

/* External declarations for a.c globals */
extern int foo;
extern int shared;
extern int only_here;

/* Shadowing test: local 'foo' should NOT count as a global use */
static int shadow_test(void) {
    int foo = 1000; /* shadows global 'foo' */
    return foo;     /* local use only */
}

/* Genuine uses of externs */
int touch_externs(void) {
    return foo + shared + bar + only_here;
}

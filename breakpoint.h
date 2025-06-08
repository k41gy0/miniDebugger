#ifndef BREAKPOINT_H
#define BREAKPOINT_H
#include <sys/types.h>

typedef struct Breakpoint {
    void *addr;
    long  orig_data;
    int   enabled;
} Breakpoint;

int bp_enable(pid_t pid, Breakpoint *bp);
int bp_disable(pid_t pid, Breakpoint *bp);

#endif /* BREAKPOINT_H */

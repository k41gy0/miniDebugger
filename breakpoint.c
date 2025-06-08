#include "breakpoint.h"
#include <sys/ptrace.h>
#include <errno.h>
#include <stdio.h>

static long read_word(pid_t pid, void *addr) {
    errno = 0;
    long data = ptrace(PTRACE_PEEKTEXT, pid, addr, NULL);
    if (errno) perror("PTRACE_PEEKTEXT");
    return data;
}

static int write_word(pid_t pid, void *addr, long data) {
    if (ptrace(PTRACE_POKETEXT, pid, addr, (void *)data) < 0) {
        perror("PTRACE_POKETEXT"); return -1; }
    return 0;
}

int bp_enable(pid_t pid, Breakpoint *bp) {
    if (bp->enabled) return 0;
    long data = read_word(pid, bp->addr);
    bp->orig_data = data;
    long patched = (data & ~0xff) | 0xcc; // INT3
    if (write_word(pid, bp->addr, patched) < 0) return -1;
    bp->enabled = 1; return 0;
}

int bp_disable(pid_t pid, Breakpoint *bp) {
    if (!bp->enabled) return 0;
    if (write_word(pid, bp->addr, bp->orig_data) < 0) return -1;
    bp->enabled = 0; return 0;
}

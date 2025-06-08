#include "debug_core.h"
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int dbg_wait(Debugger *dbg, int *status) {
    if (waitpid(dbg->pid, status, 0) < 0) { perror("waitpid"); return -1; }
    return 0;
}

int dbg_launch(Debugger *dbg, char *prog, char **argv, char **envp) {
    pid_t p = fork();
    if (p == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execve(prog, argv, envp);
        perror("execve"); _exit(1);
    }
    dbg->pid = p; dbg->bp_count = 0;
    int st; dbg_wait(dbg, &st);
    return st;
}

int dbg_continue(Debugger *dbg) {
    return ptrace(PTRACE_CONT, dbg->pid, NULL, NULL);
}

int dbg_single(Debugger *dbg) {
    return ptrace(PTRACE_SINGLESTEP, dbg->pid, NULL, NULL);
}

void dbg_show_regs(pid_t pid) {
    struct user_regs_struct r;
    if (ptrace(PTRACE_GETREGS, pid, NULL, &r) < 0) { perror("GETREGS"); return; }
    printf("RIP=%llx RSP=%llx RBP=%llx\n", r.rip, r.rsp, r.rbp);
    printf("RAX=%llx RBX=%llx RCX=%llx RDX=%llx\n", r.rax, r.rbx, r.rcx, r.rdx);
    printf("RSI=%llx RDI=%llx\n", r.rsi, r.rdi);
}

int dbg_set_reg(pid_t pid, const char *name, unsigned long long v) {
    struct user_regs_struct r;
    if (ptrace(PTRACE_GETREGS, pid, NULL, &r) < 0) return -1;
    int updated = 0;
#define SET(n) if(strcmp(name,#n)==0){r.n=v;updated=1;}
    SET(rax); SET(rbx); SET(rcx); SET(rdx);
    SET(rsi); SET(rdi); SET(rbp); SET(rsp);
    SET(r8);  SET(r9);  SET(r10); SET(r11);
    SET(r12); SET(r13); SET(r14); SET(r15);
    SET(rip);
#undef SET
    if (!updated) {
        fprintf(stderr, "Unknown register %s\n", name);
        return -1;
    }
    if (ptrace(PTRACE_SETREGS, pid, NULL, &r) < 0) {
        perror("SETREGS"); return -1;
    }
    return 0;
}

int dbg_read_mem(pid_t pid, unsigned long long a, size_t len) {
    for (size_t i = 0; i < len; i += sizeof(long)) {
        errno = 0;
        long d = ptrace(PTRACE_PEEKDATA, pid, (void *)(a + i), NULL);
        if (errno) { perror("PEEKDATA"); return -1; }
        printf("0x%llx : 0x%016lx\n", a + i, d);
    }
    return 0;
}

int dbg_write_mem(pid_t pid, unsigned long long a, unsigned long long v) {
    if (ptrace(PTRACE_POKEDATA, pid, (void *)a, (void *)v) < 0) {
        perror("POKEDATA"); return -1;
    }
    return 0;
}

int dbg_add_bp(Debugger *dbg, unsigned long long addr) {
    if (dbg->bp_count >= MAX_BREAKPOINTS) {
        fprintf(stderr, "Too many breakpoints\n");
        return -1;
    }
    Breakpoint *bp = &dbg->bps[dbg->bp_count];
    memset(bp, 0, sizeof(*bp));
    bp->addr = (void *)addr;
    if (bp_enable(dbg->pid, bp) == 0) {
        dbg->bp_count++;
        return 0;
    } else {
        return -1;
    }
}


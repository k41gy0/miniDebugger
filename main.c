#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "debug_core.h"
#include "elf_parser.h"

static void usage(const char *self) {
    fprintf(stderr, "Usage: %s <program> [args...]\n", self);
}

int main(int argc, char **argv, char **envp) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    Debugger dbg;
    char **child_argv = &argv[1];

    int status = dbg_launch(&dbg, child_argv[0], child_argv, envp);
    printf("[+] launched pid %d (status 0x%x)\n", dbg.pid, status);

    char line[256];
    while (1) {
        printf("(mydbg) ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = '\0';
	
        if (strcmp(line, "c") == 0 || strcmp(line, "continue") == 0) {
	  dbg_continue(&dbg);
	  dbg_wait(&dbg, &status);
	  if (WIFSTOPPED(status)) {
	    printf("[!] stopped by sig %d\n", WSTOPSIG(status));
	  } else if (WIFEXITED(status)) {
	    printf("[!] program exited (%d)\n", WEXITSTATUS(status));
	    break;
	  }

        } else if (strcmp(line, "s") == 0 || strcmp(line, "step") == 0) {
	  dbg_single(&dbg);
	  dbg_wait(&dbg, &status);
	  dbg_show_regs(dbg.pid);

        } else if (strcmp(line, "regs") == 0) {
	  dbg_show_regs(dbg.pid);

        } else if (strncmp(line, "setreg", 6) == 0) {
	  char reg[32]; unsigned long long val;
	  if (sscanf(line + 6, " %31s %llx", reg, &val) == 2) {
	    dbg_set_reg(dbg.pid, reg, val);
	  } else {
	    puts("Usage: setreg <REG> <VAL>");
	  }

        } else if (strncmp(line, "break ", 6) == 0) {
	  char *arg = line + 6;
	  if(arg[0] == '*'){
	    unsigned long long addr;
	    if (sscanf(arg + 1, " %llx", &addr) == 1) {
	      dbg_add_bp(&dbg, addr);
	      printf("[+] breakpoint set at 0x%llx\n", addr);
	    }
	    
	  } else {
	    Elf_structure elf;
	    if (parse_elf(argv[1], &elf) == 0) {
	      if (lookup_symbol(&elf, arg) == 0) {
                dbg_add_bp(&dbg, elf.sym_addr);
                printf("[+] breakpoint set at symbol '%s' (0x%llx)\n", arg, (unsigned long long)elf.sym_addr);
	      } else {
                printf("[-] Symbol '%s' not found.\n", arg);
	      }
	      free_elf(&elf);
	    } else {
	      printf("[-] Failed to parse ELF.\n");
	    }
	  }
	  
        } else if (line[0] == 'x') {
	  unsigned long long addr; int len = 8;
	  if (sscanf(line + 1, " %llx %d", &addr, &len) >= 1) {
	    dbg_read_mem(dbg.pid, addr, len);
	  } else {
	    puts("Usage: x <addr> [len]");
	  }
	  
        } else if (strncmp(line, "setmem", 6) == 0) {
	  unsigned long long addr, val;
	  if (sscanf(line + 6, " %llx %llx", &addr, &val) == 2) {
	    dbg_write_mem(dbg.pid, addr, val);
	  } else {
	    puts("Usage: setmem <addr> <val>");
	  }
	  
        } else if (strcmp(line, "q") == 0 || strcmp(line, "quit") == 0) {
	  puts("Bye.");
	  break;
	  
        } else {
           puts("Unknown command");
        }
    }

    return 0;
}

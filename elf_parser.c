#include "elf_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static int read_file(const char *fname, uint8_t **buf, size_t *len) {
    int fd = open(fname, O_RDONLY);
    if (fd < 0) { perror("open"); return -1; }
    struct stat st;
    if (fstat(fd, &st) < 0) { perror("fstat"); close(fd); return -1; }
    *buf = malloc(st.st_size);
    if (!*buf) { perror("malloc"); close(fd); return -1; }
    if (read(fd, *buf, st.st_size) != st.st_size) {
        perror("read"); free(*buf); close(fd); return -1; }
    *len = st.st_size; close(fd); return 0;
}

int parse_elf(const char *filename, Elf_structure *out) {
    memset(out, 0, sizeof(*out));
    size_t len;
    if (read_file(filename, &out->mem, &len) < 0) return -1;
    out->ehdr = (Elf64_Ehdr *)out->mem;
    out->phdr = (Elf64_Phdr *)(out->mem + out->ehdr->e_phoff);
    out->shdr = (Elf64_Shdr *)(out->mem + out->ehdr->e_shoff);
    return 0;
}

int lookup_symbol(Elf_structure *elf, const char *name) {
    for (int i = 0; i < elf->ehdr->e_shnum; ++i) {
        if (elf->shdr[i].sh_type != SHT_SYMTAB) continue;
        Elf64_Sym *symtab = (Elf64_Sym *)(elf->mem + elf->shdr[i].sh_offset);
        char *strtab = (char *)(elf->mem + elf->shdr[elf->shdr[i].sh_link].sh_offset);
        size_t n = elf->shdr[i].sh_size / sizeof(Elf64_Sym);
        for (size_t j = 0; j < n; ++j) {
            if (strcmp(name, &strtab[symtab[j].st_name]) == 0) {
                elf->sym_addr = symtab[j].st_value;
                return 0;
            }
        }
    }
    return -1;
}

void free_elf(Elf_structure *elf) {
    free(elf->mem);
    elf->mem = NULL;
}

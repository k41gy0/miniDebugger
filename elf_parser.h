#ifndef ELF_PARSER_H
#define ELF_PARSER_H

#include <stdint.h>
#include <elf.h>

typedef struct Elf_Structure {
    Elf64_Ehdr *ehdr;
    Elf64_Phdr *phdr;
    Elf64_Shdr *shdr;
    uint8_t    *mem;
    Elf64_Addr  sym_addr;
} Elf_structure;

int  parse_elf(const char *filename, Elf_structure *out);
int  lookup_symbol(Elf_structure *elf, const char *name);
void free_elf(Elf_structure *elf);

#endif /* ELF_PARSER_H */

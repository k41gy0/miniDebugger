CC      = gcc
CFLAGS  = -Wall -Wextra -g
OBJS    = main.o elf_parser.o breakpoint.o debug_core.o
TARGET  = mydbg

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

main.o: main.c debug_core.h breakpoint.h elf_parser.h
elf_parser.o: elf_parser.c elf_parser.h
breakpoint.o: breakpoint.c breakpoint.h
debug_core.o: debug_core.c debug_core.h breakpoint.h

clean:
	rm -f $(OBJS) $(TARGET)

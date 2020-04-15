#include <dwarf.h>
#include <elfutils/libdw.h>
#include <errno.h>
#include <fcntl.h>
#include <libelf.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

static Elf* ElfObject = 0;

int main(void)
{
    int Result = 0;

    elf_version(EV_CURRENT);

    int FileDescriptor = open("a.out", O_RDONLY, 0);
    ElfObject = elf_begin(FileDescriptor, ELF_C_READ, 0);

    if (ElfObject == 0) {
        fprintf(stderr, "elf_begin() error: %s\n", elf_errmsg(elf_errno()));
        exit(1);
    }

    elf_end(ElfObject);

    return 0;
}

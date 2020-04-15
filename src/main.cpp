#include <dwarf.h>
#include <elfutils/libdw.h>
#include <errno.h>
#include <fcntl.h>
#include <libelf.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

static Elf* ElfObject = 0;
static Dwarf* DwarfObject = 0;

void DwarfPrintFunctionInfo()
{
    int Result = 0;

    Dwarf_Off Offset = 0;
    Dwarf_Off PreviousOffset = 0;

    size_t HdrOffset = 0;

    while (dwarf_nextcu(DwarfObject, Offset, &Offset, &HdrOffset, 0, 0, 0) == 0) {
        fprintf(stdout, "Compilation Unit - Offset:%llu - HdrOffset:%d\n", Offset, HdrOffset);

        Dwarf_Die* ResultDie;
        Dwarf_Die CUDie;

        ResultDie = dwarf_offdie(DwarfObject, PreviousOffset + HdrOffset, &CUDie);
        if (ResultDie == NULL) {
            fprintf(stderr, "dwarf_offdie() == NULL\n");
            break;
        }

        PreviousOffset = Offset;
    }
}

int main(int argc, char** argv)
{
    int Result = 0;

    elf_version(EV_CURRENT);

    int FileDescriptor = open(argv[1], O_RDONLY, 0);

    ElfObject = elf_begin(FileDescriptor, ELF_C_READ, 0);
    if (ElfObject == 0) {
        fprintf(stderr, "elf_begin() error: %s\n", elf_errmsg(elf_errno()));
        exit(1);
    }

    DwarfObject = dwarf_begin_elf(ElfObject, DWARF_C_READ, 0);
    if (DwarfObject == 0) {
        fprintf(stderr, "dwarf_begin_elf() error\n");
        exit(1);
    }

    // main part
    DwarfPrintFunctionInfo();

    dwarf_end(DwarfObject);
    elf_end(ElfObject);

    return 0;
}

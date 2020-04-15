#include <dwarf.h>
#include <elfutils/libdw.h>
#include <errno.h>
#include <fcntl.h>
#include <libelf.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

void HandleDwarfFunction(Dwarf_Die* Die);

static Elf* ElfObject = 0;
static Dwarf* DwarfObject = 0;

void (*TagFunctions[75])(Dwarf_Die* Die) = { 0 };

void InitFunctionArray()
{
    TagFunctions[0x03] = HandleDwarfFunction;
    TagFunctions[0x1d] = HandleDwarfFunction;
    TagFunctions[0x2e] = HandleDwarfFunction;
}

const char* GetTagDirectoryName(Dwarf_Die* Die)
{
    Dwarf_Attribute Attribute = {};
    dwarf_attr_integrate(Die, DW_AT_comp_dir, &Attribute);
    return dwarf_formstring(&Attribute);
}

const char* GetTagFileName(Dwarf_Die* Die)
{
    Dwarf_Attribute Attribute = {};
    dwarf_attr_integrate(Die, DW_AT_name, &Attribute);
    return dwarf_formstring(&Attribute);
}

const char* GetTagName(Dwarf_Die* Die)
{
    Dwarf_Attribute Attribute = {};
    dwarf_attr_integrate(Die, DW_AT_name, &Attribute);
    return dwarf_formstring(&Attribute);
}

Dwarf_Word GetTagLine(Dwarf_Die* Die)
{
    Dwarf_Word Line = 0;
    Dwarf_Attribute Attribute = {};

    dwarf_attr_integrate(Die, DW_AT_decl_line, &Attribute);
    dwarf_formudata(&Attribute, &Line);

    return Line;
}

void HandleDwarfFunction(Dwarf_Die* Die)
{
    const char* Name = 0;
    Dwarf_Word Line = 0;

    Name = GetTagName(Die);
    Line = GetTagLine(Die);

    fprintf(stdout, "Function: %s:%llu\n", Name, Line);
}

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

    InitFunctionArray();

    // main part
    DwarfPrintFunctionInfo();

    dwarf_end(DwarfObject);
    elf_end(ElfObject);

    return 0;
}

#include <dwarf.h>
#include <elfutils/libdw.h>
#include <errno.h>
#include <fcntl.h>
#include <libelf.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

void HandleDwarfSubprogram(Dwarf_Die* Die);

static Elf* ElfObject = 0;
static Dwarf* DwarfObject = 0;

void (*TagFunctions[75])(Dwarf_Die* Die) = { 0 };

void InitFunctionArray()
{
    // TagFunctions[0x03] = HandleDwarfFunction;
    // TagFunctions[0x1d] = HandleDwarfFunction;
    TagFunctions[0x2e] = HandleDwarfSubprogram;
}

const char* GetTagDirectoryName(Dwarf_Die* Die)
{
    Dwarf_Attribute Attribute = {};
    dwarf_attr_integrate(Die, DW_AT_comp_dir, &Attribute);
    return dwarf_formstring(&Attribute);
}

Dwarf_Word GetTagDeclFile(Dwarf_Die* Die)
{
    int Result = 0;

    Dwarf_Word Data = 0;
    Dwarf_Attribute Attribute = {};

    dwarf_attr_integrate(Die, DW_AT_decl_file, &Attribute);
    dwarf_formudata(&Attribute, &Data);

    return Data;
}

const char* GetTagName(Dwarf_Die* Die)
{
    Dwarf_Attribute Attribute = {};
    dwarf_attr_integrate(Die, DW_AT_name, &Attribute);
    return dwarf_formstring(&Attribute);
}

Dwarf_Word GetTagDeclLine(Dwarf_Die* Die)
{
    Dwarf_Word Line = 0;
    Dwarf_Attribute Attribute = {};

    dwarf_attr(Die, DW_AT_decl_line, &Attribute);
    dwarf_formudata(&Attribute, &Line);

    return Line;
}

void HandleDwarfSubprogram(Dwarf_Die* Die)
{
    const char* Name = 0;
    Dwarf_Word File = 0;
    Dwarf_Word Line = 0;

    Name = GetTagName(Die);
    File = GetTagDeclFile(Die);
    Line = GetTagDeclLine(Die);

    // DW_AT_external              yes(1)
    //                   DW_AT_name                  GetTagDirectoryName
    //                   DW_AT_decl_file             0x00000001 /home/romeu/Documents/Projects/untitled-debugger-project/src/main.cpp
    //                   DW_AT_decl_line             0x00000018
    //                   DW_AT_decl_column           0x0000000d
    //                   DW_AT_linkage_name          _Z19GetTagDirectoryNameP9Dwarf_Die
    //                   DW_AT_type                  <0x00000901>
    //                   DW_AT_low_pc                0x0000127e
    //                   DW_AT_high_pc               <offset-from-lowpc>106
    //                   DW_AT_frame_base            len 0x0001: 9c: DW_OP_call_frame_cfa
    //                   DW_AT_GNU_all_tail_call_sites yes(1)
    //                   DW_AT_sibling               <0x00001640>

    fprintf(stdout, "DW_TAG_subprogram\n"
                    "\tDW_AT_name: %s\n"
                    "\tDW_AT_decl_line: %d\n"
                    "\tDW_AT_decl_file: %llu\n",
            Name, Line, File);
}

void DwarfPrintFunctionInfo()
{
    int Result = 0;

    Dwarf_Off Offset = 0;
    Dwarf_Off PreviousOffset = 0;

    size_t HdrOffset = 0;

    while (dwarf_nextcu(DwarfObject, Offset, &Offset, &HdrOffset, 0, 0, 0) == 0) {
        fprintf(stdout, "Compilation Unit: Offset:%llu - HdrOffset:%d\n", Offset, HdrOffset);

        Dwarf_Die ResultDie;
        Dwarf_Die CUDie;

        dwarf_offdie(DwarfObject, PreviousOffset + HdrOffset, &CUDie);

        fprintf(stdout, "File: %s/%s\n", GetTagDirectoryName(&CUDie), GetTagName(&CUDie));

        if (dwarf_child(&CUDie, &ResultDie) != 0) {
            continue;
        }

        do {
            int Tag;
            Tag = dwarf_tag(&ResultDie);

            switch (Tag) {
                case DW_TAG_subprogram:
                    TagFunctions[Tag](&ResultDie);
                    break;
                // case DW_TAG_entry_point:
                // case DW_TAG_inlined_subroutine:
                default:
                    break;
            }
        } while (dwarf_siblingof(&ResultDie, &ResultDie) == 0);

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

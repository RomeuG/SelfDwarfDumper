#include <errno.h>
#include <fcntl.h>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>
#include <libelf.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

struct SourceFiles {
    char** Files;
    Dwarf_Signed Count;
};

static Dwarf_Debug GlobalDwarfDebug;
static Dwarf_Error GlobalDwarfError;
static struct SourceFiles GlobalSourceFiles;

void HandleDwarfSubprogram(Dwarf_Die Die);

void (*TagFunctions[75])(Dwarf_Die Die) = { 0 };

void GetAllSourceFiles(Dwarf_Die Die)
{
    dwarf_srcfiles(Die, &GlobalSourceFiles.Files, &GlobalSourceFiles.Count, 0);

    fprintf(stdout, "Detected files:\n");
    for (int Index = 0; Index < GlobalSourceFiles.Count; Index++) {
        fprintf(stdout, "\t%s\n", GlobalSourceFiles.Files[Index]);
    }
}

void InitFunctionArray()
{
    // TagFunctions[0x03] = HandleDwarfFunction;
    // TagFunctions[0x1d] = HandleDwarfFunction;
    TagFunctions[0x2e] = HandleDwarfSubprogram;
}

char* GetTagDirectoryName(Dwarf_Die Die)
{
    char* DirectoryName = 0;
    Dwarf_Attribute Attribute = 0;

    dwarf_attr(Die, DW_AT_comp_dir, &Attribute, 0);
    dwarf_formstring(Attribute, &DirectoryName, 0);

    return DirectoryName;
}

Dwarf_Unsigned GetTagDeclFile(Dwarf_Die Die)
{
    int Result = 0;

    Dwarf_Unsigned Data = 0;
    Dwarf_Attribute Attribute = 0;

    dwarf_attr(Die, DW_AT_decl_file, &Attribute, 0);
    dwarf_formudata(Attribute, &Data, 0);

    return Data;
}

const char* GetTagName(Dwarf_Die Die)
{
    char* Name = 0;
    Dwarf_Attribute Attribute = 0;

    dwarf_attr(Die, DW_AT_name, &Attribute, 0);
    dwarf_formstring(Attribute, &Name, 0);

    return Name;
}

Dwarf_Unsigned GetTagDeclLine(Dwarf_Die Die)
{
    Dwarf_Unsigned Line = 0;
    Dwarf_Attribute Attribute = 0;

    dwarf_attr(Die, DW_AT_decl_line, &Attribute, 0);
    dwarf_formudata(Attribute, &Line, 0);

    return Line;
}

Dwarf_Unsigned GetTagDeclColumn(Dwarf_Die Die)
{
    Dwarf_Unsigned Column = 0;
    Dwarf_Attribute Attribute = 0;

    dwarf_attr(Die, DW_AT_decl_column, &Attribute, 0);
    dwarf_formudata(Attribute, &Column, 0);

    return Column;
}

const char* GetTagLinkageName(Dwarf_Die Die)
{
    char* LinkageName = 0;
    Dwarf_Attribute Attribute = 0;

    int Result = dwarf_attr(Die, DW_AT_linkage_name, &Attribute, 0);
    if (Result != 0) {
        return 0;
    }

    dwarf_formstring(Attribute, &LinkageName, 0);

    return LinkageName;
}

Dwarf_Unsigned GetTagType(Dwarf_Die Die)
{
    Dwarf_Off Type = 0;
    Dwarf_Attribute Attribute = 0;

    int Result = dwarf_attr(Die, DW_AT_type, &Attribute, 0);
    if (Result != DW_DLV_OK) {
        return 0;
    }

    dwarf_formref(Attribute, &Type, 0);

    return Type;
}

Dwarf_Addr GetTagLowPC(Dwarf_Die Die)
{
    Dwarf_Addr LowPC = 0;
    Dwarf_Attribute Attribute = 0;

    int Result = dwarf_attr(Die, DW_AT_low_pc, &Attribute, 0);
    if (Result != DW_DLV_OK) {
        return 0;
    }

    dwarf_formaddr(Attribute, &LowPC, 0);

    return LowPC;
}

Dwarf_Unsigned GetTagHighPC(Dwarf_Die Die)
{
    Dwarf_Unsigned HighPC = 0;
    Dwarf_Attribute Attribute = 0;

    int Result = dwarf_attr(Die, DW_AT_high_pc, &Attribute, 0);
    if (Result != DW_DLV_OK) {
        return 0;
    }

    dwarf_formudata(Attribute, &HighPC, 0);

    return HighPC;
}

Dwarf_Bool GetTagExternal(Dwarf_Die Die)
{
    Dwarf_Bool External = 0;
    Dwarf_Attribute Attribute = 0;

    dwarf_attr(Die, DW_AT_external, &Attribute, 0);
    dwarf_formflag(Attribute, &External, 0);

    return External;
}

Dwarf_Unsigned GetTagFrameBase(Dwarf_Die Die)
{
    Dwarf_Unsigned Length = 0;
    Dwarf_Ptr Pointer = 0;
    Dwarf_Attribute Attribute = 0;

    int Result = dwarf_attr(Die, DW_AT_frame_base, &Attribute, 0);
    if (Result != DW_DLV_OK) {
        return 0;
    }

    Result = dwarf_formexprloc(Attribute, &Length, &Pointer, &GlobalDwarfError);
    if (Result != DW_DLV_OK) {
        return 0;
    }

    return Length;
}

void HandleDwarfSubprogram(Dwarf_Die Die)
{
    Dwarf_Bool External = GetTagExternal(Die);
    const char* Name = GetTagName(Die);
    Dwarf_Unsigned Line = GetTagDeclLine(Die);
    Dwarf_Unsigned File = GetTagDeclFile(Die);
    Dwarf_Unsigned Column = GetTagDeclColumn(Die);
    const char* LinkageName = GetTagLinkageName(Die);
    Dwarf_Unsigned Type = GetTagType(Die);
    Dwarf_Addr LowPC = GetTagLowPC(Die);
    Dwarf_Unsigned HighPC = GetTagHighPC(Die);
    Dwarf_Unsigned FrameBase = GetTagFrameBase(Die);

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
                    "\tDW_AT_external: %d\n"
                    "\tDW_AT_name: %s\n"
                    "\tDW_AT_decl_file: %llu\n"
                    "\tDW_AT_decl_line: %d\n"
                    "\tDW_AT_decl_column: %llu\n"
                    "\tDW_AT_linkage_name: %s\n"
                    "\tDW_AT_type: %llu\n"
                    "\tDW_AT_low_pc: 0x%0.8x\n"
                    "\tDW_AT_high_pc: %llu\n"
                    "\tDW_AT_frame_base: 0x%0.8x\n",
            External,
            Name, File, Line, Column, LinkageName, Type, LowPC, HighPC, FrameBase);
}

void DwarfPrintFunctionInfo()
{
    int Result = 0;
    int CUResult = 0;

    CUResult = dwarf_next_cu_header_c(GlobalDwarfDebug, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    while (CUResult == DW_DLV_OK) {
        Dwarf_Die CUDie;
        Dwarf_Die ChildDie;

        Result = dwarf_siblingof_b(GlobalDwarfDebug, 0, 1, &CUDie, &GlobalDwarfError);
        if (Result != DW_DLV_OK) {
            fprintf(stderr, "dwarf_siblingof() error: %s\n", dwarf_errmsg(GlobalDwarfError));
            exit(1);
        }

        GetAllSourceFiles(CUDie);
        fprintf(stdout, "File: %s/%s\n", GetTagDirectoryName(CUDie), GetTagName(CUDie));

        if (dwarf_child(CUDie, &ChildDie, &GlobalDwarfError) != DW_DLV_OK) {
            fprintf(stdout, "dwarf_child() NOK: %s\n", dwarf_errmsg(GlobalDwarfError));
            continue;
        }

        do {
            Dwarf_Half Tag;
            Result = dwarf_tag(ChildDie, &Tag, &GlobalDwarfError);
            if (Result != DW_DLV_OK) {
                fprintf(stdout, "dwarf_tag() error: %s\n", dwarf_errmsg(GlobalDwarfError));
                exit(1);
            }

            switch (Tag) {
                case DW_TAG_subprogram:
                    TagFunctions[Tag](ChildDie);
                    break;
                // case DW_TAG_entry_point:
                // case DW_TAG_inlined_subroutine:
                default:
                    break;
            }
        } while (dwarf_siblingof(GlobalDwarfDebug, ChildDie, &ChildDie, 0) == 0);

        CUResult = dwarf_next_cu_header_c(GlobalDwarfDebug, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
}

int main(int argc, char** argv)
{
    int FileDescriptor;
    Dwarf_Handler DwarfHandler;
    Dwarf_Ptr DwarfErrArg;
    Dwarf_Error DwarfError;

    InitFunctionArray();

    FileDescriptor = open("a.out", O_RDONLY);

    int DwarfInitResult = dwarf_init(FileDescriptor, DW_DLC_READ, DwarfHandler, DwarfErrArg, &GlobalDwarfDebug, &DwarfError);
    if (DwarfInitResult != DW_DLV_OK) {
        fprintf(stderr, "dwarf_init() error.\n");
        exit(-1);
    }

    DwarfPrintFunctionInfo();

    int DwarfFinishResult = dwarf_finish(GlobalDwarfDebug, &DwarfError);
    if (DwarfFinishResult != DW_DLV_OK) {
        fprintf(stderr, "dwarf_init() error.\n");
        exit(-1);
    }

    return 0;
}

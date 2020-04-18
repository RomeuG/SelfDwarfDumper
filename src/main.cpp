#include <errno.h>
#include <fcntl.h>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>
#include <libelf.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define TESTMACRO 0

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

char* GetTagString(Dwarf_Die Die, Dwarf_Half AttributeCode)
{
    int Result = 0;

    char* Value = 0;
    Dwarf_Attribute Attribute = 0;

    Result = dwarf_attr(Die, AttributeCode, &Attribute, 0);
    if (Result != DW_DLV_OK) {
        return 0;
    }

    Result = dwarf_formstring(Attribute, &Value, 0);
    if (Result != DW_DLV_OK) {
        return 0;
    }

    return Value;
}

Dwarf_Unsigned GetTagUnsignedData(Dwarf_Die Die, Dwarf_Half AttributeCode)
{
    int Result = 0;

    Dwarf_Unsigned Value = 0;
    Dwarf_Attribute Attribute = 0;

    Result = dwarf_attr(Die, AttributeCode, &Attribute, 0);
    if (Result != DW_DLV_OK) {
        return 0;
    }

    Result = dwarf_formudata(Attribute, &Value, 0);
    if (Result != DW_DLV_OK) {
        return 0;
    }

    return Value;
}

Dwarf_Off GetTagRef(Dwarf_Die Die, Dwarf_Half AttributeCode)
{
    int Result = 0;

    Dwarf_Off Value = 0;
    Dwarf_Attribute Attribute = 0;

    Result = dwarf_attr(Die, AttributeCode, &Attribute, 0);
    if (Result != DW_DLV_OK) {
        return 0;
    }

    Result = dwarf_formref(Attribute, &Value, 0);
    if (Result != DW_DLV_OK) {
        return 0;
    }

    return Value;
}

Dwarf_Bool GetTagFlag(Dwarf_Die Die, Dwarf_Half AttributeCode)
{
    int Result = 0;

    Dwarf_Bool Value = 0;
    Dwarf_Attribute Attribute = 0;

    Result = dwarf_attr(Die, AttributeCode, &Attribute, 0);
    if (Result != DW_DLV_OK) {
        return 0;
    }

    Result = dwarf_formflag(Attribute, &Value, 0);
    if (Result != DW_DLV_OK) {
        return 0;
    }

    return Value;
}

Dwarf_Addr GetTagAddress(Dwarf_Die Die, Dwarf_Half AttributeCode)
{
    int Result = 0;

    Dwarf_Addr Value = 0;
    Dwarf_Attribute Attribute = 0;

    Result = dwarf_attr(Die, AttributeCode, &Attribute, 0);
    if (Result != DW_DLV_OK) {
        return 0;
    }

    Result = dwarf_formaddr(Attribute, &Value, 0);
    if (Result != DW_DLV_OK) {
        return 0;
    }

    return Value;
}

Dwarf_Unsigned GetTagExprLoc(Dwarf_Die Die, Dwarf_Half AttributeCode)
{
    Dwarf_Unsigned Length = 0;
    Dwarf_Ptr Pointer = 0;
    Dwarf_Attribute Attribute = 0;

    int Result = dwarf_attr(Die, AttributeCode, &Attribute, 0);
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
    Dwarf_Bool External = GetTagFlag(Die, DW_AT_external);
    const char* Name = GetTagString(Die, DW_AT_name);
    Dwarf_Unsigned Line = GetTagUnsignedData(Die, DW_AT_decl_line);
    Dwarf_Unsigned File = GetTagUnsignedData(Die, DW_AT_decl_file);
    Dwarf_Unsigned Column = GetTagUnsignedData(Die, DW_AT_decl_column);
    const char* LinkageName = GetTagString(Die, DW_AT_linkage_name);
    Dwarf_Off Type = GetTagRef(Die, DW_AT_type);
    Dwarf_Addr LowPC = GetTagAddress(Die, DW_AT_low_pc);
    Dwarf_Unsigned HighPC = GetTagUnsignedData(Die, DW_AT_high_pc);
    Dwarf_Unsigned FrameBase = GetTagExprLoc(Die, DW_AT_frame_base);
    Dwarf_Off Sibling = GetTagRef(Die, DW_AT_sibling);

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
                    "\tDW_AT_decl_file: %s\n"
                    "\tDW_AT_decl_line: %d\n"
                    "\tDW_AT_decl_column: %llu\n"
                    "\tDW_AT_linkage_name: %s\n"
                    "\tDW_AT_type: %llu\n"
                    "\tDW_AT_low_pc: 0x%0.8x\n"
                    "\tDW_AT_high_pc: %llu\n"
                    "\tDW_AT_frame_base: 0x%0.8x\n"
                    "\tDW_AT_sibling: 0x%0.8x\n",
            External, Name, GlobalSourceFiles.Files[File - 1], Line, Column, LinkageName, Type, LowPC, HighPC, FrameBase, Sibling);
}

void HandleDwarfCompilationUnit(Dwarf_Die CUDie)
{
    char* Name = GetTagString(CUDie, DW_AT_name);
    char* Directory = GetTagString(CUDie, DW_AT_comp_dir);
    Dwarf_Off MacroOffset = GetTagRef(CUDie, DW_AT_macros);

    fprintf(stdout,
            "File: %s/%s\n"
            "Macro Offset: 0x%0.8x\n",
            Directory, Name, MacroOffset);
}

// void HandleMacroDefUndef(Dwarf_Macro_Context MacroContext)
// {
// 	dwarf_get_macro_defundef(MacroContext, );
// }

void HandleDwarfCompilationUnitMacros(Dwarf_Die CUDie)
{
    Dwarf_Unsigned Version = 0;
    Dwarf_Macro_Context MacroContext = {};
    Dwarf_Unsigned MacroUnitOffset = 0;
    Dwarf_Unsigned MacroOpsCount = 0;
    Dwarf_Unsigned MacroOpsDataLength = 0;

    int Result = dwarf_get_macro_context(CUDie, &Version, &MacroContext, &MacroUnitOffset, &MacroOpsCount, &MacroOpsDataLength, 0);
    if (Result != DW_DLV_OK) {
        fprintf(stderr, "dwarf_get_macro_context() error\n");
        exit(1);
    }

    for (int Index = 0; Index < MacroOpsCount; Index++) {
        Dwarf_Unsigned SectionOffset = 0;
        Dwarf_Half MacroOperator = 0;
        Dwarf_Half FormsCount = 0;
        const Dwarf_Small* FormCodeArray = 0;

        Result = dwarf_get_macro_op(MacroContext, Index, &SectionOffset, &MacroOperator, &FormsCount, &FormCodeArray, 0);
        if (Result != DW_DLV_OK) {
            continue;
        }

        switch (MacroOperator) {
            case 0:
                break;
            case DW_MACRO_define:
            case DW_MACRO_undef:
            case DW_MACRO_define_strp:
            case DW_MACRO_undef_strp:
            case DW_MACRO_define_strx:
            case DW_MACRO_undef_strx:
            case DW_MACRO_define_sup:
            case DW_MACRO_undef_sup: {
                Dwarf_Unsigned MLine = 0;
                Dwarf_Unsigned MIndex = 0;
                Dwarf_Unsigned MOffset = 0;
                Dwarf_Half MFormsCount = 0;
                const char* MacroString = 0;
                // HandleMacroDefUndef(MacroContext);
                Result = dwarf_get_macro_defundef(MacroContext, Index, &MLine, &MIndex, &MOffset, &MFormsCount, &MacroString, 0);
                if (Result != DW_DLV_OK) {
                    exit(1);
                }
                fprintf(stdout, "Macro String: %s\n", MacroString);
            } break;
        }
    }

    dwarf_dealloc_macro_context(MacroContext);
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
        HandleDwarfCompilationUnit(CUDie);
        HandleDwarfCompilationUnitMacros(CUDie);

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

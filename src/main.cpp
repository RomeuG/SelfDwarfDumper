#include <errno.h>
#include <fcntl.h>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>
#include <libelf.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define TESTMACRO 0
#define STR(a) #a

// custom array stuff
struct Array {
    Dwarf_Unsigned* array;
    size_t size;
    size_t used;
};

void ArrayInit(Array* a, size_t InitialSize)
{
    a->array = (Dwarf_Unsigned*)calloc(InitialSize, sizeof(Dwarf_Unsigned));
    a->size = InitialSize;
    a->used = 0;
}

void ArrayInsert(Array* a, Dwarf_Unsigned NewValue)
{
    if (a->used == a->size) {
        a->size *= 2;
        a->array = (Dwarf_Unsigned*)realloc(a->array, a->size * sizeof(Dwarf_Unsigned));
    }

    a->array[a->used++] = NewValue;
}

void ArrayFree(Array* a)
{
    free(a->array);
    a->array = 0;
    a->used = 0;
    a->size = 0;
}

struct SourceFiles {
    char** Files;
    Dwarf_Signed Count;
};

static Dwarf_Debug GlobalDwarfDebug;
static Dwarf_Error GlobalDwarfError;
static struct SourceFiles GlobalSourceFiles;

Array GlobalArray;

void HandleDwarfFormalParameter(Dwarf_Die Die);
void HandleDwarfSubprogram(Dwarf_Die Die);
void HandleDwarfVariable(Dwarf_Die Die);

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
    TagFunctions[DW_TAG_formal_parameter] = HandleDwarfFormalParameter;
    TagFunctions[DW_TAG_subprogram] = HandleDwarfSubprogram;
    TagFunctions[DW_TAG_variable] = HandleDwarfVariable;
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

Dwarf_Unsigned GetTagExprLoc(Dwarf_Die Die, Dwarf_Half AttributeCode, Dwarf_Ptr Pointer)
{
    Dwarf_Unsigned Length = 0;
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

void DwarfGetChildInfo(Dwarf_Die ChildDie)
{
    int Result = 0;

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
            case DW_TAG_variable:
                TagFunctions[Tag](ChildDie);
                break;
            case DW_TAG_formal_parameter:
                TagFunctions[Tag](ChildDie);
                break;
            // case DW_TAG_entry_point:
            // case DW_TAG_inlined_subroutine:
            default:
                break;
        }

        // TODO: check for children here and print them out in a
        // separate function to possibly use recursion
    } while (dwarf_siblingof(GlobalDwarfDebug, ChildDie, &ChildDie, 0) == 0);
}

void HandleDwarfFormalParameter(Dwarf_Die Die)
{
    char* Name = GetTagString(Die, DW_AT_name);
    Dwarf_Unsigned File = GetTagUnsignedData(Die, DW_AT_decl_file);
    Dwarf_Unsigned Line = GetTagUnsignedData(Die, DW_AT_decl_line);
    Dwarf_Unsigned Column = GetTagUnsignedData(Die, DW_AT_decl_column);
    Dwarf_Off Type = GetTagRef(Die, DW_AT_type);
    // TODO: make use of LocationPointer
    Dwarf_Ptr LocationPointer = 0;
    Dwarf_Unsigned Location = GetTagExprLoc(Die, DW_AT_location, LocationPointer);

    const char* FileName = File == 0 ? "(null)" : GlobalSourceFiles.Files[File - 1];

    // TODO: location actually shows more information than this
    fprintf(stdout, "DW_TAG_formal_parameter\n"
                    "\tDW_AT_name: %s\n"
                    "\tDW_AT_decl_file: %s\n"
                    "\tDW_AT_decl_line: %d\n"
                    "\tDW_AT_decl_column: %llu\n"
                    "\tDW_AT_type: %llu\n"
                    "\tDW_AT_location: %llu\n",
            Name, FileName, Line, Column, Type, Location);
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
    // TODO: make use of framebasepointer
    Dwarf_Ptr FrameBasePointer = 0;
    Dwarf_Unsigned FrameBase = GetTagExprLoc(Die, DW_AT_frame_base, FrameBasePointer);
    Dwarf_Off Sibling = GetTagRef(Die, DW_AT_sibling);

    Dwarf_Bool HasChildren = 0;
    Dwarf_Die ChildDie = 0;
    if (dwarf_child(Die, &ChildDie, &GlobalDwarfError) == DW_DLV_OK) {
        HasChildren = 1;
    }

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

    fprintf(stdout, "DW_TAG_subprogram - Children: %d\n"
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
            HasChildren, External, Name, GlobalSourceFiles.Files[File - 1], Line, Column, LinkageName, Type, LowPC, HighPC, FrameBase, Sibling);

    if (HasChildren) {
        DwarfGetChildInfo(ChildDie);
    }
}

void HandleDwarfVariable(Dwarf_Die Die)
{
    // DW_TAG_variable
    //                   DW_AT_name                  TagFunctions
    //                   DW_AT_decl_file             0x00000001 /home/romeu/Documents/Projects/untitled-debugger-project/src/main.cpp
    //                   DW_AT_decl_line             0x00000017
    //                   DW_AT_decl_column           0x00000008
    //                   DW_AT_type                  <0x00000c86>
    //                   DW_AT_external              yes(1)
    //                   DW_AT_location              len 0x0009:
    //                   034051000000000000: DW_OP_addr 0x00005140

    const char* Name = GetTagString(Die, DW_AT_name);
    Dwarf_Unsigned Line = GetTagUnsignedData(Die, DW_AT_decl_line);
    Dwarf_Unsigned File = GetTagUnsignedData(Die, DW_AT_decl_file);
    Dwarf_Unsigned Column = GetTagUnsignedData(Die, DW_AT_decl_column);
    Dwarf_Off Type = GetTagRef(Die, DW_AT_type);
    Dwarf_Bool External = GetTagFlag(Die, DW_AT_external);

    fprintf(stdout, "DW_TAG_variable\n"
                    "\tDW_AT_name: %s\n"
                    "\tDW_AT_decl_file: %s\n"
                    "\tDW_AT_decl_line: %d\n"
                    "\tDW_AT_decl_column: %llu\n"
                    "\tDW_AT_external: %d\n"
                    "\tDW_AT_type: %llu\n",
            Name, GlobalSourceFiles.Files[File - 1], Line, Column, Type, External);
}

void HandleDwarfCompilationUnit(Dwarf_Die CUDie)
{
    char* Producer = GetTagString(CUDie, DW_AT_producer);
    // TODO: print out actual language name (4 -> CPP)
    Dwarf_Unsigned Language = GetTagUnsignedData(CUDie, DW_AT_language);
    char* Name = GetTagString(CUDie, DW_AT_name);
    char* Directory = GetTagString(CUDie, DW_AT_comp_dir);
    Dwarf_Off MacroOffset = GetTagRef(CUDie, DW_AT_macros);

    fprintf(stdout, "Producer: %s\n"
                    "Language: %d\n"
                    "File: %s/%s\n"
                    "Macro Offset and Information: 0x%0.8x\n",
            Producer, Language, Directory, Name, MacroOffset);
}

void HandleMacroDefUndef(Dwarf_Macro_Context MacroContext, Dwarf_Half MacroOperator, int Index, const char* TagName)
{
    Dwarf_Unsigned MLine = 0;
    Dwarf_Unsigned MIndex = 0;
    Dwarf_Unsigned MOffset = 0;
    Dwarf_Half MFormsCount = 0;
    const char* MacroString = 0;

    int Result = dwarf_get_macro_defundef(MacroContext, Index, &MLine, &MIndex, &MOffset, &MFormsCount, &MacroString, 0);
    if (Result != DW_DLV_OK) {
        exit(1);
    }

    fprintf(stdout, "\t[%d] 0x%0.2x %s line:%d %s\n", Index, MacroOperator, TagName, MLine, MacroString);
}

void HandleMacroStartFile(Dwarf_Macro_Context MacroContext, Dwarf_Half MacroOperator, int Index, const char* TagName)
{
    Dwarf_Unsigned MLine = 0;
    Dwarf_Unsigned MIndex = 0;
    const char* MacroString = 0;

    int Result = dwarf_get_macro_startend_file(MacroContext, Index, &MLine, &MIndex, &MacroString, 0);
    if (Result != DW_DLV_OK) {
        exit(1);
    }

    fprintf(stdout, "\t[%d] 0x%0.2x %s line:%d file number: %d %s\n", Index, MacroOperator, TagName, MLine, MIndex, MacroString);
}

void HandleMacroEndFile(Dwarf_Macro_Context MacroContext, Dwarf_Half MacroOperator, int Index, const char* TagName)
{
    Dwarf_Unsigned MLine = 0;
    Dwarf_Unsigned MIndex = 0;
    const char* MacroString = 0;

    int Result = dwarf_get_macro_startend_file(MacroContext, Index, &MLine, &MIndex, &MacroString, 0);
    if (Result != DW_DLV_OK) {
        exit(1);
    }

    fprintf(stdout, "\t[%d] 0x%0.2x %s\n", Index, MacroOperator, TagName);
}

void HandleMacroImport(Dwarf_Macro_Context MacroContext, Dwarf_Half MacroOperator, int Index, const char* TagName)
{
    Dwarf_Unsigned MLine = 0;
    Dwarf_Unsigned MOffset = 0;

    int Result = dwarf_get_macro_import(MacroContext, Index, &MOffset, 0);
    if (Result != DW_DLV_OK) {
        exit(1);
    }

    ArrayInsert(&GlobalArray, MOffset);

    fprintf(stdout, "\t[%d] 0x%0.2x %s offset 0x%0.8x\n", Index, MacroOperator, TagName, MOffset);
}

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

    fprintf(stdout, "Macro data from CU-DIE at .debug_info offset 0x%0.8x:\n"
                    "Macro Version: %d\n"
                    "MacroInformationEntries count: %d, bytes length: %d\n",
            MacroUnitOffset, Version, MacroOpsCount, MacroOpsDataLength);

    for (int Index = 0; Index < MacroOpsCount; Index++) {
        Dwarf_Unsigned SectionOffset = 0;
        Dwarf_Half MacroOperator = 0;
        Dwarf_Half FormsCount = 0;
        const Dwarf_Small* FormCodeArray = 0;
        const char* MacroName = 0;

        Result = dwarf_get_macro_op(MacroContext, Index, &SectionOffset, &MacroOperator, &FormsCount, &FormCodeArray, 0);
        if (Result != DW_DLV_OK) {
            continue;
        }

        dwarf_get_MACRO_name(MacroOperator, &MacroName);

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
            case DW_MACRO_undef_sup:
                HandleMacroDefUndef(MacroContext, MacroOperator, Index, MacroName);
                break;
            case DW_MACRO_start_file:
                HandleMacroStartFile(MacroContext, MacroOperator, Index, MacroName);
                break;
            case DW_MACRO_end_file:
                HandleMacroEndFile(MacroContext, MacroOperator, Index, MacroName);
                break;
            case DW_MACRO_import:
                HandleMacroImport(MacroContext, MacroOperator, Index, MacroName);
                break;
        }
    }

    dwarf_dealloc_macro_context(MacroContext);
}

void HandleDwarfCompilationUnitMacrosByOffset(Dwarf_Die CUDie, Dwarf_Unsigned Offset)
{
    Dwarf_Unsigned Version = 0;
    Dwarf_Macro_Context MacroContext = {};
    Dwarf_Unsigned MacroOpsCount = 0;
    Dwarf_Unsigned MacroOpsDataLength = 0;

    int Result = dwarf_get_macro_context_by_offset(CUDie, Offset, &Version, &MacroContext, &MacroOpsCount, &MacroOpsDataLength, 0);
    if (Result != DW_DLV_OK) {
        fprintf(stderr, "dwarf_get_macro_context() error\n");
        exit(1);
    }

    fprintf(stdout, "Macro data from CU-DIE at .debug_info offset 0x%0.8x:\n"
                    "Macro Version: %d\n"
                    "MacroInformationEntries count: %d, bytes length: %d\n",
            Offset, Version, MacroOpsCount, MacroOpsDataLength);

    for (int Index = 0; Index < MacroOpsCount; Index++) {
        Dwarf_Unsigned SectionOffset = 0;
        Dwarf_Half MacroOperator = 0;
        Dwarf_Half FormsCount = 0;
        const Dwarf_Small* FormCodeArray = 0;
        const char* MacroName = 0;

        Result = dwarf_get_macro_op(MacroContext, Index, &SectionOffset, &MacroOperator, &FormsCount, &FormCodeArray, 0);
        if (Result != DW_DLV_OK) {
            continue;
        }

        dwarf_get_MACRO_name(MacroOperator, &MacroName);

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
            case DW_MACRO_undef_sup:
                HandleMacroDefUndef(MacroContext, MacroOperator, Index, MacroName);
                break;
            case DW_MACRO_start_file:
                HandleMacroStartFile(MacroContext, MacroOperator, Index, MacroName);
                break;
            case DW_MACRO_end_file:
                HandleMacroEndFile(MacroContext, MacroOperator, Index, MacroName);
                break;
            case DW_MACRO_import:
                HandleMacroImport(MacroContext, MacroOperator, Index, MacroName);
                break;
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

        for (int Index = 0; Index < GlobalArray.used; Index++) {
            HandleDwarfCompilationUnitMacrosByOffset(CUDie, GlobalArray.array[Index]);
        }

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
                case DW_TAG_variable:
                    TagFunctions[Tag](ChildDie);
                    break;
                // case DW_TAG_entry_point:
                // case DW_TAG_inlined_subroutine:
                default:
                    break;
            }

            // TODO: check for children here and print them out in a
            // separate function to possibly use recursion
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

    ArrayInit(&GlobalArray, 1);

    InitFunctionArray();

    FileDescriptor = open("a.out", O_RDONLY);

    int DwarfInitResult = dwarf_init(FileDescriptor, DW_DLC_READ, DwarfHandler, DwarfErrArg, &GlobalDwarfDebug, &DwarfError);
    if (DwarfInitResult != DW_DLV_OK) {
        fprintf(stderr, "dwarf_init() error.\n");
        exit(-1);
    }

    DwarfPrintFunctionInfo();

    ArrayFree(&GlobalArray);

    int DwarfFinishResult = dwarf_finish(GlobalDwarfDebug, &DwarfError);
    if (DwarfFinishResult != DW_DLV_OK) {
        fprintf(stderr, "dwarf_init() error.\n");
        exit(-1);
    }

    return 0;
}

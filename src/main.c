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

void ArrayInit(struct Array* a, size_t InitialSize)
{
    a->array = (Dwarf_Unsigned*)calloc(InitialSize, sizeof(Dwarf_Unsigned));
    a->size = InitialSize;
    a->used = 0;
}

void ArrayInsert(struct Array* a, Dwarf_Unsigned NewValue)
{
    if (a->used == a->size) {
        a->size *= 2;
        a->array = (Dwarf_Unsigned*)realloc(a->array, a->size * sizeof(Dwarf_Unsigned));
    }

    a->array[a->used++] = NewValue;
}

void ArrayFree(struct Array* a)
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

struct Array GlobalArray;

void HandleDwarfEnumerationType(Dwarf_Die Die);
void HandleDwarfEnumerator(Dwarf_Die Die);
void HandleDwarfBaseType(Dwarf_Die Die);
void HandleDwarfTypedef(Dwarf_Die Die);
void HandleDwarfArrayType(Dwarf_Die Die);
void HandleDwarfSubrangeType(Dwarf_Die Die);
void HandleDwarfPointerType(Dwarf_Die Die);
void HandleDwarfSubroutineType(Dwarf_Die Die);
void HandleDwarfStructureType(Dwarf_Die Die);
void HandleDwarfMember(Dwarf_Die Die);
void HandleDwarfFormalParameter(Dwarf_Die Die);
void HandleDwarfLexicalBlock(Dwarf_Die Die);
void HandleDwarfSubprogram(Dwarf_Die Die);
void HandleDwarfVariable(Dwarf_Die Die);

void (*TagFunctions[75])(Dwarf_Die Die) = {
    [DW_TAG_enumeration_type] = HandleDwarfEnumerationType,
    [DW_TAG_enumerator] = HandleDwarfEnumerator,
    [DW_TAG_base_type] = HandleDwarfBaseType,
    [DW_TAG_typedef] = HandleDwarfTypedef,
    [DW_TAG_array_type] = HandleDwarfArrayType,
    [DW_TAG_subrange_type] = HandleDwarfSubrangeType,
    [DW_TAG_pointer_type] = HandleDwarfPointerType,
    [DW_TAG_subroutine_type] = HandleDwarfSubroutineType,
    [DW_TAG_formal_parameter] = HandleDwarfFormalParameter,
    [DW_TAG_structure_type] = HandleDwarfStructureType,
    [DW_TAG_member] = HandleDwarfMember,
    [DW_TAG_lexical_block] = HandleDwarfLexicalBlock,
    [DW_TAG_subprogram] = HandleDwarfSubprogram,
    [DW_TAG_variable] = HandleDwarfVariable,
};

void GetAllSourceFiles(Dwarf_Die Die)
{
    dwarf_srcfiles(Die, &GlobalSourceFiles.Files, &GlobalSourceFiles.Count, 0);

    fprintf(stdout, "Detected files:\n");
    for (int Index = 0; Index < GlobalSourceFiles.Count; Index++) {
        fprintf(stdout, "\t%s\n", GlobalSourceFiles.Files[Index]);
    }
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
            case DW_TAG_enumeration_type:
            case DW_TAG_enumerator:
            case DW_TAG_subprogram:
            case DW_TAG_variable:
            case DW_TAG_formal_parameter:
            case DW_TAG_lexical_block:
            case DW_TAG_structure_type:
            case DW_TAG_member:
            case DW_TAG_array_type:
            case DW_TAG_subrange_type:
            case DW_TAG_pointer_type:
            case DW_TAG_subroutine_type:
            case DW_TAG_typedef:
            case DW_TAG_base_type:
                TagFunctions[Tag](ChildDie);
                break;
            default:
                break;
        }
    } while (dwarf_siblingof(GlobalDwarfDebug, ChildDie, &ChildDie, 0) == 0);
}

void HandleDwarfEnumerationType(Dwarf_Die Die)
{
    const char* Name = GetTagString(Die, DW_AT_name);
    Dwarf_Unsigned Encoding = GetTagUnsignedData(Die, DW_AT_encoding);
    Dwarf_Unsigned Size = GetTagUnsignedData(Die, DW_AT_byte_size);
    Dwarf_Unsigned Line = GetTagUnsignedData(Die, DW_AT_decl_line);
    Dwarf_Unsigned File = GetTagUnsignedData(Die, DW_AT_decl_file);
    Dwarf_Unsigned Column = GetTagUnsignedData(Die, DW_AT_decl_column);
    Dwarf_Off Type = GetTagRef(Die, DW_AT_type);
    Dwarf_Off Sibling = GetTagRef(Die, DW_AT_sibling);

    Dwarf_Bool HasChildren = 0;
    Dwarf_Die ChildDie = 0;
    if (dwarf_child(Die, &ChildDie, &GlobalDwarfError) == DW_DLV_OK) {
        HasChildren = 1;
    }

    const char* FileName = File == 0 ? "(null)" : GlobalSourceFiles.Files[File - 1];

    fprintf(stdout, "DW_TAG_enumeration_type - Children: %d\n"
                    "\tDW_AT_name: %s\n"
                    "\tDW_AT_encoding: %llu\n"
                    "\tDW_AT_byte_size: %llu\n"
                    "\tDW_AT_decl_file: %s\n"
                    "\tDW_AT_decl_line: %d\n"
                    "\tDW_AT_decl_column: %llu\n"
                    "\tDW_AT_type: <0x%0.8x>\n"
                    "\tDW_AT_sibling: 0x%0.8x\n",
            HasChildren, Name, Encoding, Size, FileName, Line, Column, Type, Sibling);

    if (HasChildren) {
        DwarfGetChildInfo(ChildDie);
    }
}

void HandleDwarfEnumerator(Dwarf_Die Die)
{
    const char* Name = GetTagString(Die, DW_AT_name);
    Dwarf_Unsigned Value = GetTagUnsignedData(Die, DW_AT_const_value);

    fprintf(stdout, "DW_TAG_enumerator\n"
                    "\tDW_AT_name: %s\n"
                    "\tDW_AT_const_value: %llu\n",
            Name, Value);
}

void HandleDwarfBaseType(Dwarf_Die Die)
{
    char* Name = GetTagString(Die, DW_AT_name);
    Dwarf_Off Type = GetTagRef(Die, DW_AT_type);
    Dwarf_Unsigned Size = GetTagUnsignedData(Die, DW_AT_byte_size);

    fprintf(stdout, "DW_TAG_base_type\n"
                    "\tDW_AT_name: %s\n"
                    "\tDW_AT_type: <0x%0.8x>\n"
                    "\tDW_AT_byte_size: %llu\n",
            Name, Type, Size);
}

void HandleDwarfTypedef(Dwarf_Die Die)
{
    char* Name = GetTagString(Die, DW_AT_name);
    Dwarf_Unsigned File = GetTagUnsignedData(Die, DW_AT_decl_file);
    Dwarf_Unsigned Line = GetTagUnsignedData(Die, DW_AT_decl_line);
    Dwarf_Unsigned Column = GetTagUnsignedData(Die, DW_AT_decl_column);
    Dwarf_Off Type = GetTagRef(Die, DW_AT_type);

    const char* FileName = File == 0 ? "(null)" : GlobalSourceFiles.Files[File - 1];

    fprintf(stdout, "DW_TAG_typedef\n"
                    "\tDW_AT_name: %s\n"
                    "\tDW_AT_decl_file: %s\n"
                    "\tDW_AT_decl_line: %d\n"
                    "\tDW_AT_decl_column: %llu\n"
                    "\tDW_AT_type: <0x%0.8x>\n",
            Name, FileName, Line, Column, Type);
}

void HandleDwarfArrayType(Dwarf_Die Die)
{
    Dwarf_Off Type = GetTagRef(Die, DW_AT_type);
    Dwarf_Off Sibling = GetTagRef(Die, DW_AT_sibling);

    Dwarf_Bool HasChildren = 0;
    Dwarf_Die ChildDie = 0;
    if (dwarf_child(Die, &ChildDie, &GlobalDwarfError) == DW_DLV_OK) {
        HasChildren = 1;
    }

    fprintf(stdout, "DW_TAG_array_type - Children: %d\n"
                    "\tDW_AT_type: <0x%0.8x>\n"
                    "\tDW_AT_sibling: %llu\n",
            HasChildren, Type, Sibling);

    if (HasChildren) {
        DwarfGetChildInfo(ChildDie);
    }
}

void HandleDwarfSubrangeType(Dwarf_Die Die)
{
    Dwarf_Off Type = GetTagRef(Die, DW_AT_type);
    Dwarf_Unsigned UpperBound = GetTagUnsignedData(Die, DW_AT_upper_bound);

    fprintf(stdout, "DW_TAG_subrange_type\n"
                    "\tDW_AT_type: <0x%0.8x>\n"
                    "\tDW_AT_upper_bound: %llu\n",
            Type, UpperBound);
}

void HandleDwarfPointerType(Dwarf_Die Die)
{
    Dwarf_Unsigned Size = GetTagUnsignedData(Die, DW_AT_byte_size);
    Dwarf_Off Type = GetTagRef(Die, DW_AT_type);

    fprintf(stdout, "DW_TAG_pointer_type\n"
                    "\tDW_AT_byte_size: %llu\n"
                    "\tDW_AT_type: <0x%0.8x>\n",
            Size, Type);
}

void HandleDwarfSubroutineType(Dwarf_Die Die)
{
    Dwarf_Off Sibling = GetTagRef(Die, DW_AT_sibling);

    Dwarf_Bool HasChildren = 0;
    Dwarf_Die ChildDie = 0;
    if (dwarf_child(Die, &ChildDie, &GlobalDwarfError) == DW_DLV_OK) {
        HasChildren = 1;
    }

    fprintf(stdout, "DW_TAG_subroutine_type - Children: %d\n"
                    "\tDW_AT_sibling: 0x%0.8x\n",
            HasChildren, Sibling);

    if (HasChildren) {
        DwarfGetChildInfo(ChildDie);
    }
}

void HandleDwarfStructureType(Dwarf_Die Die)
{
    char* Name = GetTagString(Die, DW_AT_name);
    Dwarf_Unsigned Size = GetTagUnsignedData(Die, DW_AT_byte_size);
    Dwarf_Unsigned File = GetTagUnsignedData(Die, DW_AT_decl_file);
    Dwarf_Unsigned Line = GetTagUnsignedData(Die, DW_AT_decl_line);
    Dwarf_Unsigned Column = GetTagUnsignedData(Die, DW_AT_decl_column);
    Dwarf_Off Sibling = GetTagRef(Die, DW_AT_sibling);

    Dwarf_Bool HasChildren = 0;
    Dwarf_Die ChildDie = 0;
    if (dwarf_child(Die, &ChildDie, &GlobalDwarfError) == DW_DLV_OK) {
        HasChildren = 1;
    }

    const char* FileName = File == 0 ? "(null)" : GlobalSourceFiles.Files[File - 1];

    fprintf(stdout, "DW_TAG_structure_type - Childre: %d\n"
                    "\tDW_AT_name: %s\n"
                    "\tDW_AT_byte_size: %llu\n"
                    "\tDW_AT_decl_file: %s\n"
                    "\tDW_AT_decl_line: %d\n"
                    "\tDW_AT_decl_column: %llu\n"
                    "\tDW_AT_sibling: %llu\n",
            HasChildren, Name, Size, FileName, Line, Column, Sibling);

    if (HasChildren) {
        DwarfGetChildInfo(ChildDie);
    }
}

void HandleDwarfMember(Dwarf_Die Die)
{
    char* Name = GetTagString(Die, DW_AT_name);
    Dwarf_Unsigned File = GetTagUnsignedData(Die, DW_AT_decl_file);
    Dwarf_Unsigned Line = GetTagUnsignedData(Die, DW_AT_decl_line);
    Dwarf_Unsigned Column = GetTagUnsignedData(Die, DW_AT_decl_column);
    Dwarf_Off Type = GetTagRef(Die, DW_AT_type);
    Dwarf_Unsigned MemberLocation = GetTagUnsignedData(Die, DW_AT_data_member_location);

    const char* FileName = File == 0 ? "(null)" : GlobalSourceFiles.Files[File - 1];

    fprintf(stdout, "DW_TAG_member\n"
                    "\tDW_AT_name: %s\n"
                    "\tDW_AT_decl_file: %s\n"
                    "\tDW_AT_decl_line: %d\n"
                    "\tDW_AT_decl_column: %llu\n"
                    "\tDW_AT_type: <0x%0.8x>\n"
                    "\tDW_AT_data_member_location: %llu\n",
            Name, FileName, Line, Column, Type, MemberLocation);
}

void HandleDwarfLexicalBlock(Dwarf_Die Die)
{
    Dwarf_Addr LowPC = GetTagAddress(Die, DW_AT_low_pc);
    Dwarf_Unsigned HighPC = GetTagUnsignedData(Die, DW_AT_high_pc);
    Dwarf_Off Sibling = GetTagRef(Die, DW_AT_sibling);

    Dwarf_Bool HasChildren = 0;
    Dwarf_Die ChildDie = 0;
    if (dwarf_child(Die, &ChildDie, &GlobalDwarfError) == DW_DLV_OK) {
        HasChildren = 1;
    }

    fprintf(stdout, "DW_TAG_lexical_block - Children: %d\n"
                    "\tDW_AT_low_pc: 0x%0.8x\n"
                    "\tDW_AT_high_pc: %llu\n"
                    "\tDW_AT_sibling: 0x%0.8x\n",
            HasChildren, LowPC, HighPC, Sibling);

    if (HasChildren) {
        DwarfGetChildInfo(ChildDie);
    }
}

void HandleDwarfFormalParameter(Dwarf_Die Die)
{
    char* Name = GetTagString(Die, DW_AT_name);
    Dwarf_Unsigned File = GetTagUnsignedData(Die, DW_AT_decl_file);
    Dwarf_Unsigned Line = GetTagUnsignedData(Die, DW_AT_decl_line);
    Dwarf_Unsigned Column = GetTagUnsignedData(Die, DW_AT_decl_column);
    Dwarf_Off Type = GetTagRef(Die, DW_AT_type);
    Dwarf_Ptr LocationPointer = 0;
    Dwarf_Unsigned Location = GetTagExprLoc(Die, DW_AT_location, LocationPointer);

    const char* FileName = File == 0 ? "(null)" : GlobalSourceFiles.Files[File - 1];

    fprintf(stdout, "DW_TAG_formal_parameter\n"
                    "\tDW_AT_name: %s\n"
                    "\tDW_AT_decl_file: %s\n"
                    "\tDW_AT_decl_line: %d\n"
                    "\tDW_AT_decl_column: %llu\n"
                    "\tDW_AT_type: <0x%0.8x>\n"
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
    Dwarf_Ptr FrameBasePointer = 0;
    Dwarf_Unsigned FrameBase = GetTagExprLoc(Die, DW_AT_frame_base, FrameBasePointer);
    Dwarf_Off Sibling = GetTagRef(Die, DW_AT_sibling);

    Dwarf_Bool HasChildren = 0;
    Dwarf_Die ChildDie = 0;
    if (dwarf_child(Die, &ChildDie, &GlobalDwarfError) == DW_DLV_OK) {
        HasChildren = 1;
    }

    const char* FileName = File == 0 ? "(null)" : GlobalSourceFiles.Files[File - 1];

    fprintf(stdout, "DW_TAG_subprogram - Children: %d\n"
                    "\tDW_AT_external: %d\n"
                    "\tDW_AT_name: %s\n"
                    "\tDW_AT_decl_file: %s\n"
                    "\tDW_AT_decl_line: %d\n"
                    "\tDW_AT_decl_column: %llu\n"
                    "\tDW_AT_linkage_name: %s\n"
                    "\tDW_AT_type: <0x%0.8x>\n"
                    "\tDW_AT_low_pc: 0x%0.8x\n"
                    "\tDW_AT_high_pc: %llu\n"
                    "\tDW_AT_frame_base: 0x%0.8x\n"
                    "\tDW_AT_sibling: 0x%0.8x\n",
            HasChildren, External, Name, FileName, Line, Column, LinkageName, Type, LowPC, HighPC, FrameBase, Sibling);

    if (HasChildren) {
        DwarfGetChildInfo(ChildDie);
    }
}

void HandleDwarfVariable(Dwarf_Die Die)
{
    const char* Name = GetTagString(Die, DW_AT_name);
    Dwarf_Unsigned Line = GetTagUnsignedData(Die, DW_AT_decl_line);
    Dwarf_Unsigned File = GetTagUnsignedData(Die, DW_AT_decl_file);
    Dwarf_Unsigned Column = GetTagUnsignedData(Die, DW_AT_decl_column);
    Dwarf_Off Type = GetTagRef(Die, DW_AT_type);
    Dwarf_Bool External = GetTagFlag(Die, DW_AT_external);

    Dwarf_Ptr LocationPointer = 0;
    Dwarf_Unsigned Location = GetTagExprLoc(Die, DW_AT_location, LocationPointer);

    const char* FileName = File == 0 ? "(null)" : GlobalSourceFiles.Files[File - 1];

    fprintf(stdout, "DW_TAG_variable\n"
                    "\tDW_AT_name: %s\n"
                    "\tDW_AT_decl_file: %s\n"
                    "\tDW_AT_decl_line: %d\n"
                    "\tDW_AT_decl_column: %llu\n"
                    "\tDW_AT_external: %d\n"
                    "\tDW_AT_type: <0x%0.8x>\n"
                    "\tDW_AT_location: %llu\n",
            Name, FileName, Line, Column, Type, External, Location);
}

void HandleDwarfCompilationUnit(Dwarf_Die CUDie)
{
    char* Producer = GetTagString(CUDie, DW_AT_producer);
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
    Dwarf_Macro_Context MacroContext = 0;
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

void HandleDwarfSourceLines(Dwarf_Die CUDie)
{
    const char* SectionName = 0;
    int Result = 0;

    Result = dwarf_get_line_section_name_from_die(CUDie, &SectionName, &GlobalDwarfError);
}

void HandleDwarfDebugStr()
{
    const char* SectionName = 0;

    Dwarf_Off StringOffset = 0;
    char* StringName = 0;
    Dwarf_Signed StringLength = 0;

    int Index = 0;
    int Result = 0;

    Result = dwarf_get_string_section_name(GlobalDwarfDebug, &SectionName, &GlobalDwarfError);
    if (Result != DW_DLV_OK) {
        exit(1);
    }

    fprintf(stdout, "String Section Name: %s\n", SectionName);

    Result = dwarf_get_str(GlobalDwarfDebug, StringOffset, &StringName, &StringLength, &GlobalDwarfError);
    for (Index = 0; Result == DW_DLV_OK; Index++) {
        fprintf(stdout, "name at offset 0x%0.8x, length %llu is '%s'\n", StringOffset, StringLength, StringName);

        StringOffset += StringLength + 1;
        Result = dwarf_get_str(GlobalDwarfDebug, StringOffset, &StringName, &StringLength, &GlobalDwarfError);
    }
}

void HandleDwarfCompilationUnitMacrosByOffset(Dwarf_Die CUDie, Dwarf_Unsigned Offset)
{
    Dwarf_Unsigned Version = 0;
    Dwarf_Macro_Context MacroContext = 0;
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
        fprintf(stdout, "\n\n");

        HandleDwarfCompilationUnit(CUDie);
        fprintf(stdout, "\n\n");

        HandleDwarfCompilationUnitMacros(CUDie);
        fprintf(stdout, "\n");

        for (int Index = 0; Index < GlobalArray.used; Index++) {
            HandleDwarfCompilationUnitMacrosByOffset(CUDie, GlobalArray.array[Index]);
            fprintf(stdout, "\n");
        }

        fprintf(stdout, "\n");

        HandleDwarfDebugStr();

        fprintf(stdout, "\n");

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
                case DW_TAG_enumeration_type:
                case DW_TAG_enumerator:
                case DW_TAG_subprogram:
                case DW_TAG_variable:
                case DW_TAG_formal_parameter:
                case DW_TAG_lexical_block:
                case DW_TAG_structure_type:
                case DW_TAG_member:
                case DW_TAG_array_type:
                case DW_TAG_subrange_type:
                case DW_TAG_pointer_type:
                case DW_TAG_subroutine_type:
                case DW_TAG_typedef:
                case DW_TAG_base_type:
                    TagFunctions[Tag](ChildDie);
                    break;
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

    ArrayInit(&GlobalArray, 1);

    FileDescriptor = open(argv[0], O_RDONLY);

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

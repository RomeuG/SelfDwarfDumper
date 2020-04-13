#include <dwarf.h>
#include <fcntl.h>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

static Dwarf_Unsigned GlobalMapEntryCount = 0;
static Dwarf_Die GlobalDwarfDie = 0;

// NOTE: only handled if Dwarf_Error == NULL in Dwarf functions
static void DwarfErrorHandler(Dwarf_Error Error, Dwarf_Ptr ErrArg)
{
    fprintf(stderr, "Libdwarf error: %d - %s\n", dwarf_errno(Error), dwarf_errmsg(Error));
    exit(-1);
}

void SectionInfo(Dwarf_Debug* DwarfDebug, const char* SectionName)
{
    const char* ActualSectionName = 0;
    Dwarf_Small IsMarkedCompressed = 0;
    Dwarf_Small IsMarkedZLibCompressed = 0;
    Dwarf_Small IsMarkedSHFCompressed = 0;
    Dwarf_Unsigned CompressedLength = 0;
    Dwarf_Unsigned UncompressedLength = 0;
    Dwarf_Error DwarfError;

    dwarf_get_real_section_name(*DwarfDebug, SectionName, &ActualSectionName,
                                &IsMarkedCompressed, &IsMarkedZLibCompressed,
                                &IsMarkedSHFCompressed, &CompressedLength,
                                &UncompressedLength, &DwarfError);

    fprintf(stdout, "SectionInfo()\nActualSectionName = %s\nIsMarkedCompressed = %d\n"
                    "IsMarkedZLibCompressed = %d\nIsMarkedSHFCompressed = %d\n"
                    "CompressedLength = %d\nUncompressedLength = %d\n\n",
            ActualSectionName,
            IsMarkedCompressed, IsMarkedZLibCompressed, IsMarkedSHFCompressed,
            CompressedLength, UncompressedLength);
}

void SectionGroupSizes(Dwarf_Debug* DwarfDebug)
{
    Dwarf_Unsigned SectionCount = 0;
    Dwarf_Unsigned GroupCount = 0;
    Dwarf_Unsigned SelectedGroup = 0;
    Dwarf_Unsigned MapEntryCount = 0;
    Dwarf_Error DwarfError;

    dwarf_sec_group_sizes(*DwarfDebug, &SectionCount, &GroupCount,
                          &SelectedGroup, &MapEntryCount, &DwarfError);

    GlobalMapEntryCount = MapEntryCount;

    fprintf(stdout, "SectionGroupSizes()\nSectionCount = %d\nGroupCount = %d\n"
                    "SelectedGroup = %d\nMapEntryCount = %d\n\n",
            SectionCount, GroupCount, SelectedGroup, MapEntryCount);
}

void SectionGroupMapping(Dwarf_Debug* DwarfDebug)
{
    Dwarf_Unsigned* GroupNumbers = (Dwarf_Unsigned*)calloc(GlobalMapEntryCount, sizeof(Dwarf_Unsigned));
    Dwarf_Unsigned* SectionNumbers = (Dwarf_Unsigned*)calloc(GlobalMapEntryCount, sizeof(Dwarf_Unsigned));
    const char** SectionNames = (const char**)calloc(GlobalMapEntryCount, sizeof(char*));
    Dwarf_Error DwarfError;

    dwarf_sec_group_map(*DwarfDebug, GlobalMapEntryCount, GroupNumbers, SectionNumbers, SectionNames, &DwarfError);

    fprintf(stdout, "SectionGroupMapping()\nSection Names:\n");

    for (int i = 0; i < GlobalMapEntryCount; i++) {
        fprintf(stdout, "%s\n", SectionNames[i]);
    }

    fprintf(stdout, "\n");

    free(GroupNumbers);
    free(SectionNumbers);
    free(SectionNames);
}

void SectionSizes(Dwarf_Debug* DwarfDebug)
{
    Dwarf_Unsigned InfoSize = 0;
    Dwarf_Unsigned AbbrevSize = 0;
    Dwarf_Unsigned LineSize = 0;
    Dwarf_Unsigned LocSize = 0;
    Dwarf_Unsigned ArangesSize = 0;
    Dwarf_Unsigned MacInfoSize = 0;
    Dwarf_Unsigned PubNamesSize = 0;
    Dwarf_Unsigned StrSize = 0;
    Dwarf_Unsigned FrameSize = 0;
    Dwarf_Unsigned RangesSize = 0;
    Dwarf_Unsigned PubTypesSize = 0;

    dwarf_get_section_max_offsets(*DwarfDebug, &InfoSize, &AbbrevSize,
                                  &LineSize, &LocSize, &ArangesSize,
                                  &MacInfoSize, &PubNamesSize, &StrSize,
                                  &FrameSize, &RangesSize, &PubTypesSize);

    fprintf(stdout, "SectionSizes()\nInfoSize = %d\n"
                    "AbbrevSize = %d\n"
                    "LineSize = %d\n"
                    "LocSize = %d\n"
                    "ArangesSize = %d\n"
                    "MacInfoSize = %d\n"
                    "PubNamesSize = %d\n"
                    "StrSize = %d\n"
                    "FrameSize = %d\n"
                    "RangesSize = %d\n"
                    "PubTypesSize = %d\n\n",
            InfoSize, AbbrevSize,
            LineSize, LocSize, ArangesSize,
            MacInfoSize, PubNamesSize, StrSize,
            FrameSize, RangesSize, PubTypesSize);
}

void CUHeaders(Dwarf_Debug* DwarfDebug, Dwarf_Bool IsInfo)
{
    Dwarf_Unsigned HeaderLength = 0;
    Dwarf_Half VersionStamp = 0;
    Dwarf_Unsigned AbbrevOffset = 0;
    Dwarf_Half AddressSize = 0;
    Dwarf_Half OffsetSize = 0;
    Dwarf_Half ExtensionSize = 0;
    Dwarf_Sig8 Signature = {};
    Dwarf_Unsigned TypeOffset = 0;
    Dwarf_Unsigned NextCUHeader = 0;
    Dwarf_Half HeaderCUType = 0;
    Dwarf_Error DwarfError;

    dwarf_next_cu_header_d(*DwarfDebug, IsInfo, &HeaderLength,
                           &VersionStamp, &AbbrevOffset, &AddressSize,
                           &OffsetSize, &ExtensionSize, &Signature,
                           &TypeOffset, &NextCUHeader, &HeaderCUType,
                           &DwarfError);

    fprintf(stdout, "CUHeaders():\n"
                    "HeaderLength = %d\n"
                    "VersionStamp = %d\n"
                    "AbbrevOffset = %d\n"
                    "AddressSize = %d\n"
                    "OffsetSize = %d\n"
                    "ExtensionSize = %d\n"
                    "Signature = %d\n"
                    "TypeOffset = %d\n"
                    "NextCUHeader = %d\n"
                    "HeaderCUType = %d\n\n",
            HeaderLength,
            VersionStamp,
            AbbrevOffset,
            AddressSize,
            OffsetSize,
            ExtensionSize,
            Signature,
            HeaderCUType,
            TypeOffset,
            NextCUHeader);
}

// Dwarf_Unsigned GetAttributeValue(Dwarf_Die DwarfDie, int Attribute)
// {
//     Dwarf_Unsigned AttributeValue = 0;
//     Dwarf_Attribute Attribute = 0;
//     Dwarf_Error DwarfError = 0;

//     dwarf_attr(DwarfDie, Attribute, &Attribute, &DwarfError);
//     dwarf_formudata(Attribute, &AttributeValue, &DwarfError);

//     return AttributeValue;
// }

void PrintDieAttributes(Dwarf_Die DwarfDie)
{
    int Result = 0;

    Dwarf_Attribute* AttributeList;
    Dwarf_Signed AttributeCount;

    Result = dwarf_attrlist(DwarfDie, &AttributeList, &AttributeCount, 0);
    if (Result == DW_DLV_ERROR) {
        fprintf(stderr, "dwarf_attrlist() error\n");
        exit(1);
    }

    for (int Index = 0; Index < AttributeCount; Index++) {
        Dwarf_Half AttributeCode;
        Result = dwarf_whatattr(AttributeList[Index], &AttributeCode, 0);
        if (Result == DW_DLV_ERROR) {
            fprintf(stderr, "dwarf_whatattr() error\n");
            exit(1);
        }

        if (AttributeCode == DW_AT_name) {
            char* String = 0;
            Result = dwarf_formstring(AttributeList[Index], &String, 0);
            if (Result == DW_DLV_ERROR) {
                fprintf(stderr, "dwarf_formstring() error\n");
                exit(1);
            }

            fprintf(stdout, "Attribute name: %s\n", String);
        }
    }
}

void UntitledFunction(Dwarf_Debug DwarfDebug)
{
    int Result;
    Dwarf_Error DwarfError = 0;
    Dwarf_Die DwarfDie = 0;
    Dwarf_Die DwarfDieChild = 0;

    Result = dwarf_next_cu_header(DwarfDebug, 0, 0, 0, 0, 0, 0);
    if (Result == DW_DLV_ERROR) {
        fprintf(stderr, "dwarf_next_cu_header() error\n");
        exit(1);
    }

    Result = dwarf_siblingof(DwarfDebug, 0, &DwarfDie, 0);
    if (Result == DW_DLV_ERROR) {
        fprintf(stderr, "dwarf_siblingof() error\n");
        exit(1);
    }

    PrintDieAttributes(DwarfDie);

    Result = dwarf_child(DwarfDie, &DwarfDieChild, 0);
    if (Result == DW_DLV_ERROR) {
        fprintf(stderr, "dwarf_child() error\n");
        exit(1);
    }
}

int main(void)
{
    int DwarfResult = -1;
    int FileDescriptor;

    Dwarf_Ptr DwarfErrArg = 0;
    Dwarf_Debug DwarfDebug;
    Dwarf_Error DwarfError;

    FileDescriptor = open("a.out", O_RDONLY);

    DwarfResult = dwarf_init(FileDescriptor, DW_DLC_READ, DwarfErrorHandler, DwarfErrArg, &DwarfDebug, &DwarfError);
    if (DwarfResult != DW_DLV_OK) {
        fprintf(stderr, "dwarf_init() error: %d\n", DwarfResult);
        exit(-1);
    }

    Dwarf_Cmdline_Options DwarfOptions = { 1 };
    dwarf_record_cmdline_options(DwarfOptions);

    fprintf(stdout, "DwarfDebug: %p\nDwarfError: %p\n\n", DwarfDebug, DwarfError);

    UntitledFunction(DwarfDebug);

    DwarfResult = dwarf_finish(DwarfDebug, &DwarfError);
    if (DwarfResult != DW_DLV_OK) {
        fprintf(stderr, "dwarf_init() error: %d\n", DwarfResult);
        exit(-1);
    }

    return 0;
}

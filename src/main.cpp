#include <dwarf.h>
#include <fcntl.h>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

static Dwarf_Unsigned GlobalMapEntryCount = 0;

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
                    "PubTypesSize = %d\n",
            InfoSize, AbbrevSize,
            LineSize, LocSize, ArangesSize,
            MacInfoSize, PubNamesSize, StrSize,
            FrameSize, RangesSize, PubTypesSize);
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

    SectionInfo(&DwarfDebug, ".debug_info");
    SectionGroupSizes(&DwarfDebug);
    SectionGroupMapping(&DwarfDebug);
    SectionSizes(&DwarfDebug);

    DwarfResult = dwarf_finish(DwarfDebug, &DwarfError);
    if (DwarfResult != DW_DLV_OK) {
        fprintf(stderr, "dwarf_init() error: %d\n", DwarfResult);
        exit(-1);
    }

    return 0;
}

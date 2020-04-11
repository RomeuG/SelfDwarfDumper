#include <dwarf.h>
#include <fcntl.h>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

// NOTE: only handled if Dwarf_Error == NULL in Dwarf functions
static void DwarfErrorHandler(Dwarf_Error Error, Dwarf_Ptr ErrArg)
{
    fprintf(stderr, "Libdwarf error: %d - %s\n", dwarf_errno(Error), dwarf_errmsg(Error));
    exit(-1);
}

void DebugInfoSectionInfo(Dwarf_Debug* DwarfDebug)
{
    const char* ActualSectionName = 0;
    Dwarf_Small IsMarkedCompressed = 0;
    Dwarf_Small IsMarkedZLibCompressed = 0;
    Dwarf_Small IsMarkedSHFCompressed = 0;
    Dwarf_Unsigned CompressedLength = 0;
    Dwarf_Unsigned UncompressedLength = 0;
    Dwarf_Error DwarfError;

    dwarf_get_real_section_name(*DwarfDebug, ".debug_info", &ActualSectionName,
                                &IsMarkedCompressed, &IsMarkedZLibCompressed,
                                &IsMarkedSHFCompressed, &CompressedLength,
                                &UncompressedLength, &DwarfError);

    fprintf(stdout, "ActualSectionName = %s\nIsMarkedCompressed = %d\n"
                    "IsMarkedZLibCompressed = %d\nIsMarkedSHFCompressed = %d\n"
                    "CompressedLength = %d\nUncompressedLength = %d\n",
            ActualSectionName,
            IsMarkedCompressed, IsMarkedZLibCompressed, IsMarkedSHFCompressed,
            CompressedLength, UncompressedLength);
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

    fprintf(stdout, "DwarfDebug: %p\nDwarfError: %p\n", DwarfDebug, DwarfError);

    DebugInfoSectionInfo(&DwarfDebug);

    DwarfResult = dwarf_finish(DwarfDebug, &DwarfError);
    if (DwarfResult != DW_DLV_OK) {
        fprintf(stderr, "dwarf_init() error: %d\n", DwarfResult);
        exit(-1);
    }

    return 0;
}

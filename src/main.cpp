#include <dwarf.h>
#include <fcntl.h>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int main(void)
{
    int FileDescriptor;
    Dwarf_Handler DwarfHandler;
    Dwarf_Ptr DwarfErrArg;
    Dwarf_Debug DwarfDebug;
    Dwarf_Error DwarfError;

    FileDescriptor = open("a.out", O_RDONLY);

    int DwarfInitResult = dwarf_init(FileDescriptor, DW_DLC_READ, DwarfHandler, DwarfErrArg, &DwarfDebug, &DwarfError);
    if (DwarfInitResult != DW_DLV_OK) {
        fprintf(stderr, "dwarf_init() error.\n");
        exit(-1);
    }

    int DwarfFinishResult = dwarf_finish(DwarfDebug, &DwarfError);
    if (DwarfFinishResult != DW_DLV_OK) {
        fprintf(stderr, "dwarf_init() error.\n");
        exit(-1);
    }

    return 0;
}

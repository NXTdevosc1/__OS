#include <setfile.h>
#include <interrupt_manager/SOD.h>
#include <stdlib.h>
#include <kernel.h>
BOOL SETFILE_LOADED = FALSE;

SETFILE gSystemSetfile = NULL;

KERNELSTATUS SystemSetfile(){
    if(SETFILE_LOADED) SOD(0, "SYSTEM SETFILE SECONDARY LOAD ATTEMPT");
    for(UINT i = 0;FileImportTable[i].Type != FILE_IMPORT_ENDOFTABLE;i++){
        if(FileImportTable[i].Type == FILE_IMPORT_SETFILE) {
            gSystemSetfile = FileImportTable[i].LoadedFileBuffer;
            break;
        }
    }
    if(!gSystemSetfile) SOD(SOD_INITIALIZATION, "CANNOT FIND SYSTEM_SETFILE");
    
    // Check Setfile Format
    if(!memcmp(gSystemSetfile->Signature, SYSTEM_SETFILE_SIGNATURE, 8) ||
    gSystemSetfile->Magic0 != SYSTEM_SETFILE_MAGIC0 ||
    !memcmp(gSystemSetfile->Description, SYSTEM_SETFILE_DESCRIPTION, LEN_SETFILE_DESCRIPTION)
    ) SOD(SOD_INITIALIZATION, "INVALID SYSTEM_SETFILE");

    return KERNEL_SOK;
}
#include <acpi/aml.h>
#include <acpi/amlobjects.h>
#include <Management/runtimesymbols.h>
#include <CPU/cpu.h>
KERNELSTATUS KERNELAPI AmlReadPackage(char* Aml, UINT PackageLength){
    char* PkgBuffer = Aml;
    // _RT_SystemDebugPrint(L"PKG_BUFFER_BYTE_STRINGS : %s", PkgBuffer);
    // _RT_SystemDebugPrint(L"PKG_BUFFER FIRST BYTES : %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x", PkgBuffer[0], PkgBuffer[1], PkgBuffer[2], PkgBuffer[3], PkgBuffer[4], PkgBuffer[5], PkgBuffer[6], PkgBuffer[7], PkgBuffer[8], PkgBuffer[9], PkgBuffer[10], PkgBuffer[11], PkgBuffer[12], PkgBuffer[13], PkgBuffer[14], PkgBuffer[15], PkgBuffer[16], PkgBuffer[17]);
    
    // Reading Term List
    char NameString[0x20] = {0};
    for(UINT i = 0;i<PackageLength;i++) {
        if(*Aml == AML_EXTENDED_OP) {
            INC_AML(1);
            UINT8 ExOp = *Aml;
            _RT_SystemDebugPrint(L"EXTENDED_OP");
            INC_AML(1);
            switch(ExOp) {
                case AML_OPERATION_REGION_EXOP:
                {
                    UINT64 NameStringBytes = 0;
                    AmlCopyName(NameString, Aml, &NameStringBytes);
                    INC_AML(NameStringBytes);
                    UINT8 RegionSpace = *Aml;
                    INC_AML(1);

                    UINT64 RegionOffset = 0;
                    UINT IncCount = 0;
                    AmlReadExpression(Aml, &RegionOffset, &IncCount);
                    INC_AML(IncCount);

                    UINT64 RegionLen = 0;
                    AmlReadExpression(Aml, &RegionLen, &IncCount);
                    INC_AML(IncCount);
                    _RT_SystemDebugPrint(L"Operation Region (%s), REGION_SPACE = %x, REGION_OFFSET : %x, REGION_LEN : %x", NameString, (UINT64)RegionSpace, (UINT64)RegionOffset, (UINT64)RegionLen);
                    break;
                }
                case AML_FIELD_EXOP:
                {
                    UINT64 IncBytes = 0;
                    UINT32 PkgLength = AmlReadPkgLength(Aml, (UINT*)&IncBytes);
                    INC_AML(IncBytes);
                    AmlCopyName(NameString, Aml, &IncBytes);
                    INC_AML(IncBytes);
                    AML_FIELD_FLAGS FieldFlags = *(AML_FIELD_FLAGS*)Aml;
                    INC_AML(1);
                    _RT_SystemDebugPrint(L"Field (%s) PACKAGE_LENGTH : %x", NameString, (UINT64)PkgLength);
                    _RT_SystemDebugPrint(L"Field Flags : AccessType : %x, LockRule : %x, UpdateRule : %x", DBGV FieldFlags.AccessType, DBGV FieldFlags.LockRule, DBGV FieldFlags.UpdateRule);
                    AmlParseFieldElement(Aml);
                    INC_AML(PkgLength - IncBytes - 2);
                    break;
                }
            }
        }else if(*Aml == AML_METHOD_OP) {
            _RT_SystemDebugPrint(L"Method ()");
            return KERNEL_SERR;
            while(1) __hlt();
        } else {
            _RT_SystemDebugPrint(L"UNKOWN_OPCODE (%x) - 1 (%x)", (UINT64)*Aml, (UINT64)*(Aml - 1));
            return KERNEL_SERR;
            while(1) __hlt();
        }
    }
    return KERNEL_SOK;        
}
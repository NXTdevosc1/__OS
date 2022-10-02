#include <acpi/aml.h>
#include <Management/runtimesymbols.h>
#include <stdlib.h>

#include <kernel.h>
#include <CPU/cpu.h>
#include <acpi/amlobjects.h>









void AcpiReadDsdt(RFACPI_DSDT Dsdt){
    UINT64 AmlLength = Dsdt->Sdt.Length - sizeof(ACPI_DSDT);
    UINT8* c1 = Dsdt->Sdt.OEMTableId;
    UINT8* c2 = Dsdt->Sdt.OEMID;
    SystemDebugPrint(L"AML_LENGTH : %x, TABLE_ID : %c%c%c%c%c%c%c%c, OEM_ID : %c%c%c%c%c%c", AmlLength, c1[0], c1[1], c1[2], c1[3], c1[4], c1[5], c1[6], c1[7], c2[0], c2[1], c2[2], c2[3], c2[4], c2[5]);
    char* Aml = Dsdt->aml;
    char DefName[0x100] = {0};
    for(UINT64 i = 0;i<AmlLength;) {
        if(*Aml == AML_ALIAS_OP) {
            INC_AML(1);

            SystemDebugPrint(L"ACPI : ALIAS_OP (%c%c%c%c %c%c%c%c)", Aml[0], Aml[1], Aml[2], Aml[3], Aml[4], Aml[5], Aml[6], Aml[7]);
            INC_AML(8);
        }
        else if(*Aml == AML_NAME_OP) {
            INC_AML(1);
            UINT64 NameStringBytes = 0;
            UINT Namelen = AmlCopyName(DefName, Aml, &NameStringBytes);
            SystemDebugPrint(L"ACPI : NAME (%s)", DefName);
            INC_AML(Namelen);
        }
        else if(*Aml == AML_SCOPE_OP) {
            // SCOPE OP
            INC_AML(1);
            PKG_LEAD_BYTE* Package = AMLSTRUCT Aml;
            INC_AML(1);
            // INC Package
            
            // Package Length Parsing
            
            UINT32 PkgLength = (UINT32)Package->PackageLength & 0xF;
            UINT8 FollowingBytes = Package->FollowingByteDataCount;
            if(FollowingBytes == 0) PkgLength |= (Package->PkgLenBelow63 << 4);
            else {
                UINT BitOff = 4;

                for(UINT c = 0;c<FollowingBytes;c++) {
                    
                    PkgLength |= ((UINT32)Aml[c] & 0xff) << BitOff;
                    BitOff+=8;
                }

            }
            

            INC_AML(FollowingBytes);
            UINT64 NameStringBytes = 0;
            UINT LenScopeName = AmlCopyName(DefName, AMLSTRUCT Aml, &NameStringBytes);
            // SystemDebugPrint(L"ACPI : SCOPE (%s) PKG=%x, FollowingBytes = %x, DECODED_LENGTH : %x, NSB : %x, FIRST_NAME_BYTES : %c%c%c%c %c%c%c%c %c%c%c%c%c", DefName, (UINT64)Package->PackageLength, (UINT64)Package->FollowingByteDataCount, (UINT64)PkgLength, NameStringBytes, Aml[0], Aml[1], Aml[2], Aml[3], Aml[4], Aml[5], Aml[6], Aml[7], Aml[8], Aml[9], Aml[10], Aml[11], Aml[12]);
            SystemDebugPrint(L"ACPI : SCOPE (%s) PKG=%x, FollowingBytes = %x, DECODED_LENGTH : %x, NSB : %x", DefName, (UINT64)Package->PackageLength, (UINT64)Package->FollowingByteDataCount, (UINT64)PkgLength, NameStringBytes, Aml[0], Aml[1], Aml[2], Aml[3], Aml[4], Aml[5], Aml[6], Aml[7], Aml[8], Aml[9], Aml[10], Aml[11], Aml[12]);
            
            // INC_AML(NameStringBytes);
            char* PkgBuffer = AMLSTRUCT (Aml + NameStringBytes);
            INC_AML(PkgLength - FollowingBytes - 1);

            // Parsing PkgBuffer
            AmlReadPackage(PkgBuffer, PkgLength);
            
            // while(1) __hlt();
        }else if(*Aml == AML_METHOD_OP) {
            INC_AML(1);
            PKG_LEAD_BYTE* Package = AMLSTRUCT Aml;
            // Package Length Parsing
            INC_AML(1);
            
            UINT32 PkgLength = (UINT32)Package->PackageLength & 0xF;
            UINT8 FollowingBytes = Package->FollowingByteDataCount;
            if(FollowingBytes == 0) PkgLength |= (Package->PkgLenBelow63 << 4);
            else if(FollowingBytes == 1) PkgLength |= (((UINT32)Aml[0] & 0xff) << 4);
            else if(FollowingBytes == 2) {
                PkgLength |= (((UINT32)Aml[0] & 0xff) << 4) | (((UINT32)Aml[1] & 0xff) << 12);
            }else if(FollowingBytes == 3) {
                PkgLength |= (((UINT32)Aml[0] & 0xff) << 4) | (((UINT32)Aml[1] & 0xff) << 12) | (((UINT32)Aml[2] & 0xff) << 20);
            }
            INC_AML(FollowingBytes);

            // UINT LenMethodName = AmlCopyName(DefName, Aml);
            SystemDebugPrint(L"Method (%s) PACKAGE_LENGTH : %x, NAME_BYTES : %c%c%c%c%c%c%c%c%c%c%c", DefName, PkgLength, Aml[0], Aml[1], Aml[2], Aml[3], Aml[4], Aml[5], Aml[6], Aml[7], Aml[8], Aml[9], Aml[10]);
            INC_AML(PkgLength - FollowingBytes - 2);
        }else {
            INC_AML(1);
        }
    }
}
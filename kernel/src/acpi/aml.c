#include <acpi/aml.h>
#include <Management/runtimesymbols.h>
#include <stdlib.h>
#define INC_AML(val) Aml+=val; i+=val;
#define AMLSTRUCT (void*)
#include <kernel.h>

#define DBGV (UINT64) // Cast Value to UINT64 For System Debug Print



UINT AmlCopyName(char* Destination, const char* Source, UINT64* NameStringBytes) {
    UINT NumNames = 1;
    UINT64 nsb = 0;
    
    if(*Source == AML_ROOT_PREFIX || *Source == '"') {
        *Destination = AML_ROOT_PREFIX;
        Source++;
        nsb++;
        Destination++;
    }

    if(*Source == AML_MULTINAME_PREFIX) {
        NumNames = (UINT)Source[1] & 0xff;
        Source+=2;
        nsb+=2;
    }
    if(*Source == AML_DUALNAME_PREFIX) {
        NumNames = 2;
        Source++;
        nsb++;
    }
    UINT len = 0;
    for(UINT x = 0;x<NumNames;x++) {
        if(x) {
            *Destination = '.';
            Destination++;
            len++;
        }
        UINT StartingCharacter = 0;
        for(UINT i = 0;i<4 /*Lead name char + 4 characters*/;i++, Source++, nsb++){
            if(!x && !i) i = nsb;
            char c = *Source;
            if(c == 0) {
                nsb++;
                break;
            }
            if(i > StartingCharacter && c == '_') {
                Source += 4 - i;
                nsb += 4 - i;
                break;
            }
            *Destination = c;
            Destination++;
            len++;
        }
    }

    *Destination = 0;
    *NameStringBytes = nsb;
    return len;
}

void AmlReadExpression(char* Aml, void* Value, UINT* LenExpression) {
    UINT8 Prefix = *Aml;
    Aml++;
    if(Prefix == AML_BYTE_PREFIX) {
        *(UINT8*)Value = *(UINT8*)Aml;
        *LenExpression = 2;
    }else if(Prefix == AML_WORD_PREFIX) {
        *(UINT16*)Value = *(UINT16*)Aml;
        *LenExpression = 3;
    }else if(Prefix == AML_DWORD_PREFIX) {
        *(UINT32*)Value = *(UINT32*)Aml; 
        *LenExpression = 5;
    }else if(Prefix == AML_ZERO_OP || Prefix == AML_ONE_OP || Prefix == AML_ONES_OP) {
        *(UINT8*)Value = Prefix;
        *LenExpression = 1;
    }
}

UINT32 AmlReadPkgLength(char* Aml, UINT* IncBytes) {
    UINT _IncBytes = 1;
    PKG_LEAD_BYTE* LeadByte = AMLSTRUCT Aml;
    Aml++;
    UINT32 PkgLength = (UINT32)LeadByte->PackageLength & 0xF;
    UINT8 FollowingBytes = LeadByte->FollowingByteDataCount;
    _IncBytes += FollowingBytes;
    if(FollowingBytes == 0) PkgLength |= (LeadByte->PkgLenBelow63 << 4);
    else {
        UINT BitOff = 4;

        for(UINT i = 0;i<FollowingBytes;i++) {
            
            PkgLength |= ((UINT32)Aml[i] & 0xff) << BitOff;
            BitOff+=8;
        }

    }
    *IncBytes = _IncBytes;
    return PkgLength;
}

KERNELSTATUS KERNELAPI AmlReadPackage(char* Aml, UINT PackageLength){
    char* PkgBuffer = Aml;
    _RT_SystemDebugPrint(L"PKG_BUFFER_BYTE_STRINGS : %s", PkgBuffer);
    _RT_SystemDebugPrint(L"PKG_BUFFER FIRST BYTES : %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x", PkgBuffer[0], PkgBuffer[1], PkgBuffer[2], PkgBuffer[3], PkgBuffer[4], PkgBuffer[5], PkgBuffer[6], PkgBuffer[7], PkgBuffer[8], PkgBuffer[9], PkgBuffer[10], PkgBuffer[11], PkgBuffer[12], PkgBuffer[13], PkgBuffer[14], PkgBuffer[15], PkgBuffer[16], PkgBuffer[17]);

    // Reading Term List
    char NameString[0x100] = {0};
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
                    _RT_SystemDebugPrint(L"Field (%s) PACKAGE_LENGTH : %x", NameString, (UINT64)PkgLength);
                    _RT_SystemDebugPrint(L"Field Flags : AccessType : %x, LockRule : %x, UpdateRule : %x", DBGV FieldFlags.AccessType, DBGV FieldFlags.LockRule, DBGV FieldFlags.UpdateRule);
                    break;
                }
            }
        }else if(*Aml == AML_METHOD_OP) {
            _RT_SystemDebugPrint(L"Method ()");
        }
    }
    return KERNEL_SOK;        
}

void AcpiReadDsdt(RFACPI_DSDT Dsdt){
    UINT64 AmlLength = Dsdt->Sdt.Length - sizeof(ACPI_DSDT);
    _RT_SystemDebugPrint(L"AML_LENGTH : %x, TABLE_ID : %s, OEM_ID : %s", AmlLength, Dsdt->Sdt.OEMTableId, Dsdt->Sdt.OEMID);
    char* Aml = Dsdt->aml;
    char DefName[0x100] = {0};
    for(UINT64 i = 0;i<AmlLength;) {
        if(*Aml == AML_ALIAS_OP) {
            INC_AML(1);

            _RT_SystemDebugPrint(L"ACPI : ALIAS_OP (%c%c%c%c %c%c%c%c)", Aml[0], Aml[1], Aml[2], Aml[3], Aml[4], Aml[5], Aml[6], Aml[7]);
            INC_AML(8);
        }
        else if(*Aml == AML_NAME_OP) {
            INC_AML(1);
            UINT64 NameStringBytes = 0;
            UINT Namelen = AmlCopyName(DefName, Aml, &NameStringBytes);
            _RT_SystemDebugPrint(L"ACPI : NAME (%s)", DefName);
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

                for(UINT i = 0;i<FollowingBytes;i++) {
                    
                    PkgLength |= ((UINT32)Aml[i] & 0xff) << BitOff;
                    BitOff+=8;
                }

            }
            

            INC_AML(FollowingBytes);
            UINT64 NameStringBytes = 0;
            UINT LenScopeName = AmlCopyName(DefName, AMLSTRUCT Aml, &NameStringBytes);
            _RT_SystemDebugPrint(L"ACPI : SCOPE (%s) PKG=%x, FollowingBytes = %x, DECODED_LENGTH : %x, NSB : %x, FIRST_NAME_BYTES : %c%c%c%c %c%c%c%c %c%c%c%c%c", DefName, (UINT64)Package->PackageLength, (UINT64)Package->FollowingByteDataCount, (UINT64)PkgLength, NameStringBytes, Aml[0], Aml[1], Aml[2], Aml[3], Aml[4], Aml[5], Aml[6], Aml[7], Aml[8], Aml[9], Aml[10], Aml[11], Aml[12]);
            // INC_AML(NameStringBytes);
            char* PkgBuffer = AMLSTRUCT (Aml + NameStringBytes);
            INC_AML(PkgLength - FollowingBytes - 1);

            // Parsing PkgBuffer
            AmlReadPackage(PkgBuffer, PkgLength);
            while(1);
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
            _RT_SystemDebugPrint(L"Method (%s) PACKAGE_LENGTH : %x, NAME_BYTES : %c%c%c%c%c%c%c%c%c%c%c", DefName, PkgLength, Aml[0], Aml[1], Aml[2], Aml[3], Aml[4], Aml[5], Aml[6], Aml[7], Aml[8], Aml[9], Aml[10]);
            INC_AML(PkgLength - FollowingBytes - 2);
        }else {
            INC_AML(1);
        }
    }
}
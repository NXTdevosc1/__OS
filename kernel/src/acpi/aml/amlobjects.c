#include <acpi/amlobjects.h>
#include <Management/runtimesymbols.h>


UINT KERNELAPI AmlCopyName(char* Destination, const char* Source, UINT64* NameStringBytes) {
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

void KERNELAPI AmlReadExpression(char* Aml, void* Value, UINT* LenExpression) {
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

UINT32 KERNELAPI AmlReadPkgLength(char* Aml, UINT* IncBytes) {
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


char* KERNELAPI AmlParseFieldElement(char* Aml) {
    char NameStr[0x20] = {0};
    UINT64 NmBytes = 0;

    UINT8 FieldType = *Aml;
    UINT i = 0; // Increment Count
    switch(FieldType) {
        case FIELD_TYPE_RESERVED:
        {
            INC_AML(1);
            UINT IncBytes = 0;
            UINT PkgLength = AmlReadPkgLength(Aml, &IncBytes);
            INC_AML(PkgLength + IncBytes);
            _RT_SystemDebugPrint(L"Reserved Field (PKG_LEN : %x)", PkgLength);
            break;
        }
        case FIELD_TYPE_ACCESS:
        {
            INC_AML(1);
            UINT8 AccessType = *Aml;
            UINT8 AccessAttributes = *(Aml + 1);
            INC_AML(2);
            _RT_SystemDebugPrint(L"ACCESS_FIELD : (Access Type : %x , Attrib : %x)", AccessType, AccessAttributes);
            break;
        }
        case FIELD_TYPE_ACCESS_EXTENDED:
        {
            INC_AML(1);
            UINT8 AccessType = *Aml;
            UINT8 AccessAttributesEx = *(Aml + 1);
            // TODO : Access Length (Undocumented)
            _RT_SystemDebugPrint(L"EXTENDED_ACCESS_FIELD : (Access Type : %x , Attrib : %x)", AccessType, AccessAttributesEx);
            while(1);

            break;
        }
        case FIELD_TYPE_CONNECT:
        {
            INC_AML(1);
            _RT_SystemDebugPrint(L"FIELD_TYPE_CONNECT");
            AmlCopyName(NameStr, Aml, &NmBytes);
            while(1);

            break;
        }
        default:
        {
            AmlCopyName(NameStr, Aml, &NmBytes);
            INC_AML(NmBytes);
            UINT IncBytes = 0;
            UINT PkgLength = AmlReadPkgLength(Aml, &IncBytes);
            INC_AML(IncBytes);
            INC_AML(PkgLength);
            _RT_SystemDebugPrint(L"FIELD_NAME : %s (Pkg Length : %x)", NameStr, PkgLength);
            break;
        }
    }
        
    return Aml;
}
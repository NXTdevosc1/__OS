// ACPI MACHINE LANGUAGE COMPILER
#pragma once
#include <krnltypes.h>
#include <acpi/acpi_defs.h>

#pragma pack(push, 1)

typedef struct _ACPI_DSDT{
    ACPI_SDT Sdt;
    char aml[]; // ACPI Machine Language
} ACPI_DSDT, *RFACPI_DSDT;

typedef struct _PKG_LEAD_BYTE {
    UINT8 PackageLength : 4; // Least significant bytes
    UINT8 PkgLenBelow63 : 2; // only used if pkglength < 63
    UINT8 FollowingByteDataCount : 2;
} PKG_LEAD_BYTE;

typedef struct _AML_FIELD_FLAGS {
    UINT8 AccessType : 4;
    UINT8 LockRule : 1;
    UINT8 UpdateRule : 2;
    UINT8 Reserved : 1;
} AML_FIELD_FLAGS;

#pragma pack(pop)

// AML OPCODES

#define AML_ALIAS_OP 0x6
#define AML_NAME_OP 0x8

/*
SCOPE Op:
- AML_SCOPE_OP
- PACKAGE_LENGTH
- NAME_STRING
- OBJECTS (TERM_LIST)
*/

#define AML_SCOPE_OP 0x10

#define AML_EXTENDED_OP 0x5B /*EXTENDED OP DEFINES ARE ENDED WITH _EXOP*/

#define AML_OPERATION_REGION_EXOP 0x80
#define AML_FIELD_EXOP 0x81

/*
Method Op :
- AML_METHOD_OP
- PACKAGE_LENGTH
- NAME_STRING
- METHOD_FLAGS
- TERM_LIST
*/
#define AML_METHOD_OP 0x14


// Prefixes
#define AML_DUALNAME_PREFIX '.'
#define AML_ROOT_PREFIX '\\'
#define AML_MULTINAME_PREFIX 0x2F

#define AML_BYTE_PREFIX 0xA
#define AML_WORD_PREFIX 0xB
#define AML_DWORD_PREFIX 0xC
#define AML_STRING_PREFIX 0xD
#define AML_QWORD_PREFIX 0xE

#define AML_ZERO_OP 0
#define AML_ONE_OP 1
#define AML_ONES_OP 0xFF

void AcpiReadDsdt(RFACPI_DSDT Dsdt);
KERNELSTATUS KERNELAPI AmlReadPackage(char* Aml, UINT PackageLength);

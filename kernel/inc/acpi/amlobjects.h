#pragma once
#include <acpi/aml.h>

#define AMLSTRUCT (void*)
#define DBGV (UINT64) // Cast Value to UINT64 For System Debug Print
#define INC_AML(val) Aml+=val; i+=val;

UINT KERNELAPI AmlCopyName(char* Destination, const char* Source, UINT64* NameStringBytes);
void KERNELAPI AmlReadExpression(char* Aml, void* Value, UINT* LenExpression);
UINT32 KERNELAPI AmlReadPkgLength(char* Aml, UINT* IncBytes);

char* KERNELAPI AmlParseFieldElement(char* Aml);

#define FIELD_TYPE_RESERVED 0
#define FIELD_TYPE_ACCESS 1
#define FIELD_TYPE_CONNECT 2
#define FIELD_TYPE_ACCESS_EXTENDED 3
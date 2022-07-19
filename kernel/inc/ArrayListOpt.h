// OPTIMIZED ARRAY LIST LIBRARY

//#include <krnltypes.h>
//
//#define ARRLIST_SIZE_SMALL 20
//#define ARRLIST_SIZE_MEDIUM 50
//#define ARRLIST_SIZE_BIG 80
//#define ARRLIST_SIZE_EXTREME 150
//#define ARRLIST_SIZE_FULL 300
//
//#define ARRLIST_ADDITIONNAL_INDEX_CLEAR -1
//
//#define ARRLIST_ELEMENT_HEADER_SIZE 1
//
//typedef struct _OPTIMIZED_ARRAY_LIST {
//	UINT64 ElementSize;
//	UINT64 ElementCount;
//	INT64 IndexMin;
//	INT64 IndexMax;
//	INT64 LastUnset;
//	INT64 AdditionnalIndex0;
//	INT64 AdditionnalIndex1;
//
//	void* ItemArray;
//	struct _OPTIMIZED_ARRAY_LIST* Next;
//} OPTARRLIST, *PARRLISTOPT;
//
//LPVOID CreateArrayListOpt(UINT64 ElementSize, UINT64 ArrayListSize);
//LPVOID AllocateArrayListEntryOpt(LPVOID _ArrayList);
//LPVOID GetItemByMutexOpt(UINT64 MutexOffset, int UseAdditionnalIndex);
//LPVOID GetItemByValueOpt(UINT64 ValOffset, UINT64 Size, void* Buffer, int UseAdditionnalIndex);
//LPVOID GetEntryByIndexOpt(UINT64 Index);
//
//LPVOID GetItemByMutexOptEx(UINT64 MutexOffset, LPVOID* List, INT64* IndexInList);
//LPVOID GetItemByValueOptEx(UINT64 ValOffset, void* Buffer, LPVOID* List, INT64* IndexInList);
//LPVOID GetEntryByIndexOptEx(UINT64 Index, LPVOID* List, INT64* IndexInList);
//
//LPVOID SetAdditionnalIndex0(LPVOID List, INT64 Index);
//LPVOID SetAdditionnalIndex1(LPVOID List, INT64 Index);
//

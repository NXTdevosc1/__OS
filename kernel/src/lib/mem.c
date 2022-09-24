#include <mem.h>
#include <CPU/cpu.h>


// All Addresses must be aligned (0x10) Boundary

void (__fastcall *_SIMD_Memset)(LPVOID ptr, UINT64 Value, UINT64 Count) = _SSE_Memset;
LPVOID (*memset)(LPVOID ptr, UINT8 value, size_t size) = _r8_SSE_Memset;
LPVOID __fastcall _Wrap_SSE_Memset(LPVOID ptr, UINT64 value, size_t size){
    if(size < 0x10) {
        __repstos(ptr, value, size);
        
        return NULL;
    }
    if(size & 0xF) {
        __repstos(ptr, value, size & 0xF);
        (char*)ptr += (size & 0xF);
        size-= 0x10 - ((UINT64)size & 0xF);
    }
    if((UINT64)ptr & 0xF) {
        _SSE_MemsetUnaligned(ptr, value, size);
    } else {
        _SSE_Memset(ptr, value, size);
    }
    return NULL;
}


LPVOID __fastcall _Wrap_AVX_Memset(LPVOID ptr, UINT64 value, size_t size) {
    if(size < 0x20) {
        __repstos(ptr, value, size);
        return NULL;
    }
    if(size & 0x1F) {
        __repstos(ptr, value, size & 0x1F);
        (char*)ptr += (size & 0x1F);
        size -= 0x20 - (size & 0x1F);
    }
    if((UINT64)ptr & 0x1F) {
        _AVX_MemsetUnaligned(ptr, value, size);
    } else {
        _AVX_Memset(ptr, value, size);
    }
    
    return NULL;
}


LPVOID __fastcall _r8_SSE_Memset(LPVOID ptr, uint8_t value, size_t size) {
    UINT64 CombinedValue = value;
    CombinedValue |= CombinedValue << 8 | CombinedValue << 16 | CombinedValue << 24
    | CombinedValue << 32 | CombinedValue << 40 | CombinedValue << 48 | CombinedValue << 56;
    return _Wrap_SSE_Memset(ptr, CombinedValue, size);
}
LPVOID __fastcall _r8_AVX_Memset(LPVOID ptr, uint8_t value, size_t size) {
    UINT64 CombinedValue = (UINT64)value;
    CombinedValue |= CombinedValue << 8 | CombinedValue << 16 | CombinedValue << 24
    | CombinedValue << 32 | CombinedValue << 40 | CombinedValue << 48 | CombinedValue << 56;
    return _Wrap_AVX_Memset(ptr, CombinedValue, size);
}

// LPVOID (__fastcall *memset)(LPVOID ptr, uint8_t value, size_t size) = _r8_SSE_Memset;

LPVOID (__fastcall *MemSetArray[3]) (LPVOID ptr, UINT64 value, size_t size) = {
    _Wrap_SSE_Memset, _Wrap_AVX_Memset, NULL
};

void memset16(LPVOID ptr, uint16_t value, size_t size){
    uint16_t* ptr_byte = (uint16_t*)(ptr);
    for(size_t i = 0;i<size;i++){
        ptr_byte[i] = value;
    }
}
void memset32(LPVOID ptr, uint32_t value, size_t size){
    UINT64 CombinedValue = (UINT64)value | ((UINT64)value << 32);
    MemSetArray[ExtensionLevel & 3](ptr, CombinedValue, size << 2);
}

void memset64(LPVOID ptr, uint64_t value, size_t size){
    uint64_t* ptr_byte = (uint64_t*)(ptr);
    for(size_t i = 0;i<size;i++){
        ptr_byte[i] = value;
    }
}  

LPVOID memcpy(LPVOID dest, LPCVOID src, size_t size){
    // while(1); // memcpy does not get called with maximum optimizations (even when intrinsics are disabled)
    if (!(size % 8)) {
        memcpy64(dest, src, size >> 3);
    }
    else if (!(size % 4)) {
        memcpy32(dest, src, size >> 2);
    }
    else if (!(size % 2)) {
        memcpy16(dest, src, size >> 1);
    }
    else {
        for (size_t i = 0; i < size; i++) {
            ((uint8_t*)dest)[i] = ((uint8_t*)src)[i];
        }
    }
    return NULL;
}

void memcpy16(LPVOID dest, LPCVOID src, size_t size) {
    UINT16* DestW = dest;
    const UINT16* SrcW = src;
    for (size_t i = 0; i < size; i++, DestW++, SrcW++) {
        *DestW = *SrcW;
    }
}

void memcpy32(LPVOID dest, LPCVOID src, size_t size) {
    UINT32* DestW = dest;
    const UINT32* SrcW = src;
    for (size_t i = 0; i < size; i++, DestW++, SrcW++) {
        *DestW = *SrcW;
    }
}

void memcpy64(LPVOID dest, LPCVOID src, size_t size) {
    UINT64* DestW = dest;
    const UINT64* SrcW = src;
    for (size_t i = 0; i < size; i++, DestW++, SrcW++) {
        *DestW = *SrcW;
    }
}


int memcmp(LPVOID x, LPVOID y, size_t size){
    for(size_t i = 0;i<size;i++){
        if(((uint8_t*)x)[i] != ((uint8_t*)y)[i]) return 0;
    }
    return 1;
}

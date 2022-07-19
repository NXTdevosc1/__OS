#include <mem.h>
#include <CPU/cpu.h>


void* memset(void* ptr, uint8_t value, size_t size){
    UINT64 CombinedValue = (UINT64)value;
    CombinedValue |= CombinedValue << 8 | CombinedValue << 16 | CombinedValue << 24
    | CombinedValue << 32 | CombinedValue << 40 | CombinedValue << 48 | CombinedValue << 56;
    if (!(size % 16)) {
        _Xmemset128(ptr, CombinedValue, size >> 4);
    }
    else if (!(size % 8)) {
        __repstos64(ptr, CombinedValue, size >> 3);
    }
    else if (!(size % 4)) {
        __repstos32(ptr, CombinedValue, size >> 2);
    }
    else if (!(size % 2)) {
        __repstos16(ptr, CombinedValue, size >> 1);
    }
    else {
        __repstos(ptr, value, size);
        // uint8_t* ptr_byte = (uint8_t*)(ptr);
        // for (size_t i = 0; i < size; i++) {

        //     ptr_byte[i] = value;
        // }
    }
    return NULL;
}
void memset16(void* ptr, uint16_t value, size_t size){
    uint16_t* ptr_byte = (uint16_t*)(ptr);
    for(size_t i = 0;i<size;i++){
        ptr_byte[i] = value;
    }
}
void memset32(void* ptr, uint32_t value, size_t size){
    size_t realsz = size << 2; // size * 4
    UINT64 V = value | ((UINT64)value << 32);
    if(!(realsz % 16)){
        _Xmemset128(ptr, V, realsz >> 4);
    }else if(!(realsz % 8)){
        __repstos64(ptr, V, realsz >> 3);
    }else{
        __repstos32(ptr, V, size);
    // uint32_t* ptr_byte = (uint32_t*)(ptr);
    // for(size_t i = 0;i<size;i++){
    //     ptr_byte[i] = value;
    // }
    }
}

void memset64(void* ptr, uint64_t value, size_t size){
    uint64_t* ptr_byte = (uint64_t*)(ptr);
    for(size_t i = 0;i<size;i++){
        ptr_byte[i] = value;
    }
}  

void* memcpy(void* dest, const void* src, size_t size){
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

void memcpy16(void* dest, const void* src, size_t size) {
    UINT16* DestW = dest;
    const UINT16* SrcW = src;
    for (size_t i = 0; i < size; i++, DestW++, SrcW++) {
        *DestW = *SrcW;
    }
}

void memcpy32(void* dest, const void* src, size_t size) {
    UINT32* DestW = dest;
    const UINT32* SrcW = src;
    for (size_t i = 0; i < size; i++, DestW++, SrcW++) {
        *DestW = *SrcW;
    }
}

void memcpy64(void* dest, const void* src, size_t size) {
    UINT64* DestW = dest;
    const UINT64* SrcW = src;
    for (size_t i = 0; i < size; i++, DestW++, SrcW++) {
        *DestW = *SrcW;
    }
}

int memcmp(void* x, void* y, size_t size){
    for(size_t i = 0;i<size;i++){
        if(((uint8_t*)x)[i] != ((uint8_t*)y)[i]) return 0;
    }
    return 1;
}
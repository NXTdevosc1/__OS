#define __DLLEXPORTS
#include <stdlib.h>
extern const int _fltused = 0;
_DECL void sprintf(char* buffer, const char* Format, ...){
    
}
_DECL void unumstr(uint64_t num, char* out){

    uint8_t size = 0;
    uint64_t sizeTest = num;
    while(sizeTest / 10 > 0)
    {
        sizeTest /=10;
        size++;
    }
    uint8_t index = 0;
    while(num / 10 > 0)
    {
        uint8_t remainder = num % 10;
        num /= 10;
        out[size - index] = remainder + '0';
        index++;
    }
    uint8_t remainder = num % 10;
    out[size - index] = remainder + '0';
    out[size + 1] = 0;
}

_DECL char* dtoa(double _Value, char* _Buffer, unsigned char DecimalSize) {
    itoa((long long)_Value, _Buffer, RADIX_DECIMAL);
    unsigned char numsz = 0;
    while (*_Buffer) {
        _Buffer++;
        numsz++;
    }
    // Perform a power operation
    double mul = 1;
    for (unsigned char i = 0; i < DecimalSize; i++) {
        mul = mul * 10;
    }
    if (mul > 1) {
        *_Buffer = '.';
        _Buffer++;
        char tmp[120] = {0};
        memset(tmp, 0, DecimalSize + numsz + 1);
        itoa((long long)(_Value * mul), tmp, RADIX_DECIMAL);
        char* t = tmp;
        t += numsz;
        unsigned char finish = 0;
        for (unsigned char i = 0; i < DecimalSize; i++) {
            if (!*t) finish = 1;
            if (finish) {
                *_Buffer = '0';
            }
            else {
                *_Buffer = *t;
            }
            _Buffer++;
            t++;
        }
        *_Buffer = 0;
        return _Buffer;
    }
    return NULL;
}
_DECL char* itoa(long long _Value, char* _Buffer, int _Radix){
    if (_Radix == RADIX_DECIMAL) { // Base 10 : Decimal
        uint8_t size = 0;
        uint64_t sizeTest = _Value;
        if (_Value < 0) {
            *_Buffer = '-';
            _Buffer++;
        }
        while (sizeTest / 10 > 0)
        {
            sizeTest /= 10;
            size++;
        }
        uint8_t index = 0;
        while (_Value / 10 > 0)
        {
            uint8_t remainder = _Value % 10;
            _Value /= 10;
            _Buffer[size - index] = remainder + '0';
            index++;
        }
        uint8_t remainder = _Value % 10;
        _Buffer[size - index] = remainder + '0';
        _Buffer[size + 1] = 0;
        return _Buffer;
    }
    else if (_Radix == RADIX_HEXADECIMAL) // Base 16 : HEX
    {
        uint8_t size = 0;
        unsigned long long ValTmp = _Value;
        do {
            size++;
        } while((ValTmp >>= 4));
        for (uint8_t i = 0; i < size; i++)
        {
            unsigned char c = _Value & 0xF;
            if(c < 0xA){
                _Buffer[size - (i + 1)] = '0' + c;
            }else{
                _Buffer[size - (i + 1)] = 'A' + (c - 0xA);
            }
            _Value >>= 4;
        }
        _Buffer[size] = 0;
        return _Buffer;
    }
    else if (_Radix == RADIX_BINARY) { // Base 1 : BINARY
        char* buff = _Buffer;
        *buff = '0'; // In case that value == 0
        if(!_Value){
            buff++;
            *buff = 0;
            return _Buffer;
        }
        unsigned char shift = 0;
        while (!(_Value & 0x8000000000000000) && _Value) {
            _Value <<= 1;
            shift++;
        }
        shift = 64 - shift;
        while (shift--) {
            if (_Value & 0x8000000000000000) {
                *buff = '1';
            }
            else *buff = '0';
            buff++;
            _Value <<= 1;
        }
        *buff = 0;
        return _Buffer;
    }

    else return 0;
}

extern void _Xmemset128U(void*, unsigned long long, unsigned long long);

_DECL void memset(void* ptr, unsigned char val, size_t size){
    unsigned long long V = val;

    V |= (V << 8) | (V << 16) | (V << 24) | (V << 32) | (V << 40) | (V << 48) | (V << 56);
    if((unsigned long long)ptr & 0xF) {
        _Xmemset128U(ptr, V, size >> 4);
        (char*)ptr+=(size & ~0xF);
        __memset(ptr, val, size & 0xF);
    } else {
        _Xmemset128(ptr, V, size >> 4);
        (char*)ptr+=(size & ~0xF);
        __memset(ptr, val, size & 0xF);
    }
}
_DECL void _memset16(void* ptr, unsigned short val, size_t size){
    unsigned short* p = ptr;
    for(size_t i = 0;i<size;i++, p++){
        *p = val;
    }
}
_DECL void _memset32(void* ptr, unsigned int val, size_t size){
    unsigned int* p = ptr;
    for(size_t i = 0;i<size;i++, p++){
        *p = val;
    }
}
_DECL void _memset64(void* ptr, unsigned long long val, size_t size){
    unsigned long long* p = ptr;
    for(size_t i = 0;i<size;i++, p++){
        *p = val;
    }
}

_DECL void memcpy(void* Destination, const void* Source, size_t size){
    if(!(size % 8)){
        _memcpy64(Destination, Source, size >> 3);
    }else if(!(size % 4)){
        _memcpy32(Destination, Source, size >> 2);
    }else if(!(size % 2)){
        _memcpy16(Destination, Source, size >> 1);
    }else{
        unsigned char* d = Destination;
        const unsigned char* s = Source;
        for(size_t i = 0;i<size;i++, d++, s++){
            *d = *s;
        }
    }
}
_DECL void _memcpy16(void* Destination, const void* Source, size_t size){
    unsigned short* d = Destination;
    const unsigned short* s = Source;
    for(size_t i = 0;i<size;i++, d++, s++){
        *d = *s;
    }
}
_DECL void _memcpy32(void* Destination, const void* Source, size_t size){
    unsigned int* d = Destination;
    const unsigned int* s = Source;
    for(size_t i = 0;i<size;i++, d++, s++){
        *d = *s;
    }
}
_DECL void _memcpy64(void* Destination, const void* Source, size_t size){
    unsigned long long* d = Destination;
    const unsigned long long* s = Source;
    for(size_t i = 0;i<size;i++, d++, s++){
        *d = *s;
    }
}


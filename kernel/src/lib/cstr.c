#include <cstr.h>
#include <math.h>
#include <preos_renderer.h> 
char uintTo_StringOutput[128] = { 0 };
const char* to_stringu64(uint64_t value){
    uint8_t size = 0;
    uint64_t sizeTest = value;
    while(sizeTest / 10 > 0)
    {
        sizeTest /=10;
        size++;
    }
    uint8_t index = 0;
    while(value / 10 > 0)
    {
        uint8_t remainder = value % 10;
        value /= 10;
        uintTo_StringOutput[size - index] = remainder + '0';
        index++;
    }
    uint8_t remainder = value % 10;
    uintTo_StringOutput[size - index] = remainder + '0';
    uintTo_StringOutput[size + 1] = 0;
    return uintTo_StringOutput;
}

uint64_t utoi(const wchar_t* _str, uint8_t len){ // unicode to int
    uint64_t ret = (_str[len-1] - L'0');
    ret = 0;
    uint16_t pw = len - 1;
    uint64_t tmp = 0;
    
    for(uint8_t i = 0;i<len;i++){
        ret += (_str[i] - L'0') * (powi(10,pw));
        pw--;
    }
    return ret;
}

char hexTo_StringOutput[128] = { 0 };

const char* to_hstring64(uint64_t value)
{
    uint64_t* valPtr = &value;
    uint8_t* ptr;
    uint8_t temp;
    uint8_t size = 8 * 2 - 1;
    for(uint8_t i = 0; i < size; i++)
    {
        ptr = ((uint8_t*)valPtr+i);
        temp = ((*ptr & 0xF0) >> 4);
        hexTo_StringOutput[size - (i*2+1)] = temp + (temp > 9 ? 55 : '0');
        temp = ((*ptr & 0x0F));
        hexTo_StringOutput[size - (i*2)] = temp + (temp > 9 ? 55 : '0');
    }
    hexTo_StringOutput[size+1] = 0;
    return hexTo_StringOutput;
}

char hexTo_StringOutput32[128] = { 0 };

const char* to_hstring32(uint32_t value)
{
    uint32_t* valPtr = &value;
    uint8_t* ptr;
    uint8_t temp;
    uint8_t size = 4 * 2 - 1;
    for(uint8_t i = 0; i < size; i++)
    {
        ptr = ((uint8_t*)valPtr+i);
        temp = ((*ptr & 0xF0) >> 4);
        hexTo_StringOutput32[size - (i*2+1)] = temp + (temp > 9 ? 55 : '0');
        temp = ((*ptr & 0x0F));
        hexTo_StringOutput32[size - (i*2)] = temp + (temp > 9 ? 55 : '0');
    }
    hexTo_StringOutput32[size+1] = 0;
    return hexTo_StringOutput32;
}

char hexTo_StringOutput16[128] = { 0 };

const char* to_hstring16(uint16_t value)
{
    uint16_t* valPtr = &value;
    uint8_t* ptr;
    uint8_t temp;
    uint8_t size = 2 * 2 - 1;
    for(uint8_t i = 0; i < size; i++)
    {
        ptr = ((uint8_t*)valPtr+i);
        temp = ((*ptr & 0xF0) >> 4);
        hexTo_StringOutput16[size - (i*2+1)] = temp + (temp > 9 ? 55 : '0');
        temp = ((*ptr & 0x0F));
        hexTo_StringOutput16[size - (i*2)] = temp + (temp > 9 ? 55 : '0');
    }
    hexTo_StringOutput16[size+1] = 0;
    return hexTo_StringOutput16;
}

char hexTo_StringOutput8[128] = { 0 };

const char* to_hstring8(uint8_t value)
{
    uint8_t* valPtr = &value;
    uint8_t* ptr;
    uint8_t temp;
    uint8_t size = 1;
    for(uint8_t i = 0; i < size; i++)
    {
        ptr = ((uint8_t*)valPtr+i);
        temp = ((*ptr & 0xF0) >> 4);
        hexTo_StringOutput8[size - (i*2+1)] = temp + (temp > 9 ? 55 : '0');
        temp = ((*ptr & 0x0F));
        hexTo_StringOutput8[size - (i*2)] = temp + (temp > 9 ? 55 : '0');
    }
    hexTo_StringOutput8[size+1] = 0;
    return hexTo_StringOutput8;
}


char intTo_StringOutput[32] = { 0 };

const char* to_string64(int64_t value){
    uint8_t isNegative = 0;
    if(value < 0)
    {
        isNegative = 1;
        value*=-1;
        intTo_StringOutput[0] = '-';
    }
    uint8_t size = 0;
    uint64_t sizeTest = value;
    while(sizeTest / 10 > 0)
    {
        sizeTest /=10;
        size++;
    }
    uint8_t index = 0;
    while(value / 10 > 0)
    {
        uint8_t remainder = value % 10;
        value /= 10;
        intTo_StringOutput[isNegative + size - index] = remainder + '0';
        index++;
    }
    uint8_t remainder = value % 10;
    intTo_StringOutput[isNegative + size - index] = remainder + '0';
    intTo_StringOutput[isNegative + size + 1] = 0;
    return intTo_StringOutput;
}



char doubleout[100] = { 0 };
const char* strdbl(double value, int radix)
{
    UINT64 dbl = 100;
    const char* buff = to_string64((UINT64)((double)value));
    for (UINT64 i = 0;; i++, buff++) {
        if (!*buff) {
            doubleout[i] = '.';
            i++;
            buff = to_string64((UINT64)((double)value * dbl));
            buff += i;
            for (UINT64 c = 0; c < radix; c++, i++) {
                doubleout[i] = buff[c];
            }
            doubleout[i] = '\0';
            break;
        }
        else {
            doubleout[i] = *buff;
        }
    }

    return doubleout;
}
#include <stdlib.h>
#include <MemoryManagement.h>
size_t strlen(const char* x){
	size_t length = 0;
	while(*x) {
        x++;
        length++;
    }
	return length;
}

uint16_t F_SW(uint16_t x){
	return (x >> 8) | (x << 8);
}

uint32_t F_SL(uint32_t x){
	return (x >> 24) | ((x<<8) & 0x00FF0000) | ((x>>8) & 0x0000FF00) | (x << 24);
}
uint16_t FI_SW(int16_t x){
	return (x >> 8) | (x << 8);
}

uint32_t FI_SL(int32_t x){
	return (x >> 24) | ((x<<8) & 0x00FF0000) | ((x>>8) & 0x0000FF00) | (x << 24);
}



unsigned int wstrlen(const wchar_t* x){
	unsigned int len = 0;
	while(x[len] != 0) len++;
	return len;
} 

unsigned char wstrcmp_nocs(const wchar_t* x, const wchar_t* y, size_t size) // wide char string compare no case sensitive [unicode format]
{
	for(size_t i = 0;i<size;i++){
		if(x[i] >= L'a' && x[i] <= L'z'){
			if(y[i] >= L'a' && y[i] <= L'z'){
				if((x[i]) != y[i]) return 0;
			}else{
				if((x[i]-32) != y[i]) return 0;
			}
		}else if(x[i] >= L'A' && x[i] <= L'Z'){
			if(y[i] >= L'A' && y[i] <= L'Z'){
				if((x[i]) != y[i]) return 0;
			}else{
				if((x[i]+32) != y[i]) return 0;
			}
		}else{
			if(x[i] != y[i]) return 0;
		}
	}
	return 1;
}

unsigned char wstrcmp(const wchar_t* x, const wchar_t* y, size_t size)
{
	for(size_t i = 0;i<size;i++){
		if(x[i] != y[i]) return 0;
	}
	return 1;
}

wchar_t* wstrcat(const wchar_t* x, const wchar_t* y){
	uint16_t sizex = wstrlen(x);
	uint16_t sizey = wstrlen(y);
	wchar_t* buffer = kmalloc((sizex+sizey)*2 + (2 /*end character code*/));
	wchar_t* ret = buffer;
	if(!ret) return NULL;
	unsigned int index = 0;
	for(uint16_t i = 0;i<sizex;i++){
		*buffer = x[i];
		buffer++;
	}
	for(uint16_t i = 0;i<sizey;i++){
		*buffer = y[i];
		buffer++;
	}
	*buffer = 0;
	return ret;
}

char* strcat(char* x, const char* y) {
	size_t sizex = strlen(x);
	size_t sizey = strlen(y);
	char* buffer = kmalloc(sizex + sizey + 1);

	char* ret = buffer;
	if (!ret) return NULL;
	for (size_t i = 0; i < sizex; i++) {
		*buffer = x[i];
		buffer++;
	}
	for (size_t i = 0; i < sizey; i++) {
		*buffer = y[i];
		buffer++;
	}
	*buffer = 0;
	return ret;
}

int strcmp(const char* x, const char* y) {
	while (*x && *y) {
		if (*x != *y) return 0;
		x++;
		y++;
	}
	return 1;
}

unsigned short* itoaw(long long _Value, unsigned short* _Buffer, int _Radix){
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
        unsigned short* buff = _Buffer;
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

char* itoa(long long _Value, char* _Buffer, int _Radix){
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
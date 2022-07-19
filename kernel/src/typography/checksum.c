#include <typography/checksum.h>

uint32_t ttf_record_calculate_checksum(uint32_t* table, uint32_t length){
    length*=4;
    uint32_t Sum = 0L;
    uint32_t *Endptr = table+((length+3) & ~3) / sizeof(uint32_t);
    while (table < Endptr)
    Sum += *table++;
    return Sum;
}
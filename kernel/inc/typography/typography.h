#pragma once
#include <stdint.h>
#include <CPU/process.h>


char LoadFont(
    void* font,
    unsigned int font_file_size
);
void DrawText(wchar_t* text, unsigned int font_size, wchar_t* font_family);
uint32_t FP_calculate_table_checksum(uint32_t* table, uint32_t length);

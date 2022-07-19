#pragma once
#include <stdint.h>
#include <typography/defs.h>



struct FONT_TABLE{
	void* font_file;
    uint8_t font_type;
	uint16_t num_glyphs;
    struct TTF_NAME_TABLE* name_table;
	void* glyphs_ptr_array;
};

void UnloadFont(struct FONT_TABLE* font_table);
#pragma once
#include <typography/defs.h>

struct OPENTYPE_GPOS_1_0{
	uint16_t script_list_offset;
	uint16_t feature_list_offset;
	uint16_t lookup_list_offset;
};
struct OPENTYPE_GPOS_1_1{
	uint16_t script_list_offset;
	uint16_t feature_list_offset;
	uint16_t lookup_list_offset;
	uint32_t feature_variations_offset; // may be NULL
};

char ttf_gpos_parse(void* gpos, struct FONT_TABLE* font_table, struct TTF_TABLE_RECORD* record);
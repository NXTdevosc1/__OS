#pragma once
#include <typography/defs.h>

struct OPENTYPE_POST_HDR{
	uint32_t version;
	int32_t italic_angle;
	int16_t underline_position;
	int16_t underline_thickness;
	uint32_t is_fixed_pitch;
	uint32_t min_mem_type42;
	uint32_t max_mem_type42;
	uint32_t min_mem_type1;
	uint32_t max_mem_type1;
};



struct OPENTYPE_POST_2_0{
	uint16_t glyph_count;
	uint16_t* glyph_name_index; // [glyph count]
	uint8_t* string_data; // last value should be 0
};

struct OPENTYPE_POST_2_5{
	uint16_t glyph_count;
	int8_t* offsets; // [num_glyphs]
};
char ttf_post_parse(void* post, struct FONT_TABLE* font_table, struct TTF_TABLE_RECORD* record);
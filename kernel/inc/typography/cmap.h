#pragma once
#include <typography/defs.h>

enum CMAP_PLATFORMS{
	CMAP_PLATFORM_UNICODE = 0,
	CMAP_PLATFORM_UNICODE_MACINTOSH = 1,
	CMAP_PLATFORM_UNICODE_MICROSOFT = 3
};

struct TTF_CMAP_FORMAT0{
	uint16_t format;
	uint16_t len;
	uint16_t lang;
	uint8_t glyph_index_array[256];
};

struct TTF_CMAP_SUBHEADER{
	uint16_t first_code;
	uint16_t entry_count;
	int16_t id_delta;
	uint16_t id_range_off;
};

struct TTF_CMAP_FORMAT2{
	uint16_t format;
	uint16_t len, lang;
	uint16_t subheader_keys[256];
	struct TTF_CMAP_SUBHEADER* subheaders; // variable
	uint16_t* glyph_index_array; // variable
};

struct TTF_CMAP_FORMAT4{
	uint16_t format, len, lang;
	uint16_t seg_countx2, search_range;
	uint16_t entry_selector, range_shift;
	uint16_t* end_code; // [seg count]
	uint16_t reserved_pad;
	uint16_t* start_code; // [seg count]
	uint16_t* id_delta; // [seg count]
	uint16_t* id_range_off; // [seg count]
	uint16_t* glyph_index_array; // variable

};

struct TTF_CMAP_ENCODING_SUBTABLE
{
	uint16_t platform_id;
	uint16_t platform_specific_id;
	uint32_t offset;
};

struct TTF_CMAP_HDR{
	uint16_t version; // set to 0
	uint16_t num_subtables;
	struct TTF_CMAP_ENCODING_SUBTABLE tables[];
};


char ttf_cmap_parse(void* cmap, struct FONT_TABLE* font_table, struct TTF_TABLE_RECORD* record);
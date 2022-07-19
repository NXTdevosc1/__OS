#pragma once
#include <typography/defs.h>


struct OPENTYPE_LANG_TAG_RECORD{
	uint16_t length;
	uint16_t lang_tag_offset;
};

struct TTF_NAME_RECORD{
	uint16_t platform_id;
	uint16_t platform_specific_id;
	uint16_t language_id;
	uint16_t name_id;
	uint16_t length;
	uint16_t offset;
};

struct TTF_NAME_TABLE{
	uint16_t format;
	uint16_t count;
	uint16_t string_offset;
	struct TTF_NAME_RECORD name_records[]; // [count]
};

// uint16 version 1
struct OPENTYPE_NAME_TABLE_V1{
	uint16_t lang_tag_count;
	struct OPENTYPE_LANG_TAG_RECORD lang_tag_records[]; // [lang_tag_count]
};

char ttf_name_parse(void* name, struct FONT_TABLE* font_table, struct TTF_TABLE_RECORD* record);
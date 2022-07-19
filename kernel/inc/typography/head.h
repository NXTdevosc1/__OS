#pragma once
#include <typography/defs.h>

struct OPENTYPE_HEAD_MAC_STYLE{
	uint16_t reserved : 9; // set to 0
	uint16_t extended : 1;
	uint16_t condensed : 1;
	uint16_t shadow : 1;
	uint16_t outline : 1;
	uint16_t underline : 1;
	uint16_t italic : 1;
	uint16_t bold : 1;
};

struct OPENTYPE_HEAD_FLAGS{
	uint16_t reserved1 : 1;
	uint16_t last_resort_font : 1;
	uint16_t optimized_for_clear_type : 1;
	uint16_t font_converted : 1;
	uint16_t lossless_font_data : 1;
	uint16_t set_to_0 : 5;
	uint16_t reserved0 : 1;
	uint16_t force_ppem_to_integer : 1;
	uint16_t instructions_may_alter_advance_width : 1;
	uint16_t instructions_may_depend_on_point_size : 1;
	uint16_t left_sidebearing_point : 1;
	uint16_t base_line : 1;
};

struct OPENTYPE_HEAD{
	uint16_t major_version;
	uint16_t minor_version;
	int32_t font_revision;
	uint32_t checksum_adjustment;
	uint32_t magic_number;
	struct OPENTYPE_HEAD_FLAGS flags;
	uint16_t unites_per_em; // from 16 to 16384 for opentype / power of 2 for truetype
	uint64_t created; // Number of seconds since 12:00 midnight that started January 1st 1904 in GMT/UTC time zone.
	uint64_t modified; // Number of seconds since 12:00 midnight that started January 1st 1904 in GMT/UTC time zone.
	int16_t xmin;
	int16_t ymin;
	int16_t xmax;
	int16_t ymax;
	struct OPENTYPE_HEAD_MAC_STYLE mac_style;
	uint16_t lowest_rect_pixels_per_em; // smallest readable size in pixels
	int16_t font_direction_hint; // Deprecated (Set to 2).0: Fully mixed directional glyphs;1: Only strongly left to right;2: Like 1 but also contains neutrals;-1: Only strongly right to left;-2: Like -1 but also contains neutrals.(A neutral character has no inherent directionality; it is not a character with zero (0) width. Spaces and punctuation are examples of neutral characters. Non-neutral characters are those with inherent directionality. For example, Roman letters (left-to-right) and Arabic letters (right-to-left) have directionality. In a “normal” Roman font where spaces and punctuation are present, the font direction hints should be set to two (2).)
	int16_t index_to_loc_format; // 0 for short offsets, 1 for long offsets
	int16_t glyph_data_format; // 0 for current format
};

char ttf_head_parse(void* head, struct FONT_TABLE* font_table, struct TTF_TABLE_RECORD* record);
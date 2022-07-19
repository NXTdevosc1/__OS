#pragma once
#include <typography/defs.h>


struct OPENTYPE_LONG_HOR_METRIC{
	uint16_t advance_width;
	int16_t left_side_bearing;
};

struct OPENTYPE_HMTX{
	struct OPENTYPE_LONG_HOR_METRIC* h_metric; // number of hmetrics in hhea
	int16_t* left_side_bearings; // [num_glyphs in maxp - num hmetrics]
};

char ttf_hmtx_parse(void* hmtx, struct FONT_TABLE* font_table, struct TTF_TABLE_RECORD* record);
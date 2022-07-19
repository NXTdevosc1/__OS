#pragma once
#include <typography/defs.h>


struct OPENTYPE_HHEA{
	uint16_t major_version;
	uint16_t minor_version;
	int16_t ascender;
	int16_t descender;
	int16_t line_gap;
	uint16_t advance_width_max;
	int16_t min_left_side_bearing;
	int16_t min_right_side_bearing;
	int16_t xmax_extent;
	int16_t caret_slope_rise;
	int16_t caret_slope_run;
	int16_t caret_offset;
	uint64_t reserved;
	int16_t metric_data_format;
	uint16_t number_of_HMetrics;
};

char ttf_hhea_parse(void* hhea, struct FONT_TABLE* font_table, struct TTF_TABLE_RECORD* record);
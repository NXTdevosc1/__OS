#pragma once
#include <typography/defs.h>
#pragma pack(push, 1)
struct TTF_SIMPLE_GLYPH_OUTLINE_FLAGS{ // bits are right to left
	uint8_t on_curve : 1;
	uint8_t x_short_vector : 1;
	uint8_t y_short_vector : 1;
	uint8_t repeat : 1;
	uint8_t this_x_is_same : 1;
	uint8_t this_y_is_same : 1;
	uint8_t reserved : 2;
};

enum TTF_SIMPLE_GLYPH_FLAGS{
	TTF_SFLAG_ON_CURVE = 1,
	TTF_SFLAG_XSHORT = 2,
	TTF_SFLAG_YSHORT = 4,
	TTF_SFLAG_REPEAT = 8,
	TTF_SFLAG_X_SAME = 16,
	TTF_SFLAG_Y_SAME = 32,
	TTF_SFLAG_OVERLAP_SIMPLE = 64,
	TTF_SFLAG_RESERVED = 128
};

struct TTF_SIMPLE_GLYPH_TABLE_COMPONENT{
	uint16_t* end_pts_of_contours; // [number_of_contours]
	uint16_t instruction_length;
	uint8_t* instructions; // [instruction_length]
	void* x_cordinates; // maybe uint16_t
	void* y_cordinates;
};

struct TTF_GLYF{
	int16_t number_of_contours;
	int16_t xmin;
	int16_t ymin;
	int16_t xmax;
	int16_t ymax;
};

enum OPENTYPE_GLYPH_CLASSDEF_TABLE{
	Opentype_gct_base_glyph = 1,
	Opentype_gct_ligature_glyph = 2,
	Opentype_gct_mark_glyph = 3,
	Opentype_gct_component_glyph = 4
};





enum OPENTYPE_SIMPLE_GLYPH_FLAGS{
	SGF_ON_CURVE_POINT = 1,
	SGF_X_SHORT_VECTOR = 2,
	SGF_Y_SHORT_VECTOR = 4,
	SGF_REPEAT_FLAG = 8,
	SGF_X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR = 0x10,
	SGF_Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR = 0x20,
	SGF_OVERLAP_SIMPLE = 0x40,
	SGF_RESERVED = 0x80
};

enum OPENTYPE_COMPOSITE_GLYPH_FLAGS{
	CGF_ARGS_ARE_WORDS = 1,
	CGF_ARGS_XY = 2,
	CGF_ROUND_XY_TO_GRID = 4,
	CGF_SCALE = 8,
	CGF_MORE_COMPONENTS = 0x20,
	CGF_XY_SCALE = 0x40,
	CGF_TWO_BY_TWO = 0x80,
	CGF_INSTRUCTIONS = 0x100,
	CGF_USE_MY_METRICS = 0x200,
	CGF_OVERLAP_COMPOUND = 0x400,
	CGF_SCALED_COMPONENT_OFFSET = 0x800,
	CGF_UNSCALED_COMPONENT_OFFSET = 0x1000
};

#pragma pack(pop)


char ttf_glyf_parse(void* glyf, struct FONT_TABLE* font_table, struct TTF_TABLE_RECORD* record);
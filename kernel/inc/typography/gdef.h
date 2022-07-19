#pragma once
#include <typography/defs.h>

struct OPENTYPE_GDEF_SELECTOR_HDR{
	uint16_t major_version; // 1
	uint16_t minor_version; // 0 (struct GDEF_V1_0) , 2 (struct GDEF_V1_2) , 3 (struct GDEF_V1_3)
};
struct OPENTYPE_GDEF_V1_0{
	uint16_t glyph_class_def_offset;
	uint16_t attach_list_offset;
	uint16_t lig_caret_list_offset;
	uint16_t mark_attach_class_def_offset;
};


struct OPENTYPE_GDEF_V1_2{
	uint16_t glyph_class_def_offset;
	uint16_t attach_list_offset;
	uint16_t lig_caret_list_offset;
	uint16_t mark_attach_class_def_offset;
	uint16_t mark_glyph_sets_def_offset;
};

struct OPENTYPE_GDEF_V1_3{
	uint16_t glyph_class_def_offset;
	uint16_t attach_list_offset;
	uint16_t lig_caret_list_offset;
	uint16_t mark_attach_class_def_offset;
	uint16_t mark_glyph_sets_def_offset;
	uint32_t item_var_store_offset;
};

char ttf_gdef_parse(void* gdef, struct FONT_TABLE* font_table, struct TTF_TABLE_RECORD* record);
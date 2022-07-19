#pragma once
#include <stdint.h>
#include <typography/driver_tables.h>
#define SFNT_VERSION_OPENTYPE "OTTO"
#define SFNT_VERSION_TRUETYPE 0x00010000
static const char* OPENTYPE_REQUIRED_TABLES[] = {
	"cmap",
	"head",
	"hhea",
	"hmtx",
	"maxp",
	"name",
	"OS/2",
	"post"
};
#pragma pack(push, 1)
struct GLYPH_CORDINATE_ENTRY{
	uint8_t start_contour;
	uint8_t on_curve;
	int16_t x;
	int16_t y;
};
struct GLYPH_DESCRIPTOR{
	int16_t xmin, ymin, xmax, ymax;
	uint16_t advance_width;
	uint16_t num_cordinates;
	uint8_t* flags;
	struct GLYPH_CORDINATE_ENTRY coordinates[];
};
#pragma pack(pop)
static const char* OPENTYPE_TABLES[] = {
	"cmap",
	"head",
	"hhea",
	"hmtx",
	"maxp",
	"name",
	"OS/2",
	"post",
	"cvt",
	"fpgm",
	"glyf",
	"loca",
	"prep",
	"gasp",
	"BASE",
	"GDEF",
	"GPOS",
	"GSUB",
	"JSTF",
	"MATH",
	"avar",
	"cvar",
	"fvar",
	"gvar",
	"HVAR",
	"MVAR",
	"STAT",
	"VVAR",
	"DSIG",
	"hdmx",
	"kern",
	"LTSH",
	"MERG",
	"meta",
	"STAT",
	"PCLT",
	"VDMX",
	"vhea",
	"vmtx"
};


struct TTF_TABLE_RECORD
{
	uint8_t table_tag[4];
	uint32_t check_sum;
	uint32_t offset;
	uint32_t table_length;
};
struct TTF_TABLE_HDR{
	uint32_t sfnt_version;
	uint16_t num_tables;
	uint16_t search_range;
	uint16_t entry_selector;
	uint16_t range_shift;
	struct TTF_TABLE_RECORD records[];
};






enum OPENTYPE_PLATFORM_IDs{
	OPENTYPE_PID_UNICODE = 0,
	OPENTYPE_PID_MACINTOSH = 1,
	OPENTYPE_PID_ISO = 2,
	OPENTYPE_PID_WINDOWS = 3,
	OPENTYPE_PID_CUSTUM = 4
};

struct OPENTYPE_ATTACH_LIST_TABLE{
	uint16_t coverage_offset;
	uint16_t glyph_count;
	uint16_t* attach_point_offsets; // [glyph_count]
};
struct OPENTYPE_ATTACH_POINT_TABLE{
	uint16_t point_count;
	uint16_t* point_indices; // [pt_count]
};
struct OPENTYPE_LIGATURE_CARRET_TABLE{
	uint16_t coverage_offset;
	uint16_t lig_glyph_count;
	uint16_t* lig_glyph_offsets; // [lg count]
};
struct OPENTYPE_LIGATURE_GLYPH_TABLE{
	uint16_t carret_count; // components - 1
	uint16_t* caret_value_offsets; // [c count]
};

struct OPENTYPE_ENCODING_RECORD{
	uint16_t platform_id;
	uint16_t encoding_id;
	uint32_t subtable_offset; // Byte offset from beginning of table to the subtable for this encoding.
	
};

// uint16_t format;


// uint16 version 0



// u16 major_version, minor_version



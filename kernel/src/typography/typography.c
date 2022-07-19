#include <typography/defs.h>
#include <typography/typography.h>
#include <preos_renderer.h>
#include <stdint.h>
#include <cstr.h>
#include <stdlib.h>
#include <MemoryManagement.h>
#include <math.h>

#include <typography/include.h>
char LoadFont(
    void* font, unsigned int font_file_size
)
{
    struct TTF_TABLE_HDR* font_hdr = font;
    struct FONT_TABLE* font_table = kmalloc(sizeof(struct FONT_TABLE));
    if(!font_table) return 0;

    memset(font_table,0,sizeof(struct FONT_TABLE));
    font_hdr->sfnt_version = SWAPINT(font_hdr->sfnt_version);
    if(memcmp(&font_hdr->sfnt_version,SFNT_VERSION_OPENTYPE,4)){
        font_table->font_type = 0;
    }else if(font_hdr->sfnt_version == SFNT_VERSION_TRUETYPE){
        font_table->font_type = 1;
    }
    else return 0;


    font_table->font_file = font;
    font_hdr->num_tables = SWAPWORD(font_hdr->num_tables);
    
    struct TTF_TABLE_RECORD* glyf_tbl = NULL;

    for(uint16_t i = 0;i<font_hdr->num_tables;i++){

        // todo : checksum

        font_hdr->records[i].check_sum = SWAPINT(font_hdr->records[i].check_sum);
        font_hdr->records[i].offset = SWAPINT(font_hdr->records[i].offset);
        font_hdr->records[i].table_length = SWAPINT(font_hdr->records[i].table_length);
        // continue;
        //    for(uint8_t x = 0;x<4;x++){
        //   GP_sf_put_char(font_hdr->records[i].table_tag[x], 0xffffff,20+(x*8),120+(i*20));  
        //   }
        
        if(memcmp((void*)&font_hdr->records[i].table_tag, "cmap", 4))
        {
            if(!ttf_cmap_parse((void*)((uint64_t)font_hdr->records[i].offset+(UINT64)font), font_table, &font_hdr->records[i])){
                UnloadFont(font_table);
                return 0;
            }
        }else if(memcmp((void*)&font_hdr->records[i].table_tag, "maxp", 4)){
            if(!ttf_maxp_parse((void*)((uint64_t)font_hdr->records[i].offset+ (UINT64)font), font_table, &font_hdr->records[i])){
                UnloadFont(font_table);
                return 0;
            }
        }else if(memcmp((void*)&font_hdr->records[i].table_tag, "glyf", 4)){
            glyf_tbl = &font_hdr->records[i]; // parse glyph after all headers are loaded
        // }else if(memcmp((void*)&font_hdr->records[i].table_tag, "head", 4)){
        //     if(!ttf_head_parse((void*)((uint64_t)font_hdr->records[i].offset+font), font_table, &font_hdr->records[i])){
        //         UnloadFont(font_table);
        //         return 0;
        //     }

        // }else if(memcmp((void*)&font_hdr->records[i].table_tag, "hmtx", 4)){
        //     if(!ttf_hmtx_parse((void*)((uint64_t)font_hdr->records[i].offset+font), font_table, &font_hdr->records[i])){
        //         UnloadFont(font_table);
        //         return 0;
        //     }

        // }else if(memcmp((void*)&font_hdr->records[i].table_tag, "hhea", 4)){
        //     if(!ttf_hhea_parse((void*)((uint64_t)font_hdr->records[i].offset+font), font_table, &font_hdr->records[i])){
        //         UnloadFont(font_table);
        //         return 0;
        //     }

        }else if(memcmp((void*)&font_hdr->records[i].table_tag, "name", 4)){
            
            if(!ttf_name_parse((void*)((uint64_t)font_hdr->records[i].offset+ (UINT64)font), font_table, &font_hdr->records[i])){
                UnloadFont(font_table);
                return 0;
            }
        }

        // }else if(memcmp((void*)&font_hdr->records[i].table_tag, "post", 4)){

        //     if(!ttf_post_parse((void*)((uint64_t)font_hdr->records[i].offset+font), font_table, &font_hdr->records[i])){
        //         UnloadFont(font_table);
        //         return 0;
        //     }

        // }else if(memcmp((void*)&font_hdr->records[i].table_tag, "GDEF", 4)){
        //     if(!ttf_gdef_parse((void*)((uint64_t)font_hdr->records[i].offset+font), font_table, &font_hdr->records[i])){
        //         UnloadFont(font_table);
        //         return 0;
        //     }

        // }else if(memcmp((void*)&font_hdr->records[i].table_tag, "GPOS", 4)){
        //     if(!ttf_gpos_parse((void*)((uint64_t)font_hdr->records[i].offset+font), font_table, &font_hdr->records[i])){
        //         UnloadFont(font_table);
        //         return 0;
        //     }

        // }else if(memcmp((void*)&font_hdr->records[i].table_tag, "GSUB", 4)){
        //     if(!ttf_gsub_parse((void*)((uint64_t)font_hdr->records[i].offset+font), font_table, &font_hdr->records[i])){
        //         UnloadFont(font_table);
        //         return 0;
        //     }
        // }
        
    }
  
    if(!glyf_tbl){
        UnloadFont(font_table);
        return 0;
    }
    

    if(!ttf_glyf_parse((void*)((uint64_t)glyf_tbl->offset+ (UINT64)font), font_table, glyf_tbl)){
        UnloadFont(font_table);
        return 0;
    }
    return 1;


}
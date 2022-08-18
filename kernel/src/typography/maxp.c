#include <typography/maxp.h>
#include <stdlib.h>
#include <MemoryManagement.h>
#include <interrupt_manager/SOD.h>
#include <cstr.h>
#include <preos_renderer.h>
char ttf_maxp_parse(void* maxp, struct FONT_TABLE* font_table, struct TTF_TABLE_RECORD* record){
    uint32_t version = SWAPINT(*(uint32_t*)maxp);
    if(version != 0x5000 /*maxp 0.5*/ && version != 0x10000 /*maxp 1.0*/) return 0;

    font_table->num_glyphs = SWAPWORD(*(uint16_t*)((char*)maxp + 4));

    font_table->glyphs_ptr_array = AllocatePool(font_table->num_glyphs * 8); // ptr list
    if(!font_table->glyphs_ptr_array) SET_SOD_OUT_OF_RESOURCES;
    memset64(font_table->glyphs_ptr_array,0,font_table->num_glyphs); // set all pointers to 0


    return 1;
}
#include <typography/name.h>
#include <MemoryManagement.h>
#include <stdlib.h>
#include <interrupt_manager/SOD.h>
char ttf_name_parse(void* name, struct FONT_TABLE* font_table, struct TTF_TABLE_RECORD* record){
    struct TTF_NAME_TABLE* hdr = name;
    hdr->format = SWAPWORD(hdr->format);
    if(hdr->format != 1 && hdr->format != 0) return 0;
    
    font_table->name_table = AllocatePool(record->table_length);
    
    // memcpy 8 byte alignment
    memcpy64(font_table->name_table,(void*)((uint64_t)font_table->font_file + record->offset),record->table_length/8);
    font_table->name_table->count = SWAPWORD(hdr->count);
    font_table->name_table->format = hdr->format;
    font_table->name_table->string_offset = SWAPWORD(hdr->string_offset);
    
    for(uint16_t i = 0;i<font_table->name_table->count;i++){
        font_table->name_table->name_records[i].platform_id = SWAPWORD(hdr->name_records[i].platform_id);
        font_table->name_table->name_records[i].platform_specific_id = SWAPWORD(hdr->name_records[i].platform_specific_id);
        font_table->name_table->name_records[i].language_id = SWAPWORD(hdr->name_records[i].language_id);
        font_table->name_table->name_records[i].name_id = SWAPWORD(hdr->name_records[i].name_id);
        font_table->name_table->name_records[i].length = SWAPWORD(hdr->name_records[i].length);
        font_table->name_table->name_records[i].offset = SWAPWORD(hdr->name_records[i].offset);
    }
    return 1;

}
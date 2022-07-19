#include <typography/glyf.h>
#include <cstr.h>
#include <preos_renderer.h>
#include <interrupt_manager/SOD.h>
#include <stdlib.h>
#include <MemoryManagement.h>
#include <kernel.h>
char DrawGlyph(struct FONT_TABLE* font_table, uint16_t glyph_index, uint32_t x_off, uint32_t y_off);
uint16_t x_glyf = 0, y_glyph = 0;
char ttf_glyf_parse(void* glyf, struct FONT_TABLE* font_table, struct TTF_TABLE_RECORD* record){
    GP_clear_screen(0);
    struct TTF_GLYF* glyph_table = glyf;
    struct TTF_SIMPLE_GLYPH_TABLE_COMPONENT* tbl = kmalloc(sizeof(struct TTF_SIMPLE_GLYPH_TABLE_COMPONENT));
    if(!tbl) SET_SOD_OUT_OF_RESOURCES;
    for(uint16_t _glyph_index = 0;_glyph_index<font_table->num_glyphs;_glyph_index++){
    glyph_table->xmin = SWAPWORD(glyph_table->xmin);
    glyph_table->ymin = SWAPWORD(glyph_table->ymin);
    glyph_table->xmax = SWAPWORD(glyph_table->xmax);
    glyph_table->ymax = SWAPWORD(glyph_table->ymax);
    glyph_table->number_of_contours = SWAPWORD(glyph_table->number_of_contours);
    char* glyph_data = (char*)((uint64_t)glyph_table + sizeof(struct TTF_GLYF));
    if(glyph_table->number_of_contours < 0){ // compound glyph
        GP_draw_sf_text("COMPOUND GLYPH LAST INDEX : ",0xffff0000,20,20);
        GP_draw_sf_text(to_stringu64(_glyph_index),0xffff0000,20+(8*29),20);
        GP_draw_sf_text(to_string64(glyph_table->xmin),0xffff0000,20,80);
        GP_draw_sf_text(to_string64(glyph_table->xmax),0xffff0000,20,100);
        GP_draw_sf_text(to_string64(glyph_table->ymin),0xffff0000,20,120);
        GP_draw_sf_text(to_string64(glyph_table->ymax),0xffff0000,20,140);
        GP_draw_sf_text(to_string64(glyph_table->number_of_contours),0xffff0000,20,160);

        
        char* ptr = glyph_data;
        for(uint8_t v = 0;v<10;v++){
            for(uint8_t k = 0;k<20;k++){
                GP_draw_sf_text(to_hstring8(((uint8_t*)ptr)[k + (v*20)]), 0xffffff,500+(k*20),20 + (v*20));
            }
        }
        uint16_t flags = SWAPWORD(*(uint16_t*)ptr);
        for(;;){
            ptr+=4; // flags + glyph index;
            if(flags & CGF_ARGS_ARE_WORDS){
                ptr+=4;
            }else ptr+=2;

            if(flags & CGF_INSTRUCTIONS){

                GP_draw_sf_text("instructions...",0xff,400,200);
                while(1);
            }

            if((flags & CGF_MORE_COMPONENTS)) {
                flags = SWAPWORD(*(uint16_t*)ptr);
            }else break;
            
        }
        
        glyph_table = (struct TTF_GLYF*)ptr;
    }else{
        
        
        
        tbl->end_pts_of_contours = (UINT16*)glyph_data;
        tbl->instruction_length = SWAPWORD(*(uint16_t*)(glyph_data + (glyph_table->number_of_contours * 2)));
        tbl->instructions = (uint8_t*)(glyph_data + (glyph_table->number_of_contours * 2) + 2);
        uint8_t* flags = (uint8_t*)(((uint64_t)tbl->instructions + tbl->instruction_length));
        struct GLYPH_DESCRIPTOR* gcd = kmalloc(sizeof(struct GLYPH_DESCRIPTOR) + ((SWAPWORD(tbl->end_pts_of_contours[glyph_table->number_of_contours - 1]) + 1)*sizeof(struct GLYPH_CORDINATE_ENTRY)));
        if(!gcd) SET_SOD_OUT_OF_RESOURCES;
        *(struct GLYPH_DESCRIPTOR**)((char*)font_table->glyphs_ptr_array + (_glyph_index * 8)) = gcd;
        ZeroMemory(gcd, sizeof (struct GLYPH_DESCRIPTOR) + ((SWAPWORD(tbl->end_pts_of_contours[glyph_table->number_of_contours - 1]) + 1) * sizeof(struct GLYPH_CORDINATE_ENTRY)));
        gcd->num_cordinates = (SWAPWORD(tbl->end_pts_of_contours[glyph_table->number_of_contours - 1]) + 1);
        gcd->xmin = glyph_table->xmin;
        gcd->xmax = glyph_table->xmax;
        gcd->ymin = glyph_table->ymin;
        gcd->ymax = glyph_table->ymax;
        if(glyph_table->number_of_contours == 0){
            // GP_clear_screen(0xffffffffffffffff);
            glyph_table = (void*)flags;

            // void* ptr = glyph_table;
            // if(SWAPWORD(*(uint16_t*)glyph_table) > 0){
            //     ptr+=SWAPWORD(*(uint16_t*)glyph_table);
            // }
            // GP_draw_sf_text(to_hstring16(SWAPWORD(*(uint16_t*)ptr)),0xff,20,200+(_glyph_index*20));
            // glyph_table = ptr;
            // // glyph_table = ((uint64_t)glyph_table + 2);
            // while(1);

        while(1);

            continue;
        }
        gcd->flags = kmalloc(gcd->num_cordinates);
        if(!gcd->flags) SET_SOD_OUT_OF_RESOURCES;

        char* ptr = flags;
        uint16_t gcd_flag_index = 0;
        register uint16_t __flag_ptr_index = 0;
        uint8_t inc_ptr = 0;
        while(gcd_flag_index<gcd->num_cordinates){
            if(flags[__flag_ptr_index] & 8 /*repeat*/){
                inc_ptr = 1;
                gcd->flags[gcd_flag_index] = flags[__flag_ptr_index];
                gcd_flag_index++;
                ptr+=2; // 1 for this flag & 1 for next byte which is count of repeat
                for(uint8_t c = 0;c<flags[__flag_ptr_index+1];c++){
                    // if(gcd_flag_index == gcd->num_cordinates) break;
                    gcd->flags[gcd_flag_index] = flags[__flag_ptr_index];
                    gcd_flag_index++;
                }
                __flag_ptr_index+=2;
            }else{
                gcd->flags[gcd_flag_index] = flags[__flag_ptr_index];
                gcd_flag_index++;
                ptr++;
                __flag_ptr_index++;
            }
            
        }

        
        
        int16_t x_cord = 0;
        uint16_t contour_index = 0;
        
        for(uint16_t i = 0;i<gcd->num_cordinates;i++){
            
            // x cordinates
            if(gcd->flags[i] & TTF_SFLAG_XSHORT){
                
                if(gcd->flags[i] & TTF_SFLAG_X_SAME){
                    x_cord += *(uint8_t*)ptr;
                }else{
                    x_cord -= (*(uint8_t*)ptr);
                }
                ptr++;
            }else{
                if(!(gcd->flags[i] & TTF_SFLAG_X_SAME)){
                    
                    x_cord += (int16_t)SWAPWORD(*(uint16_t*)ptr);
                    ptr+=2;
                }
            }
            
            gcd->coordinates[i].on_curve = gcd->flags[i] & TTF_SFLAG_ON_CURVE;
            gcd->coordinates[i].x = x_cord;
            if(SWAPWORD(tbl->end_pts_of_contours[contour_index]) + 1 == i){
                gcd->coordinates[i].start_contour = 1;
                contour_index++;
            }


        }


        gcd->coordinates[0].start_contour = 1;
        int16_t y_cord = 0;
        for(uint16_t i = 0;i<gcd->num_cordinates;i++){
            
            // y cordinates
            if(gcd->flags[i] & TTF_SFLAG_YSHORT){
                
                if(gcd->flags[i] & TTF_SFLAG_Y_SAME){
                    y_cord += *(uint8_t*)ptr;
                }else{
                    y_cord -= (*(uint8_t*)ptr);
                }
                ptr++;

            }else{
                if(!(gcd->flags[i] & TTF_SFLAG_Y_SAME)){
                    
                    y_cord += (int16_t)SWAPWORD(*(uint16_t*)ptr);
                    ptr+=2;
                }

            }
            
            gcd->coordinates[i].y = y_cord;

        }
        

            if(_glyph_index < 5){
            
            DrawGlyph(font_table, _glyph_index, 20 + (x_glyf * 120), 220 + (y_glyph * 120));
            
            }
        x_glyf++;
        if(x_glyf >= 15){
            x_glyf = 0;
            y_glyph++;
        }
        if(((uint64_t)ptr - (uint64_t)font_table) % 2) ptr++; // entries must be word aligned

        glyph_table = (struct TTF_GLYF*)ptr;

    }
    }
    free(tbl,kproc);
    
    while(1);
    
    return 1;
    
}

char DrawGlyph(struct FONT_TABLE* font_table, uint16_t glyph_index, uint32_t x_off, uint32_t y_off){
        
        struct GLYPH_DESCRIPTOR* gcd = *(struct GLYPH_DESCRIPTOR**)((char*)font_table->glyphs_ptr_array + (glyph_index * 8));
        if(!gcd) return 0;
        int16_t x_cord = 0;
        int16_t y_cord = 0;
        int16_t last_y = 0;
        int16_t last_x = 0;
        int16_t last_vy = 0;
        int16_t* cords_descx = kmalloc(gcd->num_cordinates * 2);
        int16_t* cords_descy = kmalloc(gcd->num_cordinates * 2);
        uint32_t* buffer = kmalloc(120*120*4);
        if(!buffer || !cords_descx || !cords_descy) SET_SOD_OUT_OF_RESOURCES;
        memset32(buffer,0,120*120);
        
        uint16_t size_divx = 18;
        uint16_t size_divy = 18;
        int16_t lst_start_contourx = gcd->coordinates[0].x;
        int16_t lst_start_contoury = gcd->coordinates[0].y;

        uint16_t pt_count = 1;
        for(uint16_t i = 0;i<gcd->num_cordinates;i++){
        if(gcd->coordinates[i].x > gcd->xmax || 
        gcd->coordinates[i].x < gcd->xmin ||
        gcd->coordinates[i].y > gcd->ymax ||
        gcd->coordinates[i].y < gcd->ymin
        ) return 0;
        }

        for(uint16_t i = 0;i<gcd->num_cordinates;i++){
            if(gcd->coordinates[i].start_contour){
                x_cord = gcd->coordinates[i].x;
                y_cord = gcd->coordinates[i].y;
                
                lst_start_contourx = x_cord;
                lst_start_contoury = y_cord;
            pt_count = 1;

            }else{
                
                if(!gcd->coordinates[i].on_curve){
                    cords_descx[pt_count] = gcd->coordinates[i].x/size_divx;
                    cords_descy[pt_count] = (gcd->ymax/size_divy) - gcd->coordinates[i].y / size_divy;
                    if(i == (gcd->num_cordinates - 1) || gcd->coordinates[i+1].start_contour){
                        goto check_final;
                    }else{
                    pt_count++;

                    }
                }
                
                cords_descx[0] = x_cord / size_divx;
                cords_descy[0] = (gcd->ymax/size_divy) - y_cord / size_divy;
                cords_descx[pt_count] = gcd->coordinates[i].x / size_divx;
                cords_descy[pt_count] = (gcd->ymax/size_divy) - gcd->coordinates[i].y / size_divy;
                pt_count++;
                for(float t = 0;t<1;t+=0.001){
                    int16_t x = GetBezierPoint(cords_descx,pt_count,t);
                    int16_t y = GetBezierPoint(cords_descy, pt_count, t);
                    last_x = x;
                    last_y = y;
                    *(uint32_t*)(buffer + x + (y*120) - (gcd->xmin/size_divx) - ((gcd->ymin/size_divy)*120)) = 0xffffffff;


                }
            
                x_cord = gcd->coordinates[i].x;
                y_cord = gcd->coordinates[i].y;
                pt_count = 1;
                    if(i == (gcd->num_cordinates - 1) || gcd->coordinates[i+1].start_contour){
                check_final:
                    cords_descx[0] = gcd->coordinates[i].x / size_divx;
                    cords_descy[0] = (gcd->ymax/size_divy) - gcd->coordinates[i].y / size_divy;
                    cords_descx[pt_count] = lst_start_contourx / size_divx;
                    cords_descy[pt_count] = (gcd->ymax/size_divy) - lst_start_contoury / size_divy;
                    pt_count++;
                    for(float t = 0;t<1;t+=0.001){
                        int16_t x = GetBezierPoint(cords_descx,pt_count,t);
                        int16_t y = GetBezierPoint(cords_descy, pt_count, t);
           
                        last_x = x;
                        last_y = y;
                        *(uint32_t*)(buffer + x + (y*120) - (gcd->xmin/size_divx) - ((gcd->ymin/size_divy)*120)) = 0xffffffff;
                    }
                }
            }
            
        }
        
        for(uint8_t x = 0;x<120;x++){
            uint8_t fill = 0;
            
            for(uint8_t y = 0;y<120;y++){
                if(buffer[x+(y*120)]){
                        
                    GP_set_pixel(x + x_off, y + y_off, 0xffffffff);
                        
                    
                }
                if(fill){
                }
                
            }
        }

    return 1;
}
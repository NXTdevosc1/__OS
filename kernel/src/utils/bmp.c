#include <utils/bmp.h>
#include <preos_renderer.h>
#include <cstr.h>
#include <kernel.h>
#include <krnltypes.h>
#include <Management/runtimesymbols.h>
uint8_t BmpImgDraw(void* _file, uint32_t x, uint32_t y){
    FRAME_BUFFER_DESCRIPTOR* fb = InitData.fb;
    if(!fb) return 0;
    struct BMP_HDR* hdr = _file;

    if(hdr->bits_per_pixels == 32){
        GP_draw_sf_text("BPP : 32", 0xFF, 20, 80);

        uint32_t* buffer = (uint32_t*)((uint64_t)hdr + hdr->img_offset);

        for(uint32_t y1 = 0;y1<hdr->height;y1++){
            for(uint32_t x1 = 0;x1<hdr->width;x1++){
                *(uint32_t*)(fb->FrameBufferBase + ((x1+x)*4) + ((((hdr->height - y1))+y)*fb->HorizontalResolution * 4)) = buffer[x1+(y1*hdr->width)];
            }
        }
    }else if(hdr->bits_per_pixels == 24){

        
	    GP_draw_sf_text("BPP : 24", 0xFF, 20, 80);
        
        unsigned char*  buffer = (unsigned char*)((UINT64)_file + hdr->img_offset);
        unsigned char* bf_row = buffer;
        uint64_t row_size = ((24 * hdr->width + 31)/32)*4;
        uint8_t r, g, b;
        for(uint32_t y1 = 0;y1<hdr->height;y1++){
                
            for(uint32_t x1 = 0;x1<hdr->width;x1++){
                r = buffer[2];
                g = buffer[1];    
                b = buffer[0];

                *(uint32_t*)(fb->FrameBufferBase + (((x1)+x)*4) + (((hdr->height - y1)+y)*fb->HorizontalResolution * 4)) = 0xff000000 | (r << 16) | (g << 8) | (b);
                buffer+=3;
            }
            bf_row+=row_size;
            buffer=bf_row;
        }

    
    }else return 0;
 
    return 1;
}
#include <preos_renderer.h>
#include <MemoryManagement.h>
#include <kernel.h>
#include <stdlib.h>


void GP_clear_screen(unsigned int background_color){
	memset32((void*)InitData.fb->FrameBufferBase, background_color, InitData.fb->HorizontalResolution * InitData.fb->VerticalResolution);
}
void GP_set_pixel(unsigned int x, unsigned int y, unsigned int color)
{
	((UINT32*)(InitData.fb->FrameBufferBase))[x + y * InitData.fb->HorizontalResolution] = color;
	
}
void GP_draw_rect(unsigned short x, unsigned short y, unsigned short width, unsigned short height,unsigned int background_color){
	unsigned short yheight = y;
	for(unsigned short i = 0;i<height;i++, yheight++){
		memset32((void*)(InitData.fb->FrameBufferBase + (x << 2) + (yheight * (InitData.fb->HorizontalResolution * 4))), background_color, width);
        /*for(unsigned long long b = x;b<=x+width;b++){
            *(uint32_t*)((uint64_t*)(InitData.fb->FrameBufferBase+(b*4)+(InitData.fb->HorizontalResolutiom * 4*a))) = background_color;
        }*/
    }
}
unsigned int GP_calculate_rgba_color(uint32_t x, uint32_t y,uint32_t color, unsigned char alpha)
{
	unsigned int pixel = *(uint32_t*)(InitData.fb->FrameBufferBase+(x*4)+(y*InitData.fb->HorizontalResolution*4));
	
	unsigned int ret = 0;

	//	ret >> 8 = alpha * (pixel >> 8)+(1-alpha)*color;

	return ret;


}
void GP_draw_border_rect(unsigned short x, unsigned short y, unsigned short width, unsigned short height,unsigned short border_width, unsigned int background_color, unsigned long border_color){
	


	for(unsigned long long a = y;a<=y+height;a++){
        for(unsigned long long b = x;b<=x+width;b++)
			if(a < (y+border_width) || b < (x+border_width) || b > ((x+width)-border_width) || a > ((y+height)-border_width)){
            *(uint32_t*)((uint64_t*)(InitData.fb->FrameBufferBase+(b*4)+(InitData.fb->HorizontalResolution*4*a))) = border_color;
			}else {
            *(uint32_t*)((uint64_t*)(InitData.fb->FrameBufferBase+(b*4)+(InitData.fb->HorizontalResolution*4*a))) = background_color;
        }
    }

}
void GP_draw_text(const char* data, struct PSF1_FONT* font,unsigned int color,unsigned int x,unsigned int y){
	if(!data || !font) return;
	UINT16 len = strlen(data);
	if(!len) return;
	for(int i = 0;*(char*)(data+i) != '\0';i++){

	char* font_ptr = (char*)&font->glyph_buffer[data[i]*font->header.character_size];
	for(unsigned long a = y;a<y+16;a++){
		for(unsigned long b = x;b<x+8;b++){
			if((*font_ptr & (0b10000000 >> (b - x))) > 0){
				*(uint32_t*)(InitData.fb->FrameBufferBase+(b*4)+(a*InitData.fb->HorizontalResolution*4)) = color;
			}
		}
		font_ptr++;
	}
	x+=8;
	}

}

void Gp_draw_sf_textW(const wchar_t* data,unsigned int color,unsigned int x,unsigned int y){
UINT16 len = wstrlen(data);
if(!len) return;
for(int i = 0;*(short*)(data+i) != '\0';i++){
	if(*(char*)(data+i) == '\0') continue;
	char* font_ptr = (char*)&InitData.start_font->glyph_buffer + ((*(char*)(data+i))*InitData.start_font->header.character_size);
	for(unsigned long a = y;a<y+16;a++){
		for(unsigned long b = x;b<x+8;b++){
			if((*font_ptr & (0b10000000 >> (b - x))) > 0){
				*(uint32_t*)(InitData.fb->FrameBufferBase+(b*4)+(a*InitData.fb->HorizontalResolution*4)) = color;
			}
		}
		font_ptr++;
	}
	x+=8;
}


}
void GP_sf_put_char(const char ch, unsigned int color, unsigned int x, unsigned int y){
	char* font_ptr = (char*)&InitData.start_font->glyph_buffer[ch * InitData.start_font->header.character_size];// + ((*(char*)(&ch))*InitData.start_font->header.character_size);
	for(unsigned long a = y;a<y+16;a++){
		for(unsigned long b = x;b<x+8;b++){
			if((*font_ptr & (0b10000000 >> (b - x))) > 0){
				*(uint32_t*)((unsigned long long)InitData.fb->FrameBufferBase+ (b << 2) + (a*(InitData.fb->HorizontalResolution << 2))) = color;
			}
		}
		font_ptr++;
	}
}

	
int16_t GetBezierPoint(int16_t* cordinates, int16_t cordinate_length, float percent){
	int16_t values[25] = { 0 };
memset(values,0,2*cordinate_length);
for(uint16_t a = 0;a<cordinate_length-1;a++){
	values[a] = (cordinates[a] + ((cordinates[a+1]-cordinates[a])*percent));
}
uint16_t cvalues[4] = { 0 };
for(uint16_t i = 2;i<cordinate_length;i++){
	memset(cvalues,0,2*(4-i));
for(uint16_t a = 0;a<cordinate_length-i;a++){
	cvalues[a] = (values[a] + ((values[a+1]-values[a]) * percent));
}
memset(values,0,sizeof(values));
for(uint16_t a = 0;a<cordinate_length-i;a++){
	values[a] = cvalues[a];
}
}
return values[0];
}


void GP_draw_sf_text(const char* data, unsigned int color,unsigned int x,unsigned int y){
	GP_draw_text(data,InitData.start_font,color,x,y);
}


struct RGBA calculate_rgba(struct RGBA color, uint16_t x, uint16_t y){
	register uint32_t back_color_value = *(uint32_t*)(InitData.fb->FrameBufferBase+(x*4)+(y*InitData.fb->HorizontalResolution * 4));
	register struct RGBA back_color;
	back_color.alpha = 0;
	back_color.r = (back_color_value >> 16) & 0xff;
	back_color.g = (back_color_value >> 8) & 0xff;
	back_color.b = (uint8_t)back_color_value;
	struct RGBA ret;
	if(color.alpha >= 1) return color;


	ret.r = ((1-color.alpha) * back_color.r) + (color.alpha * color.r);
	ret.g = ((1-color.alpha) * back_color.g) + (color.alpha * color.g);
	ret.b = ((1-color.alpha) * back_color.b) + (color.alpha * color.b);
	ret.alpha = color.alpha;
	return ret;
}


void DrawOsLogo(){
	GP_draw_rect((InitData.fb->HorizontalResolution-150)/2,(InitData.fb->VerticalResolution - 150 - 50)/2,150,150,0xffffffff);

}
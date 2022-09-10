#include <preos_renderer.h>
#include <MemoryManagement.h>
#include <kernel.h>
#include <stdlib.h>
#include <math.h>
#include <cstr.h>
#include <cmos.h>



void GP_clear_screen(unsigned int background_color){
	_SIMD_Memset((LPVOID)InitData.fb->FrameBufferBase, (UINT64)background_color | ((UINT64)background_color << 32), (InitData.fb->HorizontalResolution * InitData.fb->VerticalResolution) << 2);
}
extern inline void GP_set_pixel(unsigned int x, unsigned int y, unsigned int color)
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

extern inline void _SSE_BezierCopyCords(double* dest, double* src, UINT8 NumCords);
extern inline UINT64 _SSE_ComputeBezier(float* beta, UINT NumCordinates, float percent);
extern inline UINT64 GetBezierPoint(float* cordinates, float* beta, UINT8 NumCordinates, float percent){
	memcpy(beta, cordinates, NumCordinates * sizeof(float));
	// for(register UINT k = 1;k < NumCordinates;k++) {
	// 	for(register UINT i = 0;i<NumCordinates - k;i++) {
	// 		beta[i] = (1 - percent) * beta[i] + percent * beta[i + 1];
	// 	}
	// }
	return _SSE_ComputeBezier(beta, NumCordinates, percent);
	// _SSE_BezierCopyCords(beta, cordinates, NumCordinates);
	// return _SSE_ComputeBezier(beta, NumCordinates, percent);
}


void GP_draw_sf_text(const char* data, unsigned int color,unsigned int x,unsigned int y){
	GP_draw_text(data,InitData.start_font,color,x,y);
}

void CalculateRgba(struct RGBA* Color, struct RGBA* Source) {
	if(Color->alpha >= 1) return;
	Color->r = ((1-Color->alpha) * Source->r) + (Color->alpha * Color->r);
	Color->g = ((1-Color->alpha) * Source->g) + (Color->alpha * Color->g);
	Color->b = ((1-Color->alpha) * Source->b) + (Color->alpha * Color->b);
}

struct RGBA calculate_rgba(struct RGBA color, uint16_t x, uint16_t y){
	uint32_t back_color_value = *(uint32_t*)(InitData.fb->FrameBufferBase+(x*4)+(y*InitData.fb->Pitch * 4));
	struct RGBA back_color = {0};
	back_color.alpha = 0;
	back_color.r = (back_color_value >> 16) & 0xff;
	back_color.g = (back_color_value >> 8) & 0xff;
	back_color.b = (uint8_t)back_color_value;
	struct RGBA ret = {0};
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
void LineTo(INT64 x0, INT64 y0, INT64 x1, INT64 y1, UINT32 Color) {
	// Brensenham Algorithm f(x) = mx + p
	if(x0 == x1 && y0 == y1) return;

	double dx = abs(x1 - x0);
	double sx = x0 < x1 ? 1 : -1;
	double dy = -abs(y1 - y0);
	double sy = y0 < y1 ? 1 : -1;
	double error = dx + dy;

	for(;;) {
		GP_set_pixel(x0, y0, Color);
		for(UINT b = 0;b<0x10000;b++);
		if(x0 == x1 && y0 == y1) break;
		double e2 = 2 * error;
		if(e2 >= dy) {
			if(x0 == x1) break;
			error = error + dy;
			x0 = x0 + sx;
		}
		if(e2 <= dx) {
			if(y0 == y1) break;
			error = error + dx;
			y0 = y0 + sy;
		}
	}
}
void TestFill(UINT16 RectX, UINT16 RectY, UINT16 RectWidth, UINT16 RectHeight, UINT32 Color) {
	struct {
		INT64 y;
	} Intersects[0x40] = {0};
	struct {
		INT64 y;
	} ResolvedBuffer[0x40] = {0};
	Color &= 0xFFFFFF;
	for(UINT x = 0;x<RectWidth;x++) {
		UINT NumIntersects = 0;
		BOOL IS = 0;
		UINT X = x + RectX;
		for(UINT y = 0;y<RectHeight;y++) {
			UINT Y = y + RectY;
			UINT32* Pixel = (UINT32*)(InitData.fb->FrameBufferBase + (X << 2) + ((Y) * InitData.fb->Pitch));
			UINT32* YPlusOnePixel = (UINT32*)(InitData.fb->FrameBufferBase + (X << 2) + ((Y + 1) * InitData.fb->Pitch));
			if(*Pixel == Color) {
				if(IS) continue;
				Intersects[NumIntersects].y = Y;
				NumIntersects++;
				IS = 1;
			} else {
				IS = 0;
			}

			
		}
		if(NumIntersects < 2) continue;

		

		UINT Index = 0;
		UINT TargetDoubleIntersect = -1;
		QemuWriteSerialMessage("_____");
		if((NumIntersects & 1)) {
			TargetDoubleIntersect = NumIntersects / 2;
		}
		// QemuWriteSerialMessage(to_hstring16(TargetDoubleIntersect));
		// QemuWriteSerialMessage(to_hstring16(NumIntersects));

		UINT Rindx = 0;
		for(UINT i = 0;i<NumIntersects;i++, Rindx++) {
			ResolvedBuffer[Rindx].y = Intersects[i].y;
			if(i == TargetDoubleIntersect) {
				QemuWriteSerialMessage("DOUBLE_INTERSECT DETECTED.");
				Rindx++;
				ResolvedBuffer[Rindx].y = Intersects[i].y;
			}
		}
		NumIntersects = Rindx;
		// QemuWriteSerialMessage(to_hstring16(NumIntersects));
		
		INT LastIndex = -1;
		for(UINT i = 0;i<NumIntersects;i++) {
			if(LastIndex != -1) {
				LineTo(X, ResolvedBuffer[LastIndex].y, X, ResolvedBuffer[i].y, Color);
				if(TargetDoubleIntersect != -1 || x > 125) {
				// SystemDebugPrint(L"Y : %x , Y : %x", ResolvedBuffer[LastIndex].y, ResolvedBuffer[i].y);
				QemuWriteSerialMessage(to_stringu64(ResolvedBuffer[LastIndex].y));
				QemuWriteSerialMessage(to_stringu64(ResolvedBuffer[i].y));
				QemuWriteSerialMessage("-");

				}
				LastIndex = -1;
			} else {
				LastIndex = i;
			}
		}
		// if(TargetDoubleIntersect != -1 || x > 125)
		// 	SystemDebugPrint(L"______");

		
	}
}
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
	UINT64 yheight = y;
	for(UINT64 i = 0;i<height;i++, yheight++){
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
	


	for(unsigned long long a = y;a<=(UINT64)(y+height);a++){
        for(unsigned long long b = x;b<=(UINT64)(x+width);b++)
			if(a < (UINT16)(y+border_width) || b < (UINT16)(x+border_width) || b > (UINT16)((x+width)-border_width) || a > (UINT16)((y+height)-border_width)){
            *(uint32_t*)((uint64_t*)(InitData.fb->FrameBufferBase+(b*4)+(InitData.fb->HorizontalResolution*4*a))) = border_color;
			}else {
            *(uint32_t*)((uint64_t*)(InitData.fb->FrameBufferBase+(b*4)+(InitData.fb->HorizontalResolution*4*a))) = background_color;
        }
    }

}
void GP_draw_text(const char* data, struct PSF1_FONT* font,unsigned int color,unsigned int x,unsigned int y){
	if(!data || !font) return;
	UINT16 len = (UINT16)strlen(data);
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
UINT32 len = wstrlen(data);
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


// Vectorized function of :
/*
// for(register UINT k = 1;k < NumCordinates;k++) {
	// 	for(register UINT i = 0;i<NumCordinates - k;i++) {
	// 		beta[i] = (1 - percent) * beta[i] + percent * beta[i + 1];
	// 	}
	// }

// in eather SSE, AVX or AVX512
// the performance increase by the extension level
*/
extern UINT64 __fastcall _SSE_ComputeBezier(float* beta, UINT NumCordinates, float percent);
extern UINT64 __fastcall _AVX_ComputeBezier(float* beta, UINT NumCordinates, float percent);

UINT64 __fastcall GetBezierPoint(float* cordinates, float* beta, UINT8 NumCordinates, float percent){
	memcpy(beta, cordinates, ((UINT64)NumCordinates) << 2);
	if(ExtensionLevel == EXTENSION_LEVEL_SSE) {
		return _SSE_ComputeBezier(beta, NumCordinates, percent);
	} else if(ExtensionLevel == EXTENSION_LEVEL_AVX) {
		return _AVX_ComputeBezier(beta, NumCordinates, percent);
	}
	return 0;
}


void GP_draw_sf_text(const char* data, unsigned int color,unsigned int x,unsigned int y){
	GP_draw_text(data,InitData.start_font,color,x,y);
}

void CalculateRgba(struct RGBA* Color, struct RGBA* Source) {
	if(Color->alpha >= 1) return;
	Color->r = (UINT8)(((1-Color->alpha) * Source->r) + (Color->alpha * Color->r));
	Color->g = (UINT8)(((1-Color->alpha) * Source->g) + (Color->alpha * Color->g));
	Color->b = (UINT8)(((1-Color->alpha) * Source->b) + (Color->alpha * Color->b));
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


	ret.r = (UINT8)(((1-color.alpha) * back_color.r) + (color.alpha * color.r));
	ret.g = (UINT8)(((1-color.alpha) * back_color.g) + (color.alpha * color.g));
	ret.b = (UINT8)(((1-color.alpha) * back_color.b) + (color.alpha * color.b));
	ret.alpha = color.alpha;
	return ret;
}


void DrawOsLogo(){
	GP_draw_rect((UINT16)(InitData.fb->HorizontalResolution-150)/2,(UINT16)(InitData.fb->VerticalResolution - 150 - 50)/2,150,150,0xffffffff);

}



void LineTo(INT64 x0, INT64 y0, INT64 x1, INT64 y1, UINT32 Color) {
	// Brensenham Algorithm f(x) = mx + p
	if(x0 == x1 && y0 == y1) return;

	double dx = (double)__abs(x1 - x0);
	double sx = x0 < x1 ? 1 : -1;
	double dy = (double)-__abs(y1 - y0);
	double sy = y0 < y1 ? 1 : -1;
	double error = dx + dy;

	for(;;) {
		((UINT32*)InitData.fb->FrameBufferBase)[(x0 + y0 * InitData.fb->Pitch)] = Color;
		if(x0 == x1 && y0 == y1) break;
		double e2 = 2 * error;
		if(e2 >= dy) {
			if(x0 == x1) break;
			error = error + dy;
			x0 = x0 + (INT64)sx;
		}
		if(e2 <= dx) {
			if(y0 == y1) break;
			error = error + dx;
			y0 = y0 + (INT64)sy;
		}
	}
}

inline void _VertexFillBufferLine(INT64 x0, INT64 y0, INT64 x1, INT64 y1, UINT8* Buffer, UINT Pitch) {
	// Brensenham Algorithm f(x) = mx + p
	if(x0 == x1 && y0 == y1) return;

	double dx = (double)__abs(x1 - x0);
	double sx = x0 < x1 ? 1 : -1;
	double dy = (double)-__abs(y1 - y0);
	double sy = y0 < y1 ? 1 : -1;
	double error = dx + dy;

	INT64 Lasty = -1;
	for(;;) {
		if(x0 == x1 && y0 == y1) {
			Buffer[x0 + y0 * Pitch]++;
			break;
		}
		if(Lasty != y0) {
			Buffer[x0 + y0 * Pitch]++;
			Lasty = y0;
		}
		double e2 = 2 * error;
		if(e2 >= dy) {
			if(x0 == x1) break;
			error = error + dy;
			x0 = x0 + (INT64)sx;
		}
		if(e2 <= dx) {
			if(y0 == y1) break;
			error = error + dx;
			y0 = y0 + (INT64)sy;
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
		UINT TargetDoubleIntersect = (UINT)-1;
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
UINT8 Buff[0x100 * 0x100] = {0};
UINT XLinks[0x80] = {0};


#include <Management/runtimesymbols.h>
void FillVertex(UINT X, UINT Y, UINT NumCordinates, float* XCordinates, float* YCordinates, UINT32 Color) {
	float LastPtX = *XCordinates, LastPtY = *YCordinates;
	XCordinates++;
	YCordinates++;
	memset(Buff, 0, 0x100 * 0x100);
	for(UINT i = 1;i<NumCordinates;i++, XCordinates++, YCordinates++) {
		_VertexFillBufferLine((INT64)LastPtX, (INT64)LastPtY, (INT64)(*XCordinates), (INT64)(*YCordinates), Buff, 0x100);
		LastPtX = *XCordinates;
		LastPtY = *YCordinates;
	}
	UINT8* b = Buff;
	for(UINT _Y = 0;_Y<0x100;_Y++) {
		UINT NumLinks = 0;
		for(UINT _X = 0;_X<0x100;_X++,b++) {
			if(*b) {
				const UINT8 c = *b;
				for(UINT i = 0;i<c;i++) {
					XLinks[NumLinks] = _X;
					NumLinks++;
				}
			}
		}
		if(NumLinks < 2) continue;
		UINT* xl = XLinks;
		UINT LastX = 0;
		
		for(UINT i = 0;i<NumLinks;i++, xl++) {
			if(i & 1) {
				LineTo(X + LastX, Y + _Y, X + (*xl), Y + _Y, Color);
			} else {
				LastX = *xl;
			}
		}
	}
}
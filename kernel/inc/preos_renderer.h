#pragma once
#include <kernel.h>
#include <krnltypes.h>

struct QUADRATIC_BEZIER{
	unsigned int x0;
	unsigned int y0;
	unsigned int x1;
	unsigned int y1;
	unsigned int x2;
	unsigned int y2;
};

struct RGBA{
	float alpha;
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct TCB_LIMIT_BUFFER{
	unsigned short* x_arr;
	unsigned short* y_arr;
};

void GP_draw_triangular_bezier(unsigned int x, unsigned int y, struct QUADRATIC_BEZIER* bezier);
void GP_draw_text(const char* data, struct PSF1_FONT* font,unsigned int color,unsigned int x,unsigned int y);
void GP_draw_rounded_rect(unsigned short x,unsigned short y, unsigned short width, unsigned short height, unsigned int background_color, unsigned short border_radius);
void GP_draw_border_rect(unsigned short x, unsigned short y, unsigned short width, unsigned short height,unsigned short border_width, unsigned int background_color, unsigned long border_color);
unsigned int GP_calculate_rgba_color(uint32_t x, uint32_t y,uint32_t color, unsigned char alpha);
void GP_draw_rect(unsigned short x, unsigned short y, unsigned short width, unsigned short height,unsigned int background_color);
extern inline void GP_set_pixel(unsigned int x, unsigned int y, unsigned int color);
void GP_clear_screen(unsigned int background_color);
int TRIANGULAR_BEZIER_GET_POINT(int n1, int n2, float perc);
void GP_set_st_f(struct PSF1_FONT* font);
void GP_draw_sf_text(const char* data, unsigned int color,unsigned int x,unsigned int y);
void GP_sf_put_char(const char ch, unsigned int color, unsigned int x, unsigned int y);
struct RGBA calculate_rgba(struct RGBA color, uint16_t x, uint16_t y);
void Gp_draw_sf_textW(const wchar_t* data,unsigned int color,unsigned int x,unsigned int y);
void DrawOsLogo();

void LineTo(INT64 x0, INT64 y0, INT64 x1, INT64 y1, UINT32 Color);

void CalculateRgba(struct RGBA* Color, struct RGBA* Source);
extern UINT64 GetBezierPoint(float* cordinates, float* beta, UINT8 NumCordinates, float percent);

void TestFill(UINT16 RectX, UINT16 RectY, UINT16 RectWidth, UINT16 RectHeight, UINT32 Color);

void FillVertex(UINT X, UINT Y, UINT NumCordinates, float* XCordinates, float* YCordinates, UINT32 Color);
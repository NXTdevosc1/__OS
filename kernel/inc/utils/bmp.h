#pragma once
#include <stdint.h>
#pragma pack(push, 1)
struct BMP_HDR {
    char signature[2]; // "BM"
    uint32_t size;
    uint16_t reserved[2];
    uint32_t img_offset;
    uint32_t hdr_size;
    uint32_t width;
    uint32_t height;
    uint16_t planes;
    uint16_t bits_per_pixels;
    uint32_t compression_type;
    uint32_t img_size;
    uint32_t x_pixels_per_meter;
    uint32_t y_pixels_per_meter;
    uint32_t num_colors;
    uint32_t important_colors;
};

#pragma pack(pop)

uint8_t BmpImgDraw(void* _file, uint32_t x, uint32_t y);
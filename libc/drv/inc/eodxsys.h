#pragma once
#include <kerneltypes.h>

typedef struct _GRAPHICS_DISPLAY_CONFIGURATION GRAPHICS_DISPLAY_CONFIGURATION;

typedef UINT32 EODX_PIXEL;

typedef EODX_PIXEL* (__cdecl *EODX_GET_FRAME_BUFFER)(GRAPHICS_DISPLAY_CONFIGURATION*);
typedef KERNELSTATUS (__cdecl *EODX_UPDATE_DISPLAY)(GRAPHICS_DISPLAY_CONFIGURATION*);
typedef KERNELSTATUS (__cdecl *EODX_DRIVER_UNLOAD)(GRAPHICS_DISPLAY_CONFIGURATION*);

typedef struct _GRAPHICS_DISPLAY_CONFIGURATION{
    UINT64 DisplayX;
    UINT64 DisplayY;
    UINT32 BitsPerPixel;
    UINT64 Characteristics;
    UINT64 DisplayBufferLength;
    void*  DisplayBuffer;
    EODX_UPDATE_DISPLAY UpdateDisplay;
    EODX_DRIVER_UNLOAD DisplayDriverUnload;
    void* OptionnalData; // Optionnal Data for the current display driver
} GRAPHICS_DISPLAY_CONFIGURATION;


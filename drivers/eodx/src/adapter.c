#include <adapter.h>
#include <gdk.h>
#include <stdlib.h>
typedef struct {
    unsigned int PixelsPerLine;
	unsigned int HorizontalResolution;
	unsigned int VerticalResolution;
	unsigned int PixelsPerScanLine;
	unsigned long long FrameBufferAddress;
	unsigned long long FrameBufferSize;
} BASIC_GRAPHICS_OUTPUT_DESCRIPTOR;
EODX_PIXEL* FrameBuffer = NULL;
BASIC_GRAPHICS_OUTPUT_DESCRIPTOR* GraphicsOutputDescriptor = NULL;
BOOL Initialized = FALSE;
BOOL InitializeBasicDisplayAdapter(GRAPHICS_DISPLAY_CONFIGURATION* DisplayConfiguration){
    if(Initialized) return FALSE;
    GraphicsOutputDescriptor = KeGetRuntimeSymbol("$_BasicGraphicsOutputDescriptor");
    if(!GraphicsOutputDescriptor) return FALSE;
    
    // DisplayConfiguration->DisplayX = GraphicsOutputDescriptor->HorizontalResolution;
    // DisplayConfiguration->DisplayY = GraphicsOutputDescriptor->VerticalResolution;
    // DisplayConfiguration->BufferLength = sizeof(EODX_PIXEL) * DisplayConfiguration->DisplayX * DisplayConfiguration->DisplayY;
    // FrameBuffer = malloc(DisplayConfiguration->BufferLength);
    // if(!FrameBuffer) return FALSE;
    // memset((void*)FrameBuffer, 0, 0x1000);

    // DisplayConfiguration->GetFrameBuffer = BasicDisplayAdapter_GetFrameBuffer;
    // DisplayConfiguration->UpdateDisplay = BasicDisplayAdapter_UpdateDisplay;
    // DisplayConfiguration->DriverUnload = UnloadBasicDisplayAdapater;
    Initialized = TRUE;
    return TRUE;
}

EODX_PIXEL* BasicDisplayAdapter_GetFrameBuffer(GRAPHICS_DISPLAY_CONFIGURATION* DisplayConfiguration){
    if(!Initialized) return NULL;
    return FrameBuffer;
}

BOOL BasicDisplayAdapter_UpdateDisplay(GRAPHICS_DISPLAY_CONFIGURATION* DisplayConfiguration){
    if(!Initialized) return FALSE;
    // memcpy((void*)GraphicsOutputDescriptor->FrameBufferAddress, FrameBuffer, DisplayConfiguration->BufferLength);
    return TRUE;
}

HRESULT UnloadBasicDisplayAdapater(GRAPHICS_DISPLAY_CONFIGURATION* DisplayConfiguration){

    free(FrameBuffer);
    FrameBuffer = NULL;
    Initialized = FALSE;
    return 0;
}
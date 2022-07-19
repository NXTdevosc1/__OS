#pragma once
#include <kernelruntime.h>
#include <eodxsys.h>
#include <_eodx.h>

BOOL InitializeBasicDisplayAdapter(GRAPHICS_DISPLAY_CONFIGURATION* DisplayConfiguration);
HRESULT UnloadBasicDisplayAdapater(GRAPHICS_DISPLAY_CONFIGURATION* DisplayConfiguration);
EODX_PIXEL* BasicDisplayAdapter_GetFrameBuffer(GRAPHICS_DISPLAY_CONFIGURATION* DisplayConfiguration);
BOOL BasicDisplayAdapter_UpdateDisplay(GRAPHICS_DISPLAY_CONFIGURATION* DisplayConfiguration);

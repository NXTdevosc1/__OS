#include <vmsvga/vmsga.h>
#include <gdk.h>
#include <kernelruntime.h>
#include <eodxsys.h>

#define SVGA_CAP_NONE               0x00000000
#define SVGA_CAP_RECT_COPY          0x00000002
#define SVGA_CAP_CURSOR             0x00000020
#define SVGA_CAP_CURSOR_BYPASS      0x00000040   // Legacy (Use Cursor Bypass 3 instead)
#define SVGA_CAP_CURSOR_BYPASS_2    0x00000080   // Legacy (Use Cursor Bypass 3 instead)
#define SVGA_CAP_8BIT_EMULATION     0x00000100
#define SVGA_CAP_ALPHA_CURSOR       0x00000200
#define SVGA_CAP_3D                 0x00004000
#define SVGA_CAP_EXTENDED_FIFO      0x00008000
#define SVGA_CAP_MULTIMON           0x00010000   // Legacy multi-monitor support
#define SVGA_CAP_PITCHLOCK          0x00020000
#define SVGA_CAP_IRQMASK            0x00040000
#define SVGA_CAP_DISPLAY_TOPOLOGY   0x00080000   // Legacy multi-monitor support
#define SVGA_CAP_GMR                0x00100000
#define SVGA_CAP_TRACES             0x00200000

void SvgaIoWrite(VMSVGA_DEVICE* Device, UINT32 Index, UINT32 Value);
unsigned int SvgaIoRead(VMSVGA_DEVICE* Device, UINT32 Index);

typedef struct _BASIC_DISPLAY_DESCRIPTOR BASIC_DISPLAY_DESCRIPTOR;

typedef struct _BASIC_DISPLAY_DESCRIPTOR{
	unsigned int		FrameBufferId; // For multiple gpus/displays
	unsigned int 		HorizontalResolution;
	unsigned int 		VerticalResolution;
	unsigned int 		Version;
	char* 			FrameBufferBase; // for G.O.P
	unsigned long long 	FrameBufferSize;
	unsigned int 		ColorDepth;
	unsigned int 		RefreshRate;
	void*				Protocol; // Pointer used by UpdateVideo routine
	void* UpdateVideo;
	BASIC_DISPLAY_DESCRIPTOR* Next;
} BASIC_DISPLAY_DESCRIPTOR;

char TextOut[100] = {0};

void VmwareSvgaSetMode(VMSVGA_DEVICE* Device, UINT32 Width, UINT32 Height, UINT32 BitsPerPixel);

BOOL SvgaHasFifoCapability(VMSVGA_DEVICE* Device, unsigned int Capability){
    return (Device->FifoMem[SVGA_FIFO_CAPABILITIES] & Capability) != 0;
}

BOOL SvgaIsFifoRegValid(VMSVGA_DEVICE* Device, unsigned int Register){
    return Device->FifoMem[SVGA_FIFO_MIN] > (Register << 2);
}

KERNELSTATUS VmwareSvgaInitialize(PCI_CONFIGURATION_HEADER* PciConfig){
    VMSVGA_DEVICE _Device = {0};
    VMSVGA_DEVICE* Device = &_Device;
    Device->PciConfig = PciConfig;
    Device->IoBase = (UINT16)PciConfig->BaseAddress0 & ~(0xf);
    Device->FrameBufferBase = (void*)((UINT64)PciConfig->BaseAddress1 & ~(0xf));
    Device->FifoMem = (UINT32*)((UINT64)PciConfig->BaseAddress2 & ~(0xf));

    Device->FifoSize = SvgaIoRead(Device, SVGA_REG_MEM_SIZE);
    Device->Capabilities = SvgaIoRead(Device, SVGA_REG_CAPABILITIES);

    // VmwareSvgaSetMode(Device, SvgaIoRead(Device, SVGA_REG_MAX_WIDTH),
    // SvgaIoRead(Device, SVGA_REG_MAX_HEIGHT), 32
    // );
    VmwareSvgaSetMode(Device, SvgaIoRead(Device, SVGA_REG_WIDTH),
    SvgaIoRead(Device, SVGA_REG_HEIGHT), 32
    );

    KeMapMemory(Device->FrameBufferBase, (Device->FrameBufferSize + 0x1000) >> 12, PM_MAP | PM_WRITE_THROUGH);
    KeMapMemory(Device->FifoMem, (Device->FifoSize + 0x1000) >> 12, PM_MAP | PM_WRITE_THROUGH);
    

    BASIC_DISPLAY_DESCRIPTOR* BasicDisplay = KeGetRuntimeSymbol(".$_BasicGraphicsOutputDescriptor");
    if(!BasicDisplay) KeSetDeathScreen(0, L"Failed to get basic display", NULL, NULL);

    BasicDisplay->FrameBufferBase = Device->FrameBufferBase;


    SystemDebugPrint(L"VMWARE SVGA GRAPHICS ADAPTER SUCCESSFULLY INITIALIZED");
    itoa((UINT64)Device->FrameBufferBase, TextOut, RADIX_HEXADECIMAL);
    SystemDebugPrintA(TextOut);

    itoa((UINT64)Device->IoBase, TextOut, RADIX_HEXADECIMAL);
    SystemDebugPrintA(TextOut);

    itoa((UINT64)Device->FifoMem, TextOut, RADIX_HEXADECIMAL);
    SystemDebugPrintA(TextOut);

    itoa((UINT64)Device->HorizontalResolution, TextOut, RADIX_HEXADECIMAL);
    SystemDebugPrintA(TextOut);

    itoa((UINT64)SvgaIoRead(Device, SVGA_REG_MAX_WIDTH), TextOut, RADIX_HEXADECIMAL);
    SystemDebugPrintA(TextOut);

    return KERNEL_SOK;
}

void VmwareSvgaEnable(VMSVGA_DEVICE* Device){
    SvgaIoWrite(Device, SVGA_REG_ENABLE, TRUE);
}
void VmwareSvgaDisable(VMSVGA_DEVICE* Device){
    SvgaIoWrite(Device, SVGA_REG_ENABLE, FALSE);
}

unsigned int SvgaIoRead(VMSVGA_DEVICE* Device, UINT32 Index){
    OutPort(Device->IoBase + VMSVGA_INDEX_PORT, Index);
    return InPort(Device->IoBase + VMSVGA_VALUE_PORT);
}

void SvgaIoWrite(VMSVGA_DEVICE* Device, UINT32 Index, UINT32 Value){
    OutPort(Device->IoBase + VMSVGA_INDEX_PORT, Index);
    OutPort(Device->IoBase + VMSVGA_VALUE_PORT, Value);
}

void VmwareSvgaSetMode(VMSVGA_DEVICE* Device, UINT32 Width, UINT32 Height, UINT32 BitsPerPixel){
    VmwareSvgaDisable(Device);
    SvgaIoWrite(Device, SVGA_REG_CONFIG_DONE, FALSE);

    SvgaIoWrite(Device, SVGA_REG_WIDTH, Width);
    SvgaIoWrite(Device, SVGA_REG_HEIGHT, Height);
    SvgaIoWrite(Device, SVGA_REG_BITS_PER_PIXEL, BitsPerPixel);

    VmwareSvgaEnable(Device);
    Device->HorizontalResolution = Width;
    Device->VerticalResolution = Height;
    Device->BitsPerPixel = BitsPerPixel;
    Device->Pitch = SvgaIoRead(Device, SVGA_REG_BYTES_PER_LINE);
    // Initialize command FIFO

    Device->FifoMem[SVGA_FIFO_MIN] = SVGA_FIFO_NUM_REGS * 4; // numregs * 4
    Device->FifoMem[SVGA_FIFO_MAX] = Device->FifoSize;
    Device->FifoMem[SVGA_FIFO_NEXT_CMD] = Device->FifoMem[SVGA_FIFO_MIN];
    Device->FifoMem[SVGA_FIFO_STOP] = Device->FifoMem[SVGA_FIFO_MIN];

    if(SvgaHasFifoCapability(Device, SVGA_CAP_EXTENDED_FIFO) &&
    SvgaIsFifoRegValid(Device, SVGA_FIFO_GUEST_3D_HWVERSION)
    ){
        Device->FifoMem[SVGA_FIFO_GUEST_3D_HWVERSION] = SVGA3D_HWVERSION_CURRENT;
    }

    // Enable FIFO
    // SvgaIoWrite(Device, SVGA_REG_CONFIG_DONE, TRUE);



    
}
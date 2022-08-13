
#define __DLL_EXPORTS
#include <assemblydef.h>
#include <kernelruntime.h>
// #include <stdlib.h>
#include <ddk.h>
typedef struct __DEVICE_OBJECT {
    BOOL Present;
	UINT64 Id;
	RFDEVICE_OBJECT ParentDevice;
	BOOL DeviceMutex; // Mutex
	BOOL DeviceControl; // Set if the device driver is controlling the device
	BOOL DeviceDisable;
	BOOL AccessMethod; // 0 for Legacy Io Access, 1 For MMIO Access
	LPWSTR DisplayName;
	UINT32 DeviceSource;
	UINT32 DeviceClass;
	UINT32 DeviceSubclass;
	UINT32 ProgramInterface;
	UINT32 DeviceType;
	UINT64 VendorId;
	UINT64 DeviceId;
	void* DeviceConfiguration; // For ex. In PCI, this is the mmio address of the header
	UINT8 Bus; // In case that Access Method is 0 (Legacy IO Access)
	UINT8 DeviceNumber;
	UINT8 Function;
	UINT8 PciAccessType; // 0 = MMIO, 1 = IO
	RFDRIVER_OBJECT Driver;
	void* ControlInterface;
	void* ExtensionPointer; // an optionnal structure for additionnal info about the device
	BOOL DeviceInitialized; // set to 1 when the relative driver completes the device initialization, the device then becomes accessible
	UINT64 DeviceFeatures;
	KERNELSTATUS DeviceStatus;
} _DEVICE_OBJECT, *_RFDEVICE_OBJECT;

DDKEXPORT KERNELSTATUS DDKAPI SystemDebugPrintA(const char* Text){
    UINT len = 0;
    for(UINT i = 0;;i++){
        if(!Text[i]) break;
        len++;
    }
    if(len > 120) return KERNEL_SERR_INVALID_PARAMETER;
    UINT16 WText[121] = {0};
    for(UINT i = 0;i<len;i++){
        WText[i] = Text[i];
    }
    return SystemDebugPrint(WText);
}
DDKEXPORT void* DDKAPI PciGetBaseAddress(_RFDEVICE_OBJECT DeviceObject, unsigned char BarIndex, BOOL* MmIO /*0 = IO, 1 = MMIO*/){
	if(BarIndex > 5) return NULL;
	UINT64 Bar = PciDeviceConfigurationRead32(DeviceObject, PCI_BAR + (BarIndex << 2));
	if(Bar & PCI_BAR_IO) {
		*MmIO = 0;
	}else *MmIO = 1;
	if(Bar & PCI_BAR_64BIT){
		Bar |= (UINT64)PciDeviceConfigurationRead32(DeviceObject, PCI_BAR + (BarIndex << 2) + 4) << 32;
	}
	Bar&=PCI_ADDRESS_MASK;
	return (void*)Bar;
}
DDKEXPORT UINT64 DDKAPI PciDeviceConfigurationRead64(_RFDEVICE_OBJECT DeviceObject, UINT16 Offset){
	if(DeviceObject->PciAccessType == 0) {
		// MMIO Access
		return *(UINT64*)((char*)DeviceObject->DeviceConfiguration + Offset);
	}
	return IoPciRead32(DeviceObject->Bus, DeviceObject->DeviceNumber, DeviceObject->Function, Offset) | ((UINT64)IoPciRead32(DeviceObject->Bus, DeviceObject->DeviceNumber, DeviceObject->Function, Offset + 4) << 32);
}

DDKEXPORT UINT32 DDKAPI PciDeviceConfigurationRead32(_RFDEVICE_OBJECT DeviceObject, UINT16 Offset){
	if(DeviceObject->PciAccessType == 0) {
		// MMIO Access
		return *(UINT32*)((char*)DeviceObject->DeviceConfiguration + Offset);
	}
	return IoPciRead32(DeviceObject->Bus, DeviceObject->DeviceNumber, DeviceObject->Function, Offset);
}
DDKEXPORT UINT16 DDKAPI PciDeviceConfigurationRead16(_RFDEVICE_OBJECT DeviceObject, UINT16 Offset){
	if(DeviceObject->PciAccessType == 0) {
		// MMIO Access
		return *(UINT16*)((char*)DeviceObject->DeviceConfiguration + Offset);
	}
	return IoPciRead16(DeviceObject->Bus, DeviceObject->DeviceNumber, DeviceObject->Function, Offset);
}
DDKEXPORT UINT8 DDKAPI PciDeviceConfigurationRead8(_RFDEVICE_OBJECT DeviceObject, UINT16 Offset){
	if(DeviceObject->PciAccessType == 0) {
		// MMIO Access
		return *(UINT8*)((char*)DeviceObject->DeviceConfiguration + Offset);
	}
	return IoPciRead8(DeviceObject->Bus, DeviceObject->DeviceNumber, DeviceObject->Function, Offset);
}
DDKEXPORT void DDKAPI PciDeviceConfigurationWrite64(_RFDEVICE_OBJECT DeviceObject, UINT64 Value, UINT16 Offset){
	if(DeviceObject->PciAccessType == 0){
		*(UINT64*)((char*)DeviceObject->DeviceConfiguration + Offset) = Value;
	}else {
		IoPciWrite32(DeviceObject->Bus, DeviceObject->DeviceNumber, DeviceObject->Function, Value, Offset);
		IoPciWrite32(DeviceObject->Bus, DeviceObject->DeviceNumber, DeviceObject->Function, Value >> 32, Offset + 4);
	}
}

DDKEXPORT void DDKAPI PciDeviceConfigurationWrite32(_RFDEVICE_OBJECT DeviceObject, UINT32 Value, UINT16 Offset){
	if(DeviceObject->PciAccessType == 0){
		*(UINT32*)((char*)DeviceObject->DeviceConfiguration + Offset) = Value;
	}else {
		IoPciWrite32(DeviceObject->Bus, DeviceObject->DeviceNumber, DeviceObject->Function, Value, Offset);
	}
}
DDKEXPORT void DDKAPI PciDeviceConfigurationWrite16(_RFDEVICE_OBJECT DeviceObject, UINT16 Value, UINT16 Offset){
	if(DeviceObject->PciAccessType == 0){
		*(UINT16*)((char*)DeviceObject->DeviceConfiguration + Offset) = Value;
	}else {
		IoPciWrite16(DeviceObject->Bus, DeviceObject->DeviceNumber, DeviceObject->Function, Value, Offset);
	}
}
DDKEXPORT void DDKAPI PciDeviceConfigurationWrite8(_RFDEVICE_OBJECT DeviceObject, UINT8 Value, UINT16 Offset){
	if(DeviceObject->PciAccessType == 0){
		*(UINT8*)((char*)DeviceObject->DeviceConfiguration + Offset) = Value;
	}else {
		IoPciWrite8(DeviceObject->Bus, DeviceObject->DeviceNumber, DeviceObject->Function, Value, Offset);
	}
}
DDKEXPORT BOOL DDKAPI WriteDeviceLog(_RFDEVICE_OBJECT DeviceObject, LPCWSTR EventString){
	return FALSE; // UNIMPLEMENTED YET
}
DDKEXPORT BOOL DDKAPI WriteDeviceLogA(_RFDEVICE_OBJECT DeviceObject, LPCSTR EventString){
	return FALSE; // UNIMPLEMENTED YET
}

DDKEXPORT BOOL DDKAPI SetDeviceStatus(_RFDEVICE_OBJECT DeviceObject, KERNELSTATUS DeviceStatus){
	DeviceObject->DeviceStatus = DeviceStatus;
	return TRUE;
}
DDKEXPORT BOOL DDKAPI PciEnableInterrupts(_RFDEVICE_OBJECT Device){
	UINT16 Command = PciDeviceConfigurationRead16(Device, PCI_COMMAND);
	PciDeviceConfigurationWrite16(Device, Command & ~(1 << 10), PCI_COMMAND);
	return TRUE;
}
DDKEXPORT BOOL DDKAPI SetDeviceFeature(_RFDEVICE_OBJECT Device, UINT64 DeviceFeatureMask){
	Device->DeviceFeatures |= DeviceFeatureMask;
	return TRUE;
}
DDKEXPORT void* DDKAPI AllocateDeviceMemory(_RFDEVICE_OBJECT Device, UINT64 NumBytes, UINT Alignment){
	void* Heap = NULL;
	if(Device->DeviceFeatures & DEVICE_64BIT_ADDRESS_ALLOCATIONS) {
		Heap = AllocatePoolEx(NULL, NumBytes, Alignment, NULL, 0);
	}else{
		Heap = AllocatePoolEx(NULL, NumBytes, Alignment, NULL, 0xFFFFFFFF - NumBytes - Alignment);
	}
	if(Heap == NULL && Device->DeviceFeatures & DEVICE_FORCE_MEMORY_ALLOCATION) {
		// KeSetDeathScreen(0, L"DEVICE_ALLOCATION_FAILED (FORCE_ALLOCATION ENABLED)", NULL, OS_SUPPORT_LNKW);
        while(1);
	}
	ZeroMemory(Heap, NumBytes);
	return Heap;
}
DDKEXPORT BOOL DDKAPI SetDeviceExtension(_RFDEVICE_OBJECT DeviceObject, void* ExtensionPointer){
	DeviceObject->ExtensionPointer = ExtensionPointer;
	return TRUE;
}
DDKEXPORT void* DDKAPI GetDeviceExtension(_RFDEVICE_OBJECT DeviceObject){
	return DeviceObject->ExtensionPointer;
}

DDKEXPORT UINT32 DDKAPI IoPciRead32(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT16 Offset){
	UINT32 Advance = Offset & 3;
    
    if(Advance) {
        UINT32 Ret = IoPciRead16(BusNumber, Device, Function, Offset);
        Ret |= (UINT32)IoPciRead16(BusNumber, Device, Function, Offset + 2) << 16;
        return Ret;
    } else {
        register UINT32 Address = 0x80000000; // Set Enable bit
        register UINT32 b = BusNumber & 0xF;
        register UINT32 d = Device & 0x3F;
        register UINT32 f = Function & 0xF;
        Address |= Offset | (f << 8) | (d << 11) | (b << 16);
        OutPort(0xCF8, Address);
        return InPort(0xCFC);
    }
}
DDKEXPORT UINT16 DDKAPI IoPciRead16(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT16 Offset){
	register UINT32 Address = 0x80000000; // Set Enable bit
    register UINT32 b = BusNumber & 0xF;
    register UINT32 d = Device & 0x3F;
    register UINT32 f = Function & 0xF;

    UINT32 Advance = Offset & 3;
    Offset &= ~3;

    Address |= Offset | (f << 8) | (d << 11) | (b << 16);
    OutPort(0xCF8, Address);
    if(Advance) {
        UINT16 Ret = 0;
        UINT32 Val = InPortW(0xCFC);
        Val >>= (Advance) << 3;
        if(Advance == 3) {
            Val &= 0xFF;
            Ret = Val;
            OutPort(0xCF8, Address + 4);
            Val = InPortB(0xCFC);
            Ret |= Val << 8;
        } else {
            Val &= 0xFFFF;
            Ret = Val;
        }
        return Ret;
    } else return InPortW(0xCFC);
}


DDKEXPORT UINT8 DDKAPI IoPciRead8(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT16 Offset){
	register UINT32 Address = 0x80000000; // Set Enable bit
    register UINT32 b = BusNumber & 0xF;
    register UINT32 d = Device & 0x3F;
    register UINT32 f = Function & 0xF;

    UINT32 Advance = Offset & 3;
    Offset &= ~3;

    Address |= Offset | (f << 8) | (d << 11) | (b << 16);
    OutPort(0xCF8, Address);
    if(Advance) {
        UINT32 Val = InPort(0xCFC);
        Val >>= (Advance << 3);
        Val &= 0xFF;
        return Val;
    } else return InPortB(0xCFC);
}
DDKEXPORT void DDKAPI IoPciWrite32(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT32 Value, UINT16 Offset){
	
    UINT32 Advance = Offset & 3; // Offset must be 4 byte aligned
    
    if(!Advance) {
        register UINT32 Address = 0x80000000; // Set Enable bit
        register UINT32 b = BusNumber & 0xF;
        register UINT32 d = Device & 0x3F;
        register UINT32 f = Function & 0xF;
        Address |= (Offset & 0xff) | (f << 8) | (d << 11) | (b << 16);
        OutPort(0xCF8, Address);
        OutPort(0xCFC, Value);
    } else {
        IoPciWrite16(BusNumber, Device, Function, Value, Offset);
        IoPciWrite16(BusNumber, Device, Function, Value >> 16, Offset + 2);
    }
}

DDKEXPORT void DDKAPI IoPciWrite16(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT16 Value, UINT16 Offset){
	register UINT32 Address = 0x80000000; // Set Enable bit
    register UINT32 b = BusNumber & 0xF;
    register UINT32 d = Device & 0x3F;
    register UINT32 f = Function & 0xF;
    UINT32 Advance = Offset & 3; // Offset must be 4 byte aligned
    Offset &= ~3;
    Address |= (Offset & 0xff) | (f << 8) | (d << 11) | (b << 16);
    OutPort(0xCF8, Address);
    if(!Advance) {
        OutPortW(0xCFC, Value);
    } else {
        UINT32 Val = InPort(0xCFC);
        Val |= ((UINT32)Value << (Advance << 3));
        OutPort(0xCFC, Val);
        if(Advance == 3) {
            OutPort(0xCF8, Address + 4);
            Val = InPort(0xCFC);
            Val |= ((UINT32)Value >> 16);
            OutPort(0xCFC, Val);
        }
    }
}
DDKEXPORT void DDKAPI IoPciWrite8(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT8 Value, UINT16 Offset){
	register UINT32 Address = 0x80000000; // Set Enable bit
    register UINT32 b = BusNumber & 0xF;
    register UINT32 d = Device & 0x3F;
    register UINT32 f = Function & 0xF;
    UINT32 Advance = Offset & 3; // Offset must be 4 byte aligned
    Offset &= ~3;
    Address |= (Offset & 0xff) | (f << 8) | (d << 11) | (b << 16);
    OutPort(0xCF8, Address);
    if(!Advance) {
        OutPortB(0xCFC, Value);
    } else {
        UINT32 Val = InPort(0xCFC);
        Val |= ((UINT32)Value << (Advance << 3));
        OutPort(0xCFC, Val);
	}
}

DDKEXPORT BOOL DDKAPI AllocatePciBaseAddress(struct __DEVICE_OBJECT* DeviceObject, UINT BaseAddressNumber, UINT64 NumPages, UINT Flags) {
    if(BaseAddressNumber > 5) return FALSE;
    LPVOID Mem = AllocateIoMemory(NumPages, Flags);
    if(!Mem) return FALSE;
    (UINT64)Mem |= PCI_BAR_64BIT; // 64 Bit Base Address
    PciDeviceConfigurationWrite64(DeviceObject, (UINT64)Mem, PCI_BAR + (BaseAddressNumber << 2));
    return TRUE;
}
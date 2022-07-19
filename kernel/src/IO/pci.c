#include <IO/pci.h>
#include <IO/utility.h>

UINT32 KERNELAPI IoPciRead32(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT16 Offset){
    
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
UINT16 KERNELAPI IoPciRead16(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT16 Offset){
    register UINT32 Address = 0x80000000; // Set Enable bit
    register UINT32 b = BusNumber & 0xF;
    register UINT32 d = Device & 0x3F;
    register UINT32 f = Function & 0xF;

    UINT32 Advance = Offset & 3;
    Offset &= ~3;

    Address |= Offset | (f << 8) | (d << 11) | (b << 16);
    OutPort(0xCF8, Address);
    if(Advance) {
        UINT32 Ret = 0;
        UINT32 Val = InPort(0xCFC);
        Val >>= (4 - Advance) << 3;
        Ret = Val;
        Advance -= 4 - Advance;
        if(Advance == 3) {
            OutPort(0xCF8, Address + 4);
            Val = InPortB(0xCFC);
            Ret |= Val << 8;
        }
        return Ret;
    } else return InPortW(0xCFC);
}
UINT8 KERNELAPI IoPciRead8(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT16 Offset){
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
        Val >>= ((4 - Advance) << 3);
        return Val;
    } else return InPortB(0xCFC);
}

UINT64 KERNELAPI PciDeviceConfigurationRead64(RFDEVICE_OBJECT DeviceObject, UINT16 Offset) {
    if(DeviceObject->PciAccessType == 0) {
		// MMIO Access
		return *(UINT64*)((char*)DeviceObject->DeviceConfiguration + Offset);
	}
	return IoPciRead32(DeviceObject->Bus, DeviceObject->DeviceNumber, DeviceObject->Function, Offset) | ((UINT64)IoPciRead32(DeviceObject->Bus, DeviceObject->DeviceNumber, DeviceObject->Function, Offset + 4) << 32);
}

UINT32 KERNELAPI PciDeviceConfigurationRead32(RFDEVICE_OBJECT DeviceObject, UINT16 Offset) {
    if(DeviceObject->PciAccessType == 0) {
		// MMIO Access
		return *(UINT32*)((char*)DeviceObject->DeviceConfiguration + Offset);
	}
	return IoPciRead32(DeviceObject->Bus, DeviceObject->DeviceNumber, DeviceObject->Function, Offset);
}

UINT16 KERNELAPI PciDeviceConfigurationRead16(RFDEVICE_OBJECT DeviceObject, UINT16 Offset) {
    if(DeviceObject->PciAccessType == 0) {
		// MMIO Access
		return *(UINT16*)((char*)DeviceObject->DeviceConfiguration + Offset);
	}
	return IoPciRead16(DeviceObject->Bus, DeviceObject->DeviceNumber, DeviceObject->Function, Offset);

}
UINT8 KERNELAPI PciDeviceConfigurationRead8(RFDEVICE_OBJECT DeviceObject, UINT16 Offset) {
    if(DeviceObject->PciAccessType == 0) {
		// MMIO Access
		return *(UINT8*)((char*)DeviceObject->DeviceConfiguration + Offset);
	}
	return IoPciRead8(DeviceObject->Bus, DeviceObject->DeviceNumber, DeviceObject->Function, Offset);

}

void KERNELAPI PciDeviceConfigurationWrite64(RFDEVICE_OBJECT DeviceObject, UINT64 Value, UINT16 Offset) {
    if(DeviceObject->PciAccessType == 0){
		*(UINT64*)((char*)DeviceObject->DeviceConfiguration + Offset) = Value;
	}else {
		IoPciWrite32(DeviceObject->Bus, DeviceObject->DeviceNumber, DeviceObject->Function, Value, Offset);
		IoPciWrite32(DeviceObject->Bus, DeviceObject->DeviceNumber, DeviceObject->Function, Value >> 32, Offset + 4);
	}
}
void KERNELAPI PciDeviceConfigurationWrite32(RFDEVICE_OBJECT DeviceObject, UINT32 Value, UINT16 Offset) {
    if(DeviceObject->PciAccessType == 0){
		*(UINT32*)((char*)DeviceObject->DeviceConfiguration + Offset) = Value;
	}else {
		IoPciWrite32(DeviceObject->Bus, DeviceObject->DeviceNumber, DeviceObject->Function, Value, Offset);
	}
}
void KERNELAPI PciDeviceConfigurationWrite16(RFDEVICE_OBJECT DeviceObject, UINT16 Value, UINT16 Offset) {
    if(DeviceObject->PciAccessType == 0){
		*(UINT16*)((char*)DeviceObject->DeviceConfiguration + Offset) = Value;
	}else {
		IoPciWrite16(DeviceObject->Bus, DeviceObject->DeviceNumber, DeviceObject->Function, Value, Offset);
	}
}
void KERNELAPI PciDeviceConfigurationWrite8(RFDEVICE_OBJECT DeviceObject, UINT8 Value, UINT16 Offset) {
    if(DeviceObject->PciAccessType == 0){
		*(UINT8*)((char*)DeviceObject->DeviceConfiguration + Offset) = Value;
	}else {
		IoPciWrite8(DeviceObject->Bus, DeviceObject->DeviceNumber, DeviceObject->Function, Value, Offset);
	}
}


void KERNELAPI IoPciWrite32(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT32 Value, UINT16 Offset) {
    

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
void KERNELAPI IoPciWrite16(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT16 Value, UINT16 Offset) {
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
void KERNELAPI IoPciWrite8(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT8 Value, UINT16 Offset) {
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

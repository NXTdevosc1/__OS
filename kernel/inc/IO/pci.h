#pragma once
#include <krnltypes.h>
#include <IO/pcidef.h>
#include <Management/device.h>

UINT32 KERNELAPI IoPciRead32(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT16 Offset);
UINT16 KERNELAPI IoPciRead16(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT16 Offset);
UINT8 KERNELAPI IoPciRead8(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT16 Offset);


void KERNELAPI IoPciWrite32(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT32 Value, UINT16 Offset);
void KERNELAPI IoPciWrite16(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT16 Value, UINT16 Offset);
void KERNELAPI IoPciWrite8(UINT8 BusNumber, UINT8 Device, UINT8 Function, UINT8 Value, UINT16 Offset);



UINT64 KERNELAPI PciDeviceConfigurationRead64(RFDEVICE_OBJECT DeviceObject, UINT16 Offset);
UINT32 KERNELAPI PciDeviceConfigurationRead32(RFDEVICE_OBJECT DeviceObject, UINT16 Offset);
UINT16 KERNELAPI PciDeviceConfigurationRead16(RFDEVICE_OBJECT DeviceObject, UINT16 Offset);
UINT8 KERNELAPI PciDeviceConfigurationRead8(RFDEVICE_OBJECT DeviceObject, UINT16 Offset);

void KERNELAPI PciDeviceConfigurationWrite64(RFDEVICE_OBJECT DeviceObject, UINT64 Value, UINT16 Offset);
void KERNELAPI PciDeviceConfigurationWrite32(RFDEVICE_OBJECT DeviceObject, UINT32 Value, UINT16 Offset);
void KERNELAPI PciDeviceConfigurationWrite16(RFDEVICE_OBJECT DeviceObject, UINT16 Value, UINT16 Offset);
void KERNELAPI PciDeviceConfigurationWrite8(RFDEVICE_OBJECT DeviceObject, UINT8 Value, UINT16 Offset);


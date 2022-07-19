#pragma once
#include <stdint.h>
#include <acpi/acpi_defs.h>
#include <CPU/process.h>
#include <krnltypes.h>
#include <IO/pcidef.h>
//Configuration space base address allocation structures

#pragma pack(push, 1)

typedef struct _PCI_DEVICE_CONFIG{
	UINT64	BaseAddress; // enhanced configuration mechanism
	UINT16	PciSegGroupNumber;
	unsigned char	StartPciBusNumber;
	unsigned char	EndPciBusNumber;
	UINT32	Reserved;
} PCI_DEVICE_CONFIG;

typedef struct _ACPI_MCFG{
	ACPI_SDT Sdt;
	UINT64 reserved;
} ACPI_MCFG;






#pragma pack(pop)


void PciExpressInit(ACPI_SDT* Sdt);
PCI_CONFIGURATION_HEADER* KERNELAPI PcieFindDevice(UINT16 VendorId, UINT16 DeviceId);
BOOL KERNELAPI PcieEnumerateDevices(unsigned char PciClass, unsigned char PciSubClass, unsigned char ProgramInterface, PCI_CONFIGURATION_HEADER** PConfiguarationPtrs, unsigned int Max);
PCI_CONFIGURATION_HEADER* KERNELAPI PcieConfigurationRead(UINT16 PcieConfig, unsigned char Bus, unsigned char Device, unsigned char Function);

BOOL KERNELAPI GetPcieCompatibility();

UINT64 GetNumPciExpressConfigurations();
#pragma once
#include <krnlapi.h>
#include <kerneltypes.h>
#include <pcibase.h>


// Optionnal Parameter Set to -1 if not set
// RFPCI_CONFIGURATION_HEADER KERNELAPI PciExpressFindDevice(
//     _IN UINT16 VendorId,
//     _IN _OPT UINT16 DeviceId
// );

// // Optionnal Parameter Set to -1 if not set
// BOOL KERNELAPI PciExpressEnumerateDevices(
//     _IN unsigned char PciClass,
//     _IN _OPT unsigned char PciSubClass,
//     _IN _OPT unsigned char ProgramInterface,
//     _IN RFPCI_CONFIGURATION_HEADER* PciConfigurationPtrList,
//     _IN DWORD Max
// );

RFPCI_CONFIGURATION_HEADER KERNELAPI PciExpressConfigurationRead(
    _IN UINT16 ConfigurationIndex,
    _IN unsigned char Bus,
    _IN unsigned char Device,
    _IN unsigned char Function
);

BOOL KERNELAPI CheckPciExpress(); // check if system is PCIE Compatible

UINT16 KERNELAPI NumPciExpressConfigurations();

// DDKIMPORT PVOID DDKAPI PciExpressGetBaseAddress(RFPCI_CONFIGURATION_HEADER PciConfigurationHeader, UINT BaseAddressIndex);
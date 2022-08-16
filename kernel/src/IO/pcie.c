#include <IO/pcie.h>
#include <MemoryManagement.h>
#include <IO/utility.h>
#include <preos_renderer.h>
#include <cstr.h>
#include <interrupt_manager/SOD.h>
#include <CPU/cpu.h>
#include <dsk/ata.h>
#include <kernel.h>
#include <Management/device.h>
#include <IO/pcidef.h>

// PCI FUNCTION ADDR = (devices[i].base_addr + ((bus - devices[i].start_pci_bus_number) << 20) | ((slot)<<15) | (function << 12));

BOOL PcieCompatible = FALSE;
ACPI_MCFG* Mcfg = NULL;
UINT16 NumDeviceConfigs = 0;
PCI_CONFIGURATION_HEADER* KERNELAPI PcieConfigurationRead(UINT16 PcieConfig, unsigned char Bus, unsigned char Device, unsigned char Function)
{
	if(!Mcfg || PcieConfig >= NumDeviceConfigs || Device >= 32 || Function >= 8) return NULL;
	PCI_DEVICE_CONFIG* DeviceConfiguration = (PCI_DEVICE_CONFIG*)(Mcfg + 1);
	DeviceConfiguration+=PcieConfig;
	PCI_CONFIGURATION_HEADER* Header = (PCI_CONFIGURATION_HEADER*)(DeviceConfiguration->BaseAddress + ((Bus + DeviceConfiguration->StartPciBusNumber) << 20) | ((Device) << 15) | (Function << 12));
	return Header;
}

BOOL KERNELAPI PcieEnumerateDevices(unsigned char PciClass, unsigned char PciSubClass, unsigned char ProgramInterface, PCI_CONFIGURATION_HEADER** PConfiguarationPtrs, unsigned int Max){
	if(!Mcfg || !PConfiguarationPtrs || !Max || PciClass == 0xFF) return FALSE;
	PCI_DEVICE_CONFIG* DeviceConfiguration = (PCI_DEVICE_CONFIG*)(Mcfg + 1);
	for(UINT16 PcieDevice = 0;PcieDevice < NumDeviceConfigs;PcieDevice++,DeviceConfiguration++){
		unsigned int StartPciBusNumber = DeviceConfiguration->StartPciBusNumber;
		unsigned int EndPciBusNumber = DeviceConfiguration->EndPciBusNumber;
		for(unsigned int Bus = StartPciBusNumber;Bus <= EndPciBusNumber;Bus++){
			if(!Max) return TRUE;
			for(unsigned char Device = 0;Device < 32;Device++){
				if(!Max) return TRUE;
				PCI_CONFIGURATION_HEADER* DeviceConfiguration = PcieConfigurationRead(PcieDevice, Bus, Device, 0);
				// Check if device exists
				unsigned char DeviceHeaderType = DeviceConfiguration->HeaderType;
				if(!DeviceConfiguration || DeviceHeaderType == 0xFF) continue;
				unsigned char NumFunctions = 8;
				if(!(DeviceHeaderType & 0x80)){
					// Device is not multi function
					NumFunctions = 1;
				}
				for(unsigned char Function = 0;Function < NumFunctions;Function++){
					if(!Max) return TRUE;
					PCI_CONFIGURATION_HEADER* PciConfig = PcieConfigurationRead(PcieDevice, Bus, Device, Function);
					
					if(!PciConfig) SET_SOD_DRIVER_ERROR;
					if(PciConfig->HeaderType == 0xFF) continue;
					if(PciConfig->DeviceClass == PciClass){
						BOOL Skip = FALSE;
						if(PciSubClass != 0xFF && PciConfig->DeviceSubclass != PciSubClass){
							Skip = TRUE;
						}
						if(ProgramInterface != 0xFF && PciConfig->ProgramInterface != ProgramInterface){
							Skip = TRUE;
						}

						if(!Skip){
							*PConfiguarationPtrs = PciConfig;
							PConfiguarationPtrs++;
							Max--;
						}
					}
				}
			}
		}
	}
	return TRUE;
}

PCI_CONFIGURATION_HEADER* KERNELAPI PcieFindDevice(UINT16 VendorId, UINT16 DeviceId){
	if(!Mcfg || VendorId == 0xFFFF) return NULL;
	PCI_DEVICE_CONFIG* DeviceConfiguration = (PCI_DEVICE_CONFIG*)(Mcfg + 1);
	for(UINT16 PcieDevice = 0;PcieDevice < NumDeviceConfigs;PcieDevice++, DeviceConfiguration++){
		unsigned int StartPciBusNumber = DeviceConfiguration->StartPciBusNumber;
		unsigned int EndPciBusNumber = DeviceConfiguration->EndPciBusNumber;
		for(unsigned int Bus = StartPciBusNumber;Bus <= EndPciBusNumber;Bus++){
			for(unsigned char Device = 0;Device < 32;Device++){
				for(unsigned char Function = 0;Function < 8;Function++){
					PCI_CONFIGURATION_HEADER* PciConfig = PcieConfigurationRead(PcieDevice, Bus, Device, Function);
					// if(!PciConfig) SET_SOD_DRIVER_ERROR;
					if(!PciConfig || PciConfig->HeaderType == 0xFF) break;
					if(PciConfig->VendorId == VendorId){
						if(DeviceId != 0xFFFF){
							if(PciConfig->DeviceId == DeviceId) return PciConfig;
						}else
							return PciConfig;
					}
				}
			}
		}
	}
	return NULL;
}

void PciExpressInit(ACPI_SDT* Sdt){
	if(Mcfg) SOD(0, "INVALID SYSTEM, MULTIPLE PCIE's (MCFG)");
	Mcfg = (ACPI_MCFG*)Sdt;
	
	NumDeviceConfigs = (Mcfg->Sdt.Length - sizeof(ACPI_MCFG)) / sizeof(PCI_DEVICE_CONFIG); // / (16) sizeof(PCI_DEVICE_CONFIG)
	PCI_DEVICE_CONFIG* DeviceConfiguration = (PCI_DEVICE_CONFIG*)(Mcfg + 1);
	
	for(UINT16 DeviceConfigurationIndex = 0; DeviceConfigurationIndex<NumDeviceConfigs; DeviceConfigurationIndex++, DeviceConfiguration++){
		UINT64 NumPages = (DeviceConfiguration->EndPciBusNumber); // << 20 >> 12
		// Check alignement
		if((NumPages & 0xfff)) {
			NumPages += 0x1000;
			NumPages &= ~(0xfff);
		}
		NumPages <<= 8; // Shift left by 20 then shift right by 12 to get num pages
		// MapPhysicalPages(kproc->PageMap,(LPVOID)DeviceConfiguration->BaseAddress,(LPVOID)DeviceConfiguration->BaseAddress, NumPages, PM_MAP | PM_WRITE_THROUGH | PM_NX);
	}
	PcieCompatible = TRUE;
}

BOOL KERNELAPI GetPcieCompatibility() {
	return PcieCompatible;
}

UINT64 GetNumPciExpressConfigurations(){
	return NumDeviceConfigs;
}
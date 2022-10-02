#include <dsk/ata.h>
#include <IO/utility.h>
#include <preos_renderer.h>
#include <CPU/cpu.h>
#include <MemoryManagement.h>
#include <cstr.h>
#include <interrupt_manager/SOD.h>
#include <dsk/disk.h>
#include <stddef.h>
#include <fs/fs.h>
// #define ATA_DEV_BUSY 0x80
// #define ATA_DEV_DRQ 0x08

// #define HBA_PxIS_TFES (1 << 30)


// #define ATA_DEV_BUSY 0x80
// #define ATA_DEV_DRQ 0x08
 
// #define	SATA_SIG_ATA	0x00000101
// #define	SATA_SIG_ATAPI	0xEB140101
// #define	SATA_SIG_SEMB	0xC33C0101
// #define	SATA_SIG_PORT_MULTIPLIER	0x96690101
 
// #define HBA_PxCMD_CR 0x8000
// #define HBA_PxCMD_FRE 0x0010
// #define HBA_PxCMD_ST 0x0001
// #define HBA_PxCMD_FR 0x4000

// #define AHCI_DEVICE_NULL 0
// #define AHCI_DEVICE_SATA 1
// #define AHCI_DEVICE_SEMB 2
// #define AHCI_DEVICE_PORT_MULTIPLIER 3
// #define AHCI_DEVICE_SATAPI 4
 
// #define HBA_PORT_IPM_ACTIVE 1
// #define HBA_PORT_DET_PRESENT 3


// // uint8_t check_port_type(struct HBA_PORT* port){
// // 	if((port->sata_status & 0x0F) != HBA_PORT_DET_PRESENT ||
// // 	((port->sata_status >> 8) & 0x0F) != HBA_PORT_IPM_ACTIVE
// // 	){
// // 		return AHCI_DEVICE_NULL;
// // 	}
	
// // 	switch(port->signature){
// // 		case SATA_SIG_ATAPI: return AHCI_DEVICE_SATAPI;
// // 		case SATA_SIG_SEMB: return AHCI_DEVICE_SEMB;
// // 		case SATA_SIG_PORT_MULTIPLIER: return AHCI_DEVICE_PORT_MULTIPLIER;
// // 		default: return AHCI_DEVICE_SATA;
// // 	}

// // 	return AHCI_DEVICE_NULL;

// // }

// // void start_port_cmd(struct HBA_PORT* port){
// // 	while(port->command_and_status & HBA_PxCMD_CR);

// // 	port->command_and_status |= HBA_PxCMD_FR;
// // 	port->command_and_status |= HBA_PxCMD_ST;
// // }

// // void stop_port_cmd(struct HBA_PORT* port){
// // 	port->command_and_status &= ~HBA_PxCMD_ST;
// // 	port->command_and_status &= ~HBA_PxCMD_FRE;

// // 	while(1){
// // 		if(port->command_and_status & HBA_PxCMD_FR 
// // 		|| port->command_and_status & HBA_PxCMD_CR){
// // 			continue;
// // 		}

// // 		break;
// // 	}
// // }

// // void configure_port(struct HBA_PORT* port, PDEVICE_INSTANCE Device){
// // 	stop_port_cmd(port);

// // 	// configure ports

// // 	UINT64 base = (UINT64)kpalloc(10);
// // 	if(!base) SET_SOD_MEMORY_MANAGEMENT;
	
// // 	port->command_list_base_addr = (UINT32)base;
// // 	port->command_list_base_addr_high = (UINT32)(base >> 32);
	
// // 	memset((void*)base, 0, 0xA000);


	
// // 	base = (UINT64)kpalloc(10);
// // 	if(!base) SET_SOD_MEMORY_MANAGEMENT;

// // 	port->fis_base_addr = (uint32_t)base;
// // 	port->fis_base_addr_high = (uint32_t)(base >> 32);

// // 	memset((void*)base, 0, 0xA000);


// // 	struct HBA_CMD_HDR* cmd_hdr = (struct HBA_CMD_HDR*)((uint64_t)port->command_list_base_addr + (((uint64_t)port->command_list_base_addr_high << 32)));
// // 	UINT64 cmd_table_addr = (UINT64)kpalloc(10);
// // 	if(!cmd_table_addr) SET_SOD_MEMORY_MANAGEMENT;
// // 	memset((void*)cmd_table_addr, 0, 0xA000);
// // 	for(UINT64 i = 0;i<32;i++){
// // 		cmd_hdr[i].cmd_table_addr = (uint32_t)(cmd_table_addr + (i << 8));
// // 		cmd_hdr[i].cmd_table_high = (uint32_t)((cmd_table_addr + (i << 8)) >> 32);

// // 	}
// // 	start_port_cmd(port);
// // }

// // KERNELSTATUS KERNELAPI AtaApiRead(DISK_DEVICE_INSTANCE* Disk, UINT64 SectorOffset, UINT64 NumSectors, void* Buffer) {
// // 	RFPROCESS Process = GetCurrentProcess();
// // 	if (Disk->DiskInformation.MaxAddress < (SectorOffset + NumSectors)) return KERNEL_SERR_OUT_OF_RANGE;
// // 	return AtaRead(Disk->Device->PhysicalMmio, SectorOffset, NumSectors, Buffer);
// // }

// // KERNELSTATUS KERNELAPI AtaApiWrite(DISK_DEVICE_INSTANCE* Disk, UINT64 SectorOffset, UINT64 NumSectors, void* Buffer) {
// // 	RFPROCESS Process = GetCurrentProcess();
// // 	if (Disk->DiskInformation.MaxAddress < (SectorOffset + NumSectors)) return KERNEL_SERR_OUT_OF_RANGE;
// // 	return AtaWrite(Disk->Device->PhysicalMmio, SectorOffset, NumSectors, Buffer);
// // }

// // void probe_ports(struct HBA_MEM* abar, PDEVICE_INSTANCE Controller){

// // 	uint32_t pi = abar->port_implemented;

// // 	uint8_t port_type = 0;
// // 	for(uint8_t i = 0;i<32;i++){
// // 		if(pi & 1){
// // 				port_type = check_port_type(&abar->ports[i]);
// // 				switch(port_type){
// // 					case AHCI_DEVICE_NULL:{
// // 						break;
// // 					}
// // 					case AHCI_DEVICE_SATA:
// // 					case AHCI_DEVICE_SATAPI:
// // 					{
// // 						RFDEVICE_INSTANCE Device = RegisterDeviceInstance(Controller, DEVICE_CLASS_MASS_STORAGE_CONTROLLER, DEVICE_TYPE_DISK_ATA, L"ATA/ATAPI Disk Drive", DEVICE_SOURCE_PCI, &abar->ports[i]);
// // 						DeclareDeviceControl(Device);
// // 						DISK_INFO DiskInfo = { 0 };
// // 						DiskInfo.MaxAddress = 0xFFFFFFFFFFFFFFFF;
						
// // 						DISK_DEVICE_INSTANCE* Disk = CreateDisk(NULL, DISK_DEFAULT_SECURITY_DESCRIPTOR, &DiskInfo, Device);
// // 						if (!Device || !Disk) SET_SOD_MEDIA_MANAGEMENT;
// // 						DISK_MANAGEMENT_INTERFACE* Interface = kmalloc(sizeof(DISK_MANAGEMENT_INTERFACE));
// // 						HANDLE InterfaceHandle = OpenHandle(kproc->Handles, NULL, HANDLE_FLAG_FREE_ON_EXIT, 0, Interface, NULL);
// // 						if (!InterfaceHandle) SET_SOD_MEDIA_MANAGEMENT;
// // 						Interface->Read = AtaApiRead;
// // 						Interface->Write = AtaApiWrite;
						
// // 						if (!SetDeviceInterface(Device, InterfaceHandle)) SET_SOD_MEDIA_MANAGEMENT;
// // 						configure_port(&abar->ports[i], Device);
// // 						FsMountDevice(Device);
// // 						break;
// // 					}
// // 					case AHCI_DEVICE_SEMB:{
// // 						break;
// // 					}
// // 					case AHCI_DEVICE_PORT_MULTIPLIER:{
// // 						break;
// // 					}
// // 				}
// // 		}
// // 		pi >>= 1;
// // 	}
// // }

// // void AtaDeviceInit(PCI_CONFIGURATION_HEADER* Header, RFDEVICE_INSTANCE Device){
// 	// if (KERNEL_ERROR(DeclareDeviceControl(Device))) SET_SOD_MEDIA_MANAGEMENT;
// 	// UINT64 Bar0 = Header->BaseAddress0;
// 	// UINT64 Bar1 = Header->BaseAddress1;
// 	// UINT64 Bar2 = Header->BaseAddress2;
// 	// UINT64 Bar3 = Header->BaseAddress3;
// 	// UINT64 Bar4 = Header->BaseAddress4;
// 	// UINT64 Bar5 = Header->BaseAddress5;

// 	// SetDeviceArrayBaseAddresses(Device,
// 	// 	(LPVOID)Bar0, (LPVOID)Bar1, (LPVOID)Bar2, (LPVOID)Bar3,
// 	// 	(LPVOID)Bar4, (LPVOID)Bar5,
// 	// 	NULL
// 	// );

// 	// struct HBA_MEM* abar = (struct HBA_MEM*)((UINT64)Header->BaseAddress5);
// 	// MapPhysicalPages(kproc->PageMap,(LPVOID)abar,(LPVOID)abar,10, PM_MAP | PM_NX);
// 	// probe_ports(abar, Device);
// // }


// int find_command_slot(struct HBA_PORT* port){
// 	uint32_t slots = (port->sata_active | port->command_issue);
// 	for(uint32_t i = 0;i<31;i++){
// 		if((slots & 1) == 0) {
// 			return i;
// 		}
// 		slots >>= 1;
// 	}
// 	return -1;
// }
// int AtaRead(struct HBA_PORT* port, UINT64 SectorOffset, UINT64 SectorCount, void* buffer){
	
// 	uint32_t sector_low = (uint32_t)SectorOffset;
// 	uint32_t sector_high = (uint32_t)((UINT64)SectorOffset >> 32);
	
// 	port->interrupt_status = (uint32_t)(-1);
// 	int slot = find_command_slot(port);
// 	if(slot == -1) return -2;

// 	struct HBA_CMD_HDR* cmd_hdr = (struct HBA_CMD_HDR*)((uint64_t)port->command_list_base_addr | ((uint64_t)port->command_list_base_addr_high << 32));
// 	cmd_hdr+=slot;
// 	cmd_hdr->command_fis_length = sizeof(struct FIS_REG_HOST_TO_DEVICE)/4;
// 	cmd_hdr->write = 0;
// 	cmd_hdr->prdt_length = 1;
// 	struct HBA_CMD_TABLE* cmd_table = (struct HBA_CMD_TABLE*)((UINT64)cmd_hdr->cmd_table_addr | ((UINT64)cmd_hdr->cmd_table_high << 32));
// 	ZeroMemory(cmd_table, sizeof(*cmd_table) + (cmd_hdr->prdt_length * sizeof(struct HBA_PHYS_DESC_TABLE_ENTRY)));
// 	cmd_table->prdt_entry[0].data_base_addr = (uint32_t)(uint64_t)buffer; 
// 	cmd_table->prdt_entry[0].data_base_addr_high = (uint32_t)((uint64_t)buffer >> 32);
// 	cmd_table->prdt_entry[0].byte_count = (SectorCount << 9) - 1; //(8*1024-1)  always set to 1 less than the current value
// 	cmd_table->prdt_entry[0].interrupt_on_completion = 1;

// 	struct FIS_REG_HOST_TO_DEVICE* command_fis = (struct FIS_REG_HOST_TO_DEVICE*)(&cmd_table->command_fis);
// 	command_fis->fis_type = FIS_TYPE_REG_HOST_TO_DEVICE;
// 	command_fis->command_control = 1;
// 	command_fis->command = ATA_CMD_READ_DMA_EX;

// 	command_fis->lba0 = (uint8_t)sector_low;
// 	command_fis->lba1 = (uint8_t)(sector_low >> 8);
// 	command_fis->lba2 = (uint8_t)(sector_low >> 16);


// 	command_fis->lba3 = (uint8_t)(sector_high);
// 	command_fis->lba4 = (uint8_t)(sector_high >> 8);
// 	command_fis->lba5 = (uint8_t)(sector_high >> 16);
// 	command_fis->device = 1 << 6; // LBA MODE
// 	command_fis->count_low = 1;

// 	while((port->task_file_data & (ATA_DEV_BUSY | ATA_DEV_DRQ)));


// 	port->command_and_status = -1; // reset command and status
// 	port->command_issue = 1;

// 	while(1){
// 		if(!port->command_issue) break; // the command has been completed		
// 		 if(port->interrupt_status & HBA_PxIS_TFES){ // task file error
// 		 	return -1;
// 		}
// 	}

// 	return SUCCESS;
// }

// int AtaWrite(struct HBA_PORT* port, UINT64 SectorOffset, UINT64 SectorCount, void* buffer){
// 	uint32_t sector_low = (uint32_t)SectorOffset;
// 	uint32_t sector_high = (uint32_t)(SectorOffset >> 32);
	
// 	port->interrupt_status = (uint32_t)(-1); // reset interrupt status
// 	int slot = find_command_slot(port);
// 	if(slot == -1) return -2;

// 	struct HBA_CMD_HDR* cmd_hdr = (struct HBA_CMD_HDR*)((uint64_t)port->command_list_base_addr | ((uint64_t)port->command_list_base_addr_high << 32));
// 	cmd_hdr+=slot;
// 	cmd_hdr->command_fis_length = sizeof(struct FIS_REG_HOST_TO_DEVICE)/4;
// 	cmd_hdr->write = 0;
// 	cmd_hdr->prdt_length = 1;
// 	struct HBA_CMD_TABLE* cmd_table = (struct HBA_CMD_TABLE*)((uint64_t)cmd_hdr->cmd_table_addr | ((uint64_t)cmd_hdr->cmd_table_high << 32));
// 	ZeroMemory(cmd_table, sizeof(*cmd_table) + (cmd_hdr->prdt_length * sizeof(struct HBA_PHYS_DESC_TABLE_ENTRY)));
	
// 	 	cmd_table->prdt_entry[0].data_base_addr = (uint32_t)(uint64_t)buffer; 
// 	 	cmd_table->prdt_entry[0].data_base_addr_high = (uint32_t)((uint64_t)buffer >> 32);
// 		cmd_table->prdt_entry[0].byte_count = (SectorCount << 9) - 1; //(8*1024-1)  always set to 1 less than the current value
// 	 	cmd_table->prdt_entry[0].interrupt_on_completion = 1;

// 	struct FIS_REG_HOST_TO_DEVICE* command_fis = (struct FIS_REG_HOST_TO_DEVICE*)(&cmd_table->command_fis);
// 	command_fis->fis_type = FIS_TYPE_REG_HOST_TO_DEVICE;
// 	command_fis->command_control = 1;
// 	command_fis->command = ATA_CMD_WRITE_DMA_EX;

// 	command_fis->lba0 = (uint8_t)sector_low;
// 	command_fis->lba1 = (uint8_t)(sector_low >> 8);
// 	command_fis->lba2 = (uint8_t)(sector_low >> 16);


// 	command_fis->lba3 = (uint8_t)(sector_high);
// 	command_fis->lba4 = (uint8_t)(sector_high >> 8);
// 	command_fis->lba5 = (uint8_t)(sector_high >> 16);
// 	command_fis->device = 1 << 6; // LBA MODE
// 	command_fis->count_low = 1;

// 	while((port->task_file_data & (ATA_DEV_BUSY | ATA_DEV_DRQ)));


// 	port->command_and_status = -1; // reset command and status
// 	port->command_issue = 1;

// 	while(1){
// 		if(!port->command_issue) break; // the command has been completed		
// 		 if(port->interrupt_status & HBA_PxIS_TFES){ // task file error
// 		 	return -1;
// 		}
// 	}

// 	return SUCCESS;
// }
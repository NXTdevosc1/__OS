#pragma once
#include <stdint.h>
#include <IO/pcie.h>
#include <Management/device.h>

#pragma pack(push, 1)

enum ATA_CMD{
	ATA_CMD_READ_DMA_EX = 0x25,
	ATA_CMD_WRITE_DMA_EX = 0x35,
	ATA_CMD_MEDIA_EJECT = 0xED
};

enum FIS_TYPE
{
	FIS_TYPE_REG_HOST_TO_DEVICE = 0x27,
	FIS_TYPE_DEVICE_TO_HOST = 0x34,
	FIS_TYPE_DMA_ACT = 0x39,
	FIS_TYPE_DMA_SETUP = 0x41,
	FIS_TYPE_DATA = 0x46,
	FIS_TYPE_BIST = 0x58,
	FIS_TYPE_PIO_SETUP = 0x5F,
	FIS_TYPE_DEVICE_BITS = 0xA1
};

struct HBA_PORT{
	uint32_t command_list_base_addr; // 1k alligned
	uint32_t command_list_base_addr_high;
	uint32_t fis_base_addr; // 256 alligned
	uint32_t fis_base_addr_high;
	uint32_t interrupt_status;
	uint32_t interrupt_enable;
	uint32_t command_and_status;
	uint32_t reserved0;
	uint32_t task_file_data;
	uint32_t signature;
	uint32_t sata_status;
	uint32_t sata_control;
	uint32_t sata_error;
	uint32_t sata_active;
	uint32_t command_issue;
	uint32_t sata_notification;
	uint32_t fis_based_switch_control;
	uint32_t reserved1[11];
	uint32_t vendor[4];
};

struct FIS_REG_HOST_TO_DEVICE{
	uint8_t fis_type; // REG_H2D
	uint8_t port_multiplier:4;
	uint8_t reserved0:3;
	uint8_t command_control : 1; // Command : 1, otherwise Control
	uint8_t command; // command register
	uint8_t feature_low; // Feature Reg 0-7
	uint8_t lba0; // LBA 0-7
	uint8_t lba1; // LBA 8-15
	uint8_t lba2; // LBA 16-23
	uint8_t device;
	uint8_t lba3; // LBA 24-31
	uint8_t lba4; // LBA 32-39
	uint8_t lba5; // LBA 40-47
	uint8_t feature_high; // Feature Reg 8-15
	uint8_t count_low; // 0-7
	uint8_t count_high; // 8-15
	uint8_t isochronous_command_completion;
	uint8_t control; // Control Register
	uint8_t reserved1[4];
};

struct FIS_REG_DEVICE_TO_HOST{
	uint8_t fis_type; // FIS_D2H
	uint8_t port_multiplier : 4;
	uint8_t reserved0 : 2;
	uint8_t interrupt : 1;
	uint8_t reserved1 : 1;

	uint8_t status;
	uint8_t error;

	uint8_t lba0; // LBA 0-7
	uint8_t lba1; // LBA 8-15
	uint8_t lba2; // LBA 16-23
	uint8_t device;

	uint8_t lba3; // LBA 24-31
	uint8_t lba4; // LBA 32-39
	uint8_t lba5; // LBA 40-47

	uint8_t count_low; // 0-7
	uint8_t count_high; // 8-15
	uint8_t reserved3[2];
	uint8_t reserved4[4];
};

struct FIS_DATA {
	uint8_t fis_type; // FIS_DATA
	uint8_t port_multiplier : 4;
	uint8_t reserved0 : 4;
	uint8_t reserved1[2];
	uint32_t data[1]; // payload
};

struct FIS_PIO_SETUP{
	uint8_t fis_type; // FIS_PIO_SETUP;
	uint8_t port_multiplier : 4;
	uint8_t reserved0 : 1;
	uint8_t data_transfer_direction : 1; // 1 - device to host
	uint8_t interrupt : 1;
	uint8_t reserved1 : 1;

	uint8_t status;
	uint8_t error;

	uint8_t lba0, lba1, lba2, device, lba3, lba4, lba5, reserved2;

	uint8_t count_low;
	uint8_t count_high;
	uint8_t reserved3;
	uint8_t e_status;
	uint16_t transfer_count;
	uint8_t reserved4[2];
};

struct FIS_DMA_SETUP{
	uint8_t fis_type;
	uint8_t port_multiplier : 4;
	uint8_t reserved0 : 1;
	uint8_t data_transfer_direction : 1;
	uint8_t interrupt : 1;
	uint8_t auto_activate : 1; // Specifies if DMA Activate FIS is needed

	uint8_t reserved[2];

	uint64_t dma_buffer_id; // dma buffer address in host memory

	uint32_t reserved1;
	uint32_t dma_buffer_offset; // byte offset in buffer, first 2 bits must be 0
	uint32_t transfer_count;  // number of bytes to transfer, bit 0 must be 0
	uint32_t reserved2;
};



struct HBA_MEM {
	uint32_t host_capability;
	uint32_t global_host_control;
	uint32_t interrupt_status;
	uint32_t port_implemented;
	uint32_t version;
	uint32_t command_completion_coalescing_control;
	uint32_t command_completion_coalescing_ports;
	uint32_t enclosure_management_location;
	uint32_t enclosure_management_control;
	uint32_t extended_host_capabilities;
	uint32_t bohc; // BIOS/OS handoff control and status
	uint8_t reserved[0x74];
	uint8_t vendor[0x60];
	struct HBA_PORT ports[1];
};

struct HBA_FIS{
	struct FIS_DMA_SETUP dms_fis;
	uint8_t pad0[4];
	struct FIS_PIO_SETUP ps_fis;
	uint8_t pad1[12];
	struct FIS_REG_DEVICE_TO_HOST rfis;
	uint8_t pad2[4];
	uint64_t set_device_bit_fis;
	uint8_t unused_fis[64];
	uint8_t reserved[0x100-0xA0];
};

struct HBA_CMD_HDR{
	uint8_t command_fis_length : 5;
	uint8_t atapi : 1;
	uint8_t write : 1; // 1 = host to device, 0 = device to host
	uint8_t prefetchable : 1;
	uint8_t reset : 1;
	uint8_t bist:1;
	uint8_t clear_busy : 1;
	uint8_t reserved0 : 1;
	uint8_t port_multiplier : 4;
	uint16_t prdt_length;
	uint32_t prd_byte_count;
	uint32_t cmd_table_addr;
	uint32_t cmd_table_high;
	uint32_t reserved1[4];
};

struct HBA_PHYS_DESC_TABLE_ENTRY {
	uint32_t data_base_addr;
	uint32_t data_base_addr_high;
	uint32_t reserved0;
	uint32_t byte_count : 22;
	uint32_t reserved1 : 9;
	uint32_t interrupt_on_completion : 1;
};

struct HBA_CMD_TABLE{
	uint8_t command_fis[64];
	uint8_t atapi_cmd[16];
	uint8_t reserved[48];
	struct HBA_PHYS_DESC_TABLE_ENTRY prdt_entry[]; //  Physical region descriptor table entries, up to 0xffff
};

struct ATA_DEVICE{
	uint8_t port_length;
	struct HBA_PORT* availaible_ports[32];
};

#pragma pack(pop)



// void AtaDeviceInit(PCI_CONFIGURATION_HEADER* PciHeader, RFDEVICE_INSTANCE Device);

// int AtaRead(struct HBA_PORT* port, UINT64 SectorOffset, UINT64 SectorCount, void* buffer);
// int AtaWrite(struct HBA_PORT* port, UINT64 SectorOffset, UINT64 SectorCount, void* buffer);

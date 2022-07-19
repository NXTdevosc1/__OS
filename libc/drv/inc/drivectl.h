#pragma once

#include <kerneltypes.h>
#include <krnlapi.h>
#include <ddk.h>

#ifndef _IMPL_FILE_ATTRIBUTES

enum FILE_ATTRIBUTES {
	FILE_ATTRIBUTE_READ_ONLY = 1,
	FILE_ATTRIBUTE_HIDDEN = 2,
	FILE_ATTRIBUTE_SYSTEM_FILE = 4,
	FILE_ATTRIBUTE_DIRECTORY = 8
};

#define _IMPL_FILE_ATTRIBUTES

#endif


enum _DISK_SECURITY_DESCRIPTOR {
	DISK_READ = 1,
	DISK_WRITE = 2,
	DISK_SHOW_INSTANCE = 4, // Show presence of the disk to the user
	DISK_VIRTUAL = 8, // Means a virtual partition or disk like (VFS)
	DISK_UNMOUNTABLE = 0x10,
	DISK_DEFAULT_SECURITY_DESCRIPTOR = DISK_READ | DISK_WRITE | DISK_SHOW_INSTANCE,
	PARTITION_DEFAULT_SECURITY_DESCRIPTOR = DISK_READ | DISK_WRITE | DISK_SHOW_INSTANCE,
	DISK_MAIN = 0x20,  // Main partition / Disk (Only set by system)
	DISK_SYSTEM = 0x40 // e.g. Efi System Partition / Disk
};

typedef void* DRIVE_INSTANCE;

/* DRIVE IMPLEMENTATIONS */

typedef DDKSTATUS(__KERNELAPI* RAW_DRIVE_READ)(RFDEVICE_OBJECT Device, UINT64 Address, UINT64 NumBytes, void* Buffer);
typedef DDKSTATUS(__KERNELAPI* RAW_DRIVE_WRITE)(RFDEVICE_OBJECT Device, UINT64 Address, UINT64 NumBytes, void* Buffer);


typedef struct _DRIVE_INFO {
	UINT64 BaseAddress; // Set to 0 for disks, used in partitions
	UINT64 MaxAddress; // the size of the disk/partition
	UINT64 BytesInCluster; // Used on partitions
	UINT64 SectorsInCluster; // Used on partitions
	UINT64 FsType; // Used on partitions
	UINT16 DriveName[0x100]; // Or Partition Name
} DRIVE_INFO;

typedef struct _DRIVE_MANAGEMENT_INTERFACE {
    // Interface Options
    UINT64 InterfaceFlags;
    // Interface facilities
    /* Max Read/Write will make no need for the driver to recall or complicate it's routine
     * When NumBytes > MaxReadBytes the DriveController just recalls the routine
     * With Address += MaxReadBytes, And NumBytes -= MaxReadBytes
     * ---
     * This is used on the AHCI Driver as well as other ATA Drives Drivers on the system
     */
    UINT64 MaxReadBytes;
    UINT64 MaxWriteBytes;
    // Interface Routines
	RAW_DRIVE_READ Read;
	RAW_DRIVE_WRITE Write;
} DRIVE_MANAGEMENT_INTERFACE;


/* Drive must support Read & Write operations
*  Management IF Will parse filesystems on the drive, then load it's drivers
*/
DRIVE_INSTANCE DDKAPI InstallDrive(RFDEVICE_OBJECT Device, DWORD DriveFlags, SECURITY_DESCRIPTOR SecurityDescriptor, DRIVE_INFO* DriveInformation);

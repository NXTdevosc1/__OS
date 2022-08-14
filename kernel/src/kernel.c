
#include <kernel.h>
#include <typography/typography.h>
#include <cstr.h>
#include <cpu/cpu.h>
#include <IO/utility.h>
#include <interrupt_manager/idt.h>
#include <preos_renderer.h>
#include <MemoryManagement.h>
#include <dsk/ata.h>
#include <math.h>
#include <acpi/acpi.h>
#include <IO/pcie.h>
#include <interrupt_manager/SOD.h>
#include <fs/fs.h>
#include <acpi/madt.h>
#include <sysentry/sysentry.h>
#include <sys/sys.h>
#include <Management/handle.h>
#include <Management/device.h>
#include <Management/lock.h>
#include <ipc/ipc.h>
#include <smbios/smbios.h>
#include <cmos.h>
#include <timedate/rtc.h>
#include <Management/debug.h>
#include <ipc/ipcserver.h>
#include <sys/KeImplementation.h>
#include <CPU/pcid.h>
#include <loaders/pe64.h>
#include <kinit.h>
#include <stdlib.h>
#include <CPU/ioapic.h>
#include <input/mouse.h>
#include <input/keyboard.h>
#include <IO/pit.h>
#include <sys/drv.h>
#include <sys/bootconfig.h>
#include <IO/pci.h>
#include <acpi/hpet.h>
uint8_t set = 0;



__DECLARE_FLT;

uint8_t kproc_set = 0;
struct{
	uint8_t acpi_set;
	uint8_t system_management_bios_set;
} SYSTEM_TABLES_SETMAP = { 0 };

CRITICAL_LOCK TestLock = { 0 };

RFPROCESS kproc = NULL;
RFPROCESS IdleProcess = NULL;
RFPROCESS SystemInterruptsProcess = NULL;
UINT64 KeGlobalCR3 = 0;

HTHREAD ThAnother = NULL;

HANDLE_TABLE gSystemHandleTable = {0};

RFSERVER KernelServer = NULL;

PROCESS IoSpaceMemoryProcess = {0}; // This is a (non) present process that contains the heap of the IoSpace
THREAD IoSpaceMemoryThread = {0};

__declspec(allocate(_FIMPORT)) FILE_IMPORT_ENTRY FileImportTable[0x20] = {
	{FILE_IMPORT_DATA, 0, NULL, L"$BOOTCONFIG", 0, L"OS\\System\\KeConfig\\$BOOTCONFIG"},
	{FILE_IMPORT_DATA, 0, NULL, L"$DRVTBL", 0, L"OS\\System\\KeConfig\\$DRVTBL"},
	{FILE_IMPORT_DLL, 0, NULL, L"osdll.dll", 0, L"OS\\System\\osdll.dll"},
	{FILE_IMPORT_DLL, 0, NULL, L"ddk.dll", 0, L"OS\\System\\ddk.dll"},
	// {FILE_IMPORT_DRIVER, 0, NULL, L"basicdispay.sys", 0, L"OS\\System\\basicdisplay.sys"},
	// {FILE_IMPORT_DRIVER, 0, NULL, L"ata.sys", 0, L"OS\\System\\ata.sys"},
	// {FILE_IMPORT_DRIVER, 0, NULL, L"usb.sys", 0, L"OS\\System\\usb.sys"},
	{FILE_IMPORT_DEVICE_DRIVER, 0, NULL, L"ehci.sys", 0, L"OS\\System\\ehci.sys"},
	{FILE_IMPORT_DEVICE_DRIVER, 0, NULL, L"ahci.sys", 0, L"OS\\System\\ahci.sys"},
	{FILE_IMPORT_DEVICE_DRIVER, 0, NULL, L"xhci.sys", 0, L"OS\\System\\xhci.sys"},
	// {FILE_IMPORT_DRIVER, 0, NULL, L"eodx.sys", 0, L"OS\\System\\eodx.sys"},
	{0}, // End of table
};
void KERNELAPI IdleThread() {
	for(;;) {
		__hlt();
	}
}



LPWSTR KernelProcessName = L"System Kernel.";

extern void __declspec(noreturn) _start() {
	__cli();
	EnableExtendedStates();
	
	


	Pmgrt.NumProcessors = 1;

	if (!InitializeRuntimeSymbols()) SET_SOD_INITIALIZATION;
	kproc = KeCreateProcess(NULL,KernelProcessName, SUBSYSTEM_NATIVE, KERNELMODE_PROCESS);
	if(!kproc) SET_SOD_INITIALIZATION;

	

		#ifdef ___KERNEL_DEBUG___
			DebugWrite("Kernel Process & Runtime Symbols Initialized.");
		#endif

	
	// Creating Free Entries for Conventionnal Memory
	

	#ifdef ___KERNEL_DEBUG___
			DebugWrite("Memory Heaps initialized.");
		#endif
	
	ConfigureSystemSpace();
	// Setup I/O Memory process & thread
	
	if(!CreateMemoryTable(&IoSpaceMemoryProcess, &IoSpaceMemoryProcess.MemoryManagementTable)) SET_SOD_MEMORY_MANAGEMENT;
	AllocateFreeHeapDescriptor(&IoSpaceMemoryProcess, NULL, (LPVOID)((UINT64)SystemSpaceBase + SYSTEM_SPACE48_IO), ((UINT64)SystemSpaceBase + SYSTEM_SPACE48_RAM) - ((UINT64)SystemSpaceBase + SYSTEM_SPACE48_IO));
	IoSpaceMemoryThread.Process = &IoSpaceMemoryProcess;

	KernelHeapInitialize();
	// Initialize Kernel Page tables
	KernelPagingInitialize();

	

	#ifdef ___KERNEL_DEBUG___
			DebugWrite("Memory Successfully Mapped. Initializing Descriptor Tables...");
		#endif
	void* CpuBuffer = NULL;
	UINT64 CpuBufferSize = 0;
	
	GlobalInterruptDescriptorInitialize();
	GlobalSysEntryTableInitialize();


	InitProcessorDescriptors(&CpuBuffer, &CpuBufferSize);
	
	ProcessContextIdAllocate(kproc);
	KeGlobalCR3 = (UINT64)kproc->PageMap;



	
	__setCR3((unsigned long long)kproc->PageMap);


	GP_clear_screen(0x1774EA);
	while(1) __hlt();

	

#ifdef ___KERNEL_DEBUG___
			DebugWrite("CR3 Set & Descriptor tables initialized");
		#endif
	kproc->Handles = CreateHandleTable();
	kproc->FileHandles = CreateHandleTable();

	if (!kproc->Handles || !kproc->FileHandles) SET_SOD_INITIALIZATION;

	

	IdleProcess = KeCreateProcess(kproc, L"System Idle Process.", 0, KERNELMODE_PROCESS);

	if (!IdleProcess) SET_SOD_PROCESS_MANAGEMENT;

	SystemInterruptsProcess = KeCreateProcess(kproc, L"System Interrupts.", 0, KERNELMODE_PROCESS);

   // Setting up system tables
	
	GP_clear_screen(0);

	// Enable NMI
	OutPortB(0x70, InPortB(0x70) & 0x7F);
    InPortB(0x71);

	// Enable Parity/Channel Check
	UINT8 ControlPortB = InPortB(SYSTEM_CONTROL_PORT_B);
	ControlPortB |= (3 << 2);
	OutPortB(SYSTEM_CONTROL_PORT_B, ControlPortB);
	
	
	

	#ifdef ___KERNEL_DEBUG___
			DebugWrite("Initializing System Tables...");
		#endif

	
	
	if(InitData.uefi) {
		for(UINT64 i = 0;i<InitData.NumConfigurationTables;i++){
			MapPhysicalPages(kproc->PageMap, 
			(void*)InitData.SystemConfigurationTables[i].VendorTable,
			(void*)InitData.SystemConfigurationTables[i].VendorTable,
			1,
			PM_PRESENT | PM_NX
			);
			
			if(
				!SYSTEM_TABLES_SETMAP.acpi_set &&
				memcmp(InitData.SystemConfigurationTables[i].VendorTable,"RSD PTR ",8) &&
				((ACPI_RSDP_DESCRIPTOR*)(InitData.SystemConfigurationTables[i].VendorTable))->Revision == 2
			){ // Seach for ACPI 2.0 or higher
			#ifdef ___KERNEL_DEBUG___
				DebugWrite("Initializing ACPI...");
				DebugWrite(to_hstring64((UINT64)InitData.SystemConfigurationTables[i].VendorTable));
			#endif
				AcpiInit(InitData.SystemConfigurationTables[i].VendorTable);
				SYSTEM_TABLES_SETMAP.acpi_set = 1;
			}else if( !SYSTEM_TABLES_SETMAP.system_management_bios_set &&
			memcmp(InitData.SystemConfigurationTables[i].VendorTable, SMBIOS_SIGNATURE, SMBIOS_SIGNATURE_LENGTH)
			){
				#ifdef ___KERNEL_DEBUG___
				DebugWrite("Initializing SMBIOS At Address :");
				DebugWrite(to_hstring64((UINT64)InitData.SystemConfigurationTables[i].VendorTable));
			#endif
				SystemManagementBiosInitialize(InitData.SystemConfigurationTables[i].VendorTable);
				SYSTEM_TABLES_SETMAP.system_management_bios_set = 1;
			}
		}
	}else {
		// Search using Legacy BIOS Method
		for(char* i = (char*)0xE0000; i<(char*)0x100000;i++) {
			if(!SYSTEM_TABLES_SETMAP.acpi_set && 
			memcmp(i, "RSD PTR ", 8)
			) {
				AcpiInit(i);
				SYSTEM_TABLES_SETMAP.acpi_set = 1;
			} else if(!SYSTEM_TABLES_SETMAP.system_management_bios_set && 
			memcmp(i, SMBIOS_SIGNATURE, SMBIOS_SIGNATURE_LENGTH)) {
				SystemManagementBiosInitialize(i);
				SYSTEM_TABLES_SETMAP.system_management_bios_set = 1;
			}
		}
	}


	

	
	EnableApic();

	// if(!IsOemLogoDrawn())
	// 	DrawOsLogo();
		

	#ifdef ___KERNEL_DEBUG___
			DebugWrite("System Tables Successfully Initialized.");
		#endif
	


	UINT32 NumProcessors = AcpiGetNumProcessors();
	CpuSetupManagementTable(NumProcessors);
	InitSystemSpace(kproc);
	SetPriorityClass(kproc, PRIORITY_CLASS_REALTIME);
	HTHREAD KernelThread = KeCreateThread(kproc, 0, NULL, 0, NULL);
	if (!KernelThread) SET_SOD_INITIALIZATION;

	SetThreadPriority(KernelThread, THREAD_PRIORITY_BELOW_NORMAL);
	
	KernelThread->ProcessorId = GetCurrentProcessorId(); // Only Bootstrap Processor can run kernel

	CpuManagementTable[KernelThread->ProcessorId]->Initialized = TRUE;
	CpuManagementTable[KernelThread->ProcessorId]->CurrentThread = KernelThread;
	CpuManagementTable[KernelThread->ProcessorId]->CpuBuffer = CpuBuffer;
	CpuManagementTable[KernelThread->ProcessorId]->CpuBufferSize = CpuBufferSize;
	
#ifdef ___KERNEL_DEBUG___
			DebugWrite("Initializing Features...");
		#endif
	
InitFeatures();
// Reload CR3
__setCR3((UINT64)kproc->PageMap);

#ifdef ___KERNEL_DEBUG___
			DebugWrite("Setting Up LAPIC Timer...");
		#endif
	
	
	#ifdef ___KERNEL_DEBUG___
			DebugWrite("Initializing Multi Processors");
		#endif

	// Parse Boot Configuration
	RFBOOT_CONFIGURATION BootConfiguration = FileImportTable[FIMPORT_BOOT_CONFIG].LoadedFileBuffer;
	// SystemDebugPrint(L"BOOT_CONFIG : KERNEL(%d.%d), OS(%d.%d), SIGNATURE : %ls, EOHOFF : %x, ENDOFHDR : %ls", BootConfiguration->Header.MajorKernelVersion, BootConfiguration->Header.MinorKernelVersion, BootConfiguration->Header.MajorOsVersion, BootConfiguration->Header.MinorOsVersion, BootConfiguration->Header.Signature, (UINT64)&BootConfiguration->Header.HeaderEnd - (UINT64)BootConfiguration, BootConfiguration->Header.HeaderEnd);
	if(!memcmp(BootConfiguration->Header.Signature, BOOTCONFIG_SIGNATURE, 28) ||
	BootConfiguration->Header.MajorKernelVersion != MAJOR_KERNEL_VERSION ||
	BootConfiguration->Header.MinorKernelVersion != MINOR_KERNEL_VERSION ||
	!memcmp(BootConfiguration->Header.HeaderEnd, KEHEADER_END, 20) ||
	BootConfiguration->BootMode > BOOT_MODE_MAX
	) {
		SOD(SOD_INITIALIZATION, "INVALID_BOOT_CONFIGURATION");
	}



	HpetConfigure();

	SetupLocalApicTimer();

	// TaskSchedulerDisable();

	if(!BootConfiguration->DisableMultiProcessors){
		Pmgrt.NumProcessors = NumProcessors;
		MapPhysicalPages(kproc->PageMap, SMP_BOOT_ADDR, SMP_BOOT_ADDR, 1, PM_MAP);
		UINT64 SmpCodeSize = (UINT64)&SMP_TRAMPOLINE_END - (UINT64)SMP_TRAMPOLINE;
		memcpy(SMP_BOOT_ADDR, SMP_TRAMPOLINE, SmpCodeSize);

		__wbinvd();
		for (UINT64 i = 0; i < NumProcessors; i++) {
			if (i == KernelThread->ProcessorId) continue; // the bootstrap processor
			if(KERNEL_ERROR(InitializeApicCpu(i)))
			{
				SOD(SOD_PROCESSOR_INITIALIZATION, "PROCESSOR INITIALIZATION");
			}
		}
	}else{
		Pmgrt.NumProcessors = 1;
	}


	
	#ifdef ___KERNEL_DEBUG___
	DebugWrite("System Initialized. Creating Kernel IPC Server");	
	#endif


	

	if(KERNEL_ERROR(IpcServerCreate(KernelThread, IPC_MAKEADDRESS(0, 0, 0, 1), NULL, 0, &KernelServer)))
		SET_SOD_INITIALIZATION;

	KeResumeThread(KernelThread);
	#ifdef ___KERNEL_DEBUG___
	DebugWrite("Enabling Task Scheduler & __sti()");	
	#endif



	MapPhysicalPages(kproc->PageMap, 0, 0, 1, 0 /*Unmap first page (temporarely to prevent bugs and write to address 0*/);

	
	
	
	

	// Setting Up ACPI SCI_INT
	// if(KeControlIrq(AcpiSystemControlInterruptHandler, AcpiGetFadt()->SCI_Interrupt, IRQ_DELIVERY_NORMAL, IRQ_CONTROL_DISABLE_OVERRIDE_FLAGS | IRQ_CONTROL_LOW_ACTIVE | IRQ_CONTROL_LEVEL_SENSITIVE) != KERNEL_SOK) SET_SOD_INITIALIZATION;
	// init_ps2_keyboard();
	// init_ps2_mouse();
	RtcInit();
	

	
	// __sti();
	// UINT64 LastCs = 0;
	// for(;;) {
	// 	SystemDebugPrint(L"CS/S : %x, CS_LATENCY : %x", CpuManagementTable[0]->TotalClocks[0] - LastCs, CpuManagementTable[0]->LastThreadSwitchLatency[0]);
	// 	LastCs = CpuManagementTable[0]->TotalClocks[0];
	// 	// SystemDebugPrint(L"Context Switches : %x:%x , Time : %x , CS Latency : %x, OF : %x", CpuManagementTable[0]->TotalClocks[0], CpuManagementTable[0]->TotalClocks[1], CpuManagementTable[0]->HighPrecisionTime[0], (CpuManagementTable[0]->LastThreadSwitchLatency[0] * GetHighPerformanceTimerFrequency()) / 1000000, (UINT64)((UINT64)CpuManagementTable[0]->LastThreadSwitchLatency - (UINT64)CpuManagementTable[0]));
	// 	// for(int c = 0;c<0x100000;c++);
	// 	Sleep(1000);
	// }

	// while(1);
	
	// Check Driver Table

	RFDRIVER_TABLE DriverTable = FileImportTable[FIMPORT_DRVTBL].LoadedFileBuffer;
	if(!memcmp(DriverTable->Signature, DRVTBL_SIG, 20) ||
	!memcmp(DriverTable->HeaderEnd, KEHEADER_END, 20) ||
	DriverTable->TotalDrivers != DriverTable->NumKernelExtensionDrivers + DriverTable->NumThirdPartyDrivers
	) {
		SOD(SOD_INITIALIZATION, "INVALID_DRIVER_TABLE");
	}

	// Check Specified drivers
	UINT64 NumKexDrivers = 0;
	UINT64 Num3rdPartyDrivers = 0;

	

	for(UINT64 i = 0;i<DriverTable->TotalDrivers;i++){
		if(DriverTable->Drivers[i].Present){
			DRIVER_IDENTIFICATION_DATA* drv = &DriverTable->Drivers[i];
			SystemDebugPrint(L"DRVD#%d : Type %d, EN: %x, SRC : %d/%d, CLS : %x, SUBCS : %x, IF : %x, UNMSK : %x", 
			i, drv->DriverType, drv->Enabled, drv->DeviceSource, drv->DeviceSearchType, drv->DeviceClass, drv->DeviceSubclass, drv->ProgramInterface, drv->DataUnmask);
			if(DriverTable->Drivers[i].DriverType == INVALID_DRIVER_TYPE || DriverTable->Drivers[i].DriverType > MAX_DRIVER_TYPE) SOD(SOD_INITIALIZATION, "INVALID_DRIVER_TYPE");
			if(DriverTable->Drivers[i].DriverType == KERNEL_EXTENSION_DRIVER) NumKexDrivers++;
			else Num3rdPartyDrivers++;
		}
	}

	if(NumKexDrivers != DriverTable->NumKernelExtensionDrivers ||
	Num3rdPartyDrivers != DriverTable->NumThirdPartyDrivers
	) {
		SOD(SOD_INITIALIZATION, "INVALID_DRIVER_TABLE");
	}



	
	
		

	MSG_OBJECT Message = { 0 };
	

	

	#ifdef ___KERNEL_DEBUG___
	DebugWrite("Loading Essential Drivers From Kernel File Import Table.");	
	#endif
	
	Pmgrt.SystemInitialized = 1;


	RunEssentialExtensionDrivers(); // loads all drivers with type FIMPORT_DRIVER From File Import Table

	

	// Load Device Drivers (Device drivers will load other drivers if needed)
	// for e.g. hard drive driver will report it to kernel, then kernel will load the apropriate FS Drivers
	UINT64 NumPcieConfigs = GetNumPciExpressConfigurations();



	// Start by loading essential device drivers
	if(NumPcieConfigs){
		// Use MEMORY MAPPED Configuration
		for(UINT64 PcieConfig = 0;PcieConfig < NumPcieConfigs;PcieConfig++){
			
			for(UINT Bus = 0;Bus < 8;Bus++){
				for(UINT Device = 0;Device < 32;Device++){

					// Check if its a multifunction device
					UINT NumFunctions = 8;
					PCI_CONFIGURATION_HEADER* RootDeviceConfig = PcieConfigurationRead(PcieConfig, Bus, Device, 0);
					unsigned char DeviceHeaderType = RootDeviceConfig->HeaderType;
					if(DeviceHeaderType == 0xFF) continue; // device is not present
					if(!(DeviceHeaderType & 0x80)){
						NumFunctions = 1; // Device is not multifunction
					}

					for(UINT Function = 0;Function < NumFunctions;Function++){
						PCI_CONFIGURATION_HEADER* DeviceConfiguration = PcieConfigurationRead(PcieConfig, Bus, Device, Function);
						DeviceHeaderType = DeviceConfiguration->HeaderType;
						if(DeviceHeaderType == 0xFF) continue;
						if((DeviceHeaderType & ~(0x80)) == 0){
							InstallDevice(NULL, DEVICE_SOURCE_PCI, 0, 0, 0, 0, DeviceConfiguration);
						}else if((DeviceHeaderType & ~(0x80)) == 1){
							SystemDebugPrint(L"PCI-to-PCI Bridge.");
						}
					}
				}
			}
		}

	}else {
		// Otherwise use the IO PCI Configuration
		for(UINT Bus = 0;Bus < 8;Bus++){
			for(UINT Device = 0;Device<32;Device++){
				UINT8 HeaderType = IoPciRead8(Bus, Device, 0, PCI_HEADER_TYPE);
				UINT NumFunctions = 8;
				if(HeaderType == 0xFF) continue;
				if(!(HeaderType & 0x80)) NumFunctions = 1;

				for(UINT Function = 0;Function < NumFunctions;Function++){
					HeaderType = IoPciRead8(Bus, Device, Function, PCI_HEADER_TYPE);
					if(HeaderType == 0xFF) continue;
					if((HeaderType & ~(0x80)) == 0) {
						InstallDevice(NULL, DEVICE_SOURCE_PCI, 0, 0, 0, 0, IOPCI_CONFIGURATION(Bus, Device, Function));
					}else if((HeaderType & ~(0x80)) == 1){
							SystemDebugPrint(L"PCI-to-PCI Bridge.");
					}
				}

			}
		}
	}
	
	TaskSchedulerEnable(); // Enable Schedulling but not in boot processor
	RunDeviceDrivers();
	if(BootConfiguration->BootMode == BOOT_NORMAL_MODE){
		// After file systems & essential device drivers are loaded (load 3rd party drivers)

		// Load Extension Drivers
		// LoadExtensionDrivers();
		
		
	}
	
	__sti(); // Enable interrupts (and schedulling) on the boot processor

	#ifdef ___KERNEL_DEBUG___
	DebugWrite("Entering IpcServerReceive Loop (Kernel Successfully Loaded)");	
	#endif
	GP_draw_sf_text("Hello World !", 0xffffff, 20, 20);

	
	
	UINT Elapsed = 0;


	SetPriorityClass(kproc, PRIORITY_CLASS_IDLE);
	UINT64 LastCSLatency = 0;

	
	// Time in Metric Units Time * Precision(ms, ns...) / Frequency
	for(;;){
		GP_draw_sf_text(to_stringu64(Elapsed - 1), 0, 300, 500);
		GP_draw_sf_text(to_stringu64(Elapsed), 0xffff, 300, 500);
		Elapsed++;
		GP_draw_sf_text(to_stringu64(LastCSLatency), 0, 500, 500);
		LastCSLatency = CpuManagementTable[0]->LastThreadSwitchLatency[0] * 1000000 / GetHighPerformanceTimerFrequency();
		GP_draw_sf_text(to_stringu64(LastCSLatency), 0xffff, 500, 500);
		MicroSleep(100000);
	}

	UINT64 Threshold = 0, NumMessages = 0; // Message / s
	UINT64 Source = 0, MessageId = 0;

	GP_draw_rect(500, 80, 200, 100, 0);
	int seconds = 0;
	// Wait until a second has been completed


	GP_draw_sf_text("Msg/s", 0xffffff, 560, 120);
	
	RTC_TIME_DATE RtcTimeDate = {0};

	char TextOut[100] = {0};

	// while(1){
	// 	// MicroSleep()
	// }
	RtcGetTimeAndDate(&RtcTimeDate);
	seconds = RtcTimeDate.Second;
	while(RtcTimeDate.Second == seconds) RtcGetTimeAndDate(&RtcTimeDate);
	

	while (IpcServerReceive(KernelServer, &Message)) {
		// ThresholdIndex++;
		Threshold++;
		if(!(Threshold % 0x5000))
			RtcGetTimeAndDate(&RtcTimeDate);
		
		if(RtcTimeDate.Second != seconds){
			GP_draw_sf_text(TextOut, 0, 500, 120);
			NumMessages = Threshold;
			itoa(NumMessages, TextOut, RADIX_DECIMAL);
			GP_draw_sf_text(TextOut, 0xffffff, 500, 120);
			Threshold = 0;
			seconds = RtcTimeDate.Second;
		}

		// GP_draw_sf_text(to_hstring64((UINT64)Source), 0, 500, 80);
		// GP_draw_sf_text(to_hstring64((UINT64)MessageId), 0, 500, 100);
		// Source = (UINT64)Message.Header.Source;
		// MessageId = Message.Body.Message;
		// GP_draw_sf_text(to_hstring64((UINT64)Source), 0xffffff, 500, 80);
		// GP_draw_sf_text(to_hstring64((UINT64)MessageId), 0xffffff, 500, 100);

		IpcServerMessageDispatch(KernelServer);

	}
	SOD(SOD_DRIVER_ERROR, "GENERAL KERNEL ROUTINE SERVER (IPC RECEIVE) FAILURE.");
	

	for(;;){
		__hlt();
	}
	
}

#include <acpi/acpi.h>
#include <acpi/aml.h>
#include <CPU/cpu.h>
void AcpiFadt(UINT RsdpRevision, ACPI_FADT* Fadt) {
    if(Fadt->SMI_CommandPort){
            // If System Management Mode is supported, then take ownership of ACPI
            OutPortB(Fadt->SMI_CommandPort, Fadt->AcpiEnable);
        }
        RFACPI_DSDT Dsdt = NULL;
        if(RsdpRevision == 2) {
            Dsdt = (RFACPI_DSDT)Fadt->X_Dsdt;
            UINT64* Buff = (UINT64*)Dsdt->aml;
            // _RT_SystemDebugPrint(L"PREFERRED_POWER_MANAGEMENT : %d, SCI_INT : %x, DSDT : %x, (= %x %x %x %x)", (UINT64)Fadt->PreferredPowerManagementProfile, Fadt->SCI_Interrupt, Fadt->X_Dsdt, Buff[0], Buff[1], Buff[2], Buff[3]);

            if(Fadt->X_PMTimerBlock.Address){
                _RT_SystemDebugPrint(L"ACPI PM Timer (Current Count : %x) Supported. ADDR_SPACE: %x, BITWIDTH: %d, BITOFF: %d, ACCESS_SIZE : %x, ADDR : %x", AcpiReadTimer() ,(UINT64)Fadt->X_PMTimerBlock.AddressSpace, (UINT64)Fadt->X_PMTimerBlock.BitWidth, (UINT64)Fadt->X_PMTimerBlock.BitOffset, (UINT64)Fadt->X_PMTimerBlock.AccessSize, (UINT64)Fadt->X_PMTimerBlock.Address);
            }
        } else {
            Dsdt = (RFACPI_DSDT)(UINT64)Fadt->Dsdt;
            UINT64* Buff = (UINT64*)Dsdt->aml;
            // _RT_SystemDebugPrint(L"PREFERRED_POWER_MANAGEMENT : %d, SCI_INT : %x, DSDT : %x, (= %x %x %x %x)", (UINT64)Fadt->PreferredPowerManagementProfile, Fadt->SCI_Interrupt, Fadt->Dsdt, Buff[0], Buff[1], Buff[2], Buff[3]);

            // if(Fadt->PMTimerBlock){
            // 	_RT_SystemDebugPrint(L"ACPI PM Timer (Current Count : %x) Supported. ADDR_SPACE: %x, BITWIDTH: %d, BITOFF: %d, ACCESS_SIZE : %x, ADDR : %x", AcpiReadTimer() ,(UINT64)Fadt->X_PMTimerBlock.AddressSpace, (UINT64)Fadt->X_PMTimerBlock.BitWidth, (UINT64)Fadt->X_PMTimerBlock.BitOffset, (UINT64)Fadt->X_PMTimerBlock.AccessSize, (UINT64)Fadt->X_PMTimerBlock.Address);
            // }
        }
        AcpiReadDsdt(Dsdt);
        while(1) __hlt();
}
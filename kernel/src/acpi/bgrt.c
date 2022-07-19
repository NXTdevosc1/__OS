#include <acpi/bgrt.h>
#include <cstr.h>
#include <interrupt_manager/SOD.h>
#include <CPU/paging.h>
#include <utils/bmp.h>
#include <kernel.h>
void AcpiDrawOEMLogo(ACPI_SDT* Sdt){
    ACPI_BGRT* Bgrt = (ACPI_BGRT*)Sdt;
    if(Bgrt->Version != 1) SOD(SOD_INVALID_OEM_LOGO,"INVALID OEM LOGO");
    MapPhysicalPages(kproc->PageMap,(LPVOID)Bgrt->ImageAddress,(LPVOID)Bgrt->ImageAddress, 1, PM_PRESENT);
    struct BMP_HDR* BmpHeader = (struct BMP_HDR*)(Bgrt->ImageAddress);
    // Map with image size
    MapPhysicalPages(kproc->PageMap,(LPVOID)Bgrt->ImageAddress,(LPVOID)Bgrt->ImageAddress,(BmpHeader->size + 10) >> 12, PM_PRESENT);
    if(!BmpImgDraw(BmpHeader,Bgrt->OffsetX,Bgrt->OffsetY)) SOD(SOD_INVALID_OEM_LOGO,"INVALID OEM LOGO");
}
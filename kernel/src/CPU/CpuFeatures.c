#include <CPU/cpu.h>
#include <preos_renderer.h>
extern void _basicX64Init();

extern UINT64 _getCr4();
extern void _setCr4(UINT64 CR4);

// extern UINT64 _xgetbv(DWORD XstateRegister);
// extern void _xsetbv(DWORD XstateRegister, DWORD Value);

void EnableCpuFeatures() {
    CPUID_INFO CpuId = {0};
    _basicX64Init();
    _mm_setcsr(0x1F80); // set it to initial value
    __cpuid(&CpuId, 1);
    if(CpuId.ecx & CPUID1_ECX_XSAVE) {
        _setCr4(_getCr4() | (1 << 18)); // Enable OSXSAVE in CR4
        // AVX and other features may be declared but not supported, such as in HAXM
        // OSXSAVE Must be checked before enabling those features

        if(CpuId.ecx & CPUID1_ECX_AVX) {
            _xsetbv(0, 7); // Set x87 FPU, SSE and AVX in XCR0
            ExtensionLevel = EXTENSION_LEVEL_AVX;
        }
    } else {
        if(CpuId.ecx & CPUID1_ECX_AVX) {
		        GP_draw_sf_text("AVX Supported without OSXSAVE :(", 0xFFFFFF, 20, 40);
            // ExtensionLevel = EXTENSION_LEVEL_AVX;
        }
    }
}
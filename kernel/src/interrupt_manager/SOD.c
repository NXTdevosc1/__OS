#include <interrupt_manager/SOD.h>
#include <preos_renderer.h>
#include <interrupt_manager/idt.h>
#include <cstr.h>
#include <kernel.h>
#include <CPU/process.h>
#include <Management/debug.h>
#include <CPU/cpu.h>
void SOD(unsigned int code, char* message){
	__cli();
	TaskSchedulerDisable();
	// IpiBroadcast(IPI_SHUTDOWN, FALSE);

	#ifdef ___KERNEL_DEBUG___
		DebugWrite("SOD With Code :");
		DebugWrite(to_hstring32(code));
	#endif
	FRAME_BUFFER_DESCRIPTOR* fb = InitData.fb;
	GP_clear_screen(0xff000000);
	GP_draw_sf_text("): OS Has Encountered a problem and has been halt.",0xffffffff,(fb->HorizontalResolution/2)-(25*8),(fb->VerticalResolution/2)-24);
	GP_draw_sf_text("CODE : 0x",0xffffffff,(fb->HorizontalResolution/2)-(25*8),(fb->VerticalResolution/2));
	GP_draw_sf_text(to_hstring32(code),0xffffffff,(fb->HorizontalResolution/2)-(16*8),(fb->VerticalResolution/2));
	GP_draw_sf_text("ERROR : ",0xffffffff,(fb->HorizontalResolution/2)-(25*8),(fb->VerticalResolution/2)+24);
	GP_draw_sf_text(message,0xffffffff,(fb->HorizontalResolution/2)-(17*8),(fb->VerticalResolution/2)+24);
	if(code == CPU_INTERRUPT_PAGE_FAULT){
		GP_draw_sf_text("LINEAR ADDRESS : ",0xffffffff,(fb->HorizontalResolution/2)-(25*8),(fb->VerticalResolution/2)+44);
		uint64_t linear_addr = __getCR2();
		
		GP_draw_sf_text(to_hstring64(linear_addr),0xffffffff,(fb->HorizontalResolution/2)-(8*8),(fb->VerticalResolution/2)+44);
		
	}
	if(code == CPU_INTERRUPT_SIMD_FLOATING_POINT_EXCEPTION){
		GP_draw_sf_text("SSE MXCSR : ",0xffffffff,(fb->HorizontalResolution/2)-(25*8),(fb->VerticalResolution/2)+44);
		UINT64 ErrorCode = __stmxcsr();
		GP_draw_sf_text(to_hstring64(ErrorCode),0xffffffff,(fb->HorizontalResolution/2)-(8*8),(fb->VerticalResolution/2)+44);
		
	}
		GP_draw_sf_text("PROCESSOR ID : ",0xffffffff,(fb->HorizontalResolution/2)-(25*8),(fb->VerticalResolution/2)+64);
		GP_draw_sf_text(to_hstring64(GetCurrentProcessorId()),0xffffffff,(fb->HorizontalResolution/2)-(9*8),(fb->VerticalResolution/2)+64);

#ifdef ___KERNEL_DEBUG___
	if (code) {
		for (UINT i = 0; i < 20; i++) {
			GP_draw_sf_text(DebugRead(19 - i), 0xffffff, 20, 20 + i * 20);
		}
	}
#endif
	while(1){
		__hlt();
	}
}

void KERNELAPI KeSetDeathScreen(UINT64 DeathCode, UINT16* Msg, UINT16* Description, UINT16* SupportLink) {
	Pmgrt.SchedulerEnable = FALSE;
	__cli();
	GP_clear_screen(0);
	GP_draw_sf_text("): The Operating System has encountered a problem and needs to restart.", 0xffffff, 200, 200);
	GP_draw_sf_text("Status Code :", 0xffffff, 200, 240);
	GP_draw_sf_text(to_hstring64(DeathCode), 0xffffff, 200 + (14*8), 240);
	Gp_draw_sf_textW(Msg, 0xffffff, 200, 260);
	if(Description)
		Gp_draw_sf_textW(Description, 0xffffff, 200, 280);

	if(SupportLink)
		Gp_draw_sf_textW(SupportLink, 0xffffff, 200, 300);

	

	__hlt();
}
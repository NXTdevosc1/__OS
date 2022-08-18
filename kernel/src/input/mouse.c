#include <input/mouse.h>
#include <preos_renderer.h>
#include <IO/utility.h>
#include <MemoryManagement.h>
#include <interrupt_manager/idt.h>


uint32_t* previous_buffer = NULL;

FRAME_BUFFER_DESCRIPTOR* fb = NULL;
float mouse_icon_size_percent = 1.25f;
int mouse_icon_x = 0;
int mouse_icon_y = 0;
int mouse_icon_prev_x = 0;
int mouse_icon_prev_y = 0;
int prevx = 0, prevy = 0;
unsigned short mouse_icon_width = 0, mouse_icon_height = 0;
unsigned char packet_switch = 0;
unsigned char mouse_packet[3] = {0};
unsigned char flags = 0; // BITS : [0] = left_click, [1] = right_click, [2] = middle_click
uint8_t drawn = 0;
void handle_mouse_packet();

BOOL _HandleMouseInput = FALSE;

static inline void DrawMouseCursor(){
	if(drawn){
	for(uint32_t y0 = mouse_icon_prev_y; y0<mouse_icon_prev_y + 24;y0++){
		for(uint32_t x0 = mouse_icon_prev_x;x0<mouse_icon_prev_x + 24;x0++){
			*(uint32_t*)(fb->FrameBufferBase + (x0 * 4) + (y0 * (fb->HorizontalResolution * 4))) = previous_buffer[(x0-mouse_icon_prev_x)+((y0-mouse_icon_prev_y)*24)];
		}
	}
	}
	for(uint32_t y0 = mouse_icon_y; y0<mouse_icon_y + 24;y0++){
		for(uint32_t x0 = mouse_icon_x;x0<mouse_icon_x + 24;x0++){
			previous_buffer[(x0-mouse_icon_x)+((y0-mouse_icon_y)*24)] = *(uint32_t*)(fb->FrameBufferBase + (x0 * 4) + (y0 * (fb->HorizontalResolution * 4)));
			*(uint32_t*)(fb->FrameBufferBase + (x0 * 4) + (y0 * (fb->HorizontalResolution * 4))) = 0xffff0000;
		}
	}
}

struct GFX_OBJECT* mouse = NULL;

void handle_mouse_input(unsigned char data){
	// if(!_HandleMouseInput) return;
	switch(packet_switch){
		case 0:{
			if((data & (1<<3)) == 0) break;
			mouse_packet[0] = data;
			packet_switch++;
			break;
		}
		case 1:{
			mouse_packet[1] = data;
			packet_switch++;
			break;
		}
		case 2:{
			mouse_packet[2] = data;
			packet_switch = 0;
			handle_mouse_packet();
			break;
		}
	}
}

void handle_mouse_packet(){
	//GP_draw_rect(mouse_icon_x,mouse_icon_y,mouse_icon_width,mouse_icon_height,0xFF000000);
	mouse_icon_prev_x = mouse_icon_x;
	mouse_icon_prev_y = mouse_icon_y;
	prevx = mouse_icon_x;
	prevy = mouse_icon_y;
	//register struct GFX_BUFFER_RECT_OBJECT* rect = (struct GFX_BUFFER_RECT_OBJECT*)((uint64_t)mouse + sizeof(struct GFX_OBJECT) + sizeof(struct GFX_BUFFER_RECT_OBJECT));
	if(mouse_packet[0] & 1){
		// todo : left mouse button pressed
	}
	if(mouse_packet[0] & (1<<1)){
		// todo : right mouse button pressed
	}
	if(mouse_packet[0] & (1<<2)){
		// todo : middle mouse button pressed
	}

	if((mouse_packet[0] & (1<<4))){
		mouse_icon_x-=(256-mouse_packet[1]);
		if(mouse_packet[0] & (1<<6)){
		mouse_icon_x-=255;
		}
	}else{
		mouse_icon_x+=mouse_packet[1];
		if(mouse_packet[0] & (1<<6)){
			mouse_icon_x+=255;
		}
	}

	if((mouse_packet[0] & (1<<5))){
		mouse_icon_y+=(256-mouse_packet[2]);
		if(mouse_packet[0] & (1<<7)){
		mouse_icon_y+=255;
		}
	}else{
		mouse_icon_y-=mouse_packet[2];
		if(mouse_packet[0] & (1<<7)){
			mouse_icon_y-=255;
		}
	}
	
	if(mouse_icon_x >= (fb->HorizontalResolution - mouse_icon_width) || mouse_icon_x < 0){
		mouse_icon_x = mouse_icon_prev_x;
	}
	if(mouse_icon_y >= (fb->VerticalResolution - mouse_icon_height) || mouse_icon_y < 0){
		mouse_icon_y = mouse_icon_prev_y;
	}
	// if(!drawn){
	// 	for(uint32_t y0 = mouse_icon_y; y0<mouse_icon_y + 24;y0++){
	// 	for(uint32_t x0 = mouse_icon_x;x0<mouse_icon_x + 24;x0++){
	// 		previous_buffer[(x0-mouse_icon_x)+((y0-mouse_icon_y)*24)] = *(uint32_t*)(fb->FrameBufferBase + (x0 * 4) + (y0 * (fb->pitch)));
	// 	}
	// }
	// }
	// GfxSetPosition(mouse,(struct GFX_POSITION){mouse_icon_x,mouse_icon_y,0});
	// GfxDrawObject(mouse);
	//GP_draw_rect(mouse_icon_x,mouse_icon_y,mouse_icon_width,mouse_icon_height,0xFFFF0000);
	DrawMouseCursor();
	drawn = 1;

}


void mouse_wait(){
	uint64_t timeout = 100000;
	while(timeout--){
		if((InPortB(0x64) & (1<<1)) == 0){
			return;
		}
	}
}

void mouse_wait_input(){
	uint64_t timeout = 100000;
	while(timeout--){
		if(InPortB(0x64) & 1){
			return;
		}
	}
}

void mouse_write(uint8_t value){
	mouse_wait();
	OutPortB(0x64,0xD4);
	mouse_wait();
	OutPortB(0x60,value);
}

uint8_t mouse_read(){
	mouse_wait_input();
	return InPortB(0x60);
}

void init_ps2_mouse(){

	fb = InitData.fb;
	// mouse_icon_width = (fb->horizontal_resolution/100)*mouse_icon_size_percent;
	// mouse_icon_height = (fb->horizontal_resolution/100)*mouse_icon_size_percent;
	mouse_icon_width = 20;
	mouse_icon_height = 20;
	// mouse_icon_x = (fb->horizontal_resolution-mouse_icon_width)/2;
	// mouse_icon_y = (fb->vertical_resolution-mouse_icon_height)/2;
	// mouse_icon_prev_x = mouse_icon_x;
	// mouse_icon_prev_y = mouse_icon_y;
	KeControlIrq((KEIRQ_ROUTINE)INTH_mouse, 0xC, IRQ_DELIVERY_NORMAL, 0);
	previous_buffer = AllocatePoolEx(kproc, 0x200, 0, 0);
	memset(previous_buffer,0,512);
	_HandleMouseInput = TRUE;

	OutPortB(0x64,0xA8); // Enable ps2 mouse
	mouse_wait();
	OutPortB(0x64,0x20); // tell keyboad controller that we want to send a command to the mouse
	mouse_wait_input();
	uint8_t status = InPortB(0x60) | 2;
	mouse_wait();
	OutPortB(0x64,0x60);
	mouse_wait();
	OutPortB(0x60,status); // setting the correct "compaq" status byte
	mouse_write(0xF6);
	mouse_read();
	mouse_write(0xF4);
	mouse_read();
	
// mouse = GfxCreateRect(0,
// 	(struct GFX_COLOR){255,255,255,0.5},
// 	(struct GFX_POSITION){mouse_icon_x,mouse_icon_y,0},mouse_icon_width,mouse_icon_height);

}
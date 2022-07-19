#include <input/keyboard.h>
#include <preos_renderer.h>
#include <interrupt_manager/idt.h>
#include <cstr.h>
#include <IO/utility.h>

// KD = key down
// KU = key up


char QWERTY[] = {
	0,0,'1','2','3','4','5','6','7',
	'8','9','0','-','=',0,0,
	'q','w','e','r','t','y','u',
	'i','o','p','[',']',0,0,'a','s',
	'd','f','g','h','j','k','l',
	';','\'','\\',0,0,'z','x','c','v'
	,'b','n','m',',','.','/',0,0
	, 0, 0, 0, 0, 0, 0, 0, 0, ' '
};

unsigned int x = 300, y = 300;

void handle_keyboard(uint8_t kb_code){
	return;
	if(kb_code == 58 || // uppercase enable
	kb_code == 69 // num lock
	|| kb_code == 70 // scroll lock
	){ 
		OutPortB(0x60, 0xFA); // ack
		return;
	}else if(kb_code == 0xFE){ // resend
		//OutPortB(0x60, 58);
		return;
	}else if(kb_code == 0xFA) return; // ack
	if(x == 300 && y == 300){
		//GP_clear_screen(0xff000000ff000000);
	}
	// if(kb_code > sizeof(QWERTY) || QWERTY[kb_code] == 0) return;
	GP_sf_put_char(QWERTY[kb_code],0xFFFFFFFF,x,y);
	GP_draw_sf_text(to_hstring8(kb_code),0xFFFFFFFF,x+18,y);

	x+=50;
	if(x == 700){
		y+=16;
		x = 300;
	}

}

void init_ps2_keyboard(){
	OutPortB(PIC1_DATA,0b11111000);
	OutPortB(PIC2_DATA,0b11101111);
}
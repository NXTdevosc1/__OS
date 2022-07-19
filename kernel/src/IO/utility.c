#include <IO/utility.h>
#include <interrupt_manager/idt.h>

void pic_end_master(){
	OutPortB(PIC1_COMMAND,PIC_EOI);
}
void pic_end_slave(){
	OutPortB(PIC2_COMMAND,PIC_EOI);
	OutPortB(PIC1_COMMAND,PIC_EOI);
}
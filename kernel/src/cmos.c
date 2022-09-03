#include <cmos.h>
#include <IO/utility.h>

unsigned char CmosUpdateInProgress(){
    OutPortB(CMOS_ADDRESS, 0x0A);
    return (InPortB(CMOS_DATA) & 0x80);
}

unsigned char CmosRead(unsigned char Address){
    OutPortB(CMOS_ADDRESS, Address);
    return InPortB(CMOS_DATA);
}
void CmosWrite(unsigned char Address, unsigned char Value){
    OutPortB(CMOS_ADDRESS, Address);
    OutPortB(CMOS_DATA, Value);
}

#define SERIAL_COM1 0x3F8
#define QEMU_SERIAL_PORT SERIAL_COM1

void QemuWriteSerialMessage(char* Message) {
    while(*Message) {
        OutPortB(QEMU_SERIAL_PORT, *Message);
        Message++;
    }
    OutPortB(QEMU_SERIAL_PORT, '\n');
}
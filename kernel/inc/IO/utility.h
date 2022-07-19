#pragma once

void pic_end_master();
void pic_end_slave();


extern void OutPortB(unsigned short Port, unsigned char Value);
extern void OutPortW(unsigned short Port, unsigned short Value);
extern void OutPort(unsigned short Port, unsigned int Value);

extern unsigned char InPortB(unsigned short Port);
extern unsigned short InPortW(unsigned short Port);
extern unsigned int InPort(unsigned short Port);
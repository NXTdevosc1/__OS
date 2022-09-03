#pragma once

#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

enum CMOS_MEMORY{
    RTC_SECONDS = 0,
    RTC_MINUTES = 0x02,
    RTC_HOURS = 0x04,
    RTC_WEEKDAY = 0x06,
    RTC_DAYOF_MONTH = 0x07,
    RTC_MONTH = 0x08,
    RTC_YEAR = 0x09,
    RTC_CENTURY = 0x32,
    RTC_STATUS_REGISTERA = 0xA,
    RTC_STATUS_REGISTERB = 0xB,
    RTC_STATUS_REGISTERC = 0xC
};

unsigned char CmosRead(unsigned char Address);
void CmosWrite(unsigned char Address, unsigned char Value);

void QemuWriteSerialMessage(char* Message);
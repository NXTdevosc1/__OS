#pragma once
#include <krnltypes.h>
#include <cmos.h>
#define RTC_BCD_TO_BINARY(bcd) (((bcd & 0xF0) >> 1) + ((bcd & 0xF0) >> 3) + (bcd & 0xf))

#pragma pack(push, 16)


typedef struct _RTC_TIME_DATE{
    DWORD HourFormat : 1; // 0 = 12h, 1 = 24h
    DWORD ByteFormat : 1; // 1 = Binary format, 0 = BCD (Hex) format
    DWORD Reserved   : 30;
    unsigned char Second;
    unsigned char Minute;
    unsigned char Hour; // in 24 Hours
    unsigned char WeekDay; // 0 = invalid, 1 = sunday
    unsigned char DayOfMonth;
    unsigned char Month;
    unsigned char YearOfCentury;
    unsigned char Century; // 19-20 ...
    unsigned char RtcHour;
    UINT          Year;
} RTC_TIME_DATE;

#pragma pack(pop)

// Setups CMOS RTC & Enables its interrupts
void RtcInit(void);
BOOL isRtcUpdating(void);
KERNELSTATUS RtcGetTimeAndDate(RTC_TIME_DATE* TimeDate);
KERNELSTATUS RtcSetTimeAndDate(RTC_TIME_DATE* TimeDate);

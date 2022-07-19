#pragma once
#include <kerneltypes.h>
#include <usb.h>
typedef enum _USB2_PACKET_ID{
    USB2_PID_RESERVED = 0,
    // PID Type : Token
    USB_PID_TYPE_TOKEN = 1, // BITS 1:0 Of PID
    USB2_PID_OUT =      0b0001,
    USB2_PID_IN =       0b1001,
    USB2_PID_SOF =      0b0101,
    USB2_PID_SETUP =    0b0101,
    // PID Type : Data
    USB_PID_TYPE_DATA = 3,
    USB2_PID_DATA0 =    0b0011,
    USB2_PID_DATA1 =    0b1011,
    USB2_PID_DATA2 =    0b0111,
    USB2_PID_MDATA =    0b1111,
    // PID Type : Handshake
    USB_PID_TYPE_HANDSHAKE = 2,
    USB2_PID_ACK =      0b0010,
    USB2_PID_NACK =     0b1010,
    USB2_PID_STALL =    0b1110,
    USB2_PID_NYET =     0b0110, // no response yet from receiver
    // PID Type : Special
    USB_PID_TYPE_SPECIAL = 0,
    USB2_PID_PRE =      0b1100,
    USB2_PID_ERR =      0b1100,
    USB2_PID_SPLIT =    0b1000,
    USB2_PID_PING =     0b0100
} USB2_PACKET_ID;



// Setup PACKET Type used for control transfers

#pragma pack(push, 1)

typedef struct _USB2_DEVICE_REQUEST_TRANSACTION{
    USB_TOKEN_PACKET SetupToken;
    struct {
        UINT32 SyncField;
        UINT32 Pid;
        USB_DEVICE_REQUEST DeviceRequest;
        UINT16  CRC16;
        UINT8   EOP;
    } Data;
    USB_HANDSHAKE_PACKET Handshake;
    USB_TOKEN_PACKET InToken;
    struct {
        UINT64 SyncField : 32;
        UINT64 Pid : 8;
        UINT64 Ignored : 24;
        char Data[];
    } InData;
} USB2_DEVICE_REQUEST_TRANSACTION;


#pragma pack(pop)
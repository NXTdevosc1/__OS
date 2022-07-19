#pragma once
#include <kerneltypes.h>

#define PCI_SERIAL_BUS_CONTROLLER_CLASS 0xC
#define PCI_USB_CLASS PCI_SERIAL_BUS_CONTROLLER_CLASS
#define PCI_USB_SUBCLASS 0x3


typedef int USBSTATUS;
typedef void* USBBUFFER;


typedef enum _USB_DIRECTION{
    USB_OUT = 0,
    USB_IN = 1,
    USB_SETUP = 2,
    USB_DIRECTION_MAX = 2
} USB_DIRECTION;

typedef enum _USB_SPEED{
    USB_LOW_SPEED = 0,
    USB_FULL_SPEED = 1,
    USB_HIGH_SPEED = 2,
    USB_SUPER_SPEED = 3,
    USB_MAX_SPEED = 3
} USB_SPEED;

typedef enum _USB_DEVICE_REQUEST_TYPE{
    USB_DEVICE_GET_STATUS = 0,
    USB_DEVICE_CLEAR_FEATURE = 1,
    USB_DEVICE_SET_FEATURE = 2,
    USB_DEVICE_SET_ADDRESS = 5,
    USB_DEVICE_GET_DESCRIPTOR = 6,
    USB_DEVICE_SET_DESCRIPTOR = 7,
    USB_DEVICE_GET_CONFIGURATION = 8,
    USB_DEVICE_SET_CONFIGURATION = 9,
    USB_DEVICE_GET_INTERFACE = 10,
    USB_DEVICE_SET_INTERFACE = 11,
    USB_DEVICE_SYNC_FRAME = 12
} USB_DEVICE_REQUEST_TYPE;

#pragma pack(push, 1)

typedef struct _USB_DEVICE_REQUEST{
    unsigned char RequestType;
    unsigned char Request;
    unsigned short Value;
    unsigned short Index;
    unsigned short Length;
} USB_DEVICE_REQUEST;

typedef enum _USB_DESCRIPTOR_TYPE{
    USB_DESCRIPTOR_TYPE_DEVICE = 1,
    USB_DESCRIPTOR_TYPE_CONFIGURATION = 2,
    USB_DESCRIPTOR_TYPE_STRING = 3,
    USB_DESCRIPTOR_TYPE_INTERFACE = 4,
    USB_DESCRIPTOR_TYPE_ENDPOINT = 5,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER = 6,
    USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIGURATION = 7,
    USB_DESCRIPTOR_TYPE_INTERFACE_POWER = 8
} USB_DESCRIPTOR_TYPE;

typedef struct _USB_DEVICE_DESCRIPTOR{
    UINT8 Length;
    UINT8 DescriptorType;
    UINT16 UsbSpecificationReleaseNumber;
    UINT8 DeviceClass;
    UINT8 DeviceSubclass;
    UINT8 DeviceProtocol;
    UINT8 Endpoint0MaxPacketSize;
    UINT16 VendorId;
    UINT16 ProductId;
    UINT16 DeviceRelease;
    UCHAR Manufacturer; // index of string descriptor
    UCHAR Product;
    UCHAR SerialNumber;
    UINT8 NumConfigurations;
} USB_DEVICE_DESCRIPTOR;

typedef struct _USB_STRING_DESCRIPTOR{
    UINT8 Length;
    UINT8 DescriptorType;
    UINT16 String[126]; // to make 8 byte alignment
} USB_STRING_DESCRIPTOR;

typedef struct _USB_DEVICE_CONFIGURATION{
    UINT8 Length;
    UINT8 DescriptorType;
    UINT16 TotalLength;
    UINT8 NumInterfaces;
    UINT8 ConfigurationValue; // To Set In SET_CONFIGURATION Request
    UCHAR ConfigurationString;
    UINT8 Attributes;
    UINT8 MaxPower; // in 2 x mA 
} USB_DEVICE_CONFIGURATION;

typedef struct _USB_DEVICE_INTERFACE {
    UINT8 Length;
    UINT8 DescriptorType;
    UINT8 InterfaceNumber;
    UINT8 AlternateSetting;
    UINT8 NumEndpoints;
    UINT8 InterfaceClass;
    UINT8 InterfaceSubclass;
    UINT8 InterfaceProtocol;
    UCHAR InterfaceString;
} USB_DEVICE_INTERFACE;

typedef struct _USB_DEVICE_ENDPOINT{
    UINT8 Length;
    UINT8 DescriptorType;
    UINT8 EndpointAddress; // bit 7 specifies Direction
    UINT8 Attributes;
    UINT16 MaxPacketSize;
    UINT8 Interval;
} USB_DEVICE_ENDPOINT;

#pragma pack(pop)

#define USB_STATUS_SUCCESS 0 /*No status is returned*/
#define USB_STATUS_DATA_BUFFER_ERROR 1
#define USB_STATUS_TRANSACTION_ERROR 2
#define USB_STATUS_BABBLE_DETECTED 4
#define USB_STATUS_INVALID_PARAMETER 8
#define USB_STATUS_INVALID_REQUEST_PACKET 0x10
#define USB_STATUS_DEVICE_HALTED 0x20

// Controller Driver Functions



// USB Functions
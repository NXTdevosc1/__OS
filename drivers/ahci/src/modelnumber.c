#include <ahci.h>

void AhciParseModelNumber(RFAHCI_DEVICE_PORT Port) {
    UINT16 LastChar = 0xFF;
    BOOL Second = 0;
    UINT LastIndex = 0;
    UINT LastCharIndex = 0;
    for(UINT i = 0;i<40;i++) {
        
        UINT index = i + 1;
        if(Second) index = LastIndex;
        else LastIndex = i;

        Port->DriveInfo.DriveName[i] = Port->IdentifyDeviceData.ModelNumber[index];
        LastChar = Port->IdentifyDeviceData.ModelNumber[i];
        if(Port->IdentifyDeviceData.ModelNumber[index] > 0x20) {
            LastCharIndex = i;
        }
        Second ^= 1;

    }
    Port->DriveInfo.DriveName[LastCharIndex + 1] = 0;
}
#define __DLL_EXPORTS
#include <fat32.h>

void ParseLfnEntry(void* _Entry, UINT16** _Str) {
    UINT16* Str = *_Str;
    FAT32_LONG_FILE_NAME_ENTRY* Entry = _Entry;
    for(UINT8 i = 0;i<2;i++) {
        if(Entry->Name2[1-i] == 0 || Entry->Name2[1-i] == 0xFFFF) continue;
        *Str = Entry->Name2[1-i];
        Str--;
    }
    for(UINT8 i = 0;i<6;i++) {
        if(Entry->Name1[5-i] == 0 || Entry->Name1[5-i] == 0xFFFF) continue;
        *Str = Entry->Name1[5-i];
        Str--;
    }
    for(UINT8 i = 0;i<5;i++) {
        if(Entry->Name0[4-i] == 0 || Entry->Name0[4-i] == 0xFFFF) continue;
        *Str = Entry->Name0[4-i];
        Str--;
    }
    *_Str = Str;
}
void ParseSfnEntry(FAT32_FILE_ENTRY* Entry, UINT16* Str) {
    UINT16 BaseNameLen = 8;
    // Determine Length of base name
    for(UINT16 i = 0;i<8;i++) {
        if(Entry->ShortFileName[7-i] > 0x20) {
            break;
        } else BaseNameLen--;
    }
    UINT8 Flags = 0;
    if(Entry->UserAttributes & (1 << 3)) Flags |= 1; // lowercase base name
    if(Entry->UserAttributes & (1 << 4)) Flags |= 2; // lowercase extension name

    for(UINT16 i = 0;i<BaseNameLen;i++, Str++) {
        UINT8 c = Entry->ShortFileName[i];
        if(Flags & 1 && c >= 'A' && c <= 'Z') c+=0x20;
        else if(!(Flags & 1) && c >= 'a' && c <= 'z') c -= 0x20; // Convert to uppercase
        *Str = c;
    }
    if(Entry->ShortFileExtension[0] > 0x20) {
        *Str = '.';
        Str++;
        for(UINT16 i = 0;i<3;i++, Str++) {
            if(Entry->ShortFileExtension[i] <= 0x20) break;
            UINT8 c = Entry->ShortFileExtension[i];
            if(Flags & 2 && c >= 'A' && c <= 'Z') c+=0x20;
            else if(!(Flags & 2) && c >= 'a' && c <= 'z') c -= 0x20; // Convert to uppercase
            *Str = c;
        }
    }
    *Str = 0;
}

__FS_BOOL InsensitiveCompareChar(UINT16 c1, UINT16 c2) {
    if(c1 >= 'A' && c1 <= 'Z') {
        if(c2 >= 'a' && c2 <= 'z') c2-=32;
        if(c1 != c2) return 0;

        return 1;
    }else if(c1 >= 'a' && c1 <= 'z') {
        if(c2 >= 'A' && c2 <= 'Z') c2+=32;
        if(c1 != c2) return 0;

        return 1;
    }
    if(c1 != c2) return 0;

    return 1;
}

FAT32_FILE_ENTRY* Fat32FindDirectoryClusterEntry(PFAT32_PARTITION Partition, UINT16* FileName, void* ClusterBuffer, UINT16* EntryName) {
    FAT32_FILE_ENTRY* Entry = Partition->ReadCluster;
    UINT16* EntryNamePtr = (void*)0;
    UINT16 LenEntryName = 0;
    UINT16 RemainingSequences = 0;
    __FS_BOOL ParseMode = 0;
    UINT32 EntriesPerCluster = (Partition->BootSector.BiosParamBlock.ClusterSize << 9) >> 5;

    for(UINT32 x = 0;x<EntriesPerCluster;x++, Entry++) {
        if(Entry->ShortFileName[0] == 0) break; // End of directory entries
        if(Entry->ShortFileName[0] == 0xE5) continue; // Deleted entry
        
        if(ParseMode) {
            if(RemainingSequences == 0) {
                EntryNamePtr++; // Remove leading /0 or whatever char (caused by Str--)
                __FS_BOOL CorrespendantName = 1;
                UINT16* s = EntryNamePtr;
                UINT16* fn = FileName;
                
                while(*s) {
                    if(!InsensitiveCompareChar(*s, *fn)) {
                        CorrespendantName = 0;
                        break;
                    }
                    s++;
                    fn++;
                }

                
                if(*fn || !CorrespendantName) {
                    ParseMode = 0;
                    continue;
                }
                
                return Entry;
            }else {
                if((Entry->FileAttributes & 0xF) == 0xF) {
                    ParseLfnEntry(Entry, &EntryNamePtr);
                    RemainingSequences--;
                } else // Invalid LFN Chain
                {
                    ParseMode = 0;
                    goto ParseNormalEntry;
                }
            }
        } else {
        ParseNormalEntry:
            if((Entry->FileAttributes & 0xF) == 0xF && Entry->ShortFileName[0] & 0x40) {
                // LFN
                if(!(Entry->ShortFileName[0] & 0x1F)) continue; // Invalid Entry
                ParseMode = 1;
                RemainingSequences = (Entry->ShortFileName[0] & 0x1F) - 1;
                EntryNamePtr = EntryName + 0x19E;

                ParseLfnEntry(Entry, &EntryNamePtr);

            }else {
                // SFN
                ParseSfnEntry(Entry, EntryName);
                __FS_BOOL CorrespendantName = 1;
                UINT16* s = EntryName;
                UINT16* fn = FileName;
                while(*s) {
                    if(!InsensitiveCompareChar(*s, *fn)) {
                        CorrespendantName = 0;
                        break;
                    }
                    s++;
                    fn++;
                }
                if(*fn || !CorrespendantName) continue; // File Name does not end as 0 (meaning != EntryName)
                return Entry;
            }
        }
    }
    return (void*)0;
}

__FS_BOOL Fat32FindEntry(PFAT32_PARTITION Partition, UINT16* FileName, UINT32 Cluster, FAT32_FILE_ENTRY* DestEntry) {
    int ReadStatus = 0;
    UINT16 LenFileName = Fat32StrlenW(FileName);
    if(!LenFileName) return 0;

    
    UINT16 EntryName[0x200] = {0}; /*This is the max of an LFN 0x1F * 13 + Padding*/
    while(Fat32ReadCluster(Partition, &Cluster, Partition->ReadCluster, &ReadStatus) == FAT_SUCCESS) {
        FAT32_FILE_ENTRY* Entry = Fat32FindDirectoryClusterEntry(Partition, FileName, Partition->ReadCluster, EntryName);
        if(!Entry) continue;
        Fat32MemCopy(DestEntry, Entry, 0x20);
        return 1;
    }
    
    Exit:
    return 0;
}


void SetupEntryShortFileName(FAT32_FILE_ENTRY* Entry, UINT16* FileName);

void SetLfnEntry(FAT32_LONG_FILE_NAME_ENTRY* Entry, UINT16 SequenceNumber, UINT8 LfnChecksum, 
                UINT16** ReverseFileName, UINT16* RemainingSize, UINT16* _SizeFileNamePart) {
    Entry->SequenceNumber = SequenceNumber;
            Entry->LfnChecksum = LfnChecksum;
            Entry->Attributes = 0xF;
            UINT16* FileNamePart = *ReverseFileName;
            UINT16 SizeFileNamePart = *_SizeFileNamePart;
            __FS_BOOL EoLfn = 0;

            for(int c = 0;c<5;c++) {
                if(*FileNamePart && SizeFileNamePart) {
                    Entry->Name0[c] = *FileNamePart;
                    FileNamePart++;
                    SizeFileNamePart--;
                } else {
                    if(!EoLfn) {
                        Entry->Name0[c] = 0;
                        EoLfn = 1;
                    } else {
                        Entry->Name0[c] = 0xFFFF;
                    }
                }
            }

            for(int c = 0;c<6;c++) {
                if(*FileNamePart && SizeFileNamePart) {
                    Entry->Name1[c] = *FileNamePart;
                    FileNamePart++;
                    SizeFileNamePart--;
                } else {
                    if(!EoLfn) {
                        Entry->Name1[c] = 0;
                        EoLfn = 1;
                    } else {
                        Entry->Name1[c] = 0xFFFF;
                    }
                }
            }

            for(int c = 0;c<2;c++) {
                if(*FileNamePart && SizeFileNamePart) {
                    Entry->Name2[c] = *FileNamePart;
                    FileNamePart++;
                    SizeFileNamePart--;
                } else {
                    if(!EoLfn) {
                        Entry->Name2[c] = 0;
                        EoLfn = 1;
                    } else {
                        Entry->Name2[c] = 0xFFFF;
                    }
                }
            }

            if(*RemainingSize >= 13) {
                *ReverseFileName = *ReverseFileName - 13;
                *RemainingSize = *RemainingSize - 13;
                *_SizeFileNamePart = 13;
            }
            else {
                *_SizeFileNamePart = *RemainingSize;
                *ReverseFileName = *ReverseFileName - *RemainingSize;
                *RemainingSize = 0;
            }
           
}

int Fat32AllocateLongFileNameEntry(PFAT32_PARTITION Partition, UINT32 Cluster, UINT16* FileName, FAT_FILE_ATTRIBUTES FileAttributes, UINT32 NumLfnEntries) {
    
    UINT32 ClusterStart = Cluster;
    
    
    UINT32 StartChainIndex = 0;
    UINT32 StartChainCluster = 0;
    UINT32 EndChainIndex = 0;
    UINT32 EndChainCluster = 0;
    UINT32 TargetEntries = NumLfnEntries + 1; // + 1 (File Info short file name)
    UINT32 NumApprovedEntries = 0;
    int LastStatus = 0;
    UINT32 LastCluster = Cluster;

    UINT32 EntriesPerCluster = (Partition->BootSector.BiosParamBlock.ClusterSize << 9 ) / 32;

    UINT8 LfnChecksum = 0;
    // Calculate LFN Checksum
    UINT8 ShortFileName[11] = {0};
    Fat32MemCopy(ShortFileName, FAT32_LFN_MARK, 11);
    UINT8* TmpChecksumLfn = ShortFileName;
    
    for(int i = 11;i;i--) {
        LfnChecksum = ((LfnChecksum & 1) << 7) + (LfnChecksum >> 1) + *TmpChecksumLfn++;
    }

    while(Fat32ReadCluster(Partition, &Cluster, Partition->ReadCluster, &LastStatus) == FAT_SUCCESS) {
        FAT32_LONG_FILE_NAME_ENTRY* Entry = Partition->ReadCluster;
        for(UINT32 i = 0;i<EntriesPerCluster;i++, Entry++) {
            if(Entry->SequenceNumber == 0 || Entry->SequenceNumber == 0xE5) {
                if(!StartChainCluster) {
                    StartChainCluster = LastCluster;
                    StartChainIndex = i;
                }
                
                NumApprovedEntries++;
                if(NumApprovedEntries == TargetEntries) {
                    EndChainCluster = LastCluster;
                    EndChainIndex = i;
                    goto SetupLfn;
                }
            }else if(StartChainCluster) {
                StartChainCluster = 0;
                NumApprovedEntries = 0;
            }
        }
        
        LastCluster = Cluster;
    }

SetupLfn:
    if(!StartChainCluster) {
        UINT32 RequiredClusters = TargetEntries / ((Partition->BootSector.BiosParamBlock.ClusterSize << 9)/32);
        if(TargetEntries % ((Partition->BootSector.BiosParamBlock.ClusterSize << 9)/32)) RequiredClusters++;
    
        StartupInfo.Print("LFN : NO ENOUGH SPACE IN DIRECTORY : ALLOCATING_CLUSTERS\n");

    
        StartChainCluster = Fat32AllocateClusterChain(Partition, ClusterStart, RequiredClusters);
        if(!StartChainCluster) return FAT_NO_ENOUGH_SPACE;
        StartChainIndex = 0;
        EndChainCluster = Fat32GetLastCluster(Partition, ClusterStart);
        EndChainIndex = TargetEntries % ((Partition->BootSector.BiosParamBlock.ClusterSize << 9)/32);

        // Clear Clusters
        LastStatus = 0;
        
        LastCluster = StartChainCluster;
        Fat32MemClear(Partition->ReadCluster, Partition->BootSector.BiosParamBlock.ClusterSize << 9);
        while((Cluster = Fat32GetNextCluster(Partition, LastCluster))) {
            Fat32WriteCluster(Partition, LastCluster, Partition->ReadCluster);
            LastCluster = Cluster;
        }

    }

    LastStatus = 0;

    // Write lfn by reverse
    UINT16 RemainingSize = Fat32StrlenW(FileName);
    UINT16* ReverseFileName = FileName + RemainingSize;
    UINT16 SequenceNumber = NumLfnEntries;
    UINT16 SizeFileNamePart = 13;
    if(RemainingSize >= 13) {
        if(RemainingSize % 13) {
            SizeFileNamePart = (RemainingSize % 13);
        } else SizeFileNamePart = 13;
        ReverseFileName -= SizeFileNamePart;
        RemainingSize -= SizeFileNamePart;
    }
    else {
        SizeFileNamePart = RemainingSize;
        ReverseFileName -= RemainingSize;
        RemainingSize = 0;
    }
    // Small optimization
    if(StartChainCluster == EndChainCluster) {
        FAT32_LONG_FILE_NAME_ENTRY* Entry = &((FAT32_LONG_FILE_NAME_ENTRY*)Partition->ReadCluster)[StartChainIndex];
        for(UINT32 i = StartChainIndex;i< /*<= will be on end to setup file entry*/EndChainIndex;i++, Entry++, SequenceNumber--) {
            SetLfnEntry(Entry, SequenceNumber, LfnChecksum, &ReverseFileName, &RemainingSize, &SizeFileNamePart);
            if(SequenceNumber == NumLfnEntries) Entry->SequenceNumber |= 0x40;
        }
        FAT32_FILE_ENTRY* InfoEntry = &((FAT32_FILE_ENTRY*)Partition->ReadCluster)[EndChainIndex];
        Fat32MemCopy(InfoEntry->ShortFileName, ShortFileName, 11);
        InfoEntry->FileAttributes = FileAttributes;
        InfoEntry->FileSize = 0;
        InfoEntry->FirstClusterLow = 0; // New files doesn't need ready cluster (allocated on write later)
        InfoEntry->FirstClusterHigh = 0;

        Fat32WriteCluster(Partition, StartChainCluster, Partition->ReadCluster);

        
    } else {
        LastCluster = StartChainCluster;
        Cluster = StartChainCluster;
        while(Fat32ReadCluster(Partition, &Cluster, Partition->ReadCluster, &LastStatus)) {
            if(LastCluster == StartChainCluster) {
                FAT32_LONG_FILE_NAME_ENTRY* Entry = &((FAT32_LONG_FILE_NAME_ENTRY*)Partition->ReadCluster)[StartChainIndex];
                for(UINT32 i = StartChainIndex;i<EntriesPerCluster;i++, Entry++, SequenceNumber--) {
                    SetLfnEntry(Entry, SequenceNumber, LfnChecksum, &ReverseFileName, &RemainingSize, &SizeFileNamePart);
                    if(SequenceNumber == NumLfnEntries) Entry->SequenceNumber |= 0x40;
                }
            }else if(LastCluster == EndChainCluster) {
                FAT32_LONG_FILE_NAME_ENTRY* Entry = ((FAT32_LONG_FILE_NAME_ENTRY*)Partition->ReadCluster);
                for(UINT32 i = 0;i<EndChainIndex;i++, Entry++, SequenceNumber--) {
                    SetLfnEntry(Entry, SequenceNumber, LfnChecksum, &ReverseFileName, &RemainingSize, &SizeFileNamePart);
                }
                FAT32_FILE_ENTRY* InfoEntry = &((FAT32_FILE_ENTRY*)Partition->ReadCluster)[EndChainIndex];
                Fat32MemCopy(InfoEntry->ShortFileName, ShortFileName, 11);
                InfoEntry->FileAttributes = FileAttributes;
                InfoEntry->FileSize = 0;
                InfoEntry->FirstClusterLow = 0;
                InfoEntry->FirstClusterHigh = 0;
                Fat32WriteCluster(Partition, LastCluster, Partition->ReadCluster);
                return FAT_SUCCESS;
            } else {
                FAT32_LONG_FILE_NAME_ENTRY* Entry = ((FAT32_LONG_FILE_NAME_ENTRY*)Partition->ReadCluster);
                for(UINT32 i = 0;i<EntriesPerCluster;i++, Entry++, SequenceNumber--) {
                    SetLfnEntry(Entry, SequenceNumber, LfnChecksum, &ReverseFileName, &RemainingSize, &SizeFileNamePart);
                }
            }
            Fat32WriteCluster(Partition, LastCluster, Partition->ReadCluster);
            LastCluster = Cluster;
        }
        return FAT_READ_ERROR;
    }
    return FAT_SUCCESS;
}

void SetupEntryShortFileName(FAT32_FILE_ENTRY* Entry, UINT16* FileName) {
    UINT8 i = 0;
    UINT8* ShortFileName = Entry->ShortFileName;
    UINT8 BaseNameLen = 0;
    for(UINT8 c = 0;c<8;c++, i++) {
        if(*FileName < 0x20 || *FileName == '.') {
            ShortFileName[i] = 0x20;
        }else {
            UINT8 c = *FileName;
            if(c >= 'a' && c <= 'z') c -= 32; // Convert to uppercase
            ShortFileName[i] = c;
            FileName++;
            BaseNameLen++;
        }
    }
    if(*FileName == '.') {
        if(FileName[1] == 0 /*Base Name Len checked by previous functions*/) {
            ShortFileName[BaseNameLen] = '.';
        }else {
            FileName++;
            for(UINT8 c = 0;c<3;c++, i++) {
                if(*FileName < 0x20) {
                    ShortFileName[i] = 0x20;
                }else {
                    UINT8 c = *FileName;
                    if(c >= 'a' && c <= 'z') c -= 32; // Convert to uppercase
                    ShortFileName[i] = c;
                    FileName++;
                }
            }
        }
    } else {
        for(UINT8 c = 0;c<3;c++, i++) {
            ShortFileName[i] = 0x20;
        }
    }
}

// Flags : Bit 0 : NAME_LOWERCASE, BIT 1 : EXTENSION_LOWERCASE
int Fat32AllocateSfnEntry(PFAT32_PARTITION Partition, UINT32 Cluster, UINT16* FileName, FAT_FILE_ATTRIBUTES FileAttributes, UINT32 Flags) {
    int LastStatus = 0;
    UINT32 LastCluster = Cluster;
    UINT32 EntriesPerCluster = (Partition->BootSector.BiosParamBlock.ClusterSize << 9) / 32;

    UINT32 ClusterStart = Cluster;

    while (Fat32ReadCluster(Partition, &Cluster, Partition->ReadCluster, &LastStatus) == FAT_SUCCESS) {
        FAT32_FILE_ENTRY* Entry = Partition->ReadCluster;
        for(UINT32 i = 0;i<EntriesPerCluster;i++, Entry++) {
            if(Entry->ShortFileName[0] == 0 || Entry->ShortFileName[0] == 0xE5) {
                Fat32MemClear(Entry, 32);
                SetupEntryShortFileName(Entry, FileName);
                Entry->FileAttributes = FileAttributes;
                if(Flags & 1) {
                    Entry->UserAttributes = 1 << 3;
                }
                if(Flags & 2) {
                    Entry->UserAttributes |= 1 << 4;
                }
                Fat32WriteCluster(Partition, LastCluster, Partition->ReadCluster);
                return FAT_SUCCESS;
            }
        }
        LastCluster = Cluster;
    }
    
    StartupInfo.Print("NO ENOUGH SPACE IN DIRECTORY : ALLOCATING_CLUSTERS\n");

    // Allocate Cluster
    LastCluster = Fat32AllocateClusterChain(Partition, ClusterStart, 1);
    if(!LastCluster) return FAT_NO_ENOUGH_SPACE;
    Fat32MemClear(Partition->ReadCluster, Partition->BootSector.BiosParamBlock.ClusterSize << 9);
    FAT32_FILE_ENTRY* Entry = Partition->ReadCluster;
    SetupEntryShortFileName(Entry, FileName);
    Entry->FileAttributes = FileAttributes;
    if(Flags & 1) {
        Entry->UserAttributes = 1 << 3;
    }
    if(Flags & 2) {
        Entry->UserAttributes |= 1 << 4;
    }
    Fat32WriteCluster(Partition, LastCluster, Partition->ReadCluster);
    return FAT_SUCCESS;
}

int Fat32AllocateEntry(PFAT32_PARTITION Partition, UINT32 Cluster, UINT16* FileName, FAT_FILE_ATTRIBUTES FileAttributes) {
    __FS_BOOL AllocateLfn = 0;

    UINT32 NumLfnEntries = 0; // num lfn entries + 1 file name entry

    UINT16 LenFileName = Fat32StrlenW(FileName);
    if(!LenFileName) return FAT_INVALID_PARAMETER;
    __FS_BOOL FirstCharLowercase = 0;
    __FS_BOOL FirstCharExtensionLowercase = 0;
    
    if(LenFileName > 12) {
        NumLfnEntries = LenFileName / 13;
        if((LenFileName % 13)) NumLfnEntries++;
    }
    __FS_BOOL Extension = 0;
    for(UINT8 i = 0;i<10;i++) {
        if(!FileName[i]) break;
        if(FileName[i] == '.') {
            Extension = 1;
            UINT8 LenExtension = Fat32StrlenW(&FileName[i + 1]);
            if(LenExtension > 3) {
                NumLfnEntries = LenFileName / 13;
                if((LenFileName % 13)) NumLfnEntries++;
                break;
            }
        }
    }
    if(!NumLfnEntries && !Extension && LenFileName > 8) {
            NumLfnEntries = LenFileName / 13;
            if((LenFileName % 13)) NumLfnEntries++;
        }
    if(!NumLfnEntries) {
        
        __FS_BOOL NameLawSet = 0;
        UINT8 i = 0;
        for(;i<8;i++) {
            if(!NameLawSet) {
                if(FileName[i] >= 'a' && FileName[i] <= 'z') {
                    FirstCharLowercase = 1;
                    NameLawSet = 1;
                }
                else if(FileName[i] >= 'A' && FileName[i] <= 'Z') {
                    NameLawSet = 1;
                }
            }
            if(FileName[i] == '.') {
                i++;
                break;
            }
            if(FileName[i] == ' ') {
                NumLfnEntries = 1;
                break;
            }
            if(FileName[i] >= 'A' && FileName[i] <= 'Z' && FirstCharLowercase) {
                NumLfnEntries = 1;
                break;
            }
            if(FileName[i] >= 'a' && FileName[i] <= 'z' && !FirstCharLowercase) {
                NumLfnEntries = 1;
                break;
            }
        }
        UINT8 AddLen = i + 3;
        NameLawSet = 0;
        for(;i<AddLen;i++) {
            if(!NameLawSet) {
                if(FileName[i] >= 'a' && FileName[i] <= 'z') {
                    FirstCharExtensionLowercase = 1;
                    NameLawSet = 1;
                }
                else if(FileName[i] >= 'A' && FileName[i] <= 'Z') {
                    NameLawSet = 1;
                }
            }
            if(FileName[i] == ' ') {
                NumLfnEntries = 1;
                break;
            }
            if(FileName[i] >= 'A' && FileName[i] <= 'Z' && FirstCharExtensionLowercase) {
                NumLfnEntries = 1;
                break;
            }
            if(FileName[i] >= 'a' && FileName[i] <= 'z' && !FirstCharExtensionLowercase) {
                NumLfnEntries = 1;
                break;
            }
        }
    }

    if(NumLfnEntries) {
        return Fat32AllocateLongFileNameEntry(Partition, Cluster, FileName, FileAttributes, NumLfnEntries);
    } else {
        return Fat32AllocateSfnEntry(Partition, Cluster, FileName, FileAttributes, FirstCharLowercase | (FirstCharExtensionLowercase << 1));
    }
}


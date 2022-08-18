#include <fs/fat32/dir.h>
#include <MemoryManagement.h>


wchar_t* FAT32ParseShortFileNameEntry(struct FAT_DIRECTORY_ENTRY* entry, wchar_t* out, UINT64* FileNameLength){
        if(!entry || !out) return NULL;
            uint8_t c = 0;
            wchar_t* d = out;

            if(entry->allocation_status != 0xe5){
                d[c] = entry->allocation_status;
                if(d[c] >= L'A' && d[c] <= L'Z' && !(entry->file_attributes & FAT_ATTR_VOLUME_LABEL)) d[c]+=32;

                c++;
            }

            for(uint8_t i = 0;i<7;i++){
                if(entry->file_name[i] <= 0x20) break;
                d[c] = entry->file_name[i];
                if(d[c] >= L'A' && d[c] <= L'Z' && !(entry->file_attributes & FAT_ATTR_VOLUME_LABEL)) d[c]+=32;
                c++;
            }
            if(entry->file_name[7] > 0x20){
                d[c] = L'.';
                c++;
            }
            for(uint8_t i = 7;i<10;i++){
                if(entry->file_name[i] <= 0x20) break;
                d[c] = entry->file_name[i];
                if(d[c] >= L'A' && d[c] <= L'Z' && !(entry->file_attributes & FAT_ATTR_VOLUME_LABEL)) d[c]+=32;

                c++;
            }

            d[c] = 0;
            if (FileNameLength) {
                *FileNameLength = c;
            }
    return out;
}

wchar_t* FAT32ParseLongFileNameEntry(struct FAT_LFN_DIR_ENTRY* lfne, struct FAT_DIRECTORY_ENTRY** next_entry, int* err, wchar_t* out, UINT64* FileNameLength){
    if (!out) {
        if (err) *err = -2;
        return NULL;
    }
    uint8_t check = 0;
            uint16_t index = 0;
            uint8_t charsz = 1; // \0
            for(uint8_t i = 0;i<18;i++){
                if((lfne->file_attributes & 0xf)!=0xf) {
                    *next_entry = (struct FAT_DIRECTORY_ENTRY*)lfne; // file data entry
                    check = 1;
                    lfne--; // return to previous entry
                    break;
                }

                charsz+=13;
                index++;
                lfne++;
            }

            if(!check) {
                if(err) *err = -4;
                return NULL;
            }; // bad partition, will set the dirty flag in fs
            check = 0;
            wchar_t* d = out;
            
            if (charsz > MAX_FILE_NAME) {
                if (err) *err = -1;
                return NULL;
            }

            uint8_t c = 0; // character index
            for(uint8_t i = 0;i<index;i++){
                for(uint8_t f = 0;f<5;f++){
                    if(lfne->file_name_low[f] < 0x20) goto end_of_name;
                    d[c] = lfne->file_name_low[f];
                    c++;
                }
                for(uint8_t f = 0;f<6;f++){
                    if(lfne->file_name_mid[f] < 0x20) goto end_of_name;
                    d[c] = lfne->file_name_mid[f];
                    c++;
                }
                for(uint8_t f = 0;f<2;f++){
                    if(lfne->file_name_high[f] < 0x20) goto end_of_name;
                    d[c] = lfne->file_name_high[f];
                    c++;
                }
                lfne--;

                continue;

                end_of_name:
                    break;
                
            }

            d[c] = 0;
            if (FileNameLength) {
                *FileNameLength = c;
            }
    return d;
}
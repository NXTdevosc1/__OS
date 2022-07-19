#include <fs/dir.h>
#include <stdlib.h>
#include <MemoryManagement.h>
#include <stddef.h>
#include <stdlib.h>

BOOL MSABI FsCreateDirList(RFPROCESS Process, DIRECTORY_FILE_LIST* List) {
    if(!Process || !List) return FALSE;
    SZeroMemory(List);
    List->Process = Process;
    List->current = &List->ls;
    List->start = &List->ls;
    return TRUE;
}
int MSABI FsReleaseDirList(RFPROCESS process, DIRECTORY_FILE_LIST* dirls){
    if(!dirls) return -1;
    FILE_CONTENT_LIST* current = &dirls->ls;
    for(;;){
        if(!current) break;
        for(uint16_t i = 0;i<MAX_FILES_PER_DIRLS_LIST;i++){
            if(current->Files[i].FileName)
                free(current->Files[i].FileName,dirls->Process);
        }
        void* tmp = current->Next;
        free(current,process);
        SZeroMemory(current);
        current = tmp;
    }
    SZeroMemory(dirls);
    return SUCCESS;
}


int MSABI FsDirNext(DIRECTORY_FILE_LIST* list, DIR_LIST_FILE_ENTRY** out){
    if(!list || !list->start || !list->current) return -1;
    if(list->in_index >= MAX_FILES_PER_DIRLS_LIST || list->in_index >= list->file_count){
        if(list->current->Next){
            list->current = list->current->Next;
        }else list->current = list->start;

        list->in_index = 0;
    }

    for(uint8_t z = 0;z<255;z++){
        for(uint16_t i = list->in_index;i<MAX_FILES_PER_DIRLS_LIST;i++){
            if(list->current->Files[i].Set){
                *out = &list->current->Files[i];
                list->in_index = i+1;
                return SUCCESS;
            }
        }
        if (list->current->Next) {
            list->current = list->current->Next;
        }
        else return -1;
    }

    return -1;
}
int MSABI FsDirPrevious(DIRECTORY_FILE_LIST list, DIRECTORY_FILE_LIST* out){
    return -1;
}


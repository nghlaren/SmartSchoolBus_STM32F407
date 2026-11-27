#include "board.h"
#include "ff.h"
#include "diskio.h"
static DSTATUS state=STA_NOINIT;
DSTATUS disk_status(BYTE p){return p?STA_NOINIT:state;}
DSTATUS disk_initialize(BYTE p){
    if(p) return STA_NOINIT;
    state=HAL_SD_Init(&sd)==HAL_OK?0:STA_NOINIT;
    return state;
}
static bool ready(void){
    uint32_t start=HAL_GetTick();
    do{if(HAL_SD_GetCardState(&sd)==HAL_SD_CARD_TRANSFER) return true;}
    while(HAL_GetTick()-start<1000);
    return false;
}
DRESULT disk_read(BYTE p,BYTE *buf,DWORD sector,UINT n){
    if(disk_status(p)) return RES_NOTRDY;
    return ready() && HAL_SD_ReadBlocks(&sd,buf,sector,n,1000)==HAL_OK ? RES_OK:RES_ERROR;
}
DRESULT disk_write(BYTE p,const BYTE *buf,DWORD sector,UINT n){
    if(disk_status(p)) return RES_NOTRDY;
    return ready() && HAL_SD_WriteBlocks(&sd,(uint8_t*)buf,sector,n,1000)==HAL_OK && ready()?RES_OK:RES_ERROR;
}
DRESULT disk_ioctl(BYTE p,BYTE cmd,void *out){
    if(disk_status(p)) return RES_NOTRDY;
    switch(cmd){
    case CTRL_SYNC: return ready()?RES_OK:RES_ERROR;
    case GET_SECTOR_COUNT: *(DWORD*)out=sd.SdCard.LogBlockNbr; return RES_OK;
    case GET_SECTOR_SIZE: *(WORD*)out=512; return RES_OK;
    case GET_BLOCK_SIZE: *(DWORD*)out=1; return RES_OK;
    default: return RES_PARERR;
    }
}

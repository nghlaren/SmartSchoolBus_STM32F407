#include "board.h"
#include <stdio.h>
#include <string.h>
enum { CMD=1, IRQ=4, ERR=6, FIFO=9, LEVEL=10, CONTROL=12, FRAME=13, MODE_REG=17, TX=20 };
static bool io_ok;
static unsigned access_reg(unsigned reg,unsigned data,bool read){
    uint8_t tx[2]={(reg<<1)|(read?0x80:0),data},rx[2]={0};
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_RESET);
    if(HAL_SPI_TransmitReceive(&spi,tx,rx,2,10)!=HAL_OK) io_ok=false;
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_SET);
    return rx[1];
}
static void wr(unsigned r,unsigned v){access_reg(r,v,false);}
static unsigned rd(unsigned r){return access_reg(r,0,true);}
bool reader_init(void){
    io_ok=true;
    HAL_GPIO_WritePin(GPIOD,GPIO_PIN_8,GPIO_PIN_RESET); HAL_Delay(2);
    HAL_GPIO_WritePin(GPIOD,GPIO_PIN_8,GPIO_PIN_SET); HAL_Delay(50);
    unsigned version=rd(0x37);
    wr(0x2A,0x80); wr(0x2B,0xA9); wr(0x2C,0); wr(0x2D,40);
    wr(0x15,0x40); wr(MODE_REG,0x3D); wr(TX,rd(TX)|3);
    return io_ok && version!=0 && version!=255;
}
// Return -2 for the chip's no-response timer and -1 for malformed responses.
static int exchange(const uint8_t *tx,unsigned len,unsigned bits,uint8_t *rx,unsigned cap){
    wr(CMD,0); wr(IRQ,0x7F); wr(LEVEL,0x80);
    for(unsigned i=0;i<len;i++) wr(FIFO,tx[i]);
    wr(FRAME,bits); wr(CMD,0x0C); wr(FRAME,bits|0x80);
    uint32_t start=HAL_GetTick();
    while(io_ok && HAL_GetTick()-start<15){
        unsigned irq=rd(IRQ);
        if(irq&0x20){
            unsigned n=rd(LEVEL);
            if((rd(ERR)&0x1F) || (rd(CONTROL)&7) || n>cap) return -1;
            for(unsigned i=0;i<n;i++) rx[i]=rd(FIFO);
            return io_ok?(int)n:-1;
        }
        if(irq&1) return io_ok?-2:-1;
    }
    return -1;
}
int read_uid(char out[9]){
    io_ok=true;
    uint8_t request=0x26,anticollision[2]={0x93,0x20},reply[5];
    int n=exchange(&request,1,7,reply,sizeof reply);
    if(n==-2) return 0;
    // Cards still in READY may ignore REQA. Restart the RF field each poll below.
    if(n!=2) return -1;
    n=exchange(anticollision,2,0,reply,sizeof reply);
    if(n!=5 || reply[0]==0x88 || (reply[0]^reply[1]^reply[2]^reply[3])!=reply[4]) return -1;
    const char hex[]="0123456789ABCDEF";
    for(unsigned i=0;i<4;i++){
        out[2*i]=hex[reply[i]>>4];
        out[2*i+1]=hex[reply[i]&15];
    }
    out[8]=0;
    return 1;
}
void reader_cycle_field(void){
    wr(TX,rd(TX)&~3u); HAL_Delay(5); wr(TX,rd(TX)|3); HAL_Delay(5);
}

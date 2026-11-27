#include "board.h"
#include <string.h>
// 5x7 uppercase font. Titles use two pixels per source pixel.
static const uint8_t glyph[][5]={
{62,81,73,69,62},{0,66,127,64,0},{66,97,81,73,70},{33,65,69,75,49},{24,20,18,127,16},
{39,69,69,69,57},{60,74,73,73,48},{1,113,9,5,3},{54,73,73,73,54},{6,73,73,41,30},
{126,17,17,17,126},{127,73,73,73,54},{62,65,65,65,34},{127,65,65,34,28},{127,73,73,73,65},
{127,9,9,9,1},{62,65,73,73,122},{127,8,8,8,127},{0,65,127,65,0},{32,64,65,63,1},
{127,8,20,34,65},{127,64,64,64,64},{127,2,12,2,127},{127,4,8,16,127},{62,65,65,65,62},
{127,9,9,9,6},{62,65,81,33,94},{127,9,25,41,70},{70,73,73,73,49},{1,1,127,1,1},
{63,64,64,64,63},{31,32,64,32,31},{63,64,56,64,63},{99,20,8,20,99},{7,8,112,8,7},{97,81,73,69,67}};
static unsigned address;
static bool send(uint8_t prefix,const uint8_t *data,unsigned n){
    return HAL_I2C_Mem_Write(&i2c,address,prefix,I2C_MEMADD_SIZE_8BIT,(uint8_t*)data,n,50)==HAL_OK;
}
bool oled_init(void){
    address=0x3C<<1;
    if(HAL_I2C_IsDeviceReady(&i2c,address,2,10)!=HAL_OK) address=0x3D<<1;
    const uint8_t init[]={0xAE,0xD5,0x80,0xA8,31,0xD3,0,0x40,0x8D,0x14,0x20,2,0xA1,0xC8,0xDA,2,0x81,0x8F,0xD9,0xF1,0xDB,0x40,0xA4,0xA6,0xAF};
    return send(0,init,sizeof init);
}
static uint8_t column(char c,unsigned x){
    if(c>='0' && c<='9') return glyph[c-'0'][x];
    if(c>='A' && c<='Z') return glyph[10+c-'A'][x];
    if(c=='-') return 8;
    if(c==':') return x==2?0x24:0;
    if(c=='/') return (1u<<(6-x));
    if(c=='>'){static const uint8_t arrow[5]={0,65,34,20,8};return arrow[x];}
    return 0;
}
static void page_write(unsigned page,const uint8_t *buf){
    uint8_t pos[]={0xB0|page,0,0x10};
    send(0,pos,sizeof pos);
    send(0x40,buf,128);
}
void oled_line(unsigned page,const char *s){
    uint8_t buf[128]={0};
    for(unsigned i=0;i<21 && s[i];i++)
        for(unsigned x=0;x<5;x++) buf[6*i+x]=column(s[i],x);
    page_write(page,buf);
}
void oled_title(const char *s){
    uint8_t buf[2][128]={{0}};
    unsigned n=strlen(s);
    if(n>10) n=10;
    unsigned left=(128-n*12)/2;
    for(unsigned i=0;i<n;i++){
        for(unsigned x=0;x<5;x++){
            uint8_t bits=column(s[i],x);
            uint16_t expanded=0;
            for(unsigned y=0;y<7;y++) if(bits&(1u<<y)) expanded|=3u<<(2*y);
            for(unsigned dx=0;dx<2;dx++){
                buf[0][left+12*i+2*x+dx]=expanded&255;
                buf[1][left+12*i+2*x+dx]=expanded>>8;
            }
        }
    }
    page_write(0,buf[0]);
    page_write(1,buf[1]);
}


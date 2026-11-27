#include "clock.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>
static RTC_HandleTypeDef rtc;
static bool ready,lsi;
#define CLOCK_MARK 0x53425431u
bool clock_init(void){
    __HAL_RCC_PWR_CLK_ENABLE();HAL_PWR_EnableBkUpAccess();
    uint32_t source=RCC->BDCR & RCC_BDCR_RTCSEL;
    bool marked=RTC->BKP0R==CLOCK_MARK;
    if(source!=RCC_RTCCLKSOURCE_LSE && source!=RCC_RTCCLKSOURCE_LSI){
        __HAL_RCC_BACKUPRESET_FORCE();__HAL_RCC_BACKUPRESET_RELEASE();marked=false;source=0;
    }
    RCC_OscInitTypeDef osc={0};
    if(source==RCC_RTCCLKSOURCE_LSE || source==0){
        osc.OscillatorType=RCC_OSCILLATORTYPE_LSE;osc.LSEState=RCC_LSE_ON;
        if(HAL_RCC_OscConfig(&osc)==HAL_OK) source=RCC_RTCCLKSOURCE_LSE;
        else {__HAL_RCC_BACKUPRESET_FORCE();__HAL_RCC_BACKUPRESET_RELEASE();marked=false;source=RCC_RTCCLKSOURCE_LSI;}
    }
    lsi=source==RCC_RTCCLKSOURCE_LSI;
    if(lsi){
        osc.OscillatorType=RCC_OSCILLATORTYPE_LSI;osc.LSIState=RCC_LSI_ON;
        if(HAL_RCC_OscConfig(&osc)!=HAL_OK) return false;
        if(__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST)) marked=false;
    }
    __HAL_RCC_RTC_CONFIG(source);__HAL_RCC_RTC_ENABLE();
    rtc.Instance=RTC;rtc.Init.HourFormat=RTC_HOURFORMAT_24;
    rtc.Init.AsynchPrediv=127;rtc.Init.SynchPrediv=lsi?249:255;
    rtc.Init.OutPut=RTC_OUTPUT_DISABLE;rtc.Init.OutPutPolarity=RTC_OUTPUT_POLARITY_HIGH;
    rtc.Init.OutPutType=RTC_OUTPUT_TYPE_OPENDRAIN;
    ready=HAL_RTC_Init(&rtc)==HAL_OK;
    if(!marked) HAL_RTCEx_BKUPWrite(&rtc,RTC_BKP_DR0,0);
    __HAL_RCC_CLEAR_RESET_FLAGS();
    return ready;
}
bool clock_is_lsi(void){return lsi;}
bool clock_get(BusTime *t){
    if(!ready || HAL_RTCEx_BKUPRead(&rtc,RTC_BKP_DR0)!=CLOCK_MARK) return false;
    RTC_TimeTypeDef time={0};RTC_DateTypeDef date={0};
    if(HAL_RTC_GetTime(&rtc,&time,RTC_FORMAT_BIN)!=HAL_OK || HAL_RTC_GetDate(&rtc,&date,RTC_FORMAT_BIN)!=HAL_OK) return false;
    *t=(BusTime){2000u+date.Year,date.Month,date.Date,time.Hours,time.Minutes,time.Seconds};
    return clock_valid(t);
}
bool clock_set(const BusTime *t){
    if(!ready || !clock_valid(t)) return false;
    RTC_TimeTypeDef time={0};RTC_DateTypeDef date={0};
    time.Hours=t->hour;time.Minutes=t->minute;time.Seconds=t->second;
    time.DayLightSaving=RTC_DAYLIGHTSAVING_NONE;time.StoreOperation=RTC_STOREOPERATION_RESET;
    date.Year=t->year-2000;date.Month=t->month;date.Date=t->day;
    unsigned days=0;
    for(unsigned y=2000;y<t->year;y++) days+=365+(clock_days(y,2)==29);
    for(unsigned m=1;m<t->month;m++) days+=clock_days(t->year,m);
    date.WeekDay=(days+t->day-1+5)%7+1; // 2000-01-01 Saturday; Monday=1.
    HAL_RTCEx_BKUPWrite(&rtc,RTC_BKP_DR0,0);
    if(HAL_RTC_SetTime(&rtc,&time,RTC_FORMAT_BIN)!=HAL_OK || HAL_RTC_SetDate(&rtc,&date,RTC_FORMAT_BIN)!=HAL_OK) return false;
    HAL_RTCEx_BKUPWrite(&rtc,RTC_BKP_DR0,CLOCK_MARK);return true;
}
void clock_stamp(char out[20]){
    BusTime t;
    if(!clock_get(&t)){strcpy(out,"UNSET");return;}
    snprintf(out,20,"%04u-%02u-%02u %02u:%02u:%02u",t.year,t.month,t.day,t.hour,t.minute,t.second);
}
uint32_t get_fattime(void){
    BusTime t;
    if(!clock_get(&t)) t=(BusTime){2026,1,1,0,0,0};
    return ((t.year-1980)<<25)|(t.month<<21)|(t.day<<16)|(t.hour<<11)|(t.minute<<5)|(t.second/2);
}

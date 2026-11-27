#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "trip.h"
#include "clock.h"
typedef void *QueueHandle_t;
typedef unsigned BaseType_t;
typedef struct {unsigned IDR;} FakeGPIO;
static FakeGPIO port;
#define GPIOD (&port)
#define GPIO_PIN_9 512
#define pdMS_TO_TICKS(x) (x)
#define portMAX_DELAY 0
#define pdPASS 1
static uint32_t ticks;
uint32_t HAL_GetTick(void){return ticks;}
void vTaskDelay(unsigned n){(void)n;}
int xQueueSend(void *q,void *e,unsigned n){return 1;}
int xQueueReceive(void *q,void *e,unsigned n){return 0;}
void *xQueueCreate(unsigned n,unsigned size){return &port;}
int xTaskCreate(void (*f)(void*),const char *s,unsigned n,void *a,unsigned p,void *h){return 1;}
void vTaskStartScheduler(void){}
void board_init(void){}
void buzz(bool on){}
bool oled_init(void){return true;}
bool reader_init(void){return true;}
void reader_cycle_field(void){}
int read_uid(char out[9]){return 0;}
static char visible[3][22];
void oled_line(unsigned page,const char *s){
    assert(strlen(s)<=21);
    if(page>=2) strcpy(visible[page-1],s);
}
void oled_title(const char *s){assert(strlen(s)<=10);strcpy(visible[0],s);}
bool journal_open(Trip *t){return true;}
static bool storage_ok=true;
bool journal_append(const Trip *t,Action a,int slot,const char *uid,uint32_t ms){return storage_ok;}
static bool time_ok=true;
static BusTime fake_time={2026,11,26,12,30,0};
bool clock_init(void){return true;}
bool clock_is_lsi(void){return false;}
bool clock_get(BusTime *t){if(!time_ok)return false;*t=fake_time;return true;}
bool clock_set(const BusTime *t){if(!clock_valid(t))return false;fake_time=*t;time_ok=true;return true;}
void clock_stamp(char out[20]){strcpy(out,time_ok?"2026-11-26 12:30:00":"UNSET");}
bool roster_load(const Trip *t){return true;}
#define main firmware_main
#include "ui_under_test.c"
#undef main
static void press(enum Input type){
    ticks+=100;
    InputEvent e={.type=type,.down=ticks};
    handle(&e);
    render(ticks+2000);
}
static void tap(const char *uid){
    InputEvent e={.type=CARD};
    strcpy(e.uid,uid);ticks+=300;handle(&e);render(ticks);
}
int main(void){
    screen_ok=reader_ok=roster_ok=true;
    roster_count=3;
    for(unsigned i=0;i<3;i++){roster_order[i]=i;strcpy(names[i],i==0?"AN":i==1?"BINH":"CHI");}
    press(OK_KEY);assert(trip.phase==IDLE);
    press(SELECT_KEY);press(OK_KEY);assert(view==CARD_LIST);
    press(OK_KEY);tap("1234ABCD");
    press(SELECT_KEY);press(OK_KEY);tap("5678ABCD");
    press(SELECT_KEY);press(OK_KEY);tap("9012ABCD");
    press(SELECT_KEY);press(SELECT_KEY);press(OK_KEY);assert(view==HOME);
    press(OK_KEY);assert(trip.phase==PICKUP);
    tap("1234ABCD");tap("1234ABCD");tap("5678ABCD");tap("9012ABCD");assert(onboard(&trip)==3);
    assert(!transition(&trip,MANUAL_EXIT,0,trip.uid[0]));
    assert(!transition(&trip,CLEAR_CARDS,-1,"-"));
    press(OK_KEY);press(OK_KEY);assert(trip.phase==DROP);
    tap("1234ABCD");assert(onboard(&trip)==2);
    press(SELECT_KEY);press(OK_KEY);assert(view==EXIT_LIST);
    press(OK_KEY);assert(view==CONFIRM_EXIT && slot==1 && choice==0);
    press(OK_KEY);assert(view==HOME && onboard(&trip)==2); // cancel by default
    press(SELECT_KEY);press(OK_KEY);press(OK_KEY);press(SELECT_KEY);
    storage_ok=false;press(OK_KEY);assert(fault && onboard(&trip)==2); // failed sync cannot commit
    storage_ok=true;fault=false;press(OK_KEY);assert(onboard(&trip)==1 && trip.status[1]==2);
    assert(!transition(&trip,MANUAL_EXIT,1,trip.uid[1]));
    press(OK_KEY);assert(trip.phase==MISSING);
    time_ok=false; // lost clock must not prevent finishing an existing trip
    press(SELECT_KEY);press(OK_KEY);press(OK_KEY);press(SELECT_KEY);press(OK_KEY);
    assert(onboard(&trip)==0 && trip.phase==MISSING);
    press(OK_KEY);assert(trip.phase==SWEEP);
    InputEvent stale={.type=REAR_KEY,.down=sweep_start-10};handle(&stale);assert(trip.phase==SWEEP);
    press(REAR_KEY);assert(trip.phase==IDLE && view==DONE);press(OK_KEY);
    press(OK_KEY);assert(view==CLOCK_EDIT); // starting requires real date/time
    edit_time=(BusTime){2028,1,31,23,59,0};clock_field=1;press(SELECT_KEY);
    assert(edit_time.month==2 && edit_time.day==29);
    clock_field=0;press(SELECT_KEY);assert(edit_time.year==2029 && edit_time.day==28);
    for(unsigned i=0;i<5;i++)press(OK_KEY);
    assert(view==CLOCK_SAVE);press(OK_KEY);assert(time_ok && view==HOME);
    press(SELECT_KEY);press(OK_KEY);choice=roster_count;press(OK_KEY);assert(view==CONFIRM_CLEAR && choice==0);
    press(OK_KEY);assert(view==CARD_LIST && any_registered(&trip));
    press(OK_KEY);press(SELECT_KEY);press(OK_KEY);assert(!any_registered(&trip));
    // Reordered roster registration must use stable ID, not row number.
    roster_order[0]=2;roster_order[2]=0;
    press(SELECT_KEY);press(OK_KEY);press(OK_KEY);tap("AAAAAAAA");
    assert(!strcmp(trip.uid[2],"AAAAAAAA") && !trip.uid[0][0]);
    view=HOME;choice=0;press(OK_KEY);assert(trip.phase==PICKUP);
    assert(clock_days(2000,2)==29 && clock_days(2027,2)==28);
    assert(!clock_valid(&(BusTime){2026,2,29,0,0,0}));
    assert(!clock_valid(&(BusTime){2100,1,1,0,0,0}));
    puts("PASS UI: manual exit, cancel, SD failure, sweep, RTC edit, reset, reordered roster");
}

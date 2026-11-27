#include "board.h"
#include "journal.h"
#include "clock.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>

enum Input { CARD, SELECT_KEY, OK_KEY, REAR_KEY, BAD_CARD };
typedef struct { enum Input type; uint32_t down; char uid[9]; } InputEvent;
typedef enum { HOME, CARD_LIST, SCAN_CARD, CONFIRM_REPLACE, CONFIRM_DROP, DONE, CONFIRM_CLEAR, EXIT_LIST, CONFIRM_EXIT, CLOCK_EDIT, CLOCK_SAVE } View;
static QueueHandle_t inputs;
static Trip trip;
static View view=HOME;
static unsigned choice,slot;
static bool fault,reader_ok,screen_ok,roster_ok;
static BusTime edit_time;
static unsigned clock_field;
static const char *const clock_fields[]={"YEAR","MONTH","DAY","HOUR","MINUTE"};
static unsigned *time_field(void){
    switch(clock_field){case 0:return &edit_time.year;case 1:return &edit_time.month;
    case 2:return &edit_time.day;case 3:return &edit_time.hour;default:return &edit_time.minute;}
}
static bool active_cards(void){
    for(unsigned i=0;i<roster_count;i++) if(trip.uid[roster_order[i]][0]) return true;
    return false;
}
static unsigned exit_slot(unsigned n){
    for(unsigned i=0;i<roster_count;i++){unsigned k=roster_order[i];if(trip.status[k]==1 && n--==0) return k;}
    return STUDENTS;
}
static void begin_clock(void){
    if(!clock_get(&edit_time)) edit_time=(BusTime){2026,1,1,0,0,0};
    edit_time.second=0;clock_field=0;view=CLOCK_EDIT;choice=0;
}
static uint32_t sweep_start,notice_start;
static char notice_title[11],notice_detail[22];
static volatile uint32_t sound_level,beep_serial;

static void message(const char *title,const char *detail){
    snprintf(notice_title,sizeof notice_title,"%s",title);
    snprintf(notice_detail,sizeof notice_detail,"%s",detail);
    notice_start=HAL_GetTick();
}
// Only this task drives the buzzer after board initialization.
// Storage and OLED operations cannot stretch a short confirmation tone.
static void sound_task(void *arg){
    (void)arg;
    uint32_t seen=beep_serial,start=HAL_GetTick()-200;
    for(;;){
        uint32_t now=HAL_GetTick();
        if(seen!=beep_serial){seen=beep_serial;start=now;}
        bool on=now-start<100;
        if(sound_level==1) on=now%10000<100;
        if(sound_level==2) on=now%1000<200;
        buzz(on);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void post(enum Input type,uint32_t down,const char *uid){
    InputEvent e={.type=type,.down=down};
    if(uid) memcpy(e.uid,uid,sizeof e.uid);
    xQueueSend(inputs,&e,portMAX_DELAY);
}
void reader_cycle_field(void);
static void cards(void *arg){
    (void)arg;
    char last[9]="",uid[9]; unsigned absent=0;
    for(;;){
        reader_cycle_field();
        int r=read_uid(uid);
        if(r==0){if(++absent>=3) last[0]=0;}
        else if(r>0){
            absent=0;
            if(strcmp(uid,last)){strcpy(last,uid);post(CARD,HAL_GetTick(),uid);}
        }else{
            if(!last[0]){strcpy(last,"ERROR");post(BAD_CARD,HAL_GetTick(),NULL);}
            absent=0;
        }
        vTaskDelay(pdMS_TO_TICKS(65));
    }
}
static void keys(void *arg){
    (void)arg;
    typedef struct { bool raw,stable,armed; uint32_t change,down; } Key;
    Key k[3]={0};
    for(int i=0;i<3;i++) k[i].raw=k[i].stable=true; // Must observe release after boot.
    for(;;){
        uint32_t now=HAL_GetTick();
        for(int i=0;i<3;i++){
            bool low=!(GPIOD->IDR & (GPIO_PIN_9<<i));
            if(low!=k[i].raw){k[i].raw=low;k[i].change=now;}
            if(low==k[i].stable || now-k[i].change<30) continue;
            k[i].stable=low;
            if(!low){
                if(i==1 && k[i].armed) post(OK_KEY,k[i].down,NULL);
                k[i].armed=true;
            }else if(k[i].armed){
                k[i].down=k[i].change;
                if(i!=1) post(i==0?SELECT_KEY:REAR_KEY,k[i].down,NULL);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


static bool commit(Action action,int index,const char *uid){
    Trip next=trip;
    if(!transition(&next,action,index,uid)){
        message("TRY AGAIN","ACTION NOT ALLOWED");
        return false;
    }
    if(!journal_append(&next,action,index,uid,HAL_GetTick())){
        fault=true;
        sound_level=1;
        return false;
    }
    trip=next;
    notice_title[0]=0;
    choice=0;
    beep_serial++;
    if(trip.phase==SWEEP) sweep_start=HAL_GetTick();
    return true;
}
static void handle_card(const InputEvent *event){
    int index=student(&trip,event->uid);
    char detail[22];
    if(view==SCAN_CARD){
        if(index>=0){message("CARD USED",names[index]);return;}
        Action action=trip.uid[slot][0]?REPLACE:REGISTER;
        if(commit(action,slot,event->uid)){
            view=CARD_LIST;
            choice=0;
            while(choice<roster_count && roster_order[choice]!=slot) choice++;
            message("CARD SAVED",names[slot]);
        }
        return;
    }
    if(view!=HOME || trip.phase==IDLE || trip.phase==SWEEP) return;
    if(index<0 || !roster_active(index)){message("UNKNOWN","REGISTER THIS CARD");return;}
    if(trip.phase==PICKUP && trip.status[index]==1){
        message("ALREADY IN",names[index]);return;
    }
    if(trip.status[index]==2){message("EXIT DONE",names[index]);return;}
    if(trip.phase!=PICKUP && trip.status[index]==0){
        message("NOT ON BUS",names[index]);return;
    }
    bool boarding=trip.phase==PICKUP;
    if(commit(boarding?BOARD:EXIT_BUS,index,event->uid)){
        snprintf(detail,sizeof detail,"%s ON BUS %02u",names[index],(unsigned)onboard(&trip)%100);
        message(boarding?"WELCOME":"GOODBYE",detail);
    }
}
static void handle_ok(void){
    switch(view){
    case EXIT_LIST:
        slot=exit_slot(choice);
        if(slot==STUDENTS){view=HOME;choice=0;return;}
        view=CONFIRM_EXIT;choice=0;return;
    case CONFIRM_EXIT:
        if(choice && !commit(MANUAL_EXIT,slot,trip.uid[slot])) return;
        view=HOME;choice=0;return;
    case CLOCK_EDIT:
        if(++clock_field==5){view=CLOCK_SAVE;choice=0;}
        return;
    case CLOCK_SAVE:
        if(choice){view=HOME;choice=0;return;}
        if(clock_set(&edit_time)){view=HOME;choice=0;message("TIME SAVED",clock_is_lsi()?"LSI CHECK TIME DAILY":"LOCAL TIME UPDATED");}
        else message("RTC ERROR","CHECK CLOCK HARDWARE");
        return;
    case CARD_LIST:
        if(choice==roster_count){view=CONFIRM_CLEAR;choice=0;return;}
        if(choice==roster_count+1){view=HOME;choice=0;return;}
        slot=roster_order[choice];
        view=trip.uid[slot][0]?CONFIRM_REPLACE:SCAN_CARD;
        choice=0;
        return;
    case CONFIRM_CLEAR:
        if(choice==0){view=CARD_LIST;choice=roster_count;return;}
        if(commit(CLEAR_CARDS,-1,"-")){
            view=HOME;
            message("RESET DONE","ALL CARDS REMOVED");
        }
        return;
    case CONFIRM_REPLACE:
        view=choice?CARD_LIST:SCAN_CARD;
        choice=0;
        while(choice<roster_count && roster_order[choice]!=slot) choice++;
        return;
    case SCAN_CARD:
        view=CARD_LIST;choice=0;
        while(choice<roster_count && roster_order[choice]!=slot) choice++;
        return;
    case CONFIRM_DROP:
        if(choice==0 && !commit(MODE,-1,"-")) return;
        view=HOME;choice=0;return;
    case DONE:
        view=HOME;choice=0;return;
    case HOME:
        break;
    }
    if(trip.phase==IDLE){
        if(choice==1){view=CARD_LIST;choice=0;return;}
        if(choice==2){begin_clock();return;}
        BusTime now;
        if(!clock_get(&now)){begin_clock();message("SET TIME","REQUIRED TO START");return;}
        if(!active_cards()){message("NO CARDS","SELECT TO ADD CARDS");return;}
        commit(START,-1,"-");
    }else if(trip.phase==PICKUP){
        view=CONFIRM_DROP;choice=0;
    }else if(trip.phase==DROP || trip.phase==MISSING){
        if(choice && onboard(&trip)){view=EXIT_LIST;choice=0;return;}
        commit(END,-1,"-");
    }
}
static void handle(const InputEvent *event){
    notice_title[0]=0;
    if(event->type==BAD_CARD){message("READ ERROR","REMOVE AND TAP AGAIN");return;}
    if(event->type==CARD){handle_card(event);return;}
    if(event->type==REAR_KEY){
        if(fresh_rear_press(trip.phase,event->down,sweep_start) && commit(REAR,-1,"-"))
            view=DONE;
        return;
    }
    if(event->type==OK_KEY){handle_ok();return;}
    if(view==CARD_LIST) choice=(choice+1)%(roster_count+2);
    else if(view==EXIT_LIST) choice=(choice+1)%((unsigned)onboard(&trip)+1);
    else if(view==CLOCK_EDIT){
        static const unsigned low[]={2000,1,1,0,0},high[]={2099,12,31,23,59};
        unsigned max=clock_field==2?clock_days(edit_time.year,edit_time.month):high[clock_field];
        unsigned *value=time_field();if(++*value>max) *value=low[clock_field];
        unsigned days=clock_days(edit_time.year,edit_time.month);if(edit_time.day>days) edit_time.day=days;
    }else if(view==HOME && trip.phase==IDLE) choice=(choice+1)%3;
    else if(view==CONFIRM_DROP || view==CONFIRM_REPLACE || view==CONFIRM_CLEAR ||
            view==CONFIRM_EXIT || view==CLOCK_SAVE ||
            (view==HOME && (trip.phase==DROP || trip.phase==MISSING) && onboard(&trip))) choice^=1;
}
static void render(uint32_t now){
    static char previous[3][22];
    char title[22]="",detail[22]="",action[22]="";
    switch(view){
    case EXIT_LIST:{
        unsigned selected=exit_slot(choice);
        if(selected==STUDENTS){strcpy(title,"BACK");strcpy(detail,"RETURN TO TRIP");strcpy(action,"> OK BACK");}
        else {snprintf(title,sizeof title,"%s",names[selected]);snprintf(detail,sizeof detail,"HS%02u  STILL ON BUS",selected+1);strcpy(action,"> OK MANUAL EXIT");}
        break;
    }
    case CONFIRM_EXIT:
        snprintf(title,sizeof title,"%s",names[slot]);
        strcpy(detail,"CONFIRM CHILD IS OUT");
        strcpy(action,choice?"> OK CONFIRM EXIT":"> OK CANCEL");break;
    case CLOCK_EDIT:
        snprintf(title,sizeof title,"%04u",*time_field());
        snprintf(detail,sizeof detail,"SET %s",clock_fields[clock_field]);
        strcpy(action,"SELECT CHANGE OK NEXT");break;
    case CLOCK_SAVE:
        strcpy(title,"SAVE TIME");
        snprintf(detail,sizeof detail,"%04u-%02u-%02u %02u:%02u",edit_time.year,edit_time.month,edit_time.day,edit_time.hour,edit_time.minute);
        strcpy(action,choice?"> OK CANCEL":"> OK SAVE");break;
    case CARD_LIST:
        strcpy(title,"CARDS");
        if(choice==roster_count){
            strcpy(detail,"REMOVE ALL SAVED UIDS");
            strcpy(action,"> OK RESET CARDS");
        }else if(choice==roster_count+1){
            strcpy(detail,"RETURN TO MAIN MENU");
            strcpy(action,"> OK BACK");
        }else{
            snprintf(title,sizeof title,"%s",names[roster_order[choice]]);
            snprintf(detail,sizeof detail,"HS%02u  %s",roster_order[choice]+1,
                     trip.uid[roster_order[choice]][0]?"CARD SAVED":"NO CARD");
            snprintf(action,sizeof action,"> OK %s",trip.uid[roster_order[choice]][0]?"REPLACE":"REGISTER");
        }
        break;
    case CONFIRM_CLEAR:
        strcpy(title,"RESET");
        strcpy(detail,"DELETE ALL CARDS");
        strcpy(action,choice?"> OK DELETE ALL":"> OK CANCEL");
        break;
    case SCAN_CARD:
        strcpy(title,"TAP CARD");
        snprintf(detail,sizeof detail,"ASSIGN TO %s",names[slot]);
        strcpy(action,"> OK CANCEL");
        break;
    case CONFIRM_REPLACE:
        strcpy(title,"REPLACE");
        snprintf(detail,sizeof detail,"%s NEW CARD",names[slot]);
        strcpy(action,choice?"> OK CANCEL":"> OK YES");
        break;
    case CONFIRM_DROP:
        strcpy(title,"DROP OFF");
        strcpy(detail,"START DROPPING OFF");
        strcpy(action,choice?"> OK GO BACK":"> OK CONFIRM");
        break;
    case DONE:
        strcpy(title,"ALL DONE");
        strcpy(detail,"BUS CHECK RECORDED");
        strcpy(action,"> OK HOME");
        break;
    case HOME:
        switch(trip.phase){
        case IDLE:
            strcpy(title,"READY");
            clock_stamp(detail);
            strcpy(action,choice==2?"> OK SET CLOCK":choice==1?"> OK MANAGE CARDS":"> OK START TRIP");
            break;
        case PICKUP:
            snprintf(title,sizeof title,"%d ON BUS",onboard(&trip));
            strcpy(detail,"PICKUP  TAP TO BOARD");
            strcpy(action,"> OK START DROP OFF");
            break;
        case DROP:
        case MISSING:
            snprintf(title,sizeof title,"%d ON BUS",onboard(&trip));
            strcpy(detail,"DROP OFF  TAP TO EXIT");
            if(trip.phase==MISSING && onboard(&trip)){
                unsigned pick=(now/2000)%(unsigned)onboard(&trip);
                for(unsigned i=0;i<STUDENTS;i++){
                    if(trip.status[i]!=1) continue;
                    if(pick--==0){snprintf(detail,sizeof detail,"ON BUS %s",names[i]);break;}
                }
            }
            if(!onboard(&trip)){strcpy(title,"ALL OUT");strcpy(detail,"READY FOR BUS CHECK");}
            strcpy(action,choice && onboard(&trip)?"> OK MANUAL EXIT":onboard(&trip)?"> OK END TRIP":"> OK CHECK BUS");
            break;
        case SWEEP:{
            uint32_t elapsed=now-sweep_start;
            if(elapsed>=60000) strcpy(title,"OVERDUE");
            else snprintf(title,sizeof title,"CHECK %02lu",(unsigned long)(60-elapsed/1000));
            strcpy(detail,"WALK AND CHECK SEATS");
            strcpy(action,"> PRESS REAR CHECKED");
            break;
        }
        }
        break;
    }
    if(notice_title[0] && now-notice_start<1800){
        snprintf(title,sizeof title,"%s",notice_title);
        snprintf(detail,sizeof detail,"%s",notice_detail);
    }
    if(fault || !roster_ok || !reader_ok || !screen_ok){
        strcpy(title,"STOP");
        strcpy(detail,fault?"SD OR LOG ERROR":!roster_ok?"ROSTER CSV ERROR":!reader_ok?"RFID READER ERROR":"DISPLAY ERROR");
        strcpy(action,"CHECK DEVICE  RESTART");
    }
    if(strcmp(title,previous[0])){oled_title(title);strcpy(previous[0],title);}
    if(strcmp(detail,previous[1])){oled_line(2,detail);strcpy(previous[1],detail);}
    if(strcmp(action,previous[2])){oled_line(3,action);strcpy(previous[2],action);}
}
static void supervisor(void *arg){
    (void)arg;
    screen_ok=oled_init();
    for(int i=0;i<4;i++) oled_line(i,"");
    reader_ok=reader_init();
    clock_init();
    fault=!journal_open(&trip);
    roster_ok=!fault && roster_load(&trip);
    sweep_start=HAL_GetTick();
    if(trip.phase==SWEEP) sweep_start-=60000;
    if(!fault && roster_ok && screen_ok && reader_ok){
        if(xTaskCreate(keys,"Keys",256,NULL,2,NULL)!=pdPASS ||
           xTaskCreate(cards,"RFID",512,NULL,1,NULL)!=pdPASS) fault=true;
    }
    for(;;){
        InputEvent event;
        if(xQueueReceive(inputs,&event,pdMS_TO_TICKS(35)) && !fault && roster_ok && screen_ok && reader_ok)
            handle(&event);
        uint32_t now=HAL_GetTick();
        bool alarm=(trip.phase==MISSING && onboard(&trip)>0) ||
                   (trip.phase==SWEEP && now-sweep_start>=60000);
        sound_level=alarm?2:(fault || !roster_ok || !reader_ok || !screen_ok)?1:0;
        if(screen_ok) render(now);
    }
}
int main(void){
    board_init();
    inputs=xQueueCreate(24,sizeof(InputEvent));
    if(inputs && xTaskCreate(sound_task,"Sound",256,NULL,4,NULL)==pdPASS &&
       xTaskCreate(supervisor,"Trip",1536,NULL,3,NULL)==pdPASS) vTaskStartScheduler();
    buzz(false);
    for(;;){}
}



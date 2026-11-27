#include "journal.h"
#include "ff.h"
#include "clock.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
static FATFS volume;
static FIL file;
static unsigned seq;
static bool dated;
static const char header2[]="seq,uptime_ms,trip,event,slot,uid,onboard,datetime,student_id,crc32\n";
static const char header[]="seq,uptime_ms,trip,event,slot,uid,onboard,crc32\n";
static uint32_t crc(const char *s){
    uint32_t v=~0u;
    while(*s){v^=(unsigned char)*s++; for(int i=0;i<8;i++) v=(v>>1)^((v&1)?0xEDB88320u:0);}
    return ~v;
}
static bool append(const char *s){
    UINT n;
    return f_write(&file,s,strlen(s),&n)==FR_OK && n==strlen(s) && f_sync(&file)==FR_OK;
}
static bool replay(Trip *t,char *line){
    char *tail=strrchr(line,',');
    if(!tail || strlen(tail+1)!=9 || tail[9]!='\n') return false;
    *tail++=0;
    char *end; unsigned long check=strtoul(tail,&end,16);
    if(end!=tail+8 || check!=crc(line)) return false;
    char *field[9]={line};
    int fields=dated?9:7;
    for(int i=1;i<fields;i++){
        char *sep=strchr(field[i-1],',');
        if(!sep) return false;
        *sep=0;field[i]=sep+1;
    }
    unsigned number=strtoul(field[0],&end,10);
    if(*end) return false;
    unsigned trip=strtoul(field[2],&end,10);
    if(*end) return false;
    int slot=strtol(field[4],&end,10);
    if(*end) return false;
    unsigned count=strtoul(field[6],&end,10);
    if(*end) return false;
    if(dated){
        char id[8];
        if(slot>=0 && slot<STUDENTS) snprintf(id,sizeof id,"HS%02d",slot+1);
        else strcpy(id,"-");
        if(strcmp(field[8],id) || (strcmp(field[7],"UNSET") && strlen(field[7])!=19)) return false;
    }
    const char *event=field[3],*uid=field[5];
    for(Action a=REGISTER;a<ACTIONS;a++){
        if(strcmp(events[a],event)) continue;
        if(number!=seq+1 || !transition(t,a,slot,uid) || t->trip!=trip || onboard(t)!=(int)count) return false;
        seq=number; return true;
    }
    return false;
}
bool journal_open(Trip *t){
    memset(t,0,sizeof *t);seq=0;dated=false;
    if(f_mount(&volume,"",1)!=FR_OK || f_open(&file,"TRIP_LOG.CSV",FA_OPEN_ALWAYS|FA_WRITE|FA_READ)!=FR_OK) return false;
    if(!f_size(&file)){dated=true;return append(header2);}
    char line[192];
    if(!f_gets(line,sizeof line,&file)) return false;
    if(!strcmp(line,header2)) dated=true;
    else if(strcmp(line,header)) return false;
    while(f_gets(line,sizeof line,&file)){
        // One schema boundary upgrades old logs without deleting old events.
        if(!dated && !strcmp(line,header2)){dated=true;continue;}
        if(!replay(t,line)) return false;
    }
    if(f_error(&file) || !f_eof(&file)) return false;
    if(!dated){if(!append(header2)) return false;dated=true;}
    return true;
}
bool journal_append(const Trip *next,Action a,int slot,const char *uid,uint32_t ms){
    char body[150],line[180],stamp[20],id[8];
    clock_stamp(stamp);
    if(slot>=0 && slot<STUDENTS) snprintf(id,sizeof id,"HS%02d",slot+1);
    else strcpy(id,"-");
    snprintf(body,sizeof body,"%u,%lu,%u,%s,%d,%s,%d,%s,%s",seq+1,(unsigned long)ms,next->trip,events[a],slot,uid,onboard(next),stamp,id);
    snprintf(line,sizeof line,"%s,%08lX\n",body,(unsigned long)crc(body));
    if(!append(line)) return false;
    seq++; return true;
}

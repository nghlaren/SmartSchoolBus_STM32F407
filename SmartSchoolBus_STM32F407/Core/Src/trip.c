#include "trip.h"
#include <string.h>
char names[STUDENTS][11];
unsigned roster_count,roster_order[STUDENTS];
bool roster_active(unsigned slot){return slot<STUDENTS && names[slot][0];}
bool any_registered(const Trip *t){
    for(unsigned i=0;i<STUDENTS;i++) if(t->uid[i][0]) return true;
    return false;
}
bool fresh_rear_press(Phase phase,uint32_t down,uint32_t sweep_start){
    return phase==SWEEP && (int32_t)(down-sweep_start)>0;
}
const char *const events[ACTIONS]={"REGISTER","START","MODE","BOARD","ALIGHT","END_REQUEST","REAR_CHECK_CLOSE","REPLACE","CLEAR_CARDS","MANUAL_EXIT"};
int onboard(const Trip *t){
    int count=0;
    for(int i=0;i<STUDENTS;i++) count += t->status[i]==1;
    return count;
}
int student(const Trip *t,const char *uid){
    for(int i=0;i<STUDENTS;i++) if(*uid && !strcmp(t->uid[i],uid)) return i;
    return -1;
}
bool transition(Trip *t,Action a,int slot,const char *uid){
    if(a==REGISTER || a==REPLACE || a==BOARD || a==EXIT_BUS || a==MANUAL_EXIT){
        if(slot<0 || slot>=STUDENTS || strlen(uid)!=8 || strspn(uid,"0123456789ABCDEF")!=8) return false;
        if(a!=REGISTER && a!=REPLACE && strcmp(uid,t->uid[slot])) return false;
    }
    switch(a){
    case CLEAR_CARDS:
        if(t->phase!=IDLE || onboard(t)!=0) return false;
        memset(t->uid,0,sizeof t->uid);
        memset(t->status,0,sizeof t->status);
        break;
    case REGISTER:
        if(t->phase!=IDLE || t->uid[slot][0] || student(t,uid)>=0) return false;
        strcpy(t->uid[slot],uid); break;
    case REPLACE:
        if(t->phase!=IDLE || !t->uid[slot][0] || student(t,uid)>=0) return false;
        strcpy(t->uid[slot],uid); break;
    case START:
        if(t->phase!=IDLE || !any_registered(t)) return false;
        memset(t->status,0,sizeof t->status); t->trip++; t->phase=PICKUP; break;
    case MODE:
        if(t->phase!=PICKUP && t->phase!=DROP) return false;
        t->phase=t->phase==PICKUP ? DROP : PICKUP; break;
    case BOARD:
        if(t->phase!=PICKUP || t->status[slot]!=0) return false;
        t->status[slot]=1; break;
    case EXIT_BUS:
    case MANUAL_EXIT:
        if((t->phase!=DROP && t->phase!=MISSING) || t->status[slot]!=1) return false;
        t->status[slot]=2; break;
    case END:
        if(t->phase==IDLE || t->phase==SWEEP) return false;
        t->phase=onboard(t) ? MISSING : SWEEP; break;
    case REAR:
        if(t->phase!=SWEEP || onboard(t)) return false;
        t->phase=IDLE; break;
    default: return false;
    }
    return true;
}


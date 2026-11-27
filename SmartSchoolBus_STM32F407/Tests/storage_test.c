#include "journal.h"
#include "clock.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
void clock_stamp(char out[20]){strcpy(out,"2026-11-26 12:30:00");}
static void save(Trip *t,Action a,int slot,const char *uid){
    Trip n=*t;assert(transition(&n,a,slot,uid));assert(journal_append(&n,a,slot,uid,123));*t=n;
}
int main(int argc,char **argv){
    assert(argc==2);Trip t={0};
    if(!strcmp(argv[1],"corrupt")){assert(!journal_open(&t));puts("PASS corruption rejected");return 0;}
    assert(journal_open(&t));
    if(!strcmp(argv[1],"badroster")){assert(!roster_load(&t));puts("PASS invalid roster rejected");return 0;}
    assert(roster_load(&t));
    if(!strcmp(argv[1],"init")){
        assert(roster_count==3);save(&t,REGISTER,0,"1234ABCD");save(&t,REGISTER,1,"5678ABCD");
        save(&t,START,-1,"-");save(&t,BOARD,0,t.uid[0]);save(&t,BOARD,1,t.uid[1]);save(&t,MODE,-1,"-");
        save(&t,MANUAL_EXIT,0,t.uid[0]);assert(onboard(&t)==1);
    }else if(!strcmp(argv[1],"resume")){
        assert(t.phase==DROP && t.status[0]==2 && t.status[1]==1);
        assert(roster_order[0]==1 && !strcmp(names[0],"AN NEW"));assert(!strcmp(t.uid[0],"1234ABCD"));
        save(&t,EXIT_BUS,1,t.uid[1]);save(&t,END,-1,"-");save(&t,REAR,-1,"-");save(&t,CLEAR_CARDS,-1,"-");
    }else if(!strcmp(argv[1],"reset")){assert(t.phase==IDLE && !any_registered(&t));}
    else if(!strcmp(argv[1],"legacy")){assert(!strcmp(t.uid[0],"1234ABCD"));save(&t,START,-1,"-");}
    else if(!strcmp(argv[1],"legacyresume")){assert(t.phase==PICKUP && !strcmp(t.uid[0],"1234ABCD"));}
    else assert(0);
    puts("PASS storage replay");return 0;
}

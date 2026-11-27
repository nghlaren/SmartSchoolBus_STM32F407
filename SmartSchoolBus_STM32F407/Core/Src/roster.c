#include "trip.h"
#include "ff.h"
#include <string.h>
#include <stdlib.h>
// HS01..HS32 map permanently to journal slots 0..31, independent of row order.
bool roster_load(const Trip *t){
    FIL f; char line[80]; UINT written;
    memset(names,0,sizeof names); roster_count=0;
    FRESULT r=f_open(&f,"STUDENTS.CSV",FA_READ);
    if(r==FR_NO_FILE){
        static const char defaults[]="id,name\nHS01,AN\nHS02,BINH\nHS03,CHI\n";
        if(f_open(&f,"STUDENTS.CSV",FA_CREATE_NEW|FA_WRITE)!=FR_OK) return false;
        bool ok=f_write(&f,defaults,sizeof defaults-1,&written)==FR_OK && written==sizeof defaults-1 && f_sync(&f)==FR_OK;
        if(f_close(&f)!=FR_OK || !ok) return false;
        if(f_open(&f,"STUDENTS.CSV",FA_READ)!=FR_OK) return false;
    }else if(r!=FR_OK) return false;
    bool ok=false;
    if(!f_gets(line,sizeof line,&f)) goto done;
    line[strcspn(line,"\r\n")]=0;
    char *header=line;
    if((unsigned char)line[0]==0xEF && (unsigned char)line[1]==0xBB && (unsigned char)line[2]==0xBF) header+=3;
    if(strcmp(header,"id,name")) goto done;
    while(f_gets(line,sizeof line,&f)){
        line[strcspn(line,"\r\n")]=0;
        if(!*line) continue;
        if(strlen(line)<6 || strncmp(line,"HS",2) || line[4]!=',' ||
           line[2]<'0' || line[2]>'9' || line[3]<'0' || line[3]>'9') goto done;
        unsigned id=(line[2]-'0')*10+line[3]-'0';
        const char *name=line+5; size_t n=strlen(name);
        if(id<1 || id>STUDENTS || names[id-1][0] || n<1 || n>10 ||
           strspn(name,"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -")!=n || name[0]==' ') goto done;
        strcpy(names[id-1],name);roster_order[roster_count++]=id-1;
    }
    if(f_error(&f) || !f_eof(&f) || !roster_count) goto done;
    // Never hide a passenger when recovering an unfinished trip.
    for(unsigned i=0;i<STUDENTS;i++) if(t->phase!=IDLE && t->status[i] && !roster_active(i)) goto done;
    ok=true;
done:
    if(f_close(&f)!=FR_OK) ok=false;
    return ok;
}

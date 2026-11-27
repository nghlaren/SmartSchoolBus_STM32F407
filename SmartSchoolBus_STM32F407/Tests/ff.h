#pragma once
#include <stdio.h>
#include <errno.h>
typedef int FATFS;
typedef FILE *FIL;
typedef unsigned UINT;
typedef int FRESULT;
#define FR_OK 0
#define FR_NO_FILE 4
#define FA_OPEN_ALWAYS 1
#define FA_WRITE 2
#define FA_READ 4
#define FA_CREATE_NEW 8
#define f_mount(v,s,n) ((void)(v),0)
static inline int f_open(FIL *f,const char *s,int mode){
    if(mode & FA_CREATE_NEW){FILE *old=fopen(s,"rb");if(old){fclose(old);return 8;}*f=fopen(s,"w+b");}
    else {*f=fopen(s,(mode&FA_WRITE)?"r+b":"rb");if(!*f && (mode&FA_OPEN_ALWAYS))*f=fopen(s,"w+b");}
    return *f?0:errno==ENOENT?FR_NO_FILE:1;
}
static inline long f_size(FIL *f){fseek(*f,0,SEEK_END);long n=ftell(*f);rewind(*f);return n;}
#define f_gets(s,n,f) fgets(s,n,*(f))
#define f_sync(f) fflush(*(f))
#define f_close(f) fclose(*(f))
#define f_error(f) ferror(*(f))
#define f_eof(f) feof(*(f))
static inline int f_write(FIL *f,const char *s,unsigned n,UINT *w){*w=fwrite(s,1,n,*f);return *w!=n;}

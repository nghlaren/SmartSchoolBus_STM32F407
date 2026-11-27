#include "clock.h"
unsigned clock_days(unsigned y,unsigned m){
    static const unsigned days[]={31,28,31,30,31,30,31,31,30,31,30,31};
    if(m<1 || m>12) return 0;
    return days[m-1]+(m==2 && y%4==0 && (y%100!=0 || y%400==0));
}
bool clock_valid(const BusTime *t){
    return t->year>=2000 && t->year<=2099 && t->month>=1 && t->month<=12 &&
        t->day>=1 && t->day<=clock_days(t->year,t->month) && t->hour<24 && t->minute<60 && t->second<60;
}

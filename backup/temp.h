#include "STC15F2K60S2.H"        //���롣
#include "sys.H"                 //���롣
#include"DS1302.h"
#include"displayer.h"


struct_DS1302_RTC t={0x30,0,9,0x06,9,1,0x21};

void time_show(){
    DisplayerInit();   
    
}

#include "STC15F2K60S2.H"        //必须。
#include "sys.H"                 //必须。
#include"DS1302.h"
#include"displayer.h"
#include"Key.H"

code unsigned long SysClock=11059200;  
#ifdef _displayer_H_                          //显示模块选用时必须。（数码管显示译码表，用戶可修改、增加等） 
code char decode_table[]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x00,0x08,0x40,0x01, 0x41, 0x48, 
	              /* 序号:   0   1    2	   3    4	    5    6	  7   8	   9	 10	   11		12   13    14     15     */
                /* 显示:   0   1    2    3    4     5    6    7   8    9  (无)   下-  中-  上-  上中-  中下-   */  
	                       0x3f|0x80,0x06|0x80,0x5b|0x80,0x4f|0x80,0x66|0x80,0x6d|0x80,0x7d|0x80,0x07|0x80,0x7f|0x80,0x6f|0x80 };  
             /* 带小数点     0         1         2         3         4         5         6         7         8         9        */
#endif



struct_DS1302_RTC t={0x30,0,9,0x06,9,1,0x21};

void time_show(){
    unsigned key3_act;
    int cnt=0;
    DisplayerInit();
    Key_Init();

    t=RTC_Read();
    SetDisplayerArea(0,7);
    //按下k3进行切换显示
    key3_act=GetKeyAct(enumKey3);
    
    if(key3_act==enumKeyPress){
        cnt++;
    }

    if(cnt%2==0){
        Seg7Print(t.year%10,t.hour%10,10,t.minute/10,t.minute%10,10,t.second/10,t.second);
    }
    else{
        Seg7Print(t.hour/10,t.hour%10,10,t.minute/10,t.minute%10,10,t.second/10,t.second);
    }



}

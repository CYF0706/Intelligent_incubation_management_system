#include "STC15F2K60S2.H"        //���롣
#include "sys.H"                 //���롣
#include"DS1302.h"
#include"displayer.h"
#include"Key.H"

code unsigned long SysClock=11059200;  
#ifdef _displayer_H_                          //��ʾģ��ѡ��ʱ���롣���������ʾ������Ñ����޸ġ����ӵȣ� 
code char decode_table[]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x00,0x08,0x40,0x01, 0x41, 0x48, 
	              /* ���:   0   1    2	   3    4	    5    6	  7   8	   9	 10	   11		12   13    14     15     */
                /* ��ʾ:   0   1    2    3    4     5    6    7   8    9  (��)   ��-  ��-  ��-  ����-  ����-   */  
	                       0x3f|0x80,0x06|0x80,0x5b|0x80,0x4f|0x80,0x66|0x80,0x6d|0x80,0x7d|0x80,0x07|0x80,0x7f|0x80,0x6f|0x80 };  
             /* ��С����     0         1         2         3         4         5         6         7         8         9        */
#endif



struct_DS1302_RTC t={0x30,0,9,0x06,9,1,0x21};

void time_show(){
    unsigned key3_act;
    int cnt=0;
    DisplayerInit();
    Key_Init();

    t=RTC_Read();
    SetDisplayerArea(0,7);
    //����k3�����л���ʾ
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

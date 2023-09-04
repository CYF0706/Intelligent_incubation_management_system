#include "displayer.h"
#include "STC15F2K60S2.H" //必须。
#include "sys.H"          //必须。
#include "DS1302.h"
#include "Key.H"
#include "ADC.h"
#include "beep.h"
#include "StepMotor.h"
#include "displayer.h"
#include "uart1.h"
#include <intrins.h>

//
typedef unsigned char U8;  /* defined for unsigned 8-bits integer variable 	  无符号8位整型变量  */
typedef signed char S8;    /* defined for signed 8-bits integer variable		  有符号8位整型变量  */
typedef unsigned int U16;  /* defined for unsigned 16-bits integer variable 	  无符号16位整型变量 */
typedef signed int S16;    /* defined for signed 16-bits integer variable 	  有符号16位整型变量 */
typedef unsigned long U32; /* defined for unsigned 32-bits integer variable 	  无符号32位整型变量 */
typedef signed long S32;   /* defined for signed 32-bits integer variable 	  有符号32位整型变量 */
typedef float F32;         /* single precision floating point variable (32bits) 单精度浮点数（32位长度） */
typedef double F64;        /* double precision floating point variable (64bits) 双精度浮点数（64位长度） */
//
#define uchar unsigned char
#define uint  unsigned int
// #define   Data_0_time    4

/*
系统事件表
SetEventCallBack(enumEventKey, switch_time);
SetEventCallBack(enumEventSys1S, time_add);
SetEventCallBack(enumEventSys10mS, now_seg_show);
SetEventCallBack(enumEventSys100mS, get_adc);
*/

code unsigned long SysClock = 11059200; // 必须。定义系统工作时钟频率(Hz)，用户必须修改成与实际工作频率（下载时选择的）一致
#ifdef _displayer_H_                    // 显示模块选用时必须。（数码管显示译码表，用戶可修改、增加等）
code char decode_table[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x00, 0x08, 0x40, 0x01, 0x41, 0x48,
                            /* 序号:   0   1    2	   3    4	    5    6	  7   8	   9	 10	   11		12   13    14     15     */
                            /* 显示:   0   1    2    3    4     5    6    7   8    9  (无)   下-  中-  上-  上中-  中下-   */
                            0x3f | 0x80, 0x06 | 0x80, 0x5b | 0x80, 0x4f | 0x80, 0x66 | 0x80, 0x6d | 0x80, 0x7d | 0x80, 0x07 | 0x80, 0x7f | 0x80, 0x6f | 0x80,
                            0x78, 0x48, 0x38, 0x1c};
/* 带小数点     0         1         2         3         4         5         6         7         8         9        t ,=, L*/
#endif

int code arrThermLUT[] = {239, 197, 175, 160, 150, 142, 135, 129, 124, 120, 116, 113, 109, 107, 104, 101,
                          99, 97, 95, 93, 91, 90, 88, 86, 85, 84, 82, 81, 80, 78, 77, 76,
                          75, 74, 73, 72, 71, 70, 69, 68, 67, 67, 66, 65, 64, 63, 63, 62,
                          61, 61, 60, 59, 58, 58, 57, 57, 56, 55, 55, 54, 54, 53, 52, 52,
                          51, 51, 50, 50, 49, 49, 48, 48, 47, 47, 46, 46, 45, 45, 44, 44,
                          43, 43, 42, 42, 41, 41, 41, 40, 40, 39, 39, 38, 38, 38, 37, 37,
                          36, 36, 36, 35, 35, 34, 34, 34, 33, 33, 32, 32, 32, 31, 31, 31,
                          30, 30, 29, 29, 29, 28, 28, 28, 27, 27, 27, 26, 26, 26, 25, 25,
                          24, 24, 24, 23, 23, 23, 22, 22, 22, 21, 21, 21, 20, 20, 20, 19,
                          19, 19, 18, 18, 18, 17, 17, 16, 16, 16, 15, 15, 15, 14, 14, 14,
                          13, 13, 13, 12, 12, 12, 11, 11, 11, 10, 10, 9, 9, 9, 8, 8, 8, 7,
                          7, 7, 6, 6, 5, 5, 54, 4, 3, 3, 3, 2, 2, 1, 1, 1, 0, 0, -1, -1, -1,
                          -2, -2, -3, -3, -4, -4, -5, -5, -6, -6, -7, -7, -8, -8, -9, -9,
                          -10, -10, -11, -11, -12, -13, -13, -14, -14, -15, -16, -16, -17,
                          -18, -19, -19, -20, -21, -22, -23, -24, -25, -26, -27, -28, -29,
                          -30, -32, -33, -35, -36, -38, -40, -43, -46, -50, -55, -63, 361};

typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} time_clock;

time_clock t = {0, 0, 0, 0, 0, 0};
// struct_DS1302_RTC t_copy = {0, 0, 0, 0, 0, 0, 0};

struct_ADC adc;
int xdata set_temp      = 0;
int xdata set_light     = 0;
int xdata set_wet       = 0;   // 还未实装
int xdata flag_move_egg = 0;   // 是否翻蛋
int xdata flag_get_wind = 0;   // 是否通风
int xdata flag_get_wet  = 0;   // 是否加湿
int xdata flag_get_hot  = 0;   // 是否加热
int xdata flag_get_cold = 0;   // 是否降温
int xdata sum_led       = 0x0; // 总的led灯亮的情况
int xdata flag_auto_set = 0;   // 是否进入auto状态
int xdata flag_reset    = 0;   // 是否进行重置
// unsigned char buffer[8]; // 年 月 日 时 分 秒 温度 光照
unsigned char buffer[12]; // 年 月 日 时 分 秒 温度 光照 湿度 设定温度 设定光照 设定湿度

unsigned char get_buffer[8]; // 温度 光照 湿度 是否翻蛋 是否通风 是否加湿 是否加热 是否降温
char *tar_buf = "";

// 加湿
sbit P1_0 = P1 ^ 0;
// sbit  P2_0  = 0x90 ;
U8 U8FLAG, k;
U8 U8count, U8temp;
U8 U8T_data_H, U8T_data_L, U8RH_data_H = 45, U8RH_data_L, U8checkdata;
U8 U8T_data_H_temp, U8T_data_L_temp, U8RH_data_H_temp, U8RH_data_L_temp, U8checkdata_temp;
U8 U8comdata;
U16 U16temp1, U16temp2;

void Delay1000ms() //@11.0592MHz
{
    unsigned char data i, j, k;

    _nop_();
    _nop_();
    i = 43;
    j = 6;
    k = 203;
    do {
        do {
            while (--k)
                ;
        } while (--j);
    } while (--i);
}

void Delay10ms()  //@11.0592MHz
{
  unsigned char data i, j;

  i = 108;
  j = 145;
  do {
    while(--j)
      ;
  } while(--i);
}

void Delay_10us(void) {
  unsigned char data i;
  _nop_();
  i = 25;
  while(--i)
    ;
}

void Delay3s()
{
    Delay1000ms();
    Delay1000ms();
    Delay1000ms();
}

void Delay(unsigned int j)
{
    unsigned char i;
    for (; j > 0; j--) {
        for (i = 0; i < 27; i++)
            ;
    }
}
// void Delay_10us(void)
// {
//     unsigned char i;
//     i--;
//     i--;
//     i--;
//     i--;
//     i--;
//     i--;
// }

unsigned char COM(void) {
    unsigned char i,dat=0;

  for(i = 0; i < 8; i++) {
    while(P1_0)
      ;
    while(!P1_0)
      ;
    Delay_10us();
    Delay_10us();
    Delay_10us();
    Delay_10us();
    Delay_10us();
    dat <<= 1;
    if(P1_0) {
      dat |= 1;  // 0
    };
  }  // rof
  return dat;
}


//--------------------------------
//-----湿度读取子程序 ------------
//--------------------------------
//----以下变量均为全局变量--------
//----温度高8位== U8T_data_H------
//----温度低8位== U8T_data_L------
//----湿度高8位== U8RH_data_H-----
//----湿度低8位== U8RH_data_L-----
//----校验 8位 == U8checkdata-----
//----调用相关子程序如下----------
//---- Delay();, Delay_10us();,COM();
//--------------------------------

void RH(void) {
  // 主机拉低18ms
  P1_0 = 0;
  Delay10ms();
  Delay10ms();
  P1_0 = 1;
  // 总线由上拉电阻拉高 主机延时20us
  Delay_10us();
  Delay_10us();
  Delay_10us();
  // 主机设为输入 判断从机响应信号
  // P1_0 = 1;
  // 判断从机是否有低电平响应信号 如不响应则跳出，响应则向下运行
  if(!P1_0)  // T !
  {
    while(!P1_0)
      ;
    Delay_10us();
    Delay_10us();
    Delay_10us();
    Delay_10us();
    Delay_10us();
    U8RH_data_H=COM();
  }
}
void set_buffer()
{
    // unsigned char buffer[12];     // 年 月 日 时 分 秒 温度 光照 湿度 设定温度 设定光照 设定湿度
    buffer[0]  = t.hour;
    buffer[1]  = t.month;
    buffer[2]  = t.day;
    buffer[3]  = t.hour;
    buffer[4]  = t.minute;
    buffer[5]  = t.second;
    buffer[6]  = arrThermLUT[adc.Rt >> 2];
    buffer[7]  = adc.Rop;
    buffer[8]  = U8RH_data_H;
    buffer[9]  = set_temp;
    buffer[10] = set_light;
    buffer[11] = set_wet;
}

void send_info()
{
    // 发送
    char res;
    set_buffer();
    Uart1Init(9600);
    // SetUart1Rxd(buffer,8,tar_buf,0);
    // if (GetUart1TxStatus == enumBeepBusy) {
    //     while (GetUart1TxStatus == enumUart1TxFree) {
    //         res = Uart1Print(buffer, 12);
    //     }

    // } else {
    //     res = Uart1Print(buffer, 12);
    // }
    res = Uart1Print(buffer, 12);
}

// 实现翻蛋操作 根据步进电机进行操作==>nope
void move_egg()
{
    DisplayerInit();
    SetDisplayerArea(0, 7);
    StepMotorInit();
    // 每隔8小时翻蛋一次
    if (GetStepMotorStatus(enumStepMotor1) != enumStepMotorFree) {
        EmStop(enumStepMotor1);
    }
    if (flag_reset == 0) {
        SetStepMotor(enumStepMotor1, 2, 20);
    }
}

// 通风
void get_more_wind()
{
    DisplayerInit();
    SetDisplayerArea(0, 7);
    StepMotorInit();
    if (GetStepMotorStatus(enumStepMotor2) != enumStepMotorFree) {
        EmStop(enumStepMotor2);
    }
    SetStepMotor(enumStepMotor2, 5, 100);
}

// 加湿
void get_more_wet()
{
    DisplayerInit();
    SetDisplayerArea(0, 7);
    StepMotorInit();
    if (GetStepMotorStatus(enumStepMotor2) != enumStepMotorFree) {
        EmStop(enumStepMotor2);
    }
    SetStepMotor(enumStepMotor2, 5, -100);
}

// 加热
void get_more_hot()
{
    DisplayerInit();
    SetDisplayerArea(0, 7);
    StepMotorInit();
    if (GetStepMotorStatus(enumStepMotor3) != enumStepMotorFree) {
        EmStop(enumStepMotor3);
    }
    SetStepMotor(enumStepMotor3, 5, -100);
}
// 降温
void get_more_cold()
{
    DisplayerInit();
    SetDisplayerArea(0, 7);
    StepMotorInit();
    if (GetStepMotorStatus(enumStepMotor3) != enumStepMotorFree) {
        EmStop(enumStepMotor3);
    }
    SetStepMotor(enumStepMotor3, 5, 100);
}

// 自动控制温度
void auto_control_temp()
{
    int temp_now = arrThermLUT[adc.Rt >> 2];
    if (t.day <= 6) {
        if (temp_now < 38.2) {
            // 温度低
            get_more_hot();
        }
        if (temp_now > 38.7) {
            // 温度高
            get_more_cold();
        }
    }
    if (t.day > 6 && t.day <= 14) {
        if (temp_now < 37.8) {
            // 温度低
            get_more_hot();
        }
        if (temp_now > 38.1) {
            get_more_cold();
        }
    }
    if (t.day > 14 && t.day <= 21) {
        if (temp_now < 37.3) {
            // 温度低
            get_more_hot();
        }
        if (temp_now > 37.6) {
            get_more_cold();
        }
    }
}

// 自动控制湿度
void auto_control_UR()
{
    int UR_now = U8RH_data_H;
    if (UR_now < 40) {
        // 湿度低
        get_more_wet();
    }
    if (UR_now > 70) {
        // 湿度高
        get_more_wind();
    }
}

void auto_control()
{
    if (flag_auto_set == 1) {
        // 温度自动调整
        auto_control_temp();
        // 湿度自动调整
        auto_control_UR();
    }
}

// 手动控制温度
void person_control_temp()
{
    int temp_now = arrThermLUT[adc.Rt >> 2];
    if (temp_now < set_temp - 2) {
        // 升温
        get_more_hot();
    } else if (temp_now > set_temp + 2) {
        // 降温
        get_more_cold();
    } else {
        EmStop(enumStepMotor3);
    }
}

// 手动控制湿度
void preson_control_UR()
{
    int UR_now = U8RH_data_H;
    if (UR_now < set_wet - 5) {
        // 湿度低
        get_more_wet();
    }
    if (UR_now > set_wet + 5) {
        // 湿度高
        get_more_wind();
    }
}

void person_control()
{
    // 不能判断条件为=0 因为初始值为0
    if (flag_auto_set == 2) {
        person_control_temp();
        preson_control_UR();
    }
    if (flag_auto_set == 3) {
        person_control_temp(); // 自动调节温度
        get_more_wind();
    }
    if (flag_auto_set == 4) {
        person_control_temp();
        get_more_wet();
    }
    if (flag_auto_set == 5) {
        preson_control_UR();
        get_more_hot();
    }
    if (flag_auto_set == 6) {
        preson_control_UR();
        get_more_cold();
    }
}

void time_add()
{
    if (t.hour % 3 == 0 && t.minute == 0 && t.second == 0) {
        // 翻蛋
        move_egg();
        DisplayerInit();
        SetDisplayerArea(0, 7);
    }

    if (t.second % 2 == 0) {
        RH();
    }

    auto_control();
    person_control();

    t.second++;
    if (t.second == 60) {
        t.second = 0;
        t.minute++;
        if (t.minute == 60) {
            t.minute = 0;
            t.hour++;
            if (t.hour == 24) {
                t.hour = 0;
                t.day++;
                if (t.day == 32) {
                    t.day = 1;
                    t.month++;
                    if (t.month == 13) {
                        t.month = 1;
                        t.year++;
                    }
                }
            }
        }
    }
    send_info();
}

int cnt = 0;
// 实现cnt的增加 从而实现功能的转换
void cnt_add()
{
    if (GetKeyAct(enumKey1) == enumKeyPress) {
        ++cnt;
    }
}

// 表明现在数码管显示的内容是什么
void now_seg_show()
{

    if (cnt % 5 == 0) {
        Seg7Print(t.year / 10, t.year % 10, 10, t.month / 10, t.month % 10, 10, t.day / 10, t.day % 10);
    }
    if (cnt % 5 == 1) {
        Seg7Print(t.hour / 10, t.hour % 10, 10, t.minute / 10, t.minute % 10, 10, t.second / 10, t.second % 10);
    }
    if (cnt % 5 == 2) {
        int temp_adc = arrThermLUT[adc.Rt >> 2];
        Seg7Print(26, 27, temp_adc / 10, temp_adc % 10, 5, 27, set_temp / 10, set_temp % 10);
    }
    if (cnt % 5 == 3) {
        Seg7Print(28, 27, adc.Rop / 10, adc.Rop % 10, 5, 27, set_light / 10, set_light % 10);
    }
    if (cnt % 5 == 4) {
        Seg7Print(29, 27, U8RH_data_H / 10, U8RH_data_H % 10, 5, 27, set_wet / 10, set_wet % 10);
    }
}

// 显示时间
void time_show()
{
    DisplayerInit();
    Key_Init();
    SetDisplayerArea(0, 7);
    SetEventCallBack(enumEventSys1S, time_add);
    // 按下k3进行切换显示
    SetEventCallBack(enumEventKey, cnt_add);
    SetEventCallBack(enumEventSys10mS, now_seg_show);
}

// 此处可以设置时间 通过导航键以及key键进行选择

// 将孵化期间的最佳温度进行检测及时报警
void check_temp()
{
    // 此处因为测试是无法达到孵化时所需要的温度的 但是蜂鸣器过于吵闹 所以换做led闪烁（代做）
    // 假设使用串口通信将信息传至电脑上就不需要用led
    int temp_now = arrThermLUT[adc.Rt >> 2];
    BeepInit();
    if (t.day <= 6) {
        if (temp_now < 38.3 || temp_now > 38.6) {
            // 报警
            SetBeep(2000, 200);
        }
    }
    if (t.day > 6 && t.day <= 14) {
        if (temp_now < 37.8 || temp_now > 38.1) {
            // 报警
            SetBeep(2000, 200);
        }
    }
    if (t.day > 14 && t.day <= 21) {
        if (temp_now < 37.3 || temp_now > 37.6) {
            // 报警
            SetBeep(2000, 200);
        }
    }
}

void get_adc()
{
    adc = GetADC();
    // RH();
    // check_temp();
}

// 显示温度
void temp_show()
{
    AdcInit(ADCexpEXT);
    DisplayerInit();
    Key_Init();
    SetEventCallBack(enumEventSys100mS, get_adc);
}

// 显示光照
void light_show()
{
    AdcInit(ADCexpEXT);
    DisplayerInit();
    Key_Init();
}

// 判断是否进行设置
int whether_set()
{
    int i, sum = 0;
    for (i = 0; i < 8; i++) {
        sum += get_buffer[i];
    }
    if (sum != 0 && sum != 10) {
        return 1;
    } else {
        return 2;
    }
}

// 返回2表示我此前没设置过参数，因此此处按下的单个操作只是执行一次即可
int whether_set_paraments()
{
    if (set_light != 0 || set_temp != 0 || set_wet != 0) {
        return 1;
    } else {
        return 2;
    }
}

// 接收数据 并且处理数据 因为数据可能包含多种
void deal_info()
{
    int flag_person_set = whether_set();
    int whether_set_pars;

    flag_reset = 0;
    // unsigned char get_buffer[8]温度 光照 湿度 是否翻蛋 是否通风 是否加湿 是否加热 是否降温
    set_temp      = get_buffer[0];
    set_light     = get_buffer[1];
    set_wet       = get_buffer[2];
    flag_move_egg = get_buffer[3];
    flag_get_wind = get_buffer[4];
    flag_get_wet  = get_buffer[5];
    flag_get_hot  = get_buffer[6];
    flag_get_cold = get_buffer[7];
    if (flag_get_wind == 2 && flag_move_egg == 2 && flag_get_hot == 2 && flag_get_wet == 2 && flag_get_cold == 2) {
        t.year     = 0;
        t.month    = 0;
        t.day      = 0;
        t.hour     = 0;
        t.minute   = 0;
        t.second   = 0;
        flag_reset = 1;
    }

    flag_auto_set = 0;
    if (flag_move_egg == 3) {
        EmStop(enumStepMotor2);
        EmStop(enumStepMotor3);
        flag_auto_set = 1; // 进入auto状态
        return;
    }
    whether_set_pars = whether_set_paraments(); //==>重置之后为2

    // 8个参数中有不为零的参数
    if (whether_set_pars == 1) {
        EmStop(enumStepMotor2);
        EmStop(enumStepMotor3);
        flag_auto_set = 2;
    }

    // 重置之后进入初始状态
    if (flag_reset == 1) {
        // move_egg();
        EmStop(enumStepMotor1);
        EmStop(enumStepMotor2);
        EmStop(enumStepMotor3);
        flag_auto_set = 0;
    }

    // // if (flag_person_set == 1) {
    //     EmStop(enumStepMotor2);
    //     EmStop(enumStepMotor3);
    //     if (whether_set_pars == 1) {
    //         flag_auto_set = 2;
    //         // 按下set后进入自动调节
    //         //  return;
    //     }
    // }

    if (flag_move_egg == 1) {
        if (whether_set_pars == 2) {
            move_egg();
        } else {
            move_egg();
            flag_auto_set = 2;
        }
    }

    if (flag_get_wind == 1) {
        if (whether_set_pars == 2) {
            get_more_wind();
        } else {
            get_more_wind();
            EmStop(enumStepMotor2);
            flag_auto_set = 3;
            // return;
        }
    }
    if (flag_get_wet == 1) {
        if (whether_set_pars == 2) {
            get_more_wet();
        } else {
            get_more_wet();
            EmStop(enumStepMotor2);
            flag_auto_set = 4;
            // return;
        }
    }
    if (flag_get_hot == 1) {
        if (whether_set_pars == 2) {
            get_more_hot();
        } else {
            get_more_hot();
            EmStop(enumStepMotor3);
            flag_auto_set = 5;
            // return;
        }
    }
    if (flag_get_cold == 1) {
        if (whether_set_pars == 2) {
            get_more_cold();
        } else {
            get_more_cold();
            EmStop(enumStepMotor3);
            flag_auto_set = 6;
            // return;
        }
    }
}

void main()
{
    DisplayerInit();
    SetDisplayerArea(0, 7);
    time_show();
    temp_show();
    Uart1Init(9600);
    SetUart1Rxd(get_buffer, 8, tar_buf, 0);
    SetEventCallBack(enumEventUart1Rxd, deal_info);

    MySTC_Init(); // MySTC_OS 初始化         //此行必须！！！
    while (1)     // 系统主循环              //此行必须！！！
    {
        MySTC_OS(); // MySTC_OS 加载           //此行必须！！！
    }
}
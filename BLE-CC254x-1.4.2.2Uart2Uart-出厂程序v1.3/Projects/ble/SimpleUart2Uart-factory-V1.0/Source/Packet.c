#include "stdio.h"
#include "string.h"
#include "hal_mcu.h"
#include "hal_defs.h"
#include "hal_types.h"
#include "hal_led.h"
#include "hal_adc.h"
#include "simpleBLE.h"
#include "gatt.h"
#include "simpleGATTprofile.h"
#include "simpleBLEPeripheral.h"
#include "npi.h"
#include "hal_led.h"
#include "hal_lcd.h"
#include "hal_other.h"
#include "Packet_YB.h"
#include "Packet.h"
#include "peripheral.h"
#include "hal_mcu.h"
#include "OnBoard.h"

void Heat_Control_Low(uint8 temperature);
void Heat_Control_High(uint8 temperature);
void Heat_Control_OFF(void);
void Packet_StateInit(void);
void Packet_Init(void);
void Packet_End(void);
void PacketPeriodicTask_1( void );
void Start_Beep(void);
void End_Beep(void);
uint16 Packet_ADC_14(PAK_ADC_CHANNEL channel);
uint8 Exchange_BatADC(uint16 dat);
uint8 Exchange_TempADC(uint16 dat);
uint8 Mode_base(uint16 Freq,uint16 per,uint16 jiange_time);
void ZJ_mode();
void AM_mode();
void CJ_mode();
void TN_mode();

Packet_Attribute Packet_1;
uint8 Beep_flag;

extern uint16 gapConnHandle ;
extern void DelayMS(uint16 ms);
extern void DelayUS(uint16 us);

typedef unsigned char uchar;
typedef unsigned int  uint;


uint count;             //用于定时器计数
uint Flag_Timer0_up;
uint Flag_Timer0_down;
uint Flag_TImer0_cishu;
uint pn_conut;
uint8 temp_D = 10;
uint Flag_Timer0;
uint8 Mode_conut = 0;
uint8 Mode_flag = 0;
int  qiangdu = 0;



void Packet_Init(void)
{
 
//  HAL_OFF_IO_HEAT();
  //HAL_ON_IO_SwLock();
  Packet_StateInit();
//  HAL_ON_IO_HEAT();
//  DelayMS(1500);
  Packet_1.PAK.Battery_capacity = Exchange_BatADC(Packet_ADC_14(BAT)); 
//  HAL_OFF_IO_HEAT();
  //Start_Beep();
}
void Packet_StateInit()
{
  //Packet_1.PAK.Heat_Level       = Heat_Low;  
  Packet_1.PAK.Heat_State       = Heat_OFF;
  Packet_1.PAK.Work_State       = Work_Stop;
  
  Mode_flag = 1;
  
  Packet_1.PAK.Cure_Min         = 0;
  Packet_1.PAK.Cure_Second      = 0;
//  Packet_1.PAK.Temperature      = Exchange_TempADC(Packet_ADC_14(TEMP));
  Packet_1.PAK.Temperature      = Packet_1.YB.LOCAL_MSG[2];
  Packet_1.PAK.Cure_State       = 0;
  Packet_1.YB.Disconnect_Flag   = 0;
  Packet_1.PAK.LED_Status       = 1;
//  Packet_1.PAK.LED_Status       = 0;
  Packet_1.PAK.Apparatus_Status = 1;
  Packet_1.YB.Write_Flag        = 0;
  Packet_1.YB.Connect_Chek      = 0;
  Packet_1.YB.ReConnect_Flag    = 0;
  memset(&Packet_1.YB.CMNC_MSG,   0, YB_CMNC_MSG_LEN);
  memset(&Packet_1.YB.LOCAL_MSG,  0, YB_LOCAL_MSG_LEN);
  memset(&Packet_1.YB.Control_Buf,0, YB_CONTROL_BUF_LEN);
//  HAL_TURN_OFF_LEDB();
//  LED_B = 0;
//  Packet_Attribute_1.Device_Name     = 0;
//  sprintf((char*)Packet_Attribute_1.Device_Version, DEV_VERSION);  
}
void Packet_End(void)
{
  HAL_SYSTEM_RESET();
}


const uint8  battery    [21] = {0,      5,  10,  15,  20,  25,  30,  35,  40,  45,  50,  55,  60,  65,  70,  75,  80,  85,  90,  95,  99};
//const uint16 battery_ADC[21] = {2660,2670,2680,2690,2700,2710,2728,2746,2764,2782,2800,2834,2868,2902,2936,2970,3000,3030,3060,3090,3120};
//const uint16 battery_ADC[21] = {2410,2470,2530,2590,2650,2710,2728,2746,2764,2782,2800,2834,2868,2902,2936,2970,3000,3030,3060,3090,3120};
//const uint16 battery_ADC[21] = {1160,1181,1201,1222,1242,1263,1283,1304,1324,1345,1365,1372,1379,1385,1392,1399,1406,1413,1420,1426,1433};//1024
//const uint16 battery_ADC[21] = {4220,4295,4369,4444,4518,4593,4667,4742,4816,4891,4965,4990,5015,5039,5064,5089,5114,5139,5163,5188,5213};//4096
const uint16 battery_ADC[21] = {4195,4295,4369,4444,4518,4593,4667,4704,4741,4778,4816,4832,4849,4855,4872,4888,4904,4921,4937,4954,4980};//4096

uint8 Exchange_BatADC(uint16 dat)
{
  uint16 buff;
  uint8  i;
  buff = dat;
  if (buff < battery_ADC[0])
  { 
     return 0xff;       
  }
  else if (buff > battery_ADC[20])
  {
     return battery[20];
  }
  else 
  {
    for (i = 0; i < 20; i++)
    {
       if (buff >= battery_ADC[i] && buff <= battery_ADC[i + 1])
      {
          return battery[i];
       }
    }
  }
  return 0xee;
}




#define READ_TIMES 12
#define LOST_VAL   2
uint16 Packet_ADC_14(PAK_ADC_CHANNEL channel)
{
  uint16 buf[READ_TIMES];
  uint8  i,j;
  uint16 sum=0;
  uint16 temp;
  
  for(i=0;i<READ_TIMES;i++)buf[i] = HalAdcRead (channel, HAL_ADC_RESOLUTION_14);

  for(i=0;i<READ_TIMES-1;i++)
  {
    for(j=i+1;j<READ_TIMES;j++)
    {
      if(buf[i]>buf[j])
      {
         temp=buf[i];
         buf[i]=buf[j];
         buf[j]=temp;
      }
    }
  }
  
  for(i=LOST_VAL;i<READ_TIMES-LOST_VAL;i++)      sum+=buf[i];
  
  temp=sum/(READ_TIMES-2*LOST_VAL); 
  
  return temp;
}



/****************************************************************************
* 名    称: InitLed()
* 功    能: 设置LED灯相应的IO口
* 入口参数: 无
* 出口参数: 无
****************************************************************************/
void InitLed(void)
{  
    P0SEL |= 0x80;                            //P0.7设置为外设功能，其他P0口设为通用IO口
    P1SEL &= 0x00;                           //P1口全部设为通用IO口
    P0DIR |= ((1<<0)|(1<<1)|(1<<3)|(1<<4)|(1<<6));  //P0.0 P0.1 P0.3 P0.4 P0.6定义为输出
    P0DIR &= ~((1<<2)|(1<<5)|(1<<7));       //P0.2 P0.5 P0.7定义为输入
    P0INP |=((1<<5)|(1<<7));           
    //P0.5  P0.7设置为三态
    P1DIR &=~((1<<0)|(1<<3));                      //P1.0 P1.3设置为输入
    P1INP |= (1<<0);                           //p1.0三态
    P1DIR |= (1<<2);                          //P1.2设置为输出
/*---------------------------------PWM引脚设置-----------------------------------------*/     
    P0SEL |=(1<<4);    //P0.4设置为外设I/O口：定时器1通道2
    P0SEL &=~(1<<3);
    PERCFG &= ~(1<<6);    //定时器1为外设位置1

/*-------------------------------------------------------------------------------*/      
    OUT_UP = 0;
    OUT_DOWN = 0;
//    Flag_Timer0_up = 0;
//    Flag_Timer0_down = 0;
//    Flag_TImer0_cishu = 0;
//    Flag_Timer0 = 0;  
//    pn_conut = 0;
    
} 
    

/****************************************************************************
* 名    称: InitT3()
* 功    能: 定时器初始化，系统不配置工作时钟时默认是2分频，即16MHz
* 入口参数: 无
* 出口参数: 无
****************************************************************************/
void InitT3()  //200u  改波形尖峰
{  
 /*-------------------------------PWM(周期28us，高电平18，低电平10)----------------------------------------*/  
  PERCFG |=0x03;
  T1CTL  |= 0x08;     //定时器1设置为32分频，正计数/倒计数模式 1us
  T1CCTL2 |= 0x2C;    //定时器1通道2配置为比较输出，模式101,开通道2中断
  T1CC0L = 14;     //定时器1通道0捕获/比较值低位
  T1CC0H = 0;     //定时器1通道0捕获/比较值高位  
  T1CC2L = 4;     //定时器1通道2捕获/比较值低位
  T1CC2H = 0;     //定时器1通道2捕获/比较值高位
/*--------------------------------定时器中断（100us进入一次）------------------------------------------*/ 
  T3CTL  = 0xe6;                //定时器计数器自加1为4us
  T3CCTL0 = 0x54;
  T3CC0 = 25;                  //上限值设置10  
 
  IEN1 =0x08; 
  IEN0 =0x80; 
  EA = 1;
  T3CTL |= 0x10;             // start timer 3 
  
}
uint16 cd_time = 0;
uint time_num = 0;
uint time_pn = 0;
uint8 zj_time = 0;
uint16 jiange_num = 0;
uint16 cishu = 0;
uint8 moshi_num = 0;

//定时器T3中断处理函数  100us进去一次中断
#pragma vector = T3_VECTOR 
__interrupt void T3_ISR(void) 
{       
    IRCON = 0;
    if(Mode_flag == 1)
    {
      ZJ_mode();
    }
    else if(Mode_flag == 2)
    {
      
      AM_mode();
    }
    else if(Mode_flag == 3)
    {
      CJ_mode();
    }
    else if(Mode_flag == 4)
    {
      TN_mode();
    }    
}
/*---------------------Freq：强度，per：整个周期，jiangge_time：延时（不工作的时间）------------------------------------------------------*/ 
uint8 Mode_base(uint16 Freq,uint16 per,uint16 jiange_time)
{       
    jiange_num++;
    if(jiange_time < jiange_num)
    {
      if(per == 0)
      {
        cishu++;
        return 0;
      }
    
      zj_time = per / 15;
      cd_time =  Freq * zj_time; //充电时间
      time_num++;
      
      if(cd_time > time_num)
      {
        T1CTL  |= 0x0B;
        P0SEL  |=(1<<4);
      }
      
      if(cd_time == time_num)
      {
         T1CTL  |= 0x08;
         P0SEL &=~(1<<4);
         OUT_DOWN = 0;
         
         DelayUS(4);   
         if(!time_pn)
         {
             P_Ctrl = 1;   
             DelayUS(220); //200us  
             P_Ctrl = 0;
             time_pn = 1;
         }
         else
         {
            N_Ctrl = 1;
            DelayUS(220);  
            N_Ctrl = 0;       
            time_pn = 0;
         }
         
         DelayUS(15); 
         OUT_UP = 1;
         DelayUS(8);   
         OUT_UP = 0;
      }
      
       if(time_num == per)
       {
         time_num = 0;
         jiange_num=0;
         cishu++;
       }
  }
  return 0;
}     

void ZJ_mode()
{ 
  switch(moshi_num)
  {
    case 0 :  Mode_base(qiangdu,500,800); if(cishu == 10)   moshi_num = 1;    break;
    case 1 :  Mode_base(qiangdu,500,500); if(cishu == 25)   moshi_num = 2;    break;
    case 2 :  Mode_base(qiangdu,500,250); if(cishu == 40)   moshi_num = 3;    break;
    case 3 :  Mode_base(qiangdu,500,100); if(cishu == 60)   moshi_num = 4;    break;
    case 4 :  Mode_base(qiangdu,500,50);  if(cishu == 80)   moshi_num = 5;    break;
    case 5 :  Mode_base(qiangdu,500,0);   if(cishu == 100)  moshi_num = 6;    break;
    case 6 :  Mode_base(qiangdu,300,0);   if(cishu == 130)  moshi_num = 7;    break;
    case 7 :  Mode_base(qiangdu,0,3000);  if(cishu == 131)  moshi_num = 8;    break;
    case 8 :  Mode_base(qiangdu,500,800); if(cishu == 141)  moshi_num = 9;    break;
    case 9 :  Mode_base(qiangdu,500,500); if(cishu == 156)  moshi_num = 10;   break;
    case 10 : Mode_base(qiangdu,500,250); if(cishu == 171)  moshi_num = 11;   break;
    case 11 : Mode_base(qiangdu,500,100); if(cishu == 191)  moshi_num = 12;   break;
    case 12 : Mode_base(qiangdu,500,50);  if(cishu == 211)  moshi_num = 13;   break;
    case 13 : Mode_base(qiangdu,500,0);   if(cishu == 231)  moshi_num = 14;   break;
    case 14 : Mode_base(qiangdu,300,0);   if(cishu == 261)  moshi_num = 15;   break;
    case 15 : Mode_base(qiangdu,0,3000);  if(cishu == 262)  moshi_num = 16;   break;
    case 16 : Mode_base(qiangdu,500,800); if(cishu == 272)  moshi_num = 17;   break;
    case 17 : Mode_base(qiangdu,500,500); if(cishu == 287)  moshi_num = 18;   break;  
    case 18 : Mode_base(qiangdu,500,250); if(cishu == 302)  moshi_num = 19;   break;
    case 19 : Mode_base(qiangdu,500,100); if(cishu == 322)  moshi_num = 20;   break;
    case 20 : Mode_base(qiangdu,500,50);  if(cishu == 342)  moshi_num = 21;   break;
    case 21 : Mode_base(qiangdu,500,0);   if(cishu == 362)  moshi_num = 22;   break;
    case 22 : Mode_base(qiangdu,300,0);   if(cishu == 392)  moshi_num = 23;   break;
    case 23 : Mode_base(qiangdu,0,3000);  if(cishu == 393)  moshi_num = 24;   break;
    case 24 : Mode_base(qiangdu,500,800); if(cishu == 403)  moshi_num = 25;   break;
    case 25 : Mode_base(qiangdu,500,500); if(cishu == 418)  moshi_num = 26;   break;  
    case 26 : Mode_base(qiangdu,500,250); if(cishu == 433)  moshi_num = 27;   break;
    case 27 : Mode_base(qiangdu,500,100); if(cishu == 453)  moshi_num = 28;   break;
    case 28 : Mode_base(qiangdu,500,50);  if(cishu == 473)  moshi_num = 29;   break;
    case 29 : Mode_base(qiangdu,500,0);   if(cishu == 493)  moshi_num = 30;   break;
    case 30 : Mode_base(qiangdu,300,0);   if(cishu == 523)  moshi_num = 31;   break;
    case 31 : Mode_base(qiangdu,0,3000);  if(cishu == 524)  moshi_num = 32;   break;
    case 32 : Mode_base(qiangdu,2000,500); if(cishu == 554) moshi_num = 33;   break;
    case 33 : Mode_base(qiangdu,0,4000);   if(cishu == 555) moshi_num = 34;   break;
    default: moshi_num=0;cishu = 0; break;
  }

}


void TN_mode()
{
   switch(moshi_num)
  {
    case 0 :  Mode_base(qiangdu,500,800); if(cishu == 16)   moshi_num = 1;    break;
    case 1 :  Mode_base(qiangdu,500,600); if(cishu == 36)   moshi_num = 2;    break;
    case 2 :  Mode_base(qiangdu,500,200); if(cishu == 60)   moshi_num = 3;    break;
    case 3 :  Mode_base(qiangdu,500,50);  if(cishu == 80)   moshi_num = 4;    break;
    case 4 :  Mode_base(qiangdu,500,20);  if(cishu == 95)   moshi_num = 5;    break;
    case 5 :  Mode_base(qiangdu,500,0);   if(cishu == 110)  moshi_num = 6;    break;
    case 6 :  Mode_base(qiangdu,0,2000);  if(cishu == 111)  moshi_num = 7;    break;
    case 7 :  Mode_base(qiangdu,500,800); if(cishu == 127)  moshi_num = 8;    break;
    case 8 :  Mode_base(qiangdu,500,600); if(cishu == 147)  moshi_num = 9;    break;
    case 9 :  Mode_base(qiangdu,500,200); if(cishu == 167)  moshi_num = 10;   break;
    case 10 : Mode_base(qiangdu,500,50);  if(cishu == 187)  moshi_num = 11;   break;
    case 11 : Mode_base(qiangdu,500,20);  if(cishu == 200)  moshi_num = 12;   break;
    case 12 : Mode_base(qiangdu,500,0);   if(cishu == 215)  moshi_num = 13;   break;
    case 13 : Mode_base(qiangdu,0,2000);  if(cishu == 216)  moshi_num = 14;   break;
    case 14 : Mode_base(qiangdu,500,1000);if(cishu == 230)  moshi_num = 15;   break;
    case 15 : Mode_base(qiangdu,500,200); if(cishu == 250)  moshi_num = 16;   break;
    case 16 : Mode_base(qiangdu,500,0);   if(cishu == 280)  moshi_num = 17;   break;
    case 17 : Mode_base(qiangdu,0,2000);  if(cishu == 281)  moshi_num = 18;   break;  
    case 18 : Mode_base(qiangdu,500,500); if(cishu == 300)  moshi_num = 19;   break;
    case 19 : Mode_base(qiangdu,0,2000);  if(cishu == 301)  moshi_num = 20;   break;
    case 20 : Mode_base(qiangdu,500,250); if(cishu == 320)  moshi_num = 21;   break;
    case 21 : Mode_base(qiangdu,0,2000);  if(cishu == 321)  moshi_num = 22;   break;
    case 22 : Mode_base(qiangdu,500,0);   if(cishu == 340)  moshi_num = 23;   break;
    case 23 : Mode_base(qiangdu,0,3000);  if(cishu == 341)  moshi_num = 24;   break;
    case 24 : Mode_base(qiangdu,300,0);   if(cishu == 380)  moshi_num = 25;   break;
    case 25 : Mode_base(qiangdu,0,2000);  if(cishu == 381)  moshi_num = 26;   break;
    case 26 : Mode_base(qiangdu,250,0);   if(cishu == 450)  moshi_num = 27;   break;
    case 27 : Mode_base(qiangdu,0,2000);  if(cishu == 451)  moshi_num = 28;   break;
    case 28 : Mode_base(qiangdu,1000,500);if(cishu == 470)  moshi_num = 29;   break;
    case 29 : Mode_base(qiangdu,0,2000);  if(cishu == 471)  moshi_num = 30;   break;
    case 30 : Mode_base(qiangdu,1000,0);  if(cishu == 490)  moshi_num = 31;   break;
    case 31 : Mode_base(qiangdu,0,2000);  if(cishu == 491)  moshi_num = 32;   break;
    case 32 : Mode_base(qiangdu,500,0);   if(cishu == 510)  moshi_num = 33;   break;
    case 33 : Mode_base(qiangdu,0,2000);  if(cishu == 511)  moshi_num = 34;   break;
    case 34 : Mode_base(qiangdu,400,0);   if(cishu == 540)  moshi_num = 35;   break;
    case 35 : Mode_base(qiangdu,0,2000);  if(cishu == 541)  moshi_num = 36;   break;
    case 36 : Mode_base(qiangdu,500,500); if(cishu == 560)  moshi_num = 37;   break;
    case 37 : Mode_base(qiangdu,0,2000);  if(cishu == 561)  moshi_num = 38;   break;
    case 38 : Mode_base(qiangdu,1000,500);if(cishu == 574)  moshi_num = 39;   break;
    case 39 : Mode_base(qiangdu,0,2000);  if(cishu == 575)  moshi_num = 40;   break;
    case 40 : Mode_base(qiangdu,1000,0);  if(cishu == 590)  moshi_num = 41;   break;
    case 41 : Mode_base(qiangdu,0,2000);  if(cishu == 591)  moshi_num = 42;   break;
    case 42 : Mode_base(qiangdu,400,0);   if(cishu == 610)  moshi_num = 43;   break;
    case 43 : Mode_base(qiangdu,0,2000);  if(cishu == 611)  moshi_num = 44;   break;
    case 44 : Mode_base(qiangdu,2300,0);  if(cishu == 650)  moshi_num = 45;   break;
    case 45 : Mode_base(qiangdu,0,2000);  if(cishu ==651)   moshi_num = 46;   break;
    case 46 : Mode_base(qiangdu,2000,500);if(cishu == 665)  moshi_num = 47;   break;
    case 47 : Mode_base(qiangdu,0,5000);  if(cishu == 666)  moshi_num = 48;   break;
    default: moshi_num=0;cishu = 0; break;
  }

}

void AM_mode()
{
 switch(moshi_num)
  {
    case 0 :  Mode_base(qiangdu,1000,1000);  if(cishu == 10)   moshi_num = 1;    break;
    case 1 :  Mode_base(qiangdu,0,2000);     if(cishu == 11)   moshi_num = 2;    break;
    case 2 :  Mode_base(qiangdu,1000,0);     if(cishu == 21)   moshi_num = 3;    break;
    case 3 :  Mode_base(qiangdu,0,2000);     if(cishu == 22)   moshi_num = 4;    break;
    case 4 :  Mode_base(qiangdu,500,500);    if(cishu == 32)   moshi_num = 5;    break;
    case 5 :  Mode_base(qiangdu,0,2000);     if(cishu == 33)   moshi_num = 6;    break;
    case 6 :  Mode_base(qiangdu,500,250);    if(cishu == 43)   moshi_num = 7;    break;
    case 7 :  Mode_base(qiangdu,0,2000);     if(cishu == 44)   moshi_num = 8;    break;
    case 8 :  Mode_base(qiangdu,500,0);      if(cishu == 59)   moshi_num = 9;    break;
    case 9 :  Mode_base(qiangdu,0,2000);     if(cishu == 60)   moshi_num = 10;   break;
    case 10 : Mode_base(qiangdu,400,400);    if(cishu == 80)   moshi_num = 11;   break;
    case 11 : Mode_base(qiangdu,0,2000);     if(cishu == 81)   moshi_num = 12;   break;
    case 12 : Mode_base(qiangdu,1000,500);   if(cishu == 91)   moshi_num = 13;   break;
    case 13 : Mode_base(qiangdu,0,2000);     if(cishu == 92)   moshi_num = 14;   break;
    case 14 : Mode_base(qiangdu,500,500);    if(cishu == 107)  moshi_num = 15;   break;
    case 15 : Mode_base(qiangdu,0,2000);     if(cishu == 108)  moshi_num = 16;   break;
    case 16 : Mode_base(qiangdu,500,0);      if(cishu == 128)  moshi_num = 17;   break;
    case 17 : Mode_base(qiangdu,0,2000);     if(cishu == 129)  moshi_num = 18;   break;  
    case 18 : Mode_base(qiangdu,400,0);      if(cishu == 150)  moshi_num = 19;   break;
    case 19 : Mode_base(qiangdu,0,2000);     if(cishu == 151)  moshi_num = 20;   break;
    case 20 : Mode_base(qiangdu,1000,500);   if(cishu == 161)  moshi_num = 21;   break;
    case 21 : Mode_base(qiangdu,0,2000);     if(cishu == 162)  moshi_num = 22;   break;
    case 22 : Mode_base(qiangdu,500,500);    if(cishu == 177)  moshi_num = 23;   break;
    case 23 : Mode_base(qiangdu,0,2000);     if(cishu == 178)  moshi_num = 24;   break;
    case 24 : Mode_base(qiangdu,500,0);      if(cishu == 198)  moshi_num = 25;   break;
    case 25 : Mode_base(qiangdu,0,2000);     if(cishu == 199)  moshi_num = 26;   break;
    case 26 : Mode_base(qiangdu,400,0);      if(cishu == 220)  moshi_num = 27;   break;
    case 27 : Mode_base(qiangdu,0,2000);     if(cishu == 221)  moshi_num = 28;   break;
    case 28 : Mode_base(qiangdu,2000,0);     if(cishu == 241)  moshi_num = 29;   break;
    case 29 : Mode_base(qiangdu,0,2000);     if(cishu == 242)  moshi_num = 30;   break;
    case 30 : Mode_base(qiangdu,1000,0);     if(cishu == 272)  moshi_num = 31;   break;
    case 31 : Mode_base(qiangdu,0,2000);     if(cishu == 273)  moshi_num = 32;   break;
    case 32 : Mode_base(qiangdu,400,0);      if(cishu == 300)  moshi_num = 33;   break;
    case 33 : Mode_base(qiangdu,0,2000);     if(cishu == 301)  moshi_num = 34;   break;
    case 34 : Mode_base(qiangdu,300,0);      if(cishu == 330)  moshi_num = 35;   break;
    case 35 : Mode_base(qiangdu,0,2000);     if(cishu == 331)  moshi_num = 36;   break;
    case 36 : Mode_base(qiangdu,2000,0);     if(cishu == 351)  moshi_num = 37;   break;
    case 37 : Mode_base(qiangdu,0,2000);     if(cishu == 352)  moshi_num = 38;   break;
    case 38 : Mode_base(qiangdu,1000,0);     if(cishu == 382)  moshi_num = 39;   break;
    case 39 : Mode_base(qiangdu,0,2000);     if(cishu == 383)  moshi_num = 40;   break;
    default: moshi_num=0;cishu = 0; break;
  }

}

void CJ_mode()
{
   switch(moshi_num)
  {
    case 0 :  Mode_base(qiangdu,1000,1000);  if(cishu == 10)   moshi_num = 1;    break;
    case 1 :  Mode_base(qiangdu,0,2000);     if(cishu == 11)   moshi_num = 2;    break;
    case 2 :  Mode_base(qiangdu,1000,800);   if(cishu == 21)   moshi_num = 3;    break;
    case 3 :  Mode_base(qiangdu,0,2000);     if(cishu == 22)   moshi_num = 4;    break;
    case 4 :  Mode_base(qiangdu,500,500);    if(cishu == 32)   moshi_num = 5;    break;
    case 5 :  Mode_base(qiangdu,0,2000);     if(cishu == 33)   moshi_num = 6;    break;
    case 6 :  Mode_base(qiangdu,1000,1000);  if(cishu == 43)   moshi_num = 7;    break;
    case 7 :  Mode_base(qiangdu,0,2000);     if(cishu == 44)   moshi_num = 8;    break;
    case 8 :  Mode_base(qiangdu,1000,800);   if(cishu == 59)   moshi_num = 9;    break;
    case 9 :  Mode_base(qiangdu,0,2000);     if(cishu == 60)   moshi_num = 10;   break;
    case 10 : Mode_base(qiangdu,500,500);    if(cishu == 80)   moshi_num = 11;   break;
    case 11 : Mode_base(qiangdu,0,2000);     if(cishu == 81)   moshi_num = 12;   break;
    case 12 : Mode_base(qiangdu,300,300);    if(cishu == 91)   moshi_num = 13;   break;
    case 13 : Mode_base(qiangdu,0,2000);     if(cishu == 92)   moshi_num = 14;   break;
    case 14 : Mode_base(qiangdu,500,500);    if(cishu == 107)  moshi_num = 15;   break;
    case 15 : Mode_base(qiangdu,0,2000);     if(cishu == 108)  moshi_num = 16;   break;
    case 16 : Mode_base(qiangdu,500,0);      if(cishu == 128)  moshi_num = 17;   break;
    case 17 : Mode_base(qiangdu,0,2000);     if(cishu == 129)  moshi_num = 18;   break;  
    case 18 : Mode_base(qiangdu,1000,0);     if(cishu == 150)  moshi_num = 19;   break;
    case 19 : Mode_base(qiangdu,0,2000);     if(cishu == 151)  moshi_num = 20;   break;
    case 20 : Mode_base(qiangdu,500,0);      if(cishu == 161)  moshi_num = 21;   break;
    case 21 : Mode_base(qiangdu,0,2000);     if(cishu == 162)  moshi_num = 22;   break;
    case 22 : Mode_base(qiangdu,500,500);    if(cishu == 177)  moshi_num = 23;   break;
    case 23 : Mode_base(qiangdu,0,2000);     if(cishu == 178)  moshi_num = 24;   break;
    case 24 : Mode_base(qiangdu,500,1000);   if(cishu == 198)  moshi_num = 25;   break;
    case 25 : Mode_base(qiangdu,0,2000);     if(cishu == 199)  moshi_num = 26;   break;
    case 26 : Mode_base(qiangdu,500,500);    if(cishu == 220)  moshi_num = 27;   break;
    case 27 : Mode_base(qiangdu,0,2000);     if(cishu == 221)  moshi_num = 28;   break;
    case 28 : Mode_base(qiangdu,500,1000);   if(cishu == 241)  moshi_num = 29;   break;
    case 29 : Mode_base(qiangdu,0,2000);     if(cishu == 242)  moshi_num = 30;   break;
    case 30 : Mode_base(qiangdu,1000,0);     if(cishu == 272)  moshi_num = 31;   break;
    case 31 : Mode_base(qiangdu,0,2000);     if(cishu == 273)  moshi_num = 32;   break;
    case 32 : Mode_base(qiangdu,500,0);      if(cishu == 300)  moshi_num = 33;   break;
    case 33 : Mode_base(qiangdu,0,2000);     if(cishu == 301)  moshi_num = 34;   break;
    case 34 : Mode_base(qiangdu,500,0);      if(cishu == 330)  moshi_num = 35;   break;
    case 35 : Mode_base(qiangdu,0,2000);     if(cishu == 331)  moshi_num = 36;   break;
    case 36 : Mode_base(qiangdu,1000,0);     if(cishu == 351)  moshi_num = 37;   break;
    case 37 : Mode_base(qiangdu,0,2000);     if(cishu == 352)  moshi_num = 38;   break;
    case 38 : Mode_base(qiangdu,1000,500);   if(cishu == 382)  moshi_num = 39;   break;
    case 39 : Mode_base(qiangdu,0,2000);     if(cishu == 383)  moshi_num = 40;   break;
    default: moshi_num=0;cishu = 0; break;
  }
}

void PacketPeriodicTask_1( void )//封包周期性任务
{
  //if(Packet_1.PAK.Cure_State)
 // {
    if(qiangdu == 0)
      Packet_1.PAK.Work_State = Work_Stop;
    else
      Packet_1.PAK.Work_State = Work_Run;
    
    /************************************暂停状态响应**********************************/
    if(Packet_1.PAK.Work_State == Work_Stop)
    {
       T3IE = 0;
    }
    /************************************运行状态响应***********************************/
    else if(Packet_1.PAK.Work_State == Work_Run)
    {     
        T3IE = 1;
        InitT3();            //设置T3相应的寄存器
    //    InitLed();		     //设置LED灯相应的IO口
        
//       if(Packet_1.PAK.Heat_Level == Heat_Low)
//      {
//         //temp_D = 10;//低档
//       }
//      else if(Packet_1.PAK.Heat_Level == Heat_High)
//      {
//         //temp_D = 18;//高档
//      }
//         if(Key_Up == 0)
//        {
//          DelayMS(1);
//          if(Key_Up == 0)
//          {
//            LED_B = ~LED_B;
//          }
//          while(!Key_Up);
//        }
      
      
    }
    /**********************************停止状态响应***********************************/

    else
    {;}
    
//  }
}



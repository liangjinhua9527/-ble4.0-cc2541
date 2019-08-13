#ifndef _PACKET_H_
#define _PACKET_H_
#include "Packet_YB.h"
#include "hal_adc.h"

#define Heat_High_Strat  44
#define Heat_High_Stop   46

#define Heat_Low_Start   41
#define Heat_Low_Stop    43

#define Heat_Low     1
#define Heat_High    2

#define Heat_OFF     1
#define Heat_ON      2

#define Work_Stop    1
#define Work_Run     2
#define Work_Pause   3

#define OUT_DOWN  P0_4   //充电    
#define OUT_UP    P0_3  //放电
#define P_Ctrl P0_0
#define N_Ctrl P0_1

#define DEV_VERSION   "005A\0"
typedef struct
{
  uint8    Set_time;
  uint8    Set_time_h;
  uint8    Set_time_l;
  uint8    Heat_Level;
  uint8    Heat_Level_h;
  uint8    Heat_Level_l;
  uint8    Heat_State;
  uint8    Work_State; 
  uint8    Cure_Min;
  uint8    Cure_Second;
  uint8    Cure_State; 
  uint16   Battery_capacity;
  uint8    Temperature;
  uint8    LED_Status;
  bool     Apparatus_Status;
}PAK_INFORMATION;

typedef struct
{
   uint8    Device_Name[19];
   uint8    Device_Version[5];  
}DEV_INFORMATION;

typedef struct
{
  PAK_INFORMATION  PAK;
  DEV_INFORMATION  DEV;
   YB_INFORMATION  YB;
}Packet_Attribute;

typedef enum PAK_ADC_CHANNEL
{
//  BAT  = HAL_ADC_CHANNEL_3,
  BAT  = HAL_ADC_CHANNEL_7
//  TEMP = HAL_ADC_CHANNEL_5,
}PAK_ADC_CHANNEL;

extern int qiangdu;
extern uint8 Mode_flag;
extern uint8 temp_D;
extern void End_Beep(void);
extern void BeepSound(uint16 loop1,uint16 loop2);
extern uint8 Exchange_BatADC(uint16 dat);
extern uint8 Exchange_TempADC(uint16 dat);
extern void Heat_control(uint8 temperature);
extern void Packet_StateInit();
extern void Packet_Init(void);
extern void Packet_End(void);
extern void PacketPeriodicTask_1( void );
extern uint8 ProfileReadConfig(uint16 uuid, uint8 *newValue);//通信，读，信息获取
extern void InitLed(void);
extern uint16 Packet_ADC_14(PAK_ADC_CHANNEL channel);
extern void InitT3();

#endif
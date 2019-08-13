#include "stdio.h"
#include "string.h"
#include "hal_mcu.h"
#include "gatt.h"
#include "hal_types.h"
#include "simpleGATTprofile.h"
#include "Packet_YB.h"
#include "Packet.h"
#include "Packet_BLE.h"
#include "hal_other.h"

uint8 BLE_UartData_Deal(uint8 *data,uint8 len);
uint8 ProfileReadConfig(uint16 uuid, uint8 *newValue);//通信，信息获取
void BLEUart_UUID1(uint8 *buf, uint8 numBytes);

extern uint16 gapConnHandle;
extern Packet_Attribute Packet_1;

/*****客户端控制信息*****/
uint8 BLE_UartData_Deal(uint8 *data,uint8 len)
{
  if(data[0] == 0x67 && data[1] == 0x61 && data[4] == 0x73)
  {
    if('0'<data[2] && data[2]<'4')      //工作状态
    {
      Packet_1.PAK.Work_State = data[2]-'0';
    }
    if('0'<=data[3] && data[3]<='9'&&'0'<=data[4] && data[4]<='9') //强度
    {
      //Packet_1.PAK.Heat_Level = data[3];
      if(Packet_1.PAK.Work_State == Work_Run)
      {
        Packet_1.PAK.Heat_Level_h = data[3]-'0';
        Packet_1.PAK.Heat_Level_l = data[4]-'0';
//        Packet_1.PAK.Heat_Level = Packet_1.PAK.Heat_Level_h*10 + Packet_1.PAK.Heat_Level_l;
        qiangdu = Packet_1.PAK.Heat_Level_h*10 + Packet_1.PAK.Heat_Level_l;
      }
      else
      {
//        Packet_1.PAK.Heat_Level = Heat_Low;
        qiangdu = 8;
//        Packet_1.PAK.Heat_Level = 0;
        Packet_1.PAK.LED_Status = 0;
      }
    
    }
    if('0'<data[5] && data[5]<'5')             //模式
    {
      if(Packet_1.PAK.Work_State == Work_Run)
        Mode_flag = data[5]-'0';
      else
      {
        Mode_flag = 0;
      }
    
    }
    if('0'<=data[6] && data[6]<='9'&&'0'<=data[7] && data[7]<='9') //设置的工作时间
    {
      Packet_1.PAK.Set_time_h = data[6]-'0';
      Packet_1.PAK.Set_time_l = data[7]-'0';
      Packet_1.PAK.Set_time = Packet_1.PAK.Set_time_h*10 + Packet_1.PAK.Set_time_l;
    }
    return 1;  
  }
  else
  {
    return 0;
  }
}

//unsigned char C16toStr(unsigned char num)
//{
//  return(num/10*6+num);
//}

uint8 ProfileReadConfig(uint16 uuid, uint8 *newValue)//通信，信息获取
{
  uint8  len  = 0;  
  switch(uuid)
  {
    case SIMPLEPROFILE_CHAR1_UUID:
      
      newValue[len++] = 0x67;
      newValue[len++] = 0x5A;
      /*
      if(Packet_1.PAK.Heat_State == Heat_High)
         newValue[len++] = 3;
      else*/
        newValue[len++] = Packet_1.PAK.Work_State;
      
      if(Packet_1.PAK.Work_State == Work_Run)
      {
        newValue[len++] = Packet_1.PAK.Heat_Level_h;
        newValue[len++] = Packet_1.PAK.Heat_Level_l;
      }
      else
        newValue[len++] = 0;
    
        newValue[len++] = Mode_flag;
//      newValue[len++] = Packet_1.PAK.Heat_State;
//      newValue[len++] = Packet_1.PAK.Cure_State;
//      newValue[len++] = Packet_1.YB.Disconnect_Flag;
      newValue[len++] = Packet_1.PAK.Set_time_h;
      newValue[len++] = Packet_1.PAK.Set_time_l;
      
      newValue[len++] = Packet_1.PAK.Cure_Min;
      newValue[len++] = Packet_1.PAK.Cure_Second;
//      newValue[len++] = Packet_1.YB.LOCAL_MSG[2];
      newValue[len++] = Packet_1.PAK.Battery_capacity;
     // newValue[len++] = 10 ;
//      newValue[len++] = Packet_1.YB.LOCAL_MSG[3]; //药包类型
//      newValue[len++] = Packet_1.YB.LOCAL_MSG[4]; //时间高
//      newValue[len++] = Packet_1.YB.LOCAL_MSG[5]; //时间低
//      newValue[len++] = Packet_1.YB.LOCAL_MSG[8]; //生产年
//      newValue[len++] = Packet_1.YB.LOCAL_MSG[9]; //生产月
//      newValue[len++] = Packet_1.YB.LOCAL_MSG[10];//生产日
      newValue[len++] = 0x73;
    break;
          
    default:
      len = 0;
      break;
  }
  return len;
}
void BLEUart_UUID1(uint8 *buf, uint8 numBytes)
{
  static attHandleValueNoti_t pReport;
  pReport.pValue = GATT_bm_alloc( gapConnHandle, ATT_WRITE_REQ, numBytes, NULL );
  pReport.len = numBytes;
  pReport.handle = 0x0025;
  memcpy(pReport.pValue,buf, numBytes); 
  GATT_Notification( gapConnHandle, &pReport, FALSE ); 
}
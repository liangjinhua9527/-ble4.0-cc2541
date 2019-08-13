#include "bcomdef.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"
#include "OnBoard.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_lcd.h"
#include "gatt.h"
#include "ll.h"
#include "hci.h"
#include "gapgattserver.h"
#include "gattservapp.h"
#include "peripheral.h"
#include "gapbondmgr.h"
#include "simpleGATTprofile.h"
#include "simpleBLEPeripheral.h"
#include "npi.h"
#include "osal_snv.h"
#include "simpleBLE.h"
#include "stdio.h"
#include "string.h"
#include "hal_adc.h"
#include "amomcu_buffer.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"
#include "Packet_YB.h"
#include "Packet.h"
#include "hal_other.h"

#if 1
#define READ_DATE_MAX   13
#define READ_TIME_MAX   20
uint8 simpleBLETaskId;               // 主任务

bool g_sleepFlag = TRUE;    //睡眠标志
uint8 uart_sleep_count = 0; // 睡眠计数器

bool g_rssi_flag = false;       //是否开启测距/*YYY*/
uint8 CRC_Verify(uint8 *buff,uint8 len);///CRC    
extern gaprole_States_t gapProfileState;   // 从机连接状态
// Connection handle
extern uint16 gapConnHandle;
extern Packet_Attribute Packet_1;

void simpleBLE_NpiSerialCallback( uint8 port, uint8 events );
#endif

#if 1
// 该函数延时时间为1ms， 用示波器测量过， 稍有误差， 但误差很小  --amomcu.com
void simpleBLE_Delay_1ms(int times)
{
  while(times--)
  {
      int i=0;
      for(i=1500;i>0;i--)
      {
    	  asm("nop");
      }
  }
}

// 字符串对比
uint8 str_cmp(uint8 *p1,uint8 *p2,uint8 len)
{
  uint8 i=0;
  while(i<len){
    if(p1[i]!=p2[i])
      return 0;
    i++;
  }
  return 1;
}

// 字符串转数字
uint32 str2Num(uint8* numStr, uint8 iLength)
{
    uint8 i = 0;
    int32 rtnInt = 0;
 
    /* 
          为代码简单，在确定输入的字符串都是数字的
          情况下，此处未做检查，否则要检查
          numStr[i] - '0'是否在[0, 9]这个区间内
    */
    for(; i < iLength && numStr[i] != '\0'; ++i)
        rtnInt = rtnInt * 10 + (numStr[i] - '0');    
 
    return rtnInt;
}

/*********************************************************************
 * @fn      bdAddr2Str
 *
 * @brief   Convert Bluetooth address to string
 *
 * @return  none
 */
char *bdAddr2Str( uint8 *pAddr )
{
#define B_ADDR_STR_LEN                        15

  uint8       i;
  char        hex[] = "0123456789ABCDEF";
  static char str[B_ADDR_STR_LEN];
  char        *pStr = str;
  
  *pStr++ = '0';
  *pStr++ = 'x';
  
  // Start from end of addr
  pAddr += B_ADDR_LEN;
  
  for ( i = B_ADDR_LEN; i > 0; i-- )
  {
    *pStr++ = hex[*--pAddr >> 4];
    *pStr++ = hex[*pAddr & 0x0F];
  }
  
  *pStr = 0;
  
  return str;
}
#endif
char *AH_bdAddr2Str( uint8 *pAddr)
{
  char        hex[] = "0123456789ABCDEF";
  static char str[1];
  char        *pStr = str;
 

  *pStr = hex[*pAddr & 0x0F];
   
  return str;
}



// 判断蓝牙是否连接上
// 0: 未连接上
// 1: 已连接上
//***主机和从机对状态的 描述 是不一样的***//
bool simpleBLE_IfConnected()
{
  return (gapProfileState == GAPROLE_CONNECTED);
}



// 串行口 uart 初始化
void simpleBLE_NPI_init(void)
{
    NPI_InitTransport(simpleBLE_NpiSerialCallback);
}



#if 1
static float GUA_CalcDistByRSSI(int rssi)    
{    
    uint8 A = 49;  
    float n = 3.0;  
      
    int iRssi = abs(rssi);    
    float power = (iRssi-A)/(10*n);         
    return pow(10, power);    
}

// 求滑动平均值
#define DIST_MAX   5
int nDistbuf[DIST_MAX];
uint8 index = 0;

static int dist_filer(int dist)
{
    int i = 0;
    int sum = 0;
    int max = 0;
    int min = 1000;
    if(index == DIST_MAX)
    {
         static int index2 = 0;
         nDistbuf[index2++] = dist;
         index2 %= DIST_MAX;

         // 去掉最大最小值, 再求平均
         
         for(i =0; i< DIST_MAX; i++)
         {
            if(max < nDistbuf[i])
               max = nDistbuf[i];
            if(min > nDistbuf[i])
               min = nDistbuf[i];
            
            sum += nDistbuf[i];
         }
         return (sum-max-min)/(DIST_MAX-2);
    }
    else
    {
        nDistbuf[index++] = dist;
        return 0;
    }
}
#endif

// 保存RSSI 到系统变量
void simpleBle_SetRssi(int8 rssi)
{
    //sys_config.rssi = rssi;

    if(simpleBLE_IfConnected())
    {
        char str[32];    

        float nfDist = GUA_CalcDistByRSSI(rssi);         //通过算法获得r，单位为m  
        int nDist = (int)(nfDist * 100);                    //将r的数值放大100倍，单位为cm  
        sprintf(str, "Rssi=%2d,%4dCM\r\n", (uint8) (-rssi), dist_filer(nDist));

        if(g_rssi_flag)
        {
            //NPI_WriteTransport((uint8*)str, strlen(str));
     
        }
    }  
}

// 串口打印密码  -----测试用----
void simpleBle_PrintPassword()
{
    //char strTemp[24] = {0};
    
    //sprintf(strTemp, "Password:%s\r\n", sys_config.pass);
    //NPI_WriteTransport((uint8*)strTemp, osal_strlen(strTemp));
}






// 串口回调函数， 下面把该回调函数里实现的功能讲解一下
/*
1, 思路:  当串口收到数据后，就会马上调用以下回调函数，在实际测试中发现，此回调
函数调用频繁， 如果你不执行NPI_ReadTransport函数进行读取， 那么这个回调函数就会
频繁地被执行，但是，你通过串口发送一段数据， 你本意是想处理这一完整一段的数据，所以，
我们在下面引入了时间的处理方法， 也即接收的数据够多或者超时，就读取一次数据， 
然后根据当前的状态决定执行，如果没有连接上，就把所有数据当做AT命令处理， 如果连接
上了，就把数据送到对端。
*/

//uart 回调函数
void simpleBLE_NpiSerialCallback( uint8 port, uint8 events )
{
      (void)port;

      static uint32 old_time;              //上一次时间
      static uint32 old_time_data_len = 0; //上一次时间数据长度
      uint32 new_time;                      //当前时间
      bool ret=1;
      uint8 readMaxBytes ;
      

      if (events & (HAL_UART_RX_TIMEOUT | HAL_UART_RX_FULL))   //只要有1个字节缓存数据   HAL_UART_RX_TIMEOUT必触发
      {      
          (void)port;
           uint8 numBytes = 0;
          
          numBytes = NPI_RxBufLen();           //读出串口缓冲区有多少字节
      
          if(numBytes == 0)
          {
             old_time_data_len = 0;
             return;
          }
          
          if(old_time_data_len == 0)
          {
             old_time = osal_GetSystemClock(); //有数据来时， 记录一下
             old_time_data_len = numBytes;
          }
          else
          {
             readMaxBytes = READ_DATE_MAX ;  //通讯数据最大值
             new_time = osal_GetSystemClock(); //当前时间
              
              if( (numBytes >= readMaxBytes) 
                  || ( (new_time - old_time) > READ_TIME_MAX/*ms*/))//通信间隔
              {
                  uint8 readBytes = 0;
                  uint8 *buffer = osal_mem_alloc(readMaxBytes);

                  if(!buffer)
                  {
                      return;
                  }
                  
                  if(numBytes >= readMaxBytes)
                  {
                      readBytes = readMaxBytes;
                  }
                  else
                  {
                      readBytes = numBytes;
                  }

                  NPI_ReadTransport(buffer,readBytes); //释放串口数据    
                
                  if((numBytes == READ_DATE_MAX) && (buffer[0]== 'g' ) && (buffer[readBytes-1]== 's'))
                  {    
                     if(FALSE == CRC_Verify(buffer,numBytes)) 
                     {
                       ret = FALSE;
                     }
                  }
                  else
                  {
                    ret = FALSE;
                  }
                  
                  if(ret == FALSE)
                  { 
                    ;
                  }
                  else
                  {
                     memcpy(Packet_1.YB.CMNC_MSG,buffer, readBytes); 
                     Packet_1.YB.Connect_Chek = 1;
                  }
                                
                  old_time = new_time;
                  old_time_data_len = numBytes - readBytes;
                  
                  osal_mem_free(buffer);
              }                
          }    
      }
}

                 
/******************串口CRC检验********************/
uint8 CRC_Verify(uint8 *buff,uint8 len)
{
    uint8 i;
    uint8 temp=0;
    
    for(i=1;i<(len-2);i++)
    {
        temp +=  buff[i];
    }
    temp |= 0x80; 
    if(temp==buff[len-2])return 1;
    else 	         return 0;		
}






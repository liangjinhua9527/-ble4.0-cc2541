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
uint8 simpleBLETaskId;               // ������

bool g_sleepFlag = TRUE;    //˯�߱�־
uint8 uart_sleep_count = 0; // ˯�߼�����

bool g_rssi_flag = false;       //�Ƿ������/*YYY*/
uint8 CRC_Verify(uint8 *buff,uint8 len);///CRC    
extern gaprole_States_t gapProfileState;   // �ӻ�����״̬
// Connection handle
extern uint16 gapConnHandle;
extern Packet_Attribute Packet_1;

void simpleBLE_NpiSerialCallback( uint8 port, uint8 events );
#endif

#if 1
// �ú�����ʱʱ��Ϊ1ms�� ��ʾ������������ ������ ������С  --amomcu.com
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

// �ַ����Ա�
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

// �ַ���ת����
uint32 str2Num(uint8* numStr, uint8 iLength)
{
    uint8 i = 0;
    int32 rtnInt = 0;
 
    /* 
          Ϊ����򵥣���ȷ��������ַ����������ֵ�
          ����£��˴�δ����飬����Ҫ���
          numStr[i] - '0'�Ƿ���[0, 9]���������
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



// �ж������Ƿ�������
// 0: δ������
// 1: ��������
//***�����ʹӻ���״̬�� ���� �ǲ�һ����***//
bool simpleBLE_IfConnected()
{
  return (gapProfileState == GAPROLE_CONNECTED);
}



// ���п� uart ��ʼ��
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

// �󻬶�ƽ��ֵ
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

         // ȥ�������Сֵ, ����ƽ��
         
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

// ����RSSI ��ϵͳ����
void simpleBle_SetRssi(int8 rssi)
{
    //sys_config.rssi = rssi;

    if(simpleBLE_IfConnected())
    {
        char str[32];    

        float nfDist = GUA_CalcDistByRSSI(rssi);         //ͨ���㷨���r����λΪm  
        int nDist = (int)(nfDist * 100);                    //��r����ֵ�Ŵ�100������λΪcm  
        sprintf(str, "Rssi=%2d,%4dCM\r\n", (uint8) (-rssi), dist_filer(nDist));

        if(g_rssi_flag)
        {
            //NPI_WriteTransport((uint8*)str, strlen(str));
     
        }
    }  
}

// ���ڴ�ӡ����  -----������----
void simpleBle_PrintPassword()
{
    //char strTemp[24] = {0};
    
    //sprintf(strTemp, "Password:%s\r\n", sys_config.pass);
    //NPI_WriteTransport((uint8*)strTemp, osal_strlen(strTemp));
}






// ���ڻص������� ����Ѹûص�������ʵ�ֵĹ��ܽ���һ��
/*
1, ˼·:  �������յ����ݺ󣬾ͻ����ϵ������»ص���������ʵ�ʲ����з��֣��˻ص�
��������Ƶ���� ����㲻ִ��NPI_ReadTransport�������ж�ȡ�� ��ô����ص������ͻ�
Ƶ���ر�ִ�У����ǣ���ͨ�����ڷ���һ�����ݣ� �㱾�����봦����һ����һ�ε����ݣ����ԣ�
����������������ʱ��Ĵ������� Ҳ�����յ����ݹ�����߳�ʱ���Ͷ�ȡһ�����ݣ� 
Ȼ����ݵ�ǰ��״̬����ִ�У����û�������ϣ��Ͱ��������ݵ���AT����� �������
���ˣ��Ͱ������͵��Զˡ�
*/

//uart �ص�����
void simpleBLE_NpiSerialCallback( uint8 port, uint8 events )
{
      (void)port;

      static uint32 old_time;              //��һ��ʱ��
      static uint32 old_time_data_len = 0; //��һ��ʱ�����ݳ���
      uint32 new_time;                      //��ǰʱ��
      bool ret=1;
      uint8 readMaxBytes ;
      

      if (events & (HAL_UART_RX_TIMEOUT | HAL_UART_RX_FULL))   //ֻҪ��1���ֽڻ�������   HAL_UART_RX_TIMEOUT�ش���
      {      
          (void)port;
           uint8 numBytes = 0;
          
          numBytes = NPI_RxBufLen();           //�������ڻ������ж����ֽ�
      
          if(numBytes == 0)
          {
             old_time_data_len = 0;
             return;
          }
          
          if(old_time_data_len == 0)
          {
             old_time = osal_GetSystemClock(); //��������ʱ�� ��¼һ��
             old_time_data_len = numBytes;
          }
          else
          {
             readMaxBytes = READ_DATE_MAX ;  //ͨѶ�������ֵ
             new_time = osal_GetSystemClock(); //��ǰʱ��
              
              if( (numBytes >= readMaxBytes) 
                  || ( (new_time - old_time) > READ_TIME_MAX/*ms*/))//ͨ�ż��
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

                  NPI_ReadTransport(buffer,readBytes); //�ͷŴ�������    
                
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

                 
/******************����CRC����********************/
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






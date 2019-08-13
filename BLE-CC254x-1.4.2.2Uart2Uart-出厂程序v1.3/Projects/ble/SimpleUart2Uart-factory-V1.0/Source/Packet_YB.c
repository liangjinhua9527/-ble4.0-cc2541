#include "stdio.h"
#include "string.h"
#include "hal_types.h"
#include "npi.h"
#include "hal_other.h"
#include "Packet_YB.h"
#include "Packet.h"
void YB_DataDeal(void);
void RW_YB_MSG(uint8 mode,uint8 *buf,uint8 *data);
uint8 YB_Time_Check(uint8 *buf);

extern Packet_Attribute Packet_1;

void YB_DataDeal(void)
{
    static uint8 poll = 1;
    static uint8 YB_LostCount = 0;
    
    poll = !poll;
    /*************����&��д-ҩ������*************/
    if(poll == 0)
    {  
        RW_YB_MSG(Packet_1.YB.Write_Flag,Packet_1.YB.Control_Buf,Packet_1.YB.LOCAL_MSG);
        NPI_WriteTransport((uint8*)Packet_1.YB.Control_Buf, 8);
        Packet_1.YB.Connect_Chek = 0; 
    }
    /***************���շ�������****************/
    if(poll == 1)
    {
      /***********δ����**********/
        if(Packet_1.YB.Connect_Chek == 0)
        {        
           YB_LostCount++;
           if(YB_LostCount == 6)//ҩ�����׶Ͽ�ʱ�䣬5*500+250=2750ms
           {
               YB_LostCount = 0;
               memset(&Packet_1.YB.LOCAL_MSG, 0, YB_LOCAL_MSG_LEN);
               Packet_1.YB.Disconnect_Flag = 1;
               if(Packet_1.PAK.Cure_State)
               {
                  if(Packet_1.PAK.Work_State==Work_Run)
                  {
                   Packet_1.PAK.Work_State    = Work_Stop;
                   Packet_1.YB.ReConnect_Flag = 1;
                  }
               }
               /*
               else
               {
                  Packet_1.PAK.Work_State = Work_Pause;
               }*/
            }      
         }
        /**********����***********/
        else
        { 
            YB_LostCount = 0;
            Packet_1.YB.Disconnect_Flag = 0;
            if(YB_Time_Check(Packet_1.YB.CMNC_MSG))//���ҩ��ʣ��ʱ��
            {
                memcpy(Packet_1.YB.LOCAL_MSG, Packet_1.YB.CMNC_MSG, YB_CMNC_MSG_LEN); //��ҩ����Ϣ���������ػ��棨����ɼ��뱾��ҩ����Ϣ�ʹ��ڽ���ҩ������Ϣ����У�飩
                if(!Packet_1.PAK.Cure_State)//������״̬�£���ҩ���Զ���ʼ����
                {
                    //Packet_1.PAK.Work_State    = Work_Run;
                    //Packet_1.PAK.Work_State = Work_Stop;
                    Packet_1.PAK.Cure_State = 1;
                }
                else if(Packet_1.YB.ReConnect_Flag && Packet_1.PAK.Cure_State)//�ѿ�ʼ����״̬�£���ҩ���ָ�����ʱ�Զ���ʼ�ϴ�����
                {
                    Packet_1.YB.ReConnect_Flag = 0;
                    Packet_1.PAK.Work_State    = Work_Run;
                }
                else
                {;}
                if(Packet_1.YB.Write_Flag)//д��ɹ�ȷ�ϻ��ƣ���д��־λ��ҩ����ϢӦ��ͬ�����ػ���
                {
                    if((Packet_1.YB.Control_Buf[2] == Packet_1.YB.LOCAL_MSG[4])&&(Packet_1.YB.Control_Buf[3] == Packet_1.YB.LOCAL_MSG[5]))
                    {
                      Packet_1.YB.Write_Flag = 0;
                    }
                }
            }
            else//ҩ��û��ʹ��ʱ�䣬��ͣ����
            {
                if(Packet_1.PAK.Cure_State)
               {
                   Packet_1.PAK.Work_State    = Work_Stop;
                   Packet_1.YB.ReConnect_Flag = 1;
               }/*
               else
               {
                  Packet_1.PAK.Work_State  = Work_Pause;
               }*/
            }
          }
      
     }
}
/******ҩ��ʣ��ʱ����******/
uint8 YB_Time_Check(uint8 *buf)
{
  if(buf[4] == 0 && buf[5] == 0)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

/****����&��д-ҩ������****/  
void RW_YB_MSG(uint8 mode,uint8 *buf,uint8 *data)
{
  uint8 i;
  uint8 crc_temp = 0;
  buf[0] = 0x67;//'g'
  buf[1] = 0xfa;//channel
  if(mode)
  {
     if(data[5] == 0)
     {
       buf[2] = data[4] - 1;//timerH
       buf[3] = 59;         //timerL
       buf[4] = data[6] - 1;//time
     }
     else
     {
       buf[2] = data[4];    //timerH
       buf[3] = data[5] - 1;//timerL
       buf[4] = data[6];    //time
     }
  }
  else
  {
    buf[2] = 0x00;//timerH
    buf[3] = 0x00;//timerL
    buf[4] = 0x00;//time
  }        
  buf[5] = 0x00;//null
   /****CRC����****/
  for(i=1;i<6;i++)
  {
    crc_temp += Packet_1.YB.Control_Buf[i];
  }
  crc_temp        |= 0x80;
  
  buf[6] = crc_temp;//CRC
  buf[7] = 0x73;//'s'
}

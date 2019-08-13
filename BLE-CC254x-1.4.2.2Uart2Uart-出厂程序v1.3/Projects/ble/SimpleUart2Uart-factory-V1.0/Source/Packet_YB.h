#ifndef PACKET_H
#define PACKET_H
#define YB_CMNC_MSG_LEN    13
#define YB_LOCAL_MSG_LEN   13
#define YB_CONTROL_BUF_LEN 8
typedef struct
{
  uint8    Connect_Chek;
  uint8    Disconnect_Flag;
  uint8    Write_Flag;
  uint8    ReConnect_Flag;
  uint8    CMNC_MSG[YB_CMNC_MSG_LEN]; 
  uint8    LOCAL_MSG[YB_LOCAL_MSG_LEN]; 
  uint8    Control_Buf[YB_CONTROL_BUF_LEN]; 
}YB_INFORMATION;

extern void YB_DataDeal(void);
extern void RW_YB_MSG(uint8 mode,uint8 *buf,uint8 *data);

#endif
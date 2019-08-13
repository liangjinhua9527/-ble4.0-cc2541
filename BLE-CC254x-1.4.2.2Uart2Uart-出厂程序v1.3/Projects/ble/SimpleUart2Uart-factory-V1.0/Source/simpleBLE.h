#ifndef SIMPLEBLE_H
#define SIMPLEBLE_H

#ifdef __cplusplus
extern "C"
{
#endif


// ��������¼�����
#define START_DEVICE_EVT           0x0001//�����豸          
#define SBP_PERIODIC_EVT           0x0002//ϵͳ��ѯ��ʱ��
#define SBP_WAKE_EVT               0x0040//�����¼�
#define SBP_UI_EVT            0x0008   
#define SBP_QD_EVT                 0x0010
#define SBP_GJ_EVT                 0x0020
#define SBP_YB_CMNC_EVT            0x0080
#define SBP_BLE_CMNC_EVT           0x0100


//------------------------------------------------------------------------------
//--------------------------------------------------------------------------------



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#if 0
// ����ϵͳ�ṹ������ �ýṹ���ڿ���ʱ��nv flash �ж�ȡ�� �������޸�ʱ�� ��Ҫд��nv flash
// ������ ��ʵ����ϵͳ���������ݻ�����һ�����õ�
typedef struct 
{
    /*
    ������
    0---------9600 
    1---------19200 
    2---------38400 
    3---------57600 
    4---------115200
    */
    uint8 baudrate;                 //������ �� Ŀǰ֧�ֵ��б�����
    uint8 parity;                   //У��λ    
    uint8 stopbit;                  //ֹͣλ
    
    uint8 mode;                     //����ģʽ 0:͸�� �� 1: ֱ�� , 2: iBeacon

    // �豸���ƣ�� 11 λ���ֻ���ĸ�����л��ߺ��»��ߣ��������������ַ�    
    uint8 name[12];                 

    BLE_ROLE role;                  //����ģʽ  0: �ӻ�   1: ����

    uint8 pass[7];                  //���룬 ���6λ 000000~999999 

    /*
    Para: 0 ~ 1 
    0: ���Ӳ���Ҫ����
    1: ������Ҫ����
    */
    uint8 type;                     //��Ȩģʽ
    uint8 mitm ;//��Ҫ��������
    uint8 ioCap;
    uint8 bonding;
    
    uint8 mac_addr[MAC_ADDR_CHAR_LEN+1];            //����mac��ַ ���12λ �ַ���ʾ
    uint8 connect_mac_addr[MAC_ADDR_CHAR_LEN+1];    //ָ��ȥ���ӵ�mac��ַ


    //�����ɹ����ӹ��Ĵӻ�����
    uint8 ever_connect_peripheral_mac_addr_conut;
    //�����ɹ����ӹ��Ĵӻ�����,��ǰindex�� �������Ӵӻ���ַʱ���ٲ�����ȡ
    uint8 ever_connect_peripheral_mac_addr_index;
    //����һ�γɹ����ӹ��Ĵӻ���ַindex�� �������AT+CONNL ���ָ��
    uint8 last_connect_peripheral_mac_addr_index;
    //�����ɹ����ӹ��Ĵӻ���ַ
    uint8 ever_connect_mac_status[MAX_PERIPHERAL_MAC_ADDR][MAC_ADDR_CHAR_LEN];       

    /*
    Para: 000000��009999 
    000000 ����������ӣ���
    ������Եĺ�����
    Default:001000
    */
    
    uint16 try_connect_time_ms;           // ��������ʱ��---Ŀǰ��Ч
    int8 rssi;                              //  RSSI �ź�ֵ
    uint8 rxGain;                           //  ��������ǿ��
    uint8 txPower;                          //  �����ź�ǿ��
    uint16 ibeacon_adver_time_ms;         // �㲥���

    //  ģ�鹤������  0: ���������� 1: �ȴ�AT+CON �� AT+CONNL ����
    uint8 workMode;                        
}SYS_CONFIG;
extern SYS_CONFIG sys_config;
#endif



extern uint16 simpleBLECharHdl;
extern uint16 simpleBLECharHd6;
extern bool simpleBLEChar6DoWrite;
extern bool simpleBLEChar6DoWrite2;


#if defined (RELEASE_VER)
#define LCD_WRITE_STRING(str, option)                     
#define LCD_WRITE_SCREEN(line1, line2)                    
#define LCD_WRITE_STRING_VALUE(title, value, format, line)

#if defined (HAL_LCD)
#undef HAL_LCD
#define HAL_LCD FALSE 
#endif

#else
// LCD macros
#if HAL_LCD == TRUE
#define LCD_WRITE_STRING(str, option)                       HalLcdWriteString( (str), (option))
#define LCD_WRITE_SCREEN(line1, line2)                      HalLcdWriteScreen( (line1), (line2) )
#define LCD_WRITE_STRING_VALUE(title, value, format, line)  HalLcdWriteStringValue( (title), (value), (format), (line) )
#else
#define LCD_WRITE_STRING(str, option)                     
#define LCD_WRITE_SCREEN(line1, line2)                    
#define LCD_WRITE_STRING_VALUE(title, value, format, line)
#endif
#endif



extern uint8 simpleBLETaskId;               // ������
extern uint16 simpleBLECharHdl;
extern uint16 simpleBLECharHd6;
extern bool simpleBLECentralCanSend;
extern bool simpleBLEChar6DoWrite;
extern uint8 simpleBLEPeripheral_TaskID;        // �ӻ�����



#if 1
// �ú�����ʱʱ��Ϊ1ms�� ��ʾ������������ ������ ������С  --amomcu.com
void simpleBLE_Delay_1ms(int times);

// �ַ����Ա�
uint8 str_cmp(uint8 *p1,uint8 *p2,uint8 len);

// �ַ���ת����
uint32 str2Num(uint8* numStr, uint8 iLength);

char *bdAddr2Str( uint8 *pAddr );
char *AH_bdAddr2Str( uint8 *pAddr );


// �ж������Ƿ�������
// 0: δ������
// 1: ��������
bool simpleBLE_IfConnected();




// ���п� uart ��ʼ��
void simpleBLE_NPI_init(void);

// ���ý�������
void UpdateRxGain(void);

// ���÷��书��
void UpdateTxPower(void);


// ����RSSI ��ϵͳ����
void simpleBle_SetRssi(int8 rssi);

// ���ڴ�ӡ����  -----������----
void simpleBle_PrintPassword();



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
extern void simpleBLE_NpiSerialCallback( uint8 port, uint8 events );

#endif



#ifdef __cplusplus
}
#endif

#endif /* SIMPLEBLE_H */

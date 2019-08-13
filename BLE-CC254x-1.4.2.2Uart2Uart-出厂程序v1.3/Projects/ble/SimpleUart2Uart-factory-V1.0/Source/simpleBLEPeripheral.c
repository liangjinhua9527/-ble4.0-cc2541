
/*********************************************************************
 * INCLUDES
 */

#include "bcomdef.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"

#include "OnBoard.h"
#include "hal_adc.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_lcd.h"

#include "gatt.h"

#include "hci.h"

#include "gapgattserver.h"
#include "gattservapp.h"
#include "devinfoservice.h"
#include "simpleGATTprofile.h"

#if defined( CC2540_MINIDK )
  #include "simplekeys.h"
#endif

#if defined ( PLUS_BROADCASTER )
  #include "peripheralBroadcaster.h"
#else
  #include "peripheral.h"
#endif

#include "gapbondmgr.h"

#include "simpleBLEPeripheral.h"

#if defined FEATURE_OAD
  #include "oad.h"
  #include "oad_target.h"
#endif

#include "simpleble.h"
#include "npi.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "osal_snv.h"
#include "hal_other.h"
#include "math.h"
#include "amomcu_buffer.h"
#include "Packet_YB.h"
#include "Packet.h"
#include "Packet_BLE.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */


// What is the advertising interval when device is discoverable (units of 625us, 160=100ms)
#define DEFAULT_ADVERTISING_INTERVAL          160

// Limited discoverable mode advertises for 30.72s, and then stops �涨ʱ���ڷ��͹㲥��Ȼ��ֹͣ����    
// General discoverable mode advertises indefinitely ѭ�����͹㲥
#define DEFAULT_DISCOVERABLE_MODE             GAP_ADTYPE_FLAGS_GENERAL  //��ͨ����ģʽ
//#define DEFAULT_DISCOVERABLE_MODE             GAP_ADTYPE_FLAGS_LIMITED  //���Ʒ���ģʽ

 #define DEFAULT_SLOW_ADV_DURATION            0  //��ͨ����ģʽ�㲥ʱ�䣬0=����
// Minimum connection interval (units of 1.25ms, 80=100ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL     30//80   ���Ӽ�������ݷ������йأ� ���Ӽ��Խ�̣� ��λʱ���ھ��ܷ���Խ�������

// Maximum connection interval (units of 1.25ms, 800=1000ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL     100//800   ���Ӽ�������ݷ������йأ� ���Ӽ��Խ�̣� ��λʱ���ھ��ܷ���Խ�������

// Slave latency to use if automatic parameter update request is enabled
#define DEFAULT_DESIRED_SLAVE_LATENCY         10//���������Ӽ��

// Supervision timeout value (units of 10ms, 1000=10s) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_CONN_TIMEOUT          100//1000  -����ԭ��Ͽ����Ӻ󣬳�ʱ�����¹㲥��ʱ��:  100 = 1s��IOS����С��6S

// Whether to enable automatic parameter update request when a connection is formed
#define DEFAULT_ENABLE_UPDATE_REQUEST         TRUE

// Connection Pause Peripheral time value (in seconds)���Ӻ�ķ�����������ʱ��
#define DEFAULT_CONN_PAUSE_PERIPHERAL         1

// Company Identifier: Texas Instruments Inc. (13)
#define TI_COMPANY_ID                         0x000D

#define INVALID_CONNHANDLE                    0xFFFF

#if defined ( PLUS_BROADCASTER )
  #define ADV_IN_CONN_WAIT                    500 // delay 500 ms
#endif

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

#define DEV_NAME_DEFAULT          "005A-023\0"
#define DEV_NAME_LEN               11
#define BOND_PAIR_STATUS_PAIRING   0
#define BOND_PAIR_STATUS_PAIRED    1
#define OFF_POWER_DELAY   60//(60*50ms=3s)  

#define Key_Up P0_2
#define Key_Down P1_3

//uint8 simpleBLEPeripheral_TaskID;   // Task ID for internal task/event processing
gaprole_States_t gapProfileState = GAPROLE_INIT;
bool simpleBLEChar6DoWrite2 = TRUE;
bool timerIsOn              = FALSE;          // 
bool SwCheck_loose           = 1;

// Connection handle
uint16 gapConnHandle = NULL;
extern Packet_Attribute Packet_1;

// GAP - Advertisement data (max size = 31 bytes, though this is
// best kept short to conserve power while advertisting)
static uint8 advertData[] =
{
  // Flags; this sets the device to use limited discoverable
  // mode (advertises for 30 seconds at a time) instead of general
  // discoverable mode (advertises indefinitely)
  0x02,   // length of this data
  GAP_ADTYPE_FLAGS,
  DEFAULT_DISCOVERABLE_MODE | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

  // service UUID, to notify central devices what services are included
  // in this peripheral
  0x03,   // length of this data
  GAP_ADTYPE_16BIT_MORE,      // some of the UUID's, but not all
  LO_UINT16( SIMPLEPROFILE_SERV_UUID ),
  HI_UINT16( SIMPLEPROFILE_SERV_UUID ),
  
  0x09,
  GAP_ADTYPE_MANUFACTURER_SPECIFIC,
  0x4a,
  0x69,
  0x61,
  0x6e,
  0x59,
  0x75,
  0x61,
  0x6e
};


/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void simpleBLEPeripheral_ProcessOSALMsg( osal_event_hdr_t *pMsg );
static void simpleBLEPeripheral_ProcessGATTMsg( gattMsgEvent_t *pMsg );
static void peripheralStateNotificationCB( gaprole_States_t newState );
static void peripheralRssiReadCB( int8 rssi );
static void simpleProfileChangeCB( uint8 paramID );

//typedef enum
//{
//  BOND_PAIR_STATUS_PAIRING,  //δ���
//  BOND_PAIR_STATUS_PAIRED,  //�����
//}BOND_PAIR_STATUS;
//// ��������ǰ��״̬��������벻��ȷ������ȡ�����ӣ�������
//static BOND_PAIR_STATUS gPairStatus = BOND_PAIR_STATUS_PAIRING;
uint8 gPairStatus;
void ProcessPasscodeCB(uint8 *deviceAddr,uint16 connectionHandle,uint8 uiInputs,uint8 uiOutputs );
static void ProcessPairStateCB( uint16 connHandle, uint8 state, uint8 status );


/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static gapRolesCBs_t simpleBLEPeripheral_PeripheralCBs =
{
  peripheralStateNotificationCB,       // Profile State Change Callbacks
  peripheralRssiReadCB,               // When a valid RSSI is read from controller (not used by application)
};

// GAP Bond Manager Callbacks
static gapBondCBs_t simpleBLEPeripheral_BondMgrCBs =
{

  ProcessPasscodeCB,                     // ����ص�
  ProcessPairStateCB                     // ��״̬�ص�
};

// Simple GATT Profile Callbacks
static simpleProfileCBs_t simpleBLEPeripheral_SimpleProfileCBs =
{
  simpleProfileChangeCB    // Charactersitic value change callback
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SimpleBLEPeripheral_Init
 *
 * @brief   Initialization function for the Simple BLE Peripheral App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by OSAL.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */

SYS_INFORMATION sys_information;
void SimpleBLEPeripheral_Init( uint8 task_id )
{
    simpleBLETaskId = task_id;
    
  
//  Init_Para();
  //
  //  // ���ڳ�ʼ��
// NPI_InitTransport(simpleBLE_NpiSerialCallback);
// NPI_WriteTransport("SimpleBLEPeripheral_Init\r\n", 26);  

    
    //KEY 
    RegisterForKeys(simpleBLETaskId);
    
    
    //����ȡ�ڴ�����
    int8 ret8 = osal_snv_read(0x80, sizeof(sys_information),&sys_information);
    if(NV_OPER_FAILED == ret8)
    {
      osal_memset(&sys_information,0,sizeof(sys_information));
      sprintf((char*)sys_information.name, DEV_NAME_DEFAULT);         // Ĭ���豸����
      sprintf((char*)sys_information.version, DEV_VERSION);         // Ĭ���豸�ͺ�
      sprintf((char*)sys_information.pass, "123456");                 //����
      osal_snv_write(0x80, sizeof(sys_information),&sys_information); // д���ڴ�
    } 

    // ����GAP
    VOID GAP_SetParamValue( TGAP_CONN_PAUSE_PERIPHERAL, DEFAULT_CONN_PAUSE_PERIPHERAL );//������£�ʱ������:1=1s
    // ����GAP��ɫ����
    uint8  initial_advertising_enable = FALSE;//�㲥
    uint8  tgap_gendiscadvmin         = 0;   //����ʱ����λ��ms
    //uint16 gapRole_AdvertOffTime      = 0;    //�㲥����ʱ�䣺ms
    GAP_SetParamValue( TGAP_GEN_DISC_ADV_MIN, tgap_gendiscadvmin ); //����ʱ�㲥
    //GAPRole_SetParameter( GAPROLE_ADVERT_OFF_TIME, sizeof( uint16 ), &gapRole_AdvertOffTime      );//�㲥����ʱ�䣬0Ϊ������
    GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED,  sizeof( uint8  ), &initial_advertising_enable );//�㲥ʧ��

   
    GAP_SetParamValue( TGAP_GEN_DISC_ADV_MIN, DEFAULT_SLOW_ADV_DURATION );  
    //ɨ����Ӧ����
    uint8 AttDeviceNameLen = osal_strlen((char*)sys_information.name);
    uint8 pSscanRspDataLen = ( 11 + AttDeviceNameLen);
    uint8 *pSscanRspData = osal_mem_alloc(pSscanRspDataLen);//����pSscanRspDataLen�����ڴ棻
    if(pSscanRspData)//��㲥��ʽ��ͬ����ʽ������+����+����
    {
        uint8 i = 0;
        
        // �豸����
        pSscanRspData[0] = AttDeviceNameLen + 1;
        pSscanRspData[1] = GAP_ADTYPE_LOCAL_NAME_COMPLETE;
        osal_memcpy(&pSscanRspData[2], sys_information.name, AttDeviceNameLen);
        // ���Ӳ����������С���
        i = 2 + AttDeviceNameLen;
        pSscanRspData[i+0] = 0x05;
        pSscanRspData[i+1] = GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE;
        pSscanRspData[i+2] = LO_UINT16( DEFAULT_DESIRED_MIN_CONN_INTERVAL );   
        pSscanRspData[i+3] = HI_UINT16( DEFAULT_DESIRED_MIN_CONN_INTERVAL );
        pSscanRspData[i+4] = LO_UINT16( DEFAULT_DESIRED_MAX_CONN_INTERVAL );   
        pSscanRspData[i+5] = HI_UINT16( DEFAULT_DESIRED_MAX_CONN_INTERVAL );     
        // �źŷ���ǿ��
        pSscanRspData[i+6] = 0x02;   
        pSscanRspData[i+7] = GAP_ADTYPE_POWER_LEVEL;
        pSscanRspData[i+8] = 0;       
        //д��ɨ����Ӧ
        GAPRole_SetParameter( GAPROLE_SCAN_RSP_DATA, pSscanRspDataLen, pSscanRspData );
        //�ͷ��ڴ�
        osal_mem_free(pSscanRspData);
    }

    //д��㲥����
    GAPRole_SetParameter( GAPROLE_ADVERT_DATA, sizeof( advertData ), advertData );


    uint8  enable_update_request = DEFAULT_ENABLE_UPDATE_REQUEST;    //DEFAULT_ENABLE_UPDATE_REQUEST     = TRUE
    uint16 desired_min_interval  = DEFAULT_DESIRED_MIN_CONN_INTERVAL;//DEFAULT_DESIRED_MIN_CONN_INTERVAL = 2  * 1.25ms =2  ms
    uint16 desired_max_interval  = DEFAULT_DESIRED_MAX_CONN_INTERVAL;//DEFAULT_DESIRED_MAX_CONN_INTERVAL = 20 * 1.25ms =25ms
    uint16 desired_slave_latency = DEFAULT_DESIRED_SLAVE_LATENCY;    //DEFAULT_DESIRED_SLAVE_LATENCY     = 5
    uint16 desired_conn_timeout  = DEFAULT_DESIRED_CONN_TIMEOUT;     //DEFAULT_DESIRED_CONN_TIMEOUT      = 500 = 5s
    GAPRole_SetParameter( GAPROLE_PARAM_UPDATE_ENABLE,sizeof( uint8  ), &enable_update_request);//ʹ�ܸ������Ӳ�������
    GAPRole_SetParameter( GAPROLE_MIN_CONN_INTERVAL,  sizeof( uint16 ), &desired_min_interval );//��С���Ӽ��;
    GAPRole_SetParameter( GAPROLE_MAX_CONN_INTERVAL,  sizeof( uint16 ), &desired_max_interval );//������Ӽ��
    GAPRole_SetParameter( GAPROLE_SLAVE_LATENCY,      sizeof( uint16 ), &desired_slave_latency);//���������
    GAPRole_SetParameter( GAPROLE_TIMEOUT_MULTIPLIER, sizeof( uint16 ), &desired_conn_timeout );//��ʱ�����¹㲥��ʱ��
  

    // Set the GAP Characteristics   д�뱾����
    GGS_SetParameter( GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, sys_information.name);

    // ����rssi ������������
    uint16 rssi_read_rate_1ms = 3000; //500=һ�����2��   
    GAPRole_SetParameter(GAPROLE_RSSI_READ_RATE, sizeof( uint16 ), &rssi_read_rate_1ms);
    
    //����ÿ�ι㲥�¼����
    uint16 advInt = DEFAULT_ADVERTISING_INTERVAL;//160*25us=100ms

    //GAP_SetParamValue( TGAP_LIM_DISC_ADV_INT_MIN, advInt );//����ģʽ
    //GAP_SetParamValue( TGAP_LIM_DISC_ADV_INT_MAX, advInt );
    GAP_SetParamValue( TGAP_GEN_DISC_ADV_INT_MIN, advInt );//��ͨģʽ
    GAP_SetParamValue( TGAP_GEN_DISC_ADV_INT_MAX, advInt );

  // Setup the GAP Bond Manager GAP�󶨹�������
    uint32 passcode  = str2Num(sys_information.pass, 6);
           passcode %= 1000000;
    uint8 pairMode = GAPBOND_PAIRING_MODE_INITIATE; //GAPBOND_PAIRING_MODE_INITIATE = 0x02
    uint8 mitm     = TRUE;                          //TRUE                          = 1    
    uint8 ioCap    = GAPBOND_IO_CAP_DISPLAY_ONLY;  //GAPBOND_IO_CAP_DISPLAY_ONLY = 0x00
    uint8 bonding  = TRUE;                         //TRUE                          = 1               
    GAPBondMgr_SetParameter( GAPBOND_DEFAULT_PASSCODE, sizeof ( uint32 ), &passcode );//д����֤����
    GAPBondMgr_SetParameter( GAPBOND_PAIRING_MODE,     sizeof ( uint8  ), &pairMode );//�ӻ�������ȫ����=��Ҫ��Թ���
    GAPBondMgr_SetParameter( GAPBOND_MITM_PROTECTION,  sizeof ( uint8  ), &mitm     );//��Ҫ������֤
    GAPBondMgr_SetParameter( GAPBOND_IO_CAPABILITIES,  sizeof ( uint8  ), &ioCap    );//�豸�����������
    GAPBondMgr_SetParameter( GAPBOND_BONDING_ENABLED,  sizeof ( uint8  ), &bonding  );//�򿪰󶨹���
    //GAPBondMgr_SetParameter( GAPBOND_ERASE_ALLBONDS,                   0 , NULL      );//�������Ϣ
 

  // Initialize GATT attributes ��ʼ��GATT����
   GGS_AddService          ( GATT_ALL_SERVICES );  // GAP
   GATTServApp_AddService  ( GATT_ALL_SERVICES );  // GATT attributes
   DevInfo_AddService      (                   );  // Device Information Service
   SimpleProfile_AddService( GATT_ALL_SERVICES );  // Simple GATT Profile
  
#if defined FEATURE_OAD
  VOID OADTarget_AddService();                    // OAD Profile
#endif

   //����FFF0��ͨ��1��2��6,8ֵ�ĳ�ʼ��

//   SimpleProfile_SetParameter( SIMPLEPROFILE_CHAR1, sizeof ( uint8 ), &Packet_Attribute_1.Heat_Level );//0
//   SimpleProfile_SetParameter( SIMPLEPROFILE_CHAR2, sizeof ( uint8 ), &Packet_Attribute_1.Work_State );//0
//   SimpleProfile_SetParameter( SIMPLEPROFILE_CHAR6, sizeof ( uint8 ), &Packet_Attribute_1.Heat_State );//0
//   SimpleProfile_SetParameter( SIMPLEPROFILE_CHAR8, sizeof ( Packet_Attribute_1.Device_Version ), &Packet_Attribute_1.Device_Version);//005A


   // Register for all key events - This app will handle all key events ע�ᰴ������ص�����
   RegisterForKeys( simpleBLETaskId );

  // Register callback with SimpleGATTprofile ע��͸����Ӧ�ص����� 
   VOID SimpleProfile_RegisterAppCBs( &simpleBLEPeripheral_SimpleProfileCBs );

  // �رյ�CLK�Զ���Ƶ������������ᵼ��Ƶ���Զ��л���DMA�����ܵ�Ӱ�죬С��Χ������    
  HCI_EXT_ClkDivOnHaltCmd( HCI_EXT_DISABLE_CLK_DIVIDE_ON_HALT );

  // �źŷ���ǿ��  
  HCI_EXT_SetTxPowerCmd(LL_EXT_TX_POWER_4_DBM);//LL_EXT_TX_POWER_4_DBM = 3 = +4dbm


}

/*********************************************************************
 * @fn      SimpleBLEPeripheral_ProcessEvent
 *
 * @brief   Simple BLE Peripheral Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */

uint16 SimpleBLEPeripheral_ProcessEvent( uint8 task_id, uint16 events )
{

  VOID task_id; // OSAL required parameter that isn't used in this function

  if ( events & SYS_EVENT_MSG )
  {
    uint8 *pMsg;

    if ( (pMsg = osal_msg_receive( simpleBLETaskId )) != NULL )
    {
      simpleBLEPeripheral_ProcessOSALMsg( (osal_event_hdr_t *)pMsg );
      VOID osal_msg_deallocate( pMsg );  // Release the OSAL message
    }
    return (events ^ SYS_EVENT_MSG);
   }


 /**************************��������**************************/
  if ( events & SBP_QD_EVT )
  {          
    //POW_LOCK = 0;
       Packet_Init();
       VOID GAPRole_StartDevice( &simpleBLEPeripheral_PeripheralCBs );   //ע����Χ�豸 ״̬ ֪ͨ �Ͷ�ȡRSSI�ص�����
       VOID GAPBondMgr_Register( &simpleBLEPeripheral_BondMgrCBs );      //ע��������֤�Ͱ�״̬�ص�����
       uint8  initial_advertising_enable = TRUE;//�㲥ʹ��
       GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED,  sizeof( uint8  ), &initial_advertising_enable );//�㲥ʹ��
       osal_start_timerEx( simpleBLETaskId, SBP_BLE_CMNC_EVT, 10 );
       osal_start_timerEx( simpleBLETaskId, SBP_PERIODIC_EVT, 30 );
       osal_start_timerEx( simpleBLETaskId, SBP_UI_EVT,       70 );
       osal_start_timerEx( simpleBLETaskId, SBP_YB_CMNC_EVT,  90 );
       osal_start_timerEx( simpleBLETaskId, SBP_WAKE_EVT,     110);     // ��ʱ400ms���ѣ� ��Ȼ�����˯�ߣ�ԭ����
    return ( events ^ SBP_QD_EVT );
  }
  /******************************����*****************************/
  if(Packet_1.PAK.Apparatus_Status)
  {
    
        if(Key_Up == 0)
        {
          DelayMS(1);
          if(Key_Up == 0)
          {
            qiangdu = qiangdu + 1;
            if(qiangdu >=15)
            {
               
              qiangdu = 15;
            }
          }
          while(!Key_Up);
        }
        
        if(Key_Down == 0)
        {
          DelayMS(1);
          if(Key_Down == 0)
          {
            qiangdu = qiangdu - 1;
            if(qiangdu <=0)
            {
              qiangdu = 0;
            }
          } 
          while(!Key_Down);
        }
        
        if(Power_Check == 1)
        {
          osal_start_timerEx( simpleBLETaskId, SBP_GJ_EVT, 10 );
          LED_B = 1;      
          EA = 0;                  //�����ж�
          POW_LOCK = 0;
        }
 

      /**********************�ػ�************************/
      if ( events & SBP_GJ_EVT )
      {   
        Packet_End();
        return ( events ^ SBP_GJ_EVT );
      }
      
      if ( events & SBP_WAKE_EVT )
      {      
        osal_pwrmgr_device( PWRMGR_ALWAYS_ON);      
        return ( events ^ SBP_WAKE_EVT );
      }

     /***********************״ָ̬ʾ************************/
      if ( events & SBP_UI_EVT )
      {
////        static uint8 flash = 0;
//         if(Packet_1.YB.ReConnect_Flag || Packet_1.YB.Disconnect_Flag)
//         {
////             if(!flash)
////             {
////               flash = 1;
////               HalLedSet(HAL_LED_RGB, HAL_LED_MODE_OFF);
////             }
////             else
////             {
////               flash = 0;
////               if(Packet_1.PAK.Work_State != Work_Run)
////               {
////                  W_LED();
////               }
////
////             }
//         }
//         else
//         {
////           if(Packet_1.PAK.Heat_Level == Heat_High && Packet_1.PAK.Work_State == Work_Run)
////           {
//////             R_LED();
////             LED_B = 1;
////           }
////           if(Packet_1.PAK.Heat_Level == Heat_Low && Packet_1.PAK.Work_State == Work_Run)
////           {
////             B_LED();
////             LED_B = 1;
////           }
////           if(Packet_1.PAK.Work_State == Work_Stop)
////           {
//////             W_LED();
////             LED_B = 1;
////           }
//           
////          flash = 0;
//         }
         osal_start_timerEx( simpleBLETaskId, SBP_UI_EVT, 200 );
         return ( events ^ SBP_UI_EVT );
      }

      /*************************ҩ���������***************************/
//      if(events & SBP_YB_CMNC_EVT)
//      {
//        //YB_DataDeal();
//        osal_start_timerEx( simpleBLETaskId, SBP_YB_CMNC_EVT, 250);
//        return (events ^ SBP_YB_CMNC_EVT);
//      }
     /*************************����͸������***************************/
      if ( events &   SBP_BLE_CMNC_EVT )
      {
        uint8  send_date[18] ={0};
        if((simpleBLE_IfConnected() && gPairStatus==BOND_PAIR_STATUS_PAIRED))//���Ӳ�����ж�
        {  
          ProfileReadConfig(SIMPLEPROFILE_CHAR1_UUID,send_date);//��������
          BLEUart_UUID1(send_date,SIMPLEPROFILE_CHAR1_LEN);//͸������    
        }
        osal_start_timerEx( simpleBLETaskId, SBP_BLE_CMNC_EVT, 300);
        return (events ^ SBP_BLE_CMNC_EVT);
      }
      
     /*************************�����������***************************/
      if ( events & SBP_PERIODIC_EVT )
      {
        static uint8 i;
        PacketPeriodicTask_1(); 
        if(i++ == 5)//5����һ�ε�ص�ѹ���¶�
        {
           uint16 temp;
           i = 0; 
           temp = Exchange_BatADC(Packet_ADC_14(BAT));
           if(temp == 0xff )
           {
              Packet_End();
           }
           else if(temp < Packet_1.PAK.Battery_capacity)//��ص�������,�������ʱͻ��
           {
             Packet_1.PAK.Battery_capacity = temp;
           }
           else
           {
             Packet_1.PAK.Battery_capacity = Packet_1.PAK.Battery_capacity;
           }
          // Packet_1.PAK.Temperature = Exchange_TempADC(Packet_ADC_14(TEMP));
//           Packet_1.PAK.Temperature = Packet_1.YB.LOCAL_MSG[2];
        }
        osal_start_timerEx( simpleBLETaskId, SBP_PERIODIC_EVT, 1000);
        return (events ^ SBP_PERIODIC_EVT);
      }
  }
  
  return 0;
}

/*********************************************************************
 * @fn      simpleBLEPeripheral_ProcessOSALMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void simpleBLEPeripheral_ProcessOSALMsg( osal_event_hdr_t *pMsg )
{
  switch ( pMsg->event )
  {
    case KEY_CHANGE:
      ;
      break;

    case GATT_MSG_EVENT:
      // Process GATT message
      simpleBLEPeripheral_ProcessGATTMsg( (gattMsgEvent_t *)pMsg );
      break;
      
    default:
    // do nothing
    break;
  }
}

/*********************************************************************
 * @fn      simpleBLEPeripheral_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */


/*********************************************************************
 * @fn      simpleBLEPeripheral_ProcessGATTMsg
 *
 * @brief   Process GATT messages
 *
 * @return  none
 */
static void simpleBLEPeripheral_ProcessGATTMsg( gattMsgEvent_t *pMsg )
{  
  GATT_bm_free( &pMsg->msg, pMsg->method );
}

/*********************************************************************
 * @fn      peripheralStateNotificationCB
 *
 * @brief   Notification from the profile of a state change.
 *
 * @param   newState - new state
 *
 * @return  none
 */
static void peripheralStateNotificationCB( gaprole_States_t newState )//�ӻ�״̬�ص���Ӧ����ɫgaprole_States_t;
{
  switch ( newState )
  {
    case GAPROLE_STARTED:
      {
        uint8 ownAddress[B_ADDR_LEN];
        uint8 systemId[DEVINFO_SYSTEM_ID_LEN];

        GAPRole_GetParameter(GAPROLE_BD_ADDR, ownAddress);

        // use 6 bytes of device address for 8 bytes of system ID value
        systemId[0] = ownAddress[0];
        systemId[1] = ownAddress[1];
        systemId[2] = ownAddress[2];
        systemId[3] = 0x00;
        systemId[4] = 0x00;
        systemId[5] = ownAddress[3];
        systemId[6] = ownAddress[4];
        systemId[7] = ownAddress[5];
        DevInfo_SetParameter(DEVINFO_SYSTEM_ID, DEVINFO_SYSTEM_ID_LEN, systemId);
      }
      break;

    case GAPROLE_ADVERTISING:
      {
        ;
      }
      break;

    case GAPROLE_CONNECTED:
      {     
        //NPI_WriteTransport("Connected\r\n", 11);
        simpleBLE_Delay_1ms(1);  //Ϊ�˵ȷ�������������ʱһС��
        // Get connection handle
        GAPRole_GetParameter(GAPROLE_CONNHANDLE, &gapConnHandle);
        //g_sleepFlag = FALSE;
        //osal_pwrmgr_device( PWRMGR_ALWAYS_ON);   //  ��˯�ߣ����ĺܸߵ�        
      }
      break;

    case GAPROLE_WAITING:
      {
         ;
      }
      break;

    case GAPROLE_WAITING_AFTER_TIMEOUT:
      {
         ;
      }
      break;

    case GAPROLE_ERROR:
      {
        ;
      }
      break;

    default:
      {
        ;
      }
      break;

  }

  gapProfileState = newState;

}


static void peripheralRssiReadCB( int8 rssi )
{
    simpleBle_SetRssi(rssi);
}

/*********************************************************************
 * @fn      simpleProfileChangeCB
 *
 * @brief   Callback from SimpleBLEProfile indicating a value change
 *
 * @param   paramID - parameter ID of the value that was changed.
 *
 * @return  none
 */
static void simpleProfileChangeCB( uint8 paramID )//�ֻ��˷������������Ӧ�ص�����
{
  uint8 newValue[18];
  uint8 returnBytes;

    switch( paramID )
    {
      case SIMPLEPROFILE_CHAR1:
        SimpleProfile_GetParameter( SIMPLEPROFILE_CHAR1, &newValue, &returnBytes );
        BLE_UartData_Deal(newValue,returnBytes);
        break;
            
      default:
        ;
        break;
    }
}

//#if defined( BLE_BOND_PAIR )
//�󶨹����е��������ص�����
static void ProcessPasscodeCB(uint8 *deviceAddr,uint16 connectionHandle,uint8 uiInputs,uint8 uiOutputs )
{
  uint32  passcode;

  //������������ô洢������֮ǰ�趨�����룬�����Ϳ��Զ�̬�޸���������ˡ�
  // Create random passcode
 passcode = str2Num(sys_information.pass, 6);
 passcode %= 1000000;
 GAPBondMgr_PasscodeRsp( connectionHandle, SUCCESS, passcode );
}

//�󶨹����е�״̬����������������ñ�־λ�������벻��ȷʱ���������ӡ�
static void ProcessPairStateCB( uint16 connHandle, uint8 state, uint8 status )//GAPBondMgr_ProcessGAPMsg
{
  // �����������ӣ�����뿪ʼ��״̬
  if ( state == GAPBOND_PAIRING_STATE_STARTED )
  {
    gPairStatus = BOND_PAIR_STATUS_PAIRING;
  }
  // �������ύ����󣬻�������
  else if ( state == GAPBOND_PAIRING_STATE_COMPLETE )
  {
    if ( status == SUCCESS )
    {
       gPairStatus = BOND_PAIR_STATUS_PAIRED;
    }
    else/*���벻��ȷ��������ǰ�Ѿ���*/
    {   
      if(status ==8)
      {//�Ѱ�
          gPairStatus = BOND_PAIR_STATUS_PAIRED;
      }
      else
      {
	 gPairStatus = BOND_PAIR_STATUS_PAIRING;

         GAPRole_TerminateConnection();// ��ֹ����
         //simpleBLE_Delay_1ms(100);
         //HAL_SYSTEM_RESET(); //��λ�ӻ�
      }   
      gPairStatus = BOND_PAIR_STATUS_PAIRING;
    }
    
	//�ж���Խ�����������ȷ����ֹͣ���ӡ�
    if((gapProfileState == GAPROLE_CONNECTED) && (gPairStatus == BOND_PAIR_STATUS_PAIRING))
    {
       GAPRole_TerminateConnection();// ��ֹ����     
       //HAL_SYSTEM_RESET(); //��λ�ӻ�
    }
  }
  // �������ύ����ӻ���֤�������Գɹ�״̬
  else if ( state == GAPBOND_PAIRING_STATE_BONDED )
  {
    if ( status == SUCCESS )
    {
      gPairStatus = BOND_PAIR_STATUS_PAIRED;
    }
  }
}
//#endif
/*********************************************************************
*********************************************************************/

#ifndef SIMPLEBLE_H
#define SIMPLEBLE_H

#ifdef __cplusplus
extern "C"
{
#endif


// 主任务的事件定义
#define START_DEVICE_EVT           0x0001//启动设备          
#define SBP_PERIODIC_EVT           0x0002//系统轮询定时器
#define SBP_WAKE_EVT               0x0040//唤醒事件
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
// 定于系统结构变量， 该结构会在开机时从nv flash 中读取， 数据有修改时， 需要写入nv flash
// 这样， 就实现了系统重启后数据还是上一次设置的
typedef struct 
{
    /*
    波特率
    0---------9600 
    1---------19200 
    2---------38400 
    3---------57600 
    4---------115200
    */
    uint8 baudrate;                 //波特率 ， 目前支持的列表如上
    uint8 parity;                   //校验位    
    uint8 stopbit;                  //停止位
    
    uint8 mode;                     //工作模式 0:透传 ， 1: 直驱 , 2: iBeacon

    // 设备名称，最长 11 位数字或字母，含中划线和下划线，不建议用其它字符    
    uint8 name[12];                 

    BLE_ROLE role;                  //主从模式  0: 从机   1: 主机

    uint8 pass[7];                  //密码， 最大6位 000000~999999 

    /*
    Para: 0 ~ 1 
    0: 连接不需要密码
    1: 连接需要密码
    */
    uint8 type;                     //鉴权模式
    uint8 mitm ;//需要输入密码
    uint8 ioCap;
    uint8 bonding;
    
    uint8 mac_addr[MAC_ADDR_CHAR_LEN+1];            //本机mac地址 最大12位 字符表示
    uint8 connect_mac_addr[MAC_ADDR_CHAR_LEN+1];    //指定去连接的mac地址


    //曾经成功连接过的从机个数
    uint8 ever_connect_peripheral_mac_addr_conut;
    //曾经成功连接过的从机个数,当前index， 用于增加从机地址时快速插入或读取
    uint8 ever_connect_peripheral_mac_addr_index;
    //最新一次成功连接过的从机地址index， 用于针对AT+CONNL 这个指令
    uint8 last_connect_peripheral_mac_addr_index;
    //曾经成功连接过的从机地址
    uint8 ever_connect_mac_status[MAX_PERIPHERAL_MAC_ADDR][MAC_ADDR_CHAR_LEN];       

    /*
    Para: 000000～009999 
    000000 代表持续连接，其
    余代表尝试的毫秒数
    Default:001000
    */
    
    uint16 try_connect_time_ms;           // 尝试连接时间---目前无效
    int8 rssi;                              //  RSSI 信号值
    uint8 rxGain;                           //  接收增益强度
    uint8 txPower;                          //  发射信号强度
    uint16 ibeacon_adver_time_ms;         // 广播间隔

    //  模块工作类型  0: 立即工作， 1: 等待AT+CON 或 AT+CONNL 命令
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



extern uint8 simpleBLETaskId;               // 主任务
extern uint16 simpleBLECharHdl;
extern uint16 simpleBLECharHd6;
extern bool simpleBLECentralCanSend;
extern bool simpleBLEChar6DoWrite;
extern uint8 simpleBLEPeripheral_TaskID;        // 从机任务



#if 1
// 该函数延时时间为1ms， 用示波器测量过， 稍有误差， 但误差很小  --amomcu.com
void simpleBLE_Delay_1ms(int times);

// 字符串对比
uint8 str_cmp(uint8 *p1,uint8 *p2,uint8 len);

// 字符串转数字
uint32 str2Num(uint8* numStr, uint8 iLength);

char *bdAddr2Str( uint8 *pAddr );
char *AH_bdAddr2Str( uint8 *pAddr );


// 判断蓝牙是否连接上
// 0: 未连接上
// 1: 已连接上
bool simpleBLE_IfConnected();




// 串行口 uart 初始化
void simpleBLE_NPI_init(void);

// 设置接收增益
void UpdateRxGain(void);

// 设置发射功率
void UpdateTxPower(void);


// 保存RSSI 到系统变量
void simpleBle_SetRssi(int8 rssi);

// 串口打印密码  -----测试用----
void simpleBle_PrintPassword();



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
extern void simpleBLE_NpiSerialCallback( uint8 port, uint8 events );

#endif



#ifdef __cplusplus
}
#endif

#endif /* SIMPLEBLE_H */

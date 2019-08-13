#ifndef PACKET_BLE_H
#define PACKET_BLE_H
extern uint8 BLE_UartData_Deal(uint8 *data,uint8 len);
extern uint8 ProfileReadConfig(uint16 uuid, uint8 *newValue);//通信，信息获取
extern void BLEUart_UUID1(uint8 *buf, uint8 numBytes);
#endif 
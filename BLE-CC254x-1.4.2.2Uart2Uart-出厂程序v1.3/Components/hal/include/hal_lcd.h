/******************************************************************************

 @file  hal_lcd.h

 @brief This file contains the interface to the LCD Service.

 Group: WCS, BTS
 Target Device: CC2540, CC2541

 ******************************************************************************
 
 Copyright (c) 2005-2016, Texas Instruments Incorporated
 All rights reserved.

 IMPORTANT: Your use of this Software is limited to those specific rights
 granted under the terms of a software license agreement between the user
 who downloaded the software, his/her employer (which must be your employer)
 and Texas Instruments Incorporated (the "License"). You may not use this
 Software unless you agree to abide by the terms of the License. The License
 limits your use, and you acknowledge, that the Software may not be modified,
 copied or distributed unless embedded on a Texas Instruments microcontroller
 or used solely and exclusively in conjunction with a Texas Instruments radio
 frequency transceiver, which is integrated into your product. Other than for
 the foregoing purpose, you may not use, reproduce, copy, prepare derivative
 works of, modify, distribute, perform, display or sell this Software and/or
 its documentation for any purpose.

 YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
 PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
 NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
 TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
 NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
 LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
 INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
 OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
 OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
 (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

 Should you have any questions regarding your right to use this Software,
 contact Texas Instruments Incorporated at www.TI.com.

 ******************************************************************************
 Release Name: ble_sdk_1.4.2.2
 Release Date: 2016-06-09 06:57:09
 *****************************************************************************/

#ifndef HAL_LCD_H
#define HAL_LCD_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 *                                          INCLUDES
 **************************************************************************************************/
#include "hal_board.h"

#define IO_LcdBL              P1_7
#define IO_LcdBL_BV           BV(7)
#define IO_LcdBL_SBIT         P1_7
#define IO_LcdBL_DDR          P1DIR
#define IO_LcdBL_POLARITY     !!
#define HAL_OFF_IO_LcdBL()    st( IO_LcdBL_SBIT = IO_LcdBL_POLARITY (1); )
#define HAL_ON_IO_LcdBL()     st( IO_LcdBL_SBIT = IO_LcdBL_POLARITY (0); )

#define IO_LcdRES              P1_5
#define IO_LcdRES_BV           BV(5)
#define IO_LcdRES_SBIT         P1_5
#define IO_LcdRES_DDR          P1DIR
#define IO_LcdRES_POLARITY     !!
#define HAL_OFF_IO_LcdRES()    st( IO_LcdRES_SBIT = IO_LcdRES_POLARITY (0); )
#define HAL_ON_IO_LcdRES()     st( IO_LcdRES_SBIT = IO_LcdRES_POLARITY (1); )

#define IO_LcdA0               P1_4
#define IO_LcdA0_BV            BV(4)
#define IO_LcdA0_SBIT          P1_4
#define IO_LcdA0_DDR           P1DIR
#define IO_LcdA0_POLARITY      !!
#define HAL_OFF_IO_LcdA0()     st( IO_LcdA0_SBIT = IO_LcdA0_POLARITY (0); )
#define HAL_ON_IO_LcdA0()      st( IO_LcdA0_SBIT = IO_LcdA0_POLARITY (1); )

#define IO_LcdSCL              P1_7
#define IO_LcdSCL_BV           BV(7)
#define IO_LcdSCL_SBIT         P1_7
#define IO_LcdSCL_DDR          P1DIR
#define IO_LcdSCL_POLARITY     !!
#define HAL_OFF_IO_LcdSCL()    st( IO_LcdSCL_SBIT = IO_LcdSCL_POLARITY (0); )
#define HAL_ON_IO_LcdSCL()     st( IO_LcdSCL_SBIT = IO_LcdSCL_POLARITY (1); )

#define IO_LcdSDA_H            I2CIO =  0x02
#define IO_LcdSDA_L            I2CIO =  0x00
   
extern const char ASCII_6x12[11][12];
extern const char HZ_12x12[41][24];
extern const char HZ_16x16[6][32];

/*
 * Initialize LCD Service
 */
extern void LCD_12848_Init(void);
extern void IO_LCD_Init(void);
extern void LCD_12848_End(void);
extern void Write_Com(uint8 para);
extern void Write_Data(uint8 para);
extern void LCD_Clear(void);
extern void HZ_12x12_Display(const char (*font)[24],uint8 line,uint8 col);
extern void HZ_16x16_Display(const char (*font)[32],uint8 col);
extern void ASCII_6x12_Display(const char (*font)[12],uint8 line,uint8 col);
extern void DelayMS(uint16 ms);//32MHz
extern void DelayUS(uint16 us);

/**************************************************************************************************
**************************************************************************************************/

#ifdef __cplusplus
}
#endif

#endif

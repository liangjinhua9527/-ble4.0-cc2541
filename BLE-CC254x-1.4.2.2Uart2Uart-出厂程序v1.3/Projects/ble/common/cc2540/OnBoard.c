/******************************************************************************

 @file  OnBoard.c

 @brief This file contains the UI and control for the peripherals on the
        EVAL development board
        Info:          This file targets the Chipcon CC2430DB/CC2430EB

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
 PROVIDED 揂S IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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
 Release Date: 2016-06-09 06:57:10
 *****************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "bcomdef.h"
#include "OnBoard.h"
#include "OSAL.h"
#include "OnBoard.h"

#include "hal_led.h"
#include "hal_key.h"
#include "Packet.h"
#include "simpleGATTprofile.h"
#include "simpleBLE.h"
#include "simpleBLEPeripheral.h"



/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// Task ID not initialized
#define NO_TASK_ID 0xFF


/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
uint8 OnboardKeyIntEnable;


/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */
extern uint8 LL_PseudoRand( uint8 *randData, uint8 dataLen );

#if   defined FEATURE_ABL
#include "..\..\util\ABL\app\sbl_app.c"
#elif defined FEATURE_SBL
#include "..\..\util\SBL\app\sbl_app.c"
#elif defined FEATURE_EBL
#include "..\..\util\EBL\app\sbl_app.c"
#elif defined FEATURE_UBL_MSD
#include "..\..\util\UBL\soc_8051\usb_msd\app\ubl_app.c"
#else
void appForceBoot(void);
#endif
extern void InitT3();


/*********************************************************************
 * LOCAL VARIABLES
 */

// Registered keys task ID, initialized to NOT USED.
static uint8 registeredKeysTaskID = NO_TASK_ID;

/*********************************************************************
 * @fn      InitBoard()
 * @brief   Initialize the CC2540DB Board Peripherals
 * @param   level: COLD,WARM,READY
 * @return  None
 */
void InitBoard( uint8 level )
{
  if ( level == OB_COLD )
  {
    // Interrupts off
    osal_int_disable( INTS_ALL );
  }
  else  // !OB_COLD
  {
    /* Initialize Key stuff */ 
    OnboardKeyIntEnable = HAL_KEY_INTERRUPT_DISABLE;//   HAL_KEY_INTERRUPT_ENABLE
    HalKeyConfig( OnboardKeyIntEnable, OnBoard_KeyCallback);
  }
}

/*********************************************************************
 * @fn        Onboard_rand
 *
 * @brief    Random number generator
 *
 * @param   none
 *
 * @return  uint16 - new random number
 *
 *********************************************************************/
uint16 Onboard_rand( void )
{
  uint16 randNum;

  LL_PseudoRand( (uint8 *)&randNum, 2 );

  return ( randNum );
}

/*********************************************************************
 * @fn      _itoa
 *
 * @brief   convert a 16bit number to ASCII
 *
 * @param   num -
 *          buf -
 *          radix -
 *
 * @return  void
 *
 *********************************************************************/
void _itoa(uint16 num, uint8 *buf, uint8 radix)
{
  char c,i;
  uint8 *p, rst[5];

  p = rst;
  for ( i=0; i<5; i++,p++ )
  {
    c = num % radix;  // Isolate a digit
    *p = c + (( c < 10 ) ? '0' : '7');  // Convert to Ascii
    num /= radix;
    if ( !num )
      break;
  }

  for ( c=0 ; c<=i; c++ )
    *buf++ = *p--;  // Reverse character order

  *buf = '\0';
}

/*********************************************************************
 *                        "Keyboard" Support
 *********************************************************************/

/*********************************************************************
 * Keyboard Register function
 *
 * The keyboard handler is setup to send all keyboard changes to
 * one task (if a task is registered).
 *
 * If a task registers, it will get all the keys. You can change this
 * to register for individual keys.
 *********************************************************************/
uint8 RegisterForKeys( uint8 task_id )
{
  // Allow only the first task
  if ( registeredKeysTaskID == NO_TASK_ID )
  {
    registeredKeysTaskID = task_id;
    return ( true );
  }
  else
    return ( false );
}

/*********************************************************************
 * @fn      OnBoard_SendKeys
 *
 * @brief   Send "Key Pressed" message to application.
 *
 * @param   keys  - keys that were pressed
 *          state - shifted
 *
 * @return  status
 *********************************************************************/
uint8 OnBoard_SendKeys( uint8 keys, uint8 state )
{
  keyChange_t *msgPtr;

  if ( registeredKeysTaskID != NO_TASK_ID )
  {
    // Send the address to the task
    msgPtr = (keyChange_t *)osal_msg_allocate( sizeof(keyChange_t) );
    if ( msgPtr )
    {
      msgPtr->hdr.event = KEY_CHANGE;
      msgPtr->state = state;
      msgPtr->keys = keys;

      osal_msg_send( registeredKeysTaskID, (uint8 *)msgPtr );
    }
    return ( SUCCESS );
  }
  else
    return ( FAILURE );
}

/*********************************************************************
 * @fn      OnBoard_KeyCallback
 *
 * @brief   Callback service for keys
 *
 * @param   keys  - keys that were pressed
 *          state - shifted
 *
 * @return  void
 *********************************************************************/
extern Packet_Attribute Packet_1;
extern uint8 simpleBLETaskId;      
void OnBoard_KeyCallback ( uint8 keys, uint8 state )
{
  if ( keys == HAL_KEY_SW_QDJ )      //S2--启动按键处理
  { 
    if(state == 0x02)
    {
      if(Packet_1.PAK.Apparatus_Status == 0 )
      {
        if(Power_Check == 1)
        {
          LED_B = 1; 
          POW_LOCK = 0;
        }
        else
        {
          osal_start_timerEx( simpleBLETaskId, SBP_QD_EVT, 10 );
          POW_LOCK = 1;
          LED_B = 0; 
          EA = 1; 
       }
    }
    else
    {
      osal_start_timerEx( simpleBLETaskId, SBP_GJ_EVT, 10 );
      LED_B = 1;      
      EA = 0;                  //开总中断
      POW_LOCK = 0;
     } 
    }
    if(Packet_1.PAK.Apparatus_Status)
    {
      if(state == 0x01)
      {
        if(!(Packet_1.YB.ReConnect_Flag || Packet_1.YB.Disconnect_Flag))
        {
          if(Packet_1.PAK.LED_Status==0)      //切为第一档
          {
            Packet_1.PAK.LED_Status = 1;
            Mode_flag = 1;
//            Packet_1.PAK.Work_State = Work_Stop;
//            Packet_1.PAK.Heat_Level = Heat_Low;
//            Packet_1.PAK.Cure_State = 1;
          }

          else if(Packet_1.PAK.LED_Status==1) //切为第二档
          {
            Packet_1.PAK.LED_Status = 2;
            Mode_flag = 2;
            //temp_D = 10;
            Packet_1.PAK.Work_State = Work_Run;
//            Packet_1.PAK.Heat_Level = Heat_High;
//            Packet_1.PAK.Cure_State = 0;
          }
          else if(Packet_1.PAK.LED_Status==2) //切为第三档
          {
            Packet_1.PAK.LED_Status =3;
            Mode_flag = 3;
            //temp_D = 15;
            Packet_1.PAK.Work_State = Work_Run;
//            Packet_1.PAK.Cure_State = 1;
          }
          else if(Packet_1.PAK.LED_Status==3) //切为第四档
          {
            Packet_1.PAK.LED_Status =0;
            Mode_flag = 4;
            //temp_D = 15;
            Packet_1.PAK.Work_State = Work_Run;
//            Packet_1.PAK.Cure_State = 1;
          }
//          else if(Packet_1.PAK.LED_Status==4) //切为第四档
//          {
//            Packet_1.PAK.LED_Status =0;
//            Mode_flag = 4;
//            //temp_D = 15;
//            Packet_1.PAK.Work_State = Work_Run;
////            Packet_1.PAK.Cure_State = 1;
//          }
        }
      }
    } 
  }
//  
//  if( keys == HAL_KEY_SW_UPJ )
//  {
//    if(Packet_1.PAK.Apparatus_Status)
//    {
//      if(state == 0x01)
//      {
//        if(!(Packet_1.YB.ReConnect_Flag || Packet_1.YB.Disconnect_Flag))
//        {
//
//          LED_B = 0;
//          //qiangdu = qiangdu+1;
//        }
//      }
//    }
//  }
//  
//   if( keys == HAL_KEY_SW_DOWMJ )
//  {
//    if(Packet_1.PAK.Apparatus_Status)
//    {
//      if(state == 0x01)
//      {
//        if(!(Packet_1.YB.ReConnect_Flag || Packet_1.YB.Disconnect_Flag))
//        {
//          LED_B = 1;
////        qiangdu = qiangdu-1;
////        if(qiangdu <= 0)
////        {
////          qiangdu = 0;
//        }
//      }
//    }
//  }
}

/*********************************************************************
 * @fn      Onboard_soft_reset
 *
 * @brief   Effect a soft reset.
 *
 * @param   none
 *
 * @return  none
 *
 *********************************************************************/
__near_func void Onboard_soft_reset( void )
{
  HAL_DISABLE_INTERRUPTS();
  asm("LJMP 0x0");
}

#if   defined FEATURE_ABL
#elif defined FEATURE_SBL
#elif defined FEATURE_EBL
#elif defined FEATURE_UBL_MSD
#else
/*********************************************************************
 * @fn      appForceBoot
 *
 * @brief   Common force-boot function for the HCI library to invoke.
 *
 * @param   none
 *
 * @return  void
 *********************************************************************/
void appForceBoot(void)
{
  // Dummy function for HCI library that cannot depend on the SBL build defines.
}
#endif

/*********************************************************************
*********************************************************************/

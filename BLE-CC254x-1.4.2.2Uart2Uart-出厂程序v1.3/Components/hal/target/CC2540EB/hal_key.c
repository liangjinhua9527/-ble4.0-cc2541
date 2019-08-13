/******************************************************************************

 @file  hal_key.c

 @brief This file contains the interface to the HAL KEY Service.

 Group: WCS, BTS
 Target Device: CC2540, CC2541

 ******************************************************************************
 
 Copyright (c) 2006-2016, Texas Instruments Incorporated
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
 PROVIDED �AS IS� WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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
/*********************************************************************
 NOTE: If polling is used, the hal_driver task schedules the KeyRead()
       to occur every 100ms.  This should be long enough to naturally
       debounce the keys.  The KeyRead() function remembers the key
       state of the previous poll and will only return a non-zero
       value if the key state changes.

 NOTE: If interrupts are used, the KeyRead() function is scheduled
       25ms after the interrupt occurs by the ISR.  This delay is used
       for key debouncing.  The ISR disables any further Key interrupt
       until KeyRead() is executed.  KeyRead() will re-enable Key
       interrupts after executing.  Unlike polling, when interrupts
       are enabled, the previous key state is not remembered.  This
       means that KeyRead() will return the current state of the keys
       (not a change in state of the keys).

 NOTE: If interrupts are used, the KeyRead() fucntion is scheduled by
       the ISR.  Therefore, the joystick movements will only be detected
       during a pushbutton interrupt caused by S1 or the center joystick
       pushbutton.

 NOTE: When a switch like S1 is pushed, the S1 signal goes from a normally
       high state to a low state.  This transition is typically clean.  The
       duration of the low state is around 200ms.  When the signal returns
       to the high state, there is a high likelihood of signal bounce, which
       causes a unwanted interrupts.  Normally, we would set the interrupt
       edge to falling edge to generate an interrupt when S1 is pushed, but
       because of the signal bounce, it is better to set the edge to rising
       edge to generate an interrupt when S1 is released.  The debounce logic
       can then filter out the signal bounce.  The result is that we typically
       get only 1 interrupt per button push.  This mechanism is not totally
       foolproof because occasionally, signal bound occurs during the falling
       edge as well.  A similar mechanism is used to handle the joystick
       pushbutton on the DB.  For the EB, we do not have independent control
       of the interrupt edge for the S1 and center joystick pushbutton.  As
       a result, only one or the other pushbuttons work reasonably well with
       interrupts.  The default is the make the S1 switch on the EB work more
       reliably.

*********************************************************************/

/**************************************************************************************************
 *                                            INCLUDES
 **************************************************************************************************/
#include "hal_mcu.h"
#include "hal_defs.h"
#include "hal_types.h"
#include "hal_drivers.h"
#include "hal_adc.h"
#include "hal_key.h"
#include "hal_led.h"
#include "osal.h"
#include "OnBoard.h"
#include "Packet.h"

#if (defined HAL_KEY) && (HAL_KEY == TRUE)

/**************************************************************************************************
 *                                            CONSTANTS
 **************************************************************************************************/
#define HAL_KEY_RISING_EDGE   0
#define HAL_KEY_FALLING_EDGE  1

#define HAL_KEY_DEBOUNCE_VALUE  25

/* CPU port interrupt */
#define HAL_KEY_CPU_PORT_0_IF P0IF
#define HAL_KEY_CPU_PORT_2_IF P2IF

#if defined ( CC2540_MINIDK )
/* SW_1 is at P0.0 */
#define HAL_KEY_SW_1_PORT   P1
#define HAL_KEY_SW_1_BIT    BV(5)
#define HAL_KEY_SW_1_SEL    P1SEL
#define HAL_KEY_SW_1_DIR    P1DIR

/* SW_2 is at P0.1 */
#define HAL_KEY_SW_2_PORT   P1
#define HAL_KEY_SW_2_BIT    BV(5)
#define HAL_KEY_SW_2_SEL    P1SEL
#define HAL_KEY_SW_2_DIR    P1DIR

#define HAL_KEY_SW_1_IEN      IEN1  /* CPU interrupt mask register */
#define HAL_KEY_SW_1_ICTL     P1IEN /* Port Interrupt Control register */
#define HAL_KEY_SW_1_ICTLBIT  BV(0) /* P0IEN - P0.0 enable/disable bit */
#define HAL_KEY_SW_1_IENBIT   BV(5) /* Mask bit for all of Port_0 */

#define HAL_KEY_SW_1_PXIFG    P1IFG /* Interrupt flag at source */
#define HAL_KEY_SW_2_IEN      IEN1  /* CPU interrupt mask register */
#define HAL_KEY_SW_2_ICTL     P1IEN /* Port Interrupt Control register */
#define HAL_KEY_SW_2_ICTLBIT  BV(1) /* P0IEN - P0.1 enable/disable bit */
#define HAL_KEY_SW_2_IENBIT   BV(5) /* Mask bit for all of Port_0 */
#define HAL_KEY_SW_2_PXIFG    P1IFG /* Interrupt flag at source */

#define HAL_KEY_SW_1_EDGEBIT  BV(0)

#else


/* QDJ is at P0.5 */
#define HAL_KEY_SW_QDJ_PORT     P0
#define HAL_KEY_SW_QDJ_BIT      BV(5)
#define HAL_KEY_SW_QDJ_SEL      P0SEL
#define HAL_KEY_SW_QDJ_DIR      P0DIR
 
//#define HAL_KEY_SW_UPJ_PORT     P0   
//#define HAL_KEY_SW_UPJ_BIT      BV(3)
//#define HAL_KEY_SW_UPJ_SEL      P0SEL
//#define HAL_KEY_SW_UPJ_DIR      P0DIR
// 
//#define HAL_KEY_SW_DOWMJ_PORT   P0   
//#define HAL_KEY_SW_DOWMJ_BIT    BV(5)
//#define HAL_KEY_SW_DOWMJ_SEL    P0SEL
//#define HAL_KEY_SW_DOWMJ_DIR    P0DIR
 
/* QDJ interrupts */
#define HAL_KEY_SW_QDJ_IEN      IEN0  /* CPU interrupt mask register */
#define HAL_KEY_SW_QDJ_IENBIT   BV(5) /* Mask bit for all of Port_1 */
#define HAL_KEY_SW_QDJ_ICTL     P0IEN /* Port Interrupt Control register */
#define HAL_KEY_SW_QDJ_ICTLBIT  BV(5) /* P0IEN - P0.5 enable/disable bit */
#define HAL_KEY_SW_QDJ_PXIFG    P0IFG /* Interrupt flag at source */
/* QDJ edge interrupt */
#define HAL_KEY_SW_QDJ_EDGEBIT  BV(0)//BV(3)
#define HAL_KEY_SW_QDJ_EDGE     HAL_KEY_RISING_EDGE//HAL_KEY_FALLING_EDGE




/* Joy stick move at P2.0 */
#define HAL_KEY_JOY_MOVE_PORT   P2
#define HAL_KEY_JOY_MOVE_BIT    BV(4)//BV(0)
#define HAL_KEY_JOY_MOVE_SEL    P2SEL
#define HAL_KEY_JOY_MOVE_DIR    P2DIR
/* Joy move interrupts */
#define HAL_KEY_JOY_MOVE_IEN      IEN2  /* CPU interrupt mask register */
#define HAL_KEY_JOY_MOVE_IENBIT   BV(1) /* Mask bit for all of Port_2 */
#define HAL_KEY_JOY_MOVE_ICTL     P2IEN /* Port Interrupt Control register */
#define HAL_KEY_JOY_MOVE_ICTLBIT  BV(0) /* P2IENL - P2.0<->P2.3 enable/disable bit */
#define HAL_KEY_JOY_MOVE_PXIFG    P2IFG /* Interrupt flag at source */

#define HAL_KEY_JOY_CHN   HAL_ADC_CHANNEL_6

#endif

/**************************************************************************************************
 *                                            TYPEDEFS
 **************************************************************************************************/


/**************************************************************************************************
 *                                        GLOBAL VARIABLES
 **************************************************************************************************/

typedef enum key_result
{
  funcEmpty = 0x00,	
  function1 = 0x01,
  function2 = 0x02,	
  function3 = 0x03
}ENUM_KeyResult;
typedef enum key_state
{
  Key_Waiting = 0x01,
  Key_Trigger = 0x02,	
  Key_End     = 0x03
}ENUM_KeyState;
struct KEY_Register
{
  uint8  number;
  uint8  countG;
  uint8  countS;
  ENUM_KeyState  state;
  ENUM_KeyResult result;
}KEY;


static halKeyCBack_t pHalKeyProcessFunction;
static uint8 HalKeyConfigured;
bool Hal_KeyIntEnable;            /* interrupt enable/disable flag */

/**************************************************************************************************
 *                                        FUNCTIONS - Local
 **************************************************************************************************/
void halProcessKeyInterrupt(void);
uint8 halGetJoyKeyInput(void);

/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/



/**************************************************************************************************
 * @fn      HalKeyInit
 *
 * @brief   Initilize Key Service
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/
void HalKeyInit( void )
{
//  halKeySavedKeys = 0;  // Initialize previous key to 0.

  HAL_KEY_SW_QDJ_SEL &= ~(HAL_KEY_SW_QDJ_BIT);    /* Set pin function to GPIO */
  HAL_KEY_SW_QDJ_DIR &= ~(HAL_KEY_SW_QDJ_BIT);    /* Set pin direction to Input */
  
//  HAL_KEY_SW_UPJ_SEL &= ~(HAL_KEY_SW_UPJ_BIT);    /* Set pin function to GPIO */
//  HAL_KEY_SW_UPJ_DIR &= ~(HAL_KEY_SW_UPJ_BIT);    /* Set pin direction to Input */
//  
//  HAL_KEY_SW_DOWMJ_SEL &= ~(HAL_KEY_SW_DOWMJ_BIT);    /* Set pin function to GPIO */
//  HAL_KEY_SW_DOWMJ_DIR &= ~(HAL_KEY_SW_DOWMJ_BIT);    /* Set pin direction to Input */


  /* Initialize callback function */
  //pHalKeyProcessFunction  = NULL;

  /* Start with key is not configured */
  HalKeyConfigured = FALSE;


  KEY.number = HAL_KEY_SW_QDJ_BIT;

  KEY.countG  = 0;
  KEY.countS  = 0;
  KEY.state  = Key_Waiting;
  KEY.result = funcEmpty;
  

}


/**************************************************************************************************
 * @fn      HalKeyConfig
 *
 * @brief   Configure the Key serivce
 *
 * @param   interruptEnable - TRUE/FALSE, enable/disable interrupt
 *          cback - pointer to the CallBack function
 *
 * @return  None
 **************************************************************************************************/
void HalKeyConfig (bool interruptEnable, halKeyCBack_t cback)
{
  /* Enable/Disable Interrupt or */
  Hal_KeyIntEnable = interruptEnable;

  /* Register the callback fucntion */
  pHalKeyProcessFunction = cback;

  /* Determine if interrupt is enable or not */
  if (Hal_KeyIntEnable)
  {
    /* Rising/Falling edge configuratinn */
    PICTL &= ~(HAL_KEY_SW_QDJ_EDGEBIT);    /* Clear the edge bit */
    /* For falling edge, the bit must be set. */
  #if (HAL_KEY_SW_QDJ_EDGE == HAL_KEY_FALLING_EDGE)//HAL_KEY_RISING_EDGE
    PICTL |= HAL_KEY_SW_QDJ_EDGEBIT;
  #endif

    /* Interrupt configuration:
     * - Enable interrupt generation at the port
     * - Enable CPU interrupt
     * - Clear any pending interrupt
     */


    HAL_KEY_SW_QDJ_ICTL |= HAL_KEY_SW_QDJ_ICTLBIT;
    HAL_KEY_SW_QDJ_IEN |= HAL_KEY_SW_QDJ_IENBIT;
    HAL_KEY_SW_QDJ_PXIFG = (HAL_KEY_SW_QDJ_BIT);

    /* Do this only after the hal_key is configured - to work with sleep stuff */
    if (HalKeyConfigured == TRUE)
    {
      osal_stop_timerEx(Hal_TaskID, HAL_KEY_EVENT);  /* Cancel polling if active */
    }
  }
  else    /* Interrupts NOT enabled */
  {
    HAL_KEY_SW_QDJ_ICTL &= ~(HAL_KEY_SW_QDJ_ICTLBIT); /* don't generate interrupt */
    HAL_KEY_SW_QDJ_IEN &= ~(HAL_KEY_SW_QDJ_IENBIT);   /* Clear interrupt enable bit */
    osal_set_event(Hal_TaskID, HAL_KEY_EVENT);
  }

  /* Key now is configured */
  HalKeyConfigured = TRUE;
}


/**************************************************************************************************
 * @fn      HalKeyRead
 *
 * @brief   Read the current value of a key
 *
 * @param   None
 *
 * @return  keys - current keys status
 **************************************************************************************************/
uint8 HalKeyRead ( void )
{
  uint8 keys = 0;

  if (!(HAL_KEY_SW_QDJ_PORT & HAL_KEY_SW_QDJ_BIT))    /* Key is active low */
  {
    keys |= HAL_KEY_SW_QDJ;
  }
//  if (!(HAL_KEY_SW_UPJ_PORT & HAL_KEY_SW_UPJ_BIT))    /* Key is active low */
//  {
//    keys |= HAL_KEY_SW_UPJ;
//  }
//  if (!(HAL_KEY_SW_DOWMJ_PORT & HAL_KEY_SW_DOWMJ_BIT))    /* Key is active low */
//  {
//    keys |= HAL_KEY_SW_DOWMJ;
//  }
  return keys;
}


/**************************************************************************************************
 * @fn      HalKeyPoll
 *
 * @brief   Called by hal_driver to poll the keys
 *
 * @param   None
 *
 * @return  None
 * 
 * ������Ӧ�ص�����    OnBoard_KeyCallback
 **************************************************************************************************/
//ɨ�谴��ֵ
void HalKeyPoll (void)
{
      /******************������Ӧ********************/
      //if(!(P0 & KEY.number))
       if((P0 & KEY.number))
      {
        switch(KEY.state)
        {
          case Key_Waiting:
              KEY.state = Key_Trigger;
              break;
          case Key_Trigger:
              KEY.countG++;
              if(KEY.countG>=61)  
              {
                 KEY.result = function2;
                 if (KEY.result && (pHalKeyProcessFunction))
                 {
                   (pHalKeyProcessFunction) (KEY.number, KEY.result);
                 }
                  KEY.state  = Key_End;
              }
              break;
          case Key_End:
              KEY.result = funcEmpty;
              break;
          default:
              KEY.countG = 0;
              KEY.countS = 0;
              KEY.state  = Key_Waiting;
              KEY.result = funcEmpty;
            break;
         }
       }
      /******************������Ӧ********************/
      if (!(P0 & KEY.number))
      {
        if(KEY.countS >= 1)
        {
           if((0<KEY.countG) && (KEY.countG<=60))
           {
             KEY.result = function1;
             if (KEY.result && (pHalKeyProcessFunction))
             {
               (pHalKeyProcessFunction) (KEY.number, KEY.result);
             }       
           }
           KEY.countG = 0;
           KEY.countS = 0;
           KEY.state  = Key_Waiting;
           KEY.result = funcEmpty;
        }
        KEY.countS++;
      }
}



/**************************************************************************************************
 * @fn      halProcessKeyInterrupt
 *
 * @brief   Checks to see if it's a valid key interrupt, saves interrupt driven key states for
 *          processing by HalKeyRead(), and debounces keys by scheduling HalKeyRead() 25ms later.
 *
 * @param
 *
 * @return
 **************************************************************************************************/
void halProcessKeyInterrupt (void)
{
  bool valid=FALSE;

  if (HAL_KEY_SW_QDJ_PXIFG & HAL_KEY_SW_QDJ_BIT)  /* Interrupt Flag has been set */
  {
    HAL_KEY_SW_QDJ_PXIFG &= ~(HAL_KEY_SW_QDJ_BIT); /* Clear Interrupt Flag */
    //HAL_KEY_SW_QDJ_PXIFG = ~(HAL_KEY_SW_QDJ_BIT); /* Clear Interrupt Flag */
    valid = TRUE;
  }

#if defined ( AMOMCU_UART_RX_MODE)
  if (HAL_KEY_SW_QDJ_PXIFG & HAL_KEY_SW_QDJ_BIT)  /* Interrupt Flag has been set */
  {
    HAL_KEY_SW_QDJ_PXIFG = ~(HAL_KEY_SW_QDJ_BIT); /* Clear Interrupt Flag */
    b_amomcu_uart_rx_mode = true; 
    valid = TRUE;
  }
#endif


  if (valid)
  {
    osal_start_timerEx (Hal_TaskID, HAL_KEY_EVENT, HAL_KEY_DEBOUNCE_VALUE);
  }
}

/**************************************************************************************************
 * @fn      HalKeyEnterSleep
 *
 * @brief  - Get called to enter sleep mode
 *
 * @param
 *
 * @return
 **************************************************************************************************/
void HalKeyEnterSleep ( void )
{
}

/**************************************************************************************************
 * @fn      HalKeyExitSleep
 *
 * @brief   - Get called when sleep is over
 *
 * @param
 *
 * @return  - return saved keys
 **************************************************************************************************/
uint8 HalKeyExitSleep ( void )
{
  /* Wake up and read keys */
//  return ( HalKeyRead () );
  return 0;
}

/***************************************************************************************************
 *                                    INTERRUPT SERVICE ROUTINE
 ***************************************************************************************************/

/**************************************************************************************************
 * @fn      halKeyPort0Isr
 *
 * @brief   Port0 ISR
 *
 * @param
 *
 * @return
 **************************************************************************************************/
HAL_ISR_FUNCTION( halKeyPort0Isr, P0INT_VECTOR )
{
  HAL_ENTER_ISR();
  
  if( (HAL_KEY_SW_QDJ_PXIFG & HAL_KEY_SW_QDJ_BIT) )

  {
    halProcessKeyInterrupt();
  }

  /*
    Clear the CPU interrupt flag for Port_0
    PxIFG has to be cleared before PxIF
  */

  HAL_KEY_SW_QDJ_PXIFG = 0;

  HAL_KEY_CPU_PORT_0_IF = 0;

  CLEAR_SLEEP_MODE();

  HAL_EXIT_ISR();

  return;
}

#else

void HalKeyInit(void){}
void HalKeyConfig(bool interruptEnable, halKeyCBack_t cback){}
uint8 HalKeyRead(void){ return 0;}
void HalKeyPoll(void){}

#endif
/**************************************************************************************************
**************************************************************************************************/

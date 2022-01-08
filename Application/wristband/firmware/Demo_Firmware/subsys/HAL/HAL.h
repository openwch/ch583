/********************************** (C) COPYRIGHT *******************************
* File Name          : HAL.h
* Author             : WCH
* Version            : V1.0
* Date               : 2016/05/05
* Description        : 
*******************************************************************************/



/******************************************************************************/
#ifndef __HAL_H
#define __HAL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "RTC.h"
#include "SLEEP.h"	
#include "LED.h"
#include "KEY.h"
#include "ADC.h"
	
/* hal task Event */
#define   LED_BLINK_EVENT                                   0x0001
#define   HAL_KEY_EVENT			                            0x0002
#define   HAL_REG_INIT_EVENT		                        0x2000
#define   HAL_TEST_EVENT		                            0x4000
#define   HAL_RESET_EVENT                                   0x0004
#define   HAL_SHUTDONW_EVENT                                0x0008
#define   HAL_ADC_EVENT                                     0x0010

#define   HAL_TIME_EVENT                                    0x0080

// hal sys_message
#define MESSAGE_UART                0xA0    // UART message
#define UART0_MESSAGE             (MESSAGE_UART|0 )    // UART0 message
#define UART1_MESSAGE             (MESSAGE_UART|1 )    // UART1 message

#define USB_MESSAGE              		0xB0    // USB message



#define HEARTBT_MSG_EVT         0x01
#define WRIST_MSG_EVT           0x02
#define STEP_MSG_EVT            0x03
#define SHOW_PIC_EVT            0x04
#define SHOW_TIME_EVT           0x08

/*********************************************************************
 * GLOBAL VARIABLES
 */
extern tmosTaskID halTaskID;

typedef struct  tag_uart_package
{
  tmos_event_hdr_t hdr;
  uint8            *pData;
} uartPacket_t;

typedef struct  SendMSG
{
  tmos_event_hdr_t hdr;
  void            *pData;
} SendMSG_t;

/*********************************************************************
 * GLOBAL FUNCTIONS
 */
extern void HAL_Init( void );
extern tmosEvents HAL_ProcessEvent( tmosTaskID task_id, tmosEvents events );
extern void CH58X_BLEInit( void );
extern uint16 HAL_GetInterTempValue( void );
extern void Lib_Calibration_LSI( void );

uint8 OnBoard_SendMsg(uint8_t registeredTaskID, uint8 event, uint8 state, void *data);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif

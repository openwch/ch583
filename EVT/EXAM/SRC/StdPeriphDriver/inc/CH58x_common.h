/********************************** (C) COPYRIGHT *******************************
 * File Name          : CH57x_common.h
 * Author             : WCH
 * Version            : V1.2
 * Date               : 2021/11/17
 * Description
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/


#ifndef __CH58x_COMM_H__
#define __CH58x_COMM_H__

#ifdef __cplusplus
 extern "C" {
#endif


#ifndef  NULL
#define  NULL     0
#endif
#define  ALL			0xFFFF

#ifndef __HIGH_CODE
#define __HIGH_CODE   __attribute__((section(".highcode")))
#endif

#ifndef __INTERRUPT
#ifdef INT_SOFT
#define __INTERRUPT   __attribute__((interrupt()))
#else
#define __INTERRUPT   __attribute__((interrupt("WCH-Interrupt-fast")))
#endif
#endif

#define Debug_UART0        0
#define Debug_UART1        1
#define Debug_UART2        2
#define Debug_UART3        3

#ifdef DEBUG
#include <stdio.h>
#endif

/**
 * @brief  ϵͳ��Ƶʱ�ӣ�Hz��
 */
#ifndef	 FREQ_SYS
#define  FREQ_SYS		60000000
#endif

#ifndef  SAFEOPERATE
#define  SAFEOPERATE   asm volatile("fence.i")
#endif

/**
 * @brief  32Kʱ�ӣ�Hz��
 */
#ifdef CLK_OSC32K
#if ( CLK_OSC32K == 1 )
#define CAB_LSIFQ       32000
#else
#define CAB_LSIFQ       32768
#endif
#else
#define CAB_LSIFQ       32000
#endif

#include <string.h>
#include <stdint.h>
#include "CH583SFR.h"
#include "core_riscv.h"
#include "CH58x_clk.h"
#include "CH58x_uart.h"
#include "CH58x_gpio.h"
#include "CH58x_i2c.h"
#include "CH58x_flash.h"
#include "CH58x_pwr.h"
#include "CH58x_pwm.h"
#include "CH58x_adc.h"
#include "CH58x_sys.h"
#include "CH58x_timer.h"
#include "CH58x_spi.h"
#include "CH58x_usbdev.h"
#include "CH58x_usbhost.h"
#include "ISP583.h"


#define DelayMs(x)      mDelaymS(x)
#define DelayUs(x)      mDelayuS(x)


#ifdef __cplusplus
}
#endif

#endif  // __CH58x_COMM_H__


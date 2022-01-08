/********************************** (C) COPYRIGHT *******************************
* File Name          : KEY.h
* Author             : WCH
* Version            : V1.0
* Date               : 2016/04/12
* Description        : 
*******************************************************************************/



/******************************************************************************/
#ifndef __KEY_H
#define __KEY_H

#ifdef __cplusplus
extern "C"
{
#endif




/**************************************************************************************************
 *                                              MACROS
 **************************************************************************************************/
#define KEY_CHANGE                   0xC0    // Key message

#define HAL_KEY_RISING_EDGE            0
#define HAL_KEY_FALLING_EDGE           1

#define HAL_KEY_DEBOUNCE_VALUE         25
#define HAL_KEY_POLLING_VALUE          100

/* Interrupt option - Enable or disable */
#define HAL_KEY_INTERRUPT_DISABLE    0x00
#define HAL_KEY_INTERRUPT_ENABLE     0x01

/* Key state - shift or nornal */
#define HAL_KEY_STATE_NORMAL         0x00
#define HAL_KEY_STATE_SHIFT          0x01

/* Switches (keys) */
#define HAL_KEY_SW_1 0x01  // key1
#define HAL_KEY_SW_2 0x02  // key2
#define HAL_KEY_SW_3 0x04  // key3
#define HAL_KEY_SW_4 0x08  // key4
#define HAL_KEY_SW_5 0x10  // key5
  
/* Joystick */
#define HAL_KEY_UP     0x01  // Joystick up
#define HAL_KEY_RIGHT  0x02  // Joystick right
#define HAL_KEY_CENTER 0x04  // Joystick center
#define HAL_KEY_LEFT   0x08  // Joystick left
#define HAL_KEY_DOWN   0x10  // Joystick down
 /* °´¼ü¶¨Òå */

/* 1 - KEY */
#define KEY1_BV           BV(8)
#define KEY2_BV
#define KEY3_BV
#define KEY4_BV
#define KEY5_BV

#define KEY1_PD         {R32_PB_PD_DRV |= KEY1_BV; R32_PB_PU &= ~KEY1_BV;}
#define KEY2_PD
#define KEY3_PD
#define KEY4_PD
#define KEY5_PD

#define KEY1_PU         (R32_PB_PU |= KEY1_BV)
#define KEY2_PU
#define KEY3_PU
#define KEY4_PU
#define KEY5_PU

#define KEY1_DIR        (R32_PB_DIR &= ~KEY1_BV)
#define KEY2_DIR
#define KEY3_DIR
#define KEY4_DIR
#define KEY5_DIR

#define KEY1_IN         (ACTIVE_HIGH(R32_PB_PIN&KEY1_BV))
#define KEY2_IN
#define KEY3_IN
#define KEY4_IN
#define KEY5_IN

#define HAL_PUSH_BUTTON1()        ( KEY1_IN )
#define HAL_PUSH_BUTTON2()        (0)
#define HAL_PUSH_BUTTON3()        (0)
#define HAL_PUSH_BUTTON4()        (0)
#define HAL_PUSH_BUTTON5()        (0)


#define ENABLE_TPP223()             GPIOB_SetBits(GPIO_Pin_9),    \
                                        GPIOB_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA)

#define DISABLE_TPP223()            GPIOB_ResetBits(GPIO_Pin_9),    \
                                        GPIOB_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA)


/**************************************************************************************************
 * TYPEDEFS
 **************************************************************************************************/
typedef void (*halKeyCBack_t) (uint8 keys, uint8 state);

typedef struct
{
  tmos_event_hdr_t hdr;
  uint8  state; // shift
  uint8  keys;  // keys
} keyChange_t;

/**************************************************************************************************
 *                                             GLOBAL VARIABLES
 **************************************************************************************************/
extern uint8 Hal_KeyIntEnable;

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Initialize the Key Service
 */
void HAL_KeyInit( void );

/*
 * This is for internal used by hal_driver
 */
void HAL_KeyPoll( void );

/*
 * Register the Key Service
 */
void HAL_KEY_RegisterForKeys( tmosTaskID id );

/*
 * Configure the Key Service
 */
void HalKeyConfig( uint8 interruptEnable, const halKeyCBack_t cback);

/*
 * Read the Key callback
 */
void HalKeyCallback ( uint8 keys, uint8 state );

/*
 * Read the Key status
 */
uint8 HalKeyRead( void);

/**************************************************************************************************
**************************************************************************************************/

#ifdef __cplusplus
}
#endif

#endif

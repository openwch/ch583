/********************************** (C) COPYRIGHT *******************************
 * File Name          : app_tmos.C
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2023/8/5
 * Description        : 触摸按键例程
 *******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "Touch.h"
#include "app.h"
/*********************
 *      DEFINES
 *********************/

/**********************
 *      VARIABLES
 **********************/
UINT8V timerFlag = 0;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void TKY_PeripheralInit(void);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/*********************************************************************
 * @fn      touch_dataProcess
 *
 * @brief   触摸数据处理函数（裸跑），打印获取到的按键触发情况
 *
 * @return  none
 */
void touch_dataProcess(void)
{
    uint8_t key_val = 0;
    static uint16_t print_time = 0;

    if(timerFlag)
    {
        timerFlag = 0;
        touch_KeyScan();
#if PRINT_EN
        print_time++;
        if(print_time == 500)
        {
            print_time = 0;
            touch_InfoDebug();
        }
#endif
    }
    key_val = touch_GetKey();
    switch(key_val)
    {
       case KEY_NONE   :   break;
       case KEY_0_DOWN :   PRINT("KEY_1_DOWN !\n");break;
       case KEY_0_UP   :   PRINT("KEY_1_UP   !\n");break;
       case KEY_0_LONG :   PRINT("KEY_1_LONG !\n");break;
       case KEY_1_DOWN :   PRINT("KEY_2_DOWN !\n");break;
       case KEY_1_UP   :   PRINT("KEY_2_UP   !\n");break;
       case KEY_1_LONG :   PRINT("KEY_2_LONG !\n");break;
       case KEY_2_DOWN :   PRINT("KEY_3_DOWN !\n");break;
       case KEY_2_UP   :   PRINT("KEY_3_UP   !\n");break;
       case KEY_2_LONG :   PRINT("KEY_3_LONG !\n");break;
       case KEY_3_DOWN :   PRINT("KEY_4_DOWN !\n");break;
       case KEY_3_UP   :   PRINT("KEY_4_UP   !\n");break;
       case KEY_3_LONG :   PRINT("KEY_4_LONG !\n");break;
       case KEY_4_DOWN :   PRINT("KEY_5_DOWN !\n");break;
       case KEY_4_UP   :   PRINT("KEY_5_UP   !\n");break;
       case KEY_4_LONG :   PRINT("KEY_5_LONG !\n");break;
       case KEY_5_DOWN :   PRINT("KEY_6_DOWN !\n");break;
       case KEY_5_UP   :   PRINT("KEY_6_UP   !\n");break;
       case KEY_5_LONG :   PRINT("KEY_6_LONG !\n");break;
       case KEY_6_DOWN :   PRINT("KEY_7_DOWN !\n");break;
       case KEY_6_UP   :   PRINT("KEY_7_UP   !\n");break;
       case KEY_6_LONG :   PRINT("KEY_7_LONG !\n");break;
       case KEY_7_DOWN :   PRINT("KEY_8_DOWN !\n");break;
       case KEY_7_UP   :   PRINT("KEY_8_UP   !\n");break;
       case KEY_7_LONG :   PRINT("KEY_8_LONG !\n");break;
       case KEY_8_DOWN :   PRINT("KEY_9_DOWN !\n");break;
       case KEY_8_UP   :   PRINT("KEY_9_UP   !\n");break;
       case KEY_8_LONG :   PRINT("KEY_9_LONG !\n");break;
       case KEY_9_DOWN :   PRINT("KEY_*_DOWN !\n");break;
       case KEY_9_UP   :   PRINT("KEY_*_UP   !\n");break;
       case KEY_9_LONG :   PRINT("KEY_*_LONG !\n");break;
       case KEY_10_DOWN :  PRINT("KEY_0_DOWN!\n");break;
       case KEY_10_UP  :   PRINT("KEY_0_UP  !\n");break;
       case KEY_10_LONG :  PRINT("KEY_0_LONG!\n");break;
       case KEY_11_DOWN :  PRINT("KEY_#_DOWN!\n");break;
       case KEY_11_UP  :   PRINT("KEY_#_UP  !\n");break;
       case KEY_11_LONG :  PRINT("KEY_#_LONG!\n");break;
       default : break;
    }
}


/*********************************************************************
 * @fn      touch_init
 *
 * @brief   触摸初始化函数（不使用tmos，需要设备开启定时器）
 *
 * @return  none
 */
void touch_init(void)
{
	TKY_PeripheralInit();       /* 初始化外设，例如背光和蜂鸣器等 */

	touch_InitKey();				/* 初始化触摸库  */

    TKY_SetSleepStatusValue( ~tkyQueueAll );

    TMR0_TimerInit(FREQ_SYS/1000);               //定时周期为1ms
    TMR0_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
    PFIC_EnableIRQ( TMR0_IRQn );

    dg_log("Touch Key init Finish!\n");
}


/**********************
 *   STATIC FUNCTIONS
 **********************/

/*********************************************************************
 * @fn      TKY_PeripheralInit
 *
 * @brief   触摸相关外设初始化函数
 *
 * @return  none
 */
static void TKY_PeripheralInit(void)
{
    /*You code here*/
}

/*********************************************************************
 * @fn      TMR0_IRQHandler
 *
 * @brief   定时器0中断服务函数
 *
 * @return  none
 */
__INTERRUPT
__HIGH_CODE
void TMR0_IRQHandler( void )
{
    if( TMR0_GetITFlag( TMR0_3_IT_CYC_END ) )
    {
        TMR0_ClearITFlag( TMR0_3_IT_CYC_END );
        timerFlag=1;
    }
}

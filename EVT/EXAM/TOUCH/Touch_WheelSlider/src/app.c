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
    static uint16_t wheelslider_time = 0;
    uint16_t wheelrops=0;
    if(timerFlag)
    {
        timerFlag = 0;
        touch_KeyScan();
        wheelslider_time++;
        if(wheelslider_time==25)
        {
        	wheelslider_time=0;
        	wheelrops = touch_DetectWheelSlider();
        	if(wheelrops != TOUCH_OFF_VALUE)
        	{
        		PRINT ("wheel rops:%u\r\n",wheelrops);
        	}
        }

#if PRINT_EN
        print_time++;
        if(print_time == 500)
        {
            print_time = 0;
            touch_InfoDebug();
        }
#endif
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
	TKY_PeripheralInit();       /* 初始外设，例如背光和蜂鸣器等 */
	touch_InitKey();

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

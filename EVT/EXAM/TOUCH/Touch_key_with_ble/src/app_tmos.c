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
#include "CONFIG.h"
#include "app_tmos.h"
#include "peripheral.h"
/*********************
 *      DEFINES
 *********************/
#define SLEEP_TRIGGER_TIME MS1_TO_SYSTEM_TIME(500) // 500ms
#define TRIGGER_TIME MS1_TO_SYSTEM_TIME(100)       // 100ms
#define WAKEUP_TIME MS1_TO_SYSTEM_TIME(5)          // 50ms

/**********************
 *      VARIABLES
 **********************/
tmosTaskID TouchKey_TaskID = 0x00;
uint16_t triggerTime = SLEEP_TRIGGER_TIME;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void TKY_PeripheralInit(void);
static void peripherals_EnterSleep(void);
static void peripherals_WakeUp(void);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/*********************************************************************
 * @fn      tky_on_TMOS_dataProcess
 *
 * @brief   触摸数据处理函数（基于TMOS），蓝牙连接成功后将获取到键值以通知的形式上报给上位机蓝牙
 *
 * @return  none
 */
void tky_on_TMOS_dataProcess(void)
{
    uint8_t key_val = 0;
    key_val = touch_GetKey();
    if (key_val != 0x00)
    {
        if (bleConnectState )
        {
            peripheralChar2Notify( &key_val, 1 );//将键值上报给上位机蓝牙
        }
    }
}


/*********************************************************************
 * @fn      PeriodicDealData
 *
 * @brief    触摸休眠状态处理
 *
 * @return  none
 */
void PeriodicDealData(void)
{
    TKY_LoadAndRun(); //---载入休眠前保存的部分设置---
//    GPIOTK_PinSleep(  );

    //---唤醒态，唤醒时方可切换显示内容——基线或测量值，每次有触摸时，唤醒10个wakeup时间，按此定时器设置时间为5s---
    if (wakeUpCount)
    {
        wakeUpCount--;
//        dg_log("wakeUpCount: :%d\n", wakeUpCount);
        //---wakeUpCount计数为0，唤醒态即将转休眠---
        if (wakeUpCount == 0)
        {
        	touch_ScanEnterSleep();

            tmos_stop_task(TouchKey_TaskID, WAKEUP_DATA_DEAL_EVT);
            triggerTime = SLEEP_TRIGGER_TIME;
            /*-------------------------
             * Call your peripherals sleep function
             * -----------------------*/
            peripherals_EnterSleep();
        }
    }
    else //---休眠状态时，醒来间隔进行扫描---
    {
        dg_log("wake up...\n");

        scanData = TKY_ScanForWakeUp(tkyQueueAll); //---对所选择的队列通道进行扫描---

        if (scanData) //---如扫描有异常，则调用正式扫描函数模式3~4---
        {
            TKY_SetSleepStatusValue(~scanData); //---设置休眠状态，把有异常状态的通道设置为非休眠态---
            for (uint8_t i = 0; i < 40; i++) //---并非一定要扫码64次，20次以上皆可，并且下面代码中有当扫描有按键按下，则退出循环，启动唤醒扫描---
            {
                keyData = TKY_PollForFilter();
                if (keyData) //---一旦检测到有按键按下，则退出循环扫描---
                {
                	touch_ScanWakeUp();

                    triggerTime = TRIGGER_TIME;
                    tky_DealData_start();
                    tmos_start_task(TouchKey_TaskID, WAKEUP_DATA_DEAL_EVT, 0);
                    /*-------------------------
                     * Call your peripherals WakeUp function
                     * -----------------------*/
                    peripherals_WakeUp();
                    break;
                }
            }
            if (keyData == 0)
            {
                TKY_SaveAndStop(); //---对相关寄存器进行保存---
            }
        }
        else
        {
            TKY_SaveAndStop(); //---对相关寄存器进行保存---
        }
    }
    TKY_SaveAndStop(); //---对相关寄存器进行保存---
}


/*********************************************************************
 * @fn      tky_DealData_start
 *
 * @brief   触摸扫描开启函数
 *
 * @return  none
 */
void tky_DealData_start(void)
{
    tmos_set_event(TouchKey_TaskID, DEALDATA_EVT);
}

/*********************************************************************
 * @fn      tky_DealData_stop
 *
 * @brief   触摸扫描停止函数
 *
 * @return  none
 */
void tky_DealData_stop(void)
{
    tmos_stop_task(TouchKey_TaskID, DEALDATA_EVT);
}


/*********************************************************************
 * @fn      Touch_Key_ProcessEvent
 *
 * @brief   触摸按键处理函数
 *
 * @return  none
 */
tmosEvents Touch_Key_ProcessEvent(tmosTaskID task_id, tmosEvents events)
{
    uint16_t res;

    if (events & WAKEUP_DATA_DEAL_EVT)
    {
    	touch_KeyScan();
        tky_on_TMOS_dataProcess();
#if TKY_SLEEP_EN
        if (wakeupflag)
#endif
            tmos_start_task(TouchKey_TaskID, WAKEUP_DATA_DEAL_EVT, WAKEUP_TIME);
        return (events ^ WAKEUP_DATA_DEAL_EVT);
    }

    if (events & DEALDATA_EVT)
    {
        PeriodicDealData();
#if TKY_SLEEP_EN
        if (!advState || wakeupflag)
#endif
            tmos_start_task(TouchKey_TaskID, DEALDATA_EVT, triggerTime);
        return (events ^ DEALDATA_EVT);
    }

#if PRINT_EN
    if (events & DEBUG_PRINT_EVENT)
    {
        touch_InfoDebug();

        tmos_start_task(TouchKey_TaskID, DEBUG_PRINT_EVENT, SLEEP_TRIGGER_TIME);
        return (events ^ DEBUG_PRINT_EVENT);
    }
#endif

    return 0;
}


/*********************************************************************
 * @fn      touch_on_TMOS_init
 *
 * @brief   触摸初始化函数（基于TMOS）
 *
 * @return  none
 */
void touch_on_TMOS_init(void)
{
    TouchKey_TaskID = TMOS_ProcessEventRegister(Touch_Key_ProcessEvent);
    TKY_PeripheralInit();       /* 初始外设，例如背光和蜂鸣器等 */
    touch_InitKey();

    wakeUpCount = 50; //---唤醒时间---
    wakeupflag = 1;   // 置成唤醒状态
    triggerTime = TRIGGER_TIME;
    TKY_SetSleepStatusValue(~tkyQueueAll);
#if TKY_SLEEP_EN
    tky_DealData_start();
#else
    tky_DealData_stop();
#endif

#if PRINT_EN
    tmos_set_event(TouchKey_TaskID, DEBUG_PRINT_EVENT);
#endif

    tmos_set_event(TouchKey_TaskID, WAKEUP_DATA_DEAL_EVT);
    dg_log("Touch Key init Finish!\n");
}


/**********************
 *   STATIC FUNCTIONS
 **********************/


/*********************************************************************
 * @fn      TKY_PeripheralInit
 *
 * @brief   触摸外设初始化函数，用于初始化与触摸功能相关的外设功能
 *
 * @return  none
 */
static void TKY_PeripheralInit(void)
{
    /*You code here*/
}

/*********************************************************************
 * @fn      peripherals_EnterSleep
 *
 * @brief   外设睡眠函数，在触摸准备休眠时调用
 *
 * @return  none
 */
static void peripherals_EnterSleep(void)
{
    /*You code here*/
}


/*********************************************************************
 * @fn      peripherals_WakeUp
 *
 * @brief   外设唤醒函数，在触摸被唤醒时调用
 *
 * @return  none
 */
static void peripherals_WakeUp(void)
{
    /*You code here*/
}

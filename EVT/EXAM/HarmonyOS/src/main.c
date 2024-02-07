/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2022/05/10
 * Description        : kernel_liteos_m例程，使用硬件压栈，禁用中断嵌套，中断函数不再使用修饰
 *                      __attribute__((interrupt("WCH-Interrupt-fast")))，
 *                      中断函数直接按照普通函数定义，只使用HIGHCODE修饰即可。
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/******************************************************************************/
/* 头文件包含 */
#include "CH58x_common.h"
#include "stdarg.h"
#include "los_tick.h"
#include "los_task.h"
#include "los_config.h"
#include "los_interrupt.h"
#include "los_debug.h"
#include "los_compiler.h"
#include "los_sem.h"

static UINT32 g_semId;

/*********************************************************************
 * @fn      taskSampleEntry2
 *
 * @brief   taskSampleEntry2 program.
 *
 * @return  none
 */
__HIGH_CODE
VOID taskSampleEntry2(VOID)
{
    while(1) {
      LOS_TaskDelay(10000);
      PRINT("taskSampleEntry2 running,task2 SP:%08x\n",__get_SP());
    }
}

/*********************************************************************
 * @fn      taskSampleEntry1
 *
 * @brief   taskSampleEntry1 program.
 *
 * @return  none
 */
__HIGH_CODE
VOID taskSampleEntry1(VOID)
{
    while(1) {
      LOS_TaskDelay(2000);
      PRINT("taskSampleEntry1 running,task1 SP:%08x\n",__get_SP());
    }
}

/*********************************************************************
 * @fn      taskIntSem
 *
 * @brief   taskIntSem program.
 *
 * @return  none
 */
__HIGH_CODE
VOID taskIntSem(VOID)
{
    if(LOS_BinarySemCreate(0, &g_semId) == LOS_OK)
    {
        GPIOA_ModeCfg(GPIO_Pin_12, GPIO_ModeIN_PU);
        GPIOA_ITModeCfg(GPIO_Pin_12, GPIO_ITMode_FallEdge);
        PFIC_EnableIRQ(GPIO_A_IRQn);
        while (1)
        {
            if(LOS_SemPend(g_semId, LOS_WAIT_FOREVER) == LOS_OK)
            {
                PRINT("sem get ok\n");
            }
            else
            {
                PRINT("sem get failed\n");
            }
        }
    }
    else
    {
        PRINT("sem create failed\n");
    }
}

/*********************************************************************
 * @fn      taskSample
 *
 * @brief   taskSample program.
 *
 * @return  none
 */
UINT32 taskSample(VOID)
{
    UINT32  uwRet;
    UINT32 taskID1,taskID2,taskID3,taskIDIntSem;
    TSK_INIT_PARAM_S stTask={0};

    stTask.pfnTaskEntry = (TSK_ENTRY_FUNC)taskIntSem;
    stTask.uwStackSize  = LOSCFG_BASE_CORE_TSK_DEFAULT_STACK_SIZE;
    stTask.pcName       = "int sem test";
    stTask.usTaskPrio   = 6;/* high priority */
    uwRet = LOS_TaskCreate(&taskIDIntSem, &stTask);
    if (uwRet != LOS_OK) {
        PRINT("create int sem test failed\n");
    }

    stTask.pfnTaskEntry = (TSK_ENTRY_FUNC)taskSampleEntry1;
    stTask.uwStackSize  = LOSCFG_BASE_CORE_TSK_DEFAULT_STACK_SIZE;
    stTask.pcName       = "taskSampleEntry1";
    stTask.usTaskPrio   = 6;/* high priority */
    uwRet = LOS_TaskCreate(&taskID1, &stTask);
    if (uwRet != LOS_OK) {
        PRINT("create task1 failed\n");
    }

    stTask.pfnTaskEntry = (TSK_ENTRY_FUNC)taskSampleEntry2;
    stTask.uwStackSize  = LOSCFG_BASE_CORE_TSK_DEFAULT_STACK_SIZE;
    stTask.pcName       = "taskSampleEntry2";
    stTask.usTaskPrio   = 7;/* low priority */
    uwRet = LOS_TaskCreate(&taskID2, &stTask);
    if (uwRet != LOS_OK) {
        PRINT("create task2 failed\n");
    }

    return LOS_OK;
}

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
LITE_OS_SEC_TEXT_INIT int main(void)
{
    unsigned int ret;

#if (defined(DCDC_ENABLE)) && (DCDC_ENABLE == TRUE)
    PWR_DCDCCfg(ENABLE);
#endif
    SetSysClock(CLK_SOURCE_PLL_60MHz);
#if (defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
#endif
#ifdef DEBUG
    GPIOA_SetBits(bTXD1);
    GPIOA_ModeCfg(bTXD1, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
#endif
    PRINT("start.\n");

    ret = LOS_KernelInit();
    taskSample();
    if (ret == LOS_OK)
    {
        LOS_Start();
    }

    PRINT("shouldn't run at here!!!\r\n");
    while (1) {
        __asm volatile("nop");
    }

    return 0;

}

__HIGH_CODE
void GPIOA_IRQHandler(void)
{
    /* 本函数可以作为在本工程LiteOS中的中断函数写法示例 */
    uint16_t flag;

    flag = GPIOA_ReadITFlagPort();
    if((flag & GPIO_Pin_12) != 0)
    {
        LOS_SemPost(g_semId);
    }
    GPIOA_ClearITFlagBit(flag); //清除中断标志
}

/******************************** endfile @ main ******************************/

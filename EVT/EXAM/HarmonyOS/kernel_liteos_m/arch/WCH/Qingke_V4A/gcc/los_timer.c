/*
 * Copyright (c) 2013-2020, Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd. All rights reserved.
 * Copyright (c) 2021 Nuclei Limited. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "los_timer.h"
#include "los_config.h"
#include "los_tick.h"
#include "los_reg.h"
#include "los_sched.h"
#include "core_riscv.h"
#include "los_arch_interrupt.h"
#include "los_arch_timer.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define configKERNEL_INTERRUPT_PRIORITY         0

//#define SYSTICK_TICK_CONST  (OS_SYS_CLOCK / LOSCFG_BASE_CORE_TICK_PER_SECOND)

static HWI_PROC_FUNC systick_handler;

extern UINT32 g_intCount;

__attribute__((interrupt("WCH-Interrupt-fast")))
__attribute__((section(".highcode")))
void SysTick_Handler(void)
{
    ArchIntEnter();
    /* Do systick handler registered in HalTickStart. */
    if ((void *)systick_handler != NULL)
    {
        systick_handler();
    }
    SysTick->SR = 0;
    ArchIntExit();
}

extern void SW_Handler(void);


STATIC UINT32 SysTickStart(HWI_PROC_FUNC handler);
STATIC UINT64 SysTickReload(UINT64 nextResponseTime);
STATIC UINT64 SysTickCycleGet(UINT32 *period);
STATIC VOID SysTickLock(VOID);
STATIC VOID SysTickUnlock(VOID);

STATIC ArchTickTimer g_archTickTimer =
{
    .freq = 0,
    .irqNum = SysTick_IRQn,
    .periodMax = LOSCFG_BASE_CORE_TICK_RESPONSE_MAX,
    .init = SysTickStart,
    .getCycle = SysTickCycleGet,
    .reload = SysTickReload,
    .lock = SysTickLock,
    .unlock = SysTickUnlock,
    .tickHandler = NULL,
};

STATIC UINT32 SysTickStart(HWI_PROC_FUNC handler)
{
    g_archTickTimer.freq = OS_SYS_CLOCK;
    g_cyclesPerTick = OS_SYS_CLOCK / LOSCFG_BASE_CORE_TICK_PER_SECOND;

    PFIC_EnableFastINT0(SWI_IRQn, (uint32_t)SW_Handler);                /* 提升任务切换速度，不从统一入口执行，在处理期间不可中断嵌套 */
    PFIC_EnableFastINT1(SysTick_IRQn, (uint32_t)SysTick_Handler);       /* 提升systick中断速度，不从统一入口执行，在处理期间不可中断嵌套 */

    PFIC_SetPriority(SysTick_IRQn, 0xf0);
    PFIC_SetPriority(SWI_IRQn, 0xf0);
    PFIC_EnableIRQ(SysTick_IRQn);
    PFIC_EnableIRQ(SWI_IRQn);

    systick_handler = handler;

    PRINT("HalTickStart\n");

    SysTick->CTLR = 0;
    SysTick->SR = 0;
    SysTick->CMP = g_cyclesPerTick - 1;
    SysTick->CNT = 0;
    SysTick->CTLR = 0xf;

    return LOS_OK;
}

STATIC UINT64 SysTickReload(UINT64 nextResponseTime)
{
    SysTick->CTLR &= ~(1 << 0);
    SysTick->CMP  = nextResponseTime - 1;
    SysTick->CNT  = 0;
    SysTick->SR  = 0;
    PFIC_ClearPendingIRQ(SysTick_IRQn);
    SysTick->CTLR |= (1 << 0);

    return nextResponseTime;
}

STATIC UINT64 SysTickCycleGet(UINT32 *period)
{
    UINT64 ticks;
    UINT32 intSave = LOS_IntLock();
    ticks = SysTick->CNT;
    *period = SysTick->CMP;

    LOS_IntRestore(intSave);
    return ticks;
}

STATIC VOID SysTickLock(VOID)
{
    SysTick->CTLR &= ~(1 << 0);
}

STATIC VOID SysTickUnlock(VOID)
{
    SysTick->CTLR |= (1 << 0);
}

ArchTickTimer *ArchSysTickTimerGet(VOID)
{
    return &g_archTickTimer;
}

UINT32 ArchEnterSleep(VOID)
{
    __WFI();

    return LOS_OK;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

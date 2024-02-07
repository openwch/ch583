/*
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd. All rights reserved.
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

#include "los_tick.h"
#include "securec.h"
#include "los_config.h"
#include "los_task.h"
#include "los_swtmr.h"
#include "los_sched.h"
#include "los_debug.h"
#include "stdint.h"

LITE_OS_SEC_BSS STATIC ArchTickTimer *g_sysTickTimer = NULL;
LITE_OS_SEC_BSS UINT32 g_ticksPerSec;
LITE_OS_SEC_BSS UINT32 g_uwCyclePerSec;
LITE_OS_SEC_BSS UINT32 g_cyclesPerTick;
LITE_OS_SEC_BSS UINT32 g_sysClock;
LITE_OS_SEC_BSS STATIC BOOL g_sysTimerIsInit = FALSE;
LITE_OS_SEC_BSS STATIC UINT64 g_tickTimerStartTime;

#if (LOSCFG_BASE_CORE_TICK_WTIMER == 0)
STATIC UINT64 g_tickTimerBase;
STATIC UINT64 g_oldTickTimerBase;
STATIC BOOL g_tickTimerBaseUpdate = FALSE;

LITE_OS_SEC_TEXT STATIC VOID OsUpdateSysTimeBase(VOID)
{
    UINT32 period = 0;

    if (g_tickTimerBaseUpdate == FALSE) {
        (VOID)g_sysTickTimer->getCycle(&period);
        g_tickTimerBase += period;
    }
    g_tickTimerBaseUpdate = FALSE;
}

LITE_OS_SEC_TEXT VOID OsTickTimerBaseReset(UINT64 currTime)
{
    LOS_ASSERT(currTime > g_tickTimerBase);

    g_tickTimerBase = currTime;
}
#endif

LITE_OS_SEC_TEXT VOID OsTickHandler(VOID)
{
#if (LOSCFG_BASE_CORE_TICK_WTIMER == 0)
    OsUpdateSysTimeBase();
#endif

    LOS_SchedTickHandler();
}

LITE_OS_SEC_TEXT UINT64 OsTickTimerReload(UINT64 period)
{
#if (LOSCFG_BASE_CORE_TICK_WTIMER == 0)
    g_tickTimerBase = LOS_SysCycleGet();
#endif
    return g_sysTickTimer->reload(period);
}

LITE_OS_SEC_TEXT UINT64 LOS_SysCycleGet(VOID)
{
#if (LOSCFG_BASE_CORE_TICK_WTIMER == 1)
    return g_sysTickTimer->getCycle(NULL);
#else
    UINT32 period = 0;
    UINT32 intSave = LOS_IntLock();
    UINT64 time = g_sysTickTimer->getCycle(&period);
    UINT64 schedTime = g_tickTimerBase + time;
    if (schedTime < g_oldTickTimerBase) {
        /* Turn the timer count */
        g_tickTimerBase += period;
        schedTime = g_tickTimerBase + time;
        g_tickTimerBaseUpdate = TRUE;
    }

    LOS_ASSERT(schedTime >= g_oldTickTimerBase);

    g_oldTickTimerBase = schedTime;
    LOS_IntRestore(intSave);
    return schedTime;
#endif
}

STATIC UINT32 TickTimerCheck(const ArchTickTimer *tick)
{
    if (tick == NULL) {
        return LOS_ERRNO_SYS_PTR_NULL;
    }

    if ((tick->freq == 0) ||
        (LOSCFG_BASE_CORE_TICK_PER_SECOND == 0) ||
        (LOSCFG_BASE_CORE_TICK_PER_SECOND > tick->freq)) {
        return LOS_ERRNO_SYS_CLOCK_INVALID;
    }

    if (tick->irqNum > (INT32)LOSCFG_PLATFORM_HWI_LIMIT) {
        return LOS_ERRNO_TICK_CFG_INVALID;
    }

    if (tick->periodMax == 0) {
        return LOS_ERRNO_TICK_CFG_INVALID;
    }

    if ((tick->init == NULL) || (tick->reload == NULL) ||
        (tick->lock == NULL) || (tick->unlock == NULL) ||
        (tick->getCycle == NULL)) {
        return LOS_ERRNO_SYS_HOOK_IS_NULL;
    }

    if (g_sysTimerIsInit) {
        return LOS_ERRNO_SYS_TIMER_IS_RUNNING;
    }

    return LOS_OK;
}

LITE_OS_SEC_TEXT_INIT UINT32 OsTickTimerInit(VOID)
{
    UINT32 ret;
    UINT32 intSave;
    HWI_PROC_FUNC tickHandler = (HWI_PROC_FUNC)OsTickHandler;

    g_sysTickTimer = LOS_SysTickTimerGet();
    if ((g_sysTickTimer->init == NULL) || (g_sysTickTimer->reload == NULL) ||
        (g_sysTickTimer->lock == NULL) || (g_sysTickTimer->unlock == NULL) ||
        (g_sysTickTimer->getCycle == NULL)) {
        return LOS_ERRNO_SYS_HOOK_IS_NULL;
    }

    if (g_sysTickTimer->tickHandler != NULL) {
        tickHandler = g_sysTickTimer->tickHandler;
    }

    intSave = LOS_IntLock();
    ret = g_sysTickTimer->init(tickHandler);
    if (ret != LOS_OK) {
        LOS_IntRestore(intSave);
        return ret;
    }

    if ((g_sysTickTimer->freq == 0) || (g_sysTickTimer->freq < LOSCFG_BASE_CORE_TICK_PER_SECOND)) {
        LOS_IntRestore(intSave);
        return LOS_ERRNO_SYS_CLOCK_INVALID;
    }

    if (g_sysTickTimer->irqNum > (INT32)LOSCFG_PLATFORM_HWI_LIMIT) {
        LOS_IntRestore(intSave);
        return LOS_ERRNO_TICK_CFG_INVALID;
    }

    g_sysClock = g_sysTickTimer->freq;
    g_cyclesPerTick = g_sysTickTimer->freq / LOSCFG_BASE_CORE_TICK_PER_SECOND;
    g_sysTimerIsInit = TRUE;

    LOS_IntRestore(intSave);

    return LOS_OK;
}

LITE_OS_SEC_TEXT UINT32 LOS_TickTimerRegister(const ArchTickTimer *timer, const HWI_PROC_FUNC tickHandler)
{
    UINT32 intSave;
    UINT32 ret;

    if ((timer == NULL) && (tickHandler == NULL)) {
        return LOS_ERRNO_SYS_PTR_NULL;
    }

    if (timer != NULL) {
        ret = TickTimerCheck(timer);
        if (ret != LOS_OK) {
            return ret;
        }

        intSave = LOS_IntLock();
        if (g_sysTickTimer == NULL) {
            g_sysTickTimer = LOS_SysTickTimerGet();
        }

        if (g_sysTickTimer == timer) {
            LOS_IntRestore(intSave);
            return LOS_ERRNO_SYS_TIMER_ADDR_FAULT;
        }

        errno_t errRet = memcpy_s(g_sysTickTimer, sizeof(ArchTickTimer), timer, sizeof(ArchTickTimer));
        if (errRet != EOK) {
            PRINT_ERR("%s timer addr fault! errno %d\n", __FUNCTION__, errRet);
            ret = LOS_ERRNO_SYS_TIMER_ADDR_FAULT;
        }
        LOS_IntRestore(intSave);
        return ret;
    }

    if (g_sysTimerIsInit) {
        return LOS_ERRNO_SYS_TIMER_IS_RUNNING;
    }

    intSave = LOS_IntLock();
    if (g_sysTickTimer == NULL) {
        g_sysTickTimer = LOS_SysTickTimerGet();
    }

    g_sysTickTimer->tickHandler = tickHandler;
    LOS_IntRestore(intSave);
    return LOS_OK;
}

UINT32 LOS_SysTickClockFreqAdjust(const SYS_TICK_FREQ_ADJUST_FUNC handler, UINTPTR param)
{
    UINT32 intSave;
    UINT32 freq;
    UINT32 oldFreq = g_sysClock;

    if (handler == NULL) {
        return LOS_ERRNO_SYS_HOOK_IS_NULL;
    }

    intSave = LOS_IntLock();
    g_sysTickTimer->lock();
#if (LOSCFG_BASE_CORE_TICK_WTIMER == 0)
    UINT64 currTimeCycle = LOS_SysCycleGet();
#endif

    freq = handler(param);
    if ((freq == 0) || (freq == g_sysClock)) {
        g_sysTickTimer->unlock();
        LOS_IntRestore(intSave);
        return LOS_ERRNO_SYS_CLOCK_INVALID;
    }

    g_sysTickTimer->reload(LOSCFG_BASE_CORE_TICK_RESPONSE_MAX);
    g_sysTickTimer->unlock();

#if (LOSCFG_BASE_CORE_TICK_WTIMER == 0)
    g_tickTimerBase = OsTimeConvertFreq(currTimeCycle, oldFreq, freq);
    g_oldTickTimerBase = OsTimeConvertFreq(g_oldTickTimerBase, oldFreq, freq);
    g_tickTimerStartTime = OsTimeConvertFreq(g_tickTimerStartTime, oldFreq, freq);
#endif

    g_sysTickTimer->freq = freq;
    g_sysClock = g_sysTickTimer->freq;
    g_cyclesPerTick = g_sysTickTimer->freq / LOSCFG_BASE_CORE_TICK_PER_SECOND;
    OsSchedTimeConvertFreq(oldFreq);
    LOS_IntRestore(intSave);

    return LOS_OK;
}

LITE_OS_SEC_TEXT_MINOR VOID OsTickSysTimerStartTimeSet(UINT64 currTime)
{
    g_tickTimerStartTime = currTime;
}

/*****************************************************************************
Function    : LOS_TickCountGet
Description : get current tick
Input       : None
Output      : None
Return      : current tick
*****************************************************************************/
LITE_OS_SEC_TEXT_MINOR UINT64 LOS_TickCountGet(VOID)
{
    return OS_SYS_CYCLE_TO_TICK(LOS_SysCycleGet() - g_tickTimerStartTime);
}

/*****************************************************************************
Function    : LOS_CyclePerTickGet
Description : Get System cycle number corresponding to each tick
Input       : None
Output      : None
Return      : cycle number corresponding to each tick
*****************************************************************************/
LITE_OS_SEC_TEXT_MINOR UINT32 LOS_CyclePerTickGet(VOID)
{
    return g_cyclesPerTick;
}

/*****************************************************************************
Function    : LOS_MS2Tick
Description : milliseconds convert to Tick
Input       : millisec ---------- milliseconds
Output      : None
Return      : Tick
*****************************************************************************/
LITE_OS_SEC_TEXT_MINOR UINT32 LOS_MS2Tick(UINT32 millisec)
{
    if (millisec == OS_NULL_INT) {
        return OS_NULL_INT;
    }

    return ((UINT64)millisec * LOSCFG_BASE_CORE_TICK_PER_SECOND) / OS_SYS_MS_PER_SECOND;
}

/*****************************************************************************
Function    : LOS_Tick2MS
Description : Tick convert to milliseconds
Input       : ticks ---------- ticks
Output      : None
Return      : milliseconds
*****************************************************************************/
LITE_OS_SEC_TEXT_MINOR UINT32 LOS_Tick2MS(UINT32 ticks)
{
    return ((UINT64)ticks * OS_SYS_MS_PER_SECOND) / LOSCFG_BASE_CORE_TICK_PER_SECOND;
}

/*****************************************************************************
Function    : OsCpuTick2MS
Description : cycle convert to milliseconds
Input       : cpuTick ---------- cycle
Output      : msHi    ---------- High 32 milliseconds
              msLo    ---------- Low 32 milliseconds
Return      : LOS_OK on success ,or error code on failure
*****************************************************************************/
LITE_OS_SEC_TEXT_INIT UINT32 OsCpuTick2MS(CpuTick *cpuTick, UINT32 *msHi, UINT32 *msLo)
{
    UINT64 tmpCpuTick;
    DOUBLE temp;

    if ((cpuTick == NULL) || (msHi == NULL) || (msLo == NULL)) {
        return LOS_ERRNO_SYS_PTR_NULL;
    }

    if (g_sysClock == 0) {
        return LOS_ERRNO_SYS_CLOCK_INVALID;
    }
    tmpCpuTick = ((UINT64)cpuTick->cntHi << OS_SYS_MV_32_BIT) | cpuTick->cntLo;
    temp = tmpCpuTick / ((DOUBLE)g_sysClock / OS_SYS_MS_PER_SECOND);

    tmpCpuTick = (UINT64)temp;

    *msLo = (UINT32)tmpCpuTick;
    *msHi = (UINT32)(tmpCpuTick >> OS_SYS_MV_32_BIT);

    return LOS_OK;
}

/*****************************************************************************
Function    : OsCpuTick2US
Description : cycle convert to Microsecond
Input       : cpuTick ---------- cycle
Output      : usHi    ---------- High 32 Microsecond
              usLo    ---------- Low 32 Microsecond
Return      : LOS_OK on success ,or error code on failure
*****************************************************************************/
LITE_OS_SEC_TEXT_INIT UINT32 OsCpuTick2US(CpuTick *cpuTick, UINT32 *usHi, UINT32 *usLo)
{
    UINT64 tmpCpuTick;
    DOUBLE temp;

    if ((cpuTick == NULL) || (usHi == NULL) || (usLo == NULL)) {
        return LOS_ERRNO_SYS_PTR_NULL;
    }

    if (g_sysClock == 0) {
        return LOS_ERRNO_SYS_CLOCK_INVALID;
    }
    tmpCpuTick = ((UINT64)cpuTick->cntHi << OS_SYS_MV_32_BIT) | cpuTick->cntLo;
    temp = tmpCpuTick / ((DOUBLE)g_sysClock / OS_SYS_US_PER_SECOND);

    tmpCpuTick = (UINT64)temp;

    *usLo = (UINT32)tmpCpuTick;
    *usHi = (UINT32)(tmpCpuTick >> OS_SYS_MV_32_BIT);

    return LOS_OK;
}

/*****************************************************************************
Function    : LOS_MS2Tick
Description : get current nanoseconds
Input       : None
Output      : None
Return      : nanoseconds
*****************************************************************************/
UINT64 LOS_CurrNanosec(VOID)
{
    UINT64 nanos;
    nanos = LOS_SysCycleGet() * (OS_SYS_NS_PER_SECOND / OS_SYS_NS_PER_MS) / (g_sysClock / OS_SYS_NS_PER_MS);
    return nanos;
}

/*****************************************************************************
Function    : LOS_UDelay
Description : cpu delay function
Input       : microseconds ---------- microseconds
Output      : None
Return      : None
*****************************************************************************/
VOID LOS_UDelay(UINT64 microseconds)
{
    UINT64 endTime;

    if (microseconds == 0) {
        return;
    }

    endTime = (microseconds / OS_SYS_US_PER_SECOND) * g_sysClock +
            (microseconds % OS_SYS_US_PER_SECOND) * g_sysClock / OS_SYS_US_PER_SECOND;
    endTime = LOS_SysCycleGet() + endTime;
    while (LOS_SysCycleGet() < endTime) {
    }
    return;
}

/*****************************************************************************
Function    : LOS_MDelay
Description : cpu delay function
Input       : millisec ---------- milliseconds
Output      : None
Return      : None
*****************************************************************************/
VOID LOS_MDelay(UINT32 millisec)
{
    UINT32 delayUs = (UINT32_MAX / OS_SYS_US_PER_MS) * OS_SYS_US_PER_MS;
    while (millisec > UINT32_MAX / OS_SYS_US_PER_MS) {
        LOS_UDelay(delayUs);
        millisec -= (UINT32_MAX / OS_SYS_US_PER_MS);
    }
    LOS_UDelay(millisec * OS_SYS_US_PER_MS);
    return;
}

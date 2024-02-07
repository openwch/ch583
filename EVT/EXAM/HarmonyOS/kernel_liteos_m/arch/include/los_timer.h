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

#ifndef _LOS_TIMER_H
#define _LOS_TIMER_H

#include "los_compiler.h"
#include "los_interrupt.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define TICK_CHECK                           0x4000000
#define CYCLE_CHECK                          0xFFFFFFFFU
#define SHIFT_32_BIT                          32
#define MAX_HOUR 24
#define MAX_MINUTES 60
#define MAX_SECONDS 60
#define MILSEC 1000
#define RTC_WAKEUPCLOCK_RTCCLK 32768
#define RTC_WAKEUPCLOCK_RTCCLK_DIV 16
#define RTC_CALIBRATE_SLEEP_TIME 8
#define MACHINE_CYCLE_DEALAY_TIMES (LOSCFG_BASE_CORE_TICK_PER_SECOND << 2)

typedef struct {
    UINT32        freq;
    INT32         irqNum;
    UINT64        periodMax;
    UINT32        (*init)(HWI_PROC_FUNC tickHandler);
    UINT64        (*getCycle)(UINT32 *period);
    UINT64        (*reload)(UINT64 time);
    VOID          (*lock)(VOID);
    VOID          (*unlock)(VOID);
    HWI_PROC_FUNC tickHandler;
} ArchTickTimer;

UINT32 ArchEnterSleep(VOID);

/**
 * @ingroup los_timer
 * @brief Get tick timer control block.
 *
 * @par Description:
 * This API is used to get tick timer control block.
 *
 * @param  None
 *
 * @retval #tick timer control block
 * @par Dependency:
 * <ul><li>los_timer.h: the header file that contains the API declaration.</li></ul>
 * @see None.
 */
ArchTickTimer *ArchSysTickTimerGet(VOID);

#define LOS_SysTickTimerGet ArchSysTickTimerGet

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _LOS_TIMER_H */

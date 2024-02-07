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

/**@defgroup los_debug
 * @ingroup kernel
 */

#ifndef _LOS_DEBUG_H
#define _LOS_DEBUG_H

#include "los_config.h"
#include "los_compiler.h"
#include "stdio.h"
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#if (LOSCFG_PLATFORM_EXC == 1)
enum MemMangType {
    MEM_MANG_MEMBOX,
    MEM_MANG_MEMORY,
    MEM_MANG_EMPTY
};

typedef struct {
    UINT32 type;
    UINT32 startAddr;
    UINT32 size;
    VOID *blkAddrArray;
} MemInfo;

typedef struct {
    enum MemMangType type;
    UINT32 startAddr;
    UINT32 size;
    UINT32 free;
    UINT32 blockSize;
    UINT32 errorAddr;
    UINT32 errorLen;
    UINT32 errorOwner;
} MemInfoCB;
#endif

typedef enum {
    EXC_REBOOT,
    EXC_ASSERT,
    EXC_PANIC,
    EXC_STACKOVERFLOW,
    EXC_INTERRUPT,
    EXC_TYPE_END
} EXC_TYPE;

typedef VOID (*ExcHookFn)(EXC_TYPE excType);

VOID OsExcHookRegister(ExcHookFn excHookFn);

VOID OsDoExcHook(EXC_TYPE excType);

#define LOG_EMG_LEVEL       0

#define LOG_COMMON_LEVEL    (LOG_EMG_LEVEL + 1)

#define LOG_ERR_LEVEL       (LOG_COMMON_LEVEL + 1)

#define LOG_WARN_LEVEL      (LOG_ERR_LEVEL + 1)

#define LOG_INFO_LEVEL      (LOG_WARN_LEVEL + 1)

#define LOG_DEBUG_LEVEL     (LOG_INFO_LEVEL + 1)

#ifndef PRINT_LEVEL
#define PRINT_LEVEL         LOG_ERR_LEVEL
#endif

typedef enum {
    LOG_MODULE_KERNEL,
    LOG_MODULE_FS,
    LOS_MODULE_OTHERS
} LogModuleType;

/**
 * @ingroup los_printf
 * @brief Format and print data.
 *
 * @par Description:
 * Print argument(s) according to fmt.
 *
 * @attention
 * <ul>
 * <li>None</li>
 * </ul>
 *
 * @param type  [IN] Type LogModuleType indicates the log type.
 * @param level [IN] Type LogLevel indicates the log level.
 * @param fmt   [IN] Type char* controls the output as in C printf.
 *
 * @retval None
 * @par Dependency:
 * <ul><li>los_printf.h: the header file that contains the API declaration.</li></ul>
 * @see LOS_Printf
 */
#if (LOSCFG_KERNEL_PRINTF == 1)
extern INT32 OsLogLevelCheck(INT32 level);
#define LOS_Printf(type, level, fmt, args...)   do { \
    if (!OsLogLevelCheck(level)) {                   \
        PRINT(fmt, ##args);                         \
    }                                                \
} while (0)
#elif (LOSCFG_KERNEL_PRINTF == 0)
#define LOS_Printf(type, level, fmt, args...)
#else
extern VOID HalConsoleOutput(LogModuleType type, INT32 level, const CHAR *fmt, ...);
#define LOS_Printf HalConsoleOutput
#endif

#define PRINT_DEBUG(fmt, args...)    LOS_Printf(LOG_MODULE_KERNEL, LOG_DEBUG_LEVEL, fmt, ##args)
#define PRINT_INFO(fmt, args...)     LOS_Printf(LOG_MODULE_KERNEL, LOG_INFO_LEVEL, fmt, ##args)
#define PRINT_WARN(fmt, args...)     LOS_Printf(LOG_MODULE_KERNEL, LOG_WARN_LEVEL, fmt, ##args)
#define PRINT_ERR(fmt, args...)      LOS_Printf(LOG_MODULE_KERNEL, LOG_ERR_LEVEL, fmt, ##args)
#define PRINTK(fmt, args...)         LOS_Printf(LOG_MODULE_KERNEL, LOG_COMMON_LEVEL, fmt, ##args)
#define PRINT_EMG(fmt, args...)      LOS_Printf(LOG_MODULE_KERNEL, LOG_EMG_LEVEL, fmt, ##args)

#if PRINT_LEVEL < LOG_ERR_LEVEL
#define LOS_ASSERT(judge)
#else
#define LOS_ASSERT(judge)                                                          \
    do {                                                                           \
        if ((judge) == 0) {                                                        \
            OsDoExcHook(EXC_ASSERT);                                               \
            (VOID)LOS_IntLock();                                                   \
            PRINT_ERR("ASSERT ERROR! %s, %d, %s\n", __FILE__, __LINE__, __func__); \
            while (1) { }                                                          \
        }                                                                          \
    } while (0)
#endif

typedef VOID (*BACK_TRACE_HOOK)(UINTPTR *LR, UINT32 LRSize, UINT32 jumpCount, UINTPTR SP);
extern VOID OsBackTraceHookSet(BACK_TRACE_HOOK hook);
extern VOID OsBackTraceHookCall(UINTPTR *LR, UINT32 LRSize, UINT32 jumpCount, UINTPTR SP);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _LOS_PRINTF_H */

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

/**
 * @defgroup los_config System configuration items
 * @ingroup kernel
 */

#ifndef _LOS_CONFIG_H
#define _LOS_CONFIG_H

#include "target_config.h"
#include "los_compiler.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/* =============================================================================
                                        System clock module configuration
============================================================================= */
/**
 * @ingroup los_config
 * System clock (unit: HZ)
 */
#ifndef OS_SYS_CLOCK
#define OS_SYS_CLOCK 1000000
#endif

/**
 * @ingroup los_config
 * Number of Ticks in one second
 */
#ifndef LOSCFG_BASE_CORE_TICK_PER_SECOND
#define LOSCFG_BASE_CORE_TICK_PER_SECOND                    (100UL)
#endif

/**
 * @ingroup los_config
 * Minimum response error accuracy of tick interrupts, number of ticks in one second.
 */
#ifndef LOSCFG_BASE_CORE_TICK_PER_SECOND_MINI
#define LOSCFG_BASE_CORE_TICK_PER_SECOND_MINI               (1000UL) /* 1ms */
#endif

#if (LOSCFG_BASE_CORE_TICK_PER_SECOND > LOSCFG_BASE_CORE_TICK_PER_SECOND_MINI)
    #error "LOSCFG_BASE_CORE_TICK_PER_SECOND_MINI must be greater than LOSCFG_BASE_CORE_TICK_PER_SECOND"
#endif

#if (LOSCFG_BASE_CORE_TICK_PER_SECOND_MINI > 1000UL)
    #error "LOSCFG_BASE_CORE_TICK_PER_SECOND_MINI must be less than or equal to 1000"
#endif

#if defined(LOSCFG_BASE_CORE_TICK_PER_SECOND) && \
    ((LOSCFG_BASE_CORE_TICK_PER_SECOND < 1UL) || (LOSCFG_BASE_CORE_TICK_PER_SECOND > 1000000000UL))
    #error "LOSCFG_BASE_CORE_TICK_PER_SECOND SHOULD big than 0, and less than 1000000000UL"
#endif


#if (LOSCFG_BASE_CORE_TICK_PER_SECOND <= 1000UL)
/**
 * @ingroup los_config
 * How much time one tick spent (unit:ms)
 */
#ifndef LOSCFG_BASE_CORE_TICK_PERIOD_MS
#define LOSCFG_BASE_CORE_TICK_PERIOD_MS                     (1000UL / LOSCFG_BASE_CORE_TICK_PER_SECOND)
#endif

#elif (LOSCFG_BASE_CORE_TICK_PER_SECOND <= 1000000UL)
/**
 * @ingroup los_config
 * How much time one tick spent (unit:us)
 */
#ifndef LOSCFG_BASE_CORE_TICK_PERIOD_US
#define LOSCFG_BASE_CORE_TICK_PERIOD_US                     (1000000UL / LOSCFG_BASE_CORE_TICK_PER_SECOND)
#endif

#else
/**
 * @ingroup los_config
 * How much time one tick spent (unit:ns)
 */
#ifndef LOSCFG_BASE_CORE_TICK_PERIOD_NS
#define LOSCFG_BASE_CORE_TICK_PERIOD_NS                     (1000000000UL / LOSCFG_BASE_CORE_TICK_PER_SECOND)
#endif
#endif

/**
 * @ingroup los_config
 * System timer is a 64/128 bit timer
 */
#ifndef LOSCFG_BASE_CORE_TICK_WTIMER
#define LOSCFG_BASE_CORE_TICK_WTIMER                        0
#endif

/**
 * @ingroup los_config
 * System timer count maximum
 */
#ifndef LOSCFG_BASE_CORE_TICK_RESPONSE_MAX
#define LOSCFG_BASE_CORE_TICK_RESPONSE_MAX                       0
#endif

/* =============================================================================
                                        Hardware interrupt module configuration
============================================================================= */
/**
 * @ingroup los_config
 * Configuration item for hardware interrupt tailoring
 */
#ifndef LOSCFG_PLATFORM_HWI
#define LOSCFG_PLATFORM_HWI                                 1
#endif

/**
 * @ingroup los_config
 * Configuration item for using system defined vector base address and interrupt handlers.
 * If LOSCFG_USE_SYSTEM_DEFINED_INTERRUPT is set to 0, vector base address will not be
 * modified by system. In arm, it should be noted that PendSV_Handler and SysTick_Handler should
 * be redefined to HalPendSV and OsTickHandler respectly in this case, because system depends on
 * these interrupt handlers to run normally. What's more, LOS_HwiCreate will not register handler.
 */
#ifndef LOSCFG_USE_SYSTEM_DEFINED_INTERRUPT
#define LOSCFG_USE_SYSTEM_DEFINED_INTERRUPT                 1
#endif

#if (LOSCFG_USE_SYSTEM_DEFINED_INTERRUPT == 1)
    #if (LOSCFG_PLATFORM_HWI == 0)
        #error "if LOSCFG_USE_SYSTEM_DEFINED_INTERRUPT is set to 1, then LOSCFG_PLATFORM_HWI must also be set to 1"
    #endif
#endif

/**
 * @ingroup los_config
 * Maximum number of used hardware interrupts, including Tick timer interrupts.
 */
#ifndef LOSCFG_PLATFORM_HWI_LIMIT
#define LOSCFG_PLATFORM_HWI_LIMIT                           32
#endif

/* =============================================================================
                                       Task module configuration
============================================================================= */
/**
 * @ingroup los_config
 * Minimum stack size.
 *
 * 0x80 bytes, aligned on a boundary of 8.
 */
#ifndef LOSCFG_BASE_CORE_TSK_MIN_STACK_SIZE
#define LOSCFG_BASE_CORE_TSK_MIN_STACK_SIZE                 (ALIGN(0x80, 4))
#endif

/**
 * @ingroup los_config
 * Default task priority
 */
#ifndef LOSCFG_BASE_CORE_TSK_DEFAULT_PRIO
#define LOSCFG_BASE_CORE_TSK_DEFAULT_PRIO                   10
#endif

/**
 * @ingroup los_config
 * Maximum supported number of tasks except the idle task rather than the number of usable tasks
 */
#ifndef LOSCFG_BASE_CORE_TSK_LIMIT
#define LOSCFG_BASE_CORE_TSK_LIMIT                          5
#endif

/**
 * @ingroup los_config
 * Size of the idle task stack
 */
#ifndef LOSCFG_BASE_CORE_TSK_IDLE_STACK_SIZE
#define LOSCFG_BASE_CORE_TSK_IDLE_STACK_SIZE                0x180UL
#endif

/**
 * @ingroup los_config
 * Default task stack size
 */
#ifndef LOSCFG_BASE_CORE_TSK_DEFAULT_STACK_SIZE
#define LOSCFG_BASE_CORE_TSK_DEFAULT_STACK_SIZE             0x400UL
#endif

/**
 * @ingroup los_config
 * Configuration item for task Robin tailoring
 */
#ifndef LOSCFG_BASE_CORE_TIMESLICE
#define LOSCFG_BASE_CORE_TIMESLICE                         1
#endif

/**
 * @ingroup los_config
 * Longest execution time of tasks with the same priorities
 */
#ifndef LOSCFG_BASE_CORE_TIMESLICE_TIMEOUT
#define LOSCFG_BASE_CORE_TIMESLICE_TIMEOUT                  20000 /* 20ms */
#endif

/**
 * @ingroup los_config
 * Configuration item for task (stack) monitoring module tailoring
 */
#ifndef LOSCFG_BASE_CORE_TSK_MONITOR
#define LOSCFG_BASE_CORE_TSK_MONITOR                        0
#endif

/**
 * @ingroup los_config
 * Configuration item for task perf task filter hook
 */
#ifndef LOSCFG_BASE_CORE_EXC_TSK_SWITCH
#define LOSCFG_BASE_CORE_EXC_TSK_SWITCH                     0
#endif

/*
 * @ingroup los_config
 * Configuration item for task context switch hook
 */
#ifndef LOSCFG_BASE_CORE_TSK_SWITCH_HOOK
#define LOSCFG_BASE_CORE_TSK_SWITCH_HOOK()
#endif

/**
 * @ingroup los_config
 * Define a usable task priority.Highest task priority.
 */
#ifndef LOS_TASK_PRIORITY_HIGHEST
#define LOS_TASK_PRIORITY_HIGHEST                           0
#endif

/**
 * @ingroup los_config
 * Define a usable task priority.Lowest task priority.
 */
#ifndef LOS_TASK_PRIORITY_LOWEST
#define LOS_TASK_PRIORITY_LOWEST                            31
#endif

/**
 * @ingroup los_config
 * Configuration item for task stack independent
 */
#ifndef LOSCFG_BASE_CORE_TASKSTACK_INDEPENDENT
#define LOSCFG_BASE_CORE_TASKSTACK_INDEPENDENT              0
#endif

/**
 * @ingroup los_config
 * SP align size
 */
#ifndef LOSCFG_STACK_POINT_ALIGN_SIZE
#define LOSCFG_STACK_POINT_ALIGN_SIZE                       8
#endif

/* =============================================================================
                                       Semaphore module configuration
============================================================================= */
/**
 * @ingroup los_config
 * Configuration item for semaphore module tailoring
 */
#ifndef LOSCFG_BASE_IPC_SEM
#define LOSCFG_BASE_IPC_SEM                                 1
#endif

/**
 * @ingroup los_config
 * Maximum supported number of semaphores
 */
#ifndef LOSCFG_BASE_IPC_SEM_LIMIT
#define LOSCFG_BASE_IPC_SEM_LIMIT                           6
#endif

/**
 * @ingroup los_config
 * Maximum number of semaphores.
 */
#ifndef OS_SEM_COUNTING_MAX_COUNT
#define OS_SEM_COUNTING_MAX_COUNT                           0xFFFF
#endif

/* =============================================================================
                                       Mutex module configuration
============================================================================= */
/**
 * @ingroup los_config
 * Configuration item for mutex module tailoring
 */
#ifndef LOSCFG_BASE_IPC_MUX
#define LOSCFG_BASE_IPC_MUX                                 1
#endif

/**
 * @ingroup los_config
 * Maximum supported number of mutexes
 */
#ifndef LOSCFG_BASE_IPC_MUX_LIMIT
#define LOSCFG_BASE_IPC_MUX_LIMIT                           6
#endif

/* =============================================================================
                                       Queue module configuration
============================================================================= */
/**
 * @ingroup los_config
 * Configuration item for queue module tailoring
 */
#ifndef LOSCFG_BASE_IPC_QUEUE
#define LOSCFG_BASE_IPC_QUEUE                               1
#endif

/**
 * @ingroup los_config
 * Maximum supported number of queues rather than the number of usable queues
 */
#ifndef LOSCFG_BASE_IPC_QUEUE_LIMIT
#define LOSCFG_BASE_IPC_QUEUE_LIMIT                         6
#endif

/**
 * @ingroup los_config
 * Maximum supported number of static queues rather than the number of usable queues
 */
#ifndef LOSCFG_BASE_IPC_STATIC_QUEUE_LIMIT
#define LOSCFG_BASE_IPC_STATIC_QUEUE_LIMIT                  3
#endif


/* =============================================================================
                                       Software timer module configuration
============================================================================= */
/**
 * @ingroup los_config
 * Configuration item for software timer module tailoring
 */
#ifndef LOSCFG_BASE_CORE_SWTMR
#define LOSCFG_BASE_CORE_SWTMR                              1
#endif

/**
 * @ingroup los_config
 * Maximum supported number of software timers rather than the number of usable software timers
 */
#ifndef LOSCFG_BASE_CORE_SWTMR_LIMIT
#define LOSCFG_BASE_CORE_SWTMR_LIMIT                        5
#endif

/**
 * @ingroup los_config
 * Software timer task stack size
 */
#ifndef LOSCFG_BASE_CORE_TSK_SWTMR_STACK_SIZE
#define LOSCFG_BASE_CORE_TSK_SWTMR_STACK_SIZE               LOSCFG_BASE_CORE_TSK_DEFAULT_STACK_SIZE
#endif

/**
 * @ingroup los_config
 * Configurate item for software timer align tailoring
 */
#ifndef LOSCFG_BASE_CORE_SWTMR_ALIGN
#define LOSCFG_BASE_CORE_SWTMR_ALIGN                        0
#endif

#if (LOSCFG_BASE_CORE_SWTMR_ALIGN == 1)
    #if (LOSCFG_BASE_CORE_SWTMR == 0)
        #error "if LOSCFG_BASE_CORE_SWTMR_ALIGN is set to 1, then LOSCFG_BASE_CORE_SWTMR must also be set to 1"
    #endif
#endif

/**
 * @ingroup los_config
 * Maximum size of a software timer queue
 */
#ifndef OS_SWTMR_HANDLE_QUEUE_SIZE
#define OS_SWTMR_HANDLE_QUEUE_SIZE                          (LOSCFG_BASE_CORE_SWTMR_LIMIT + 0)
#endif

/**
 * @ingroup los_config
 * Minimum divisor of software timer multiple alignment
 */
#ifndef LOS_COMMON_DIVISOR
#define LOS_COMMON_DIVISOR                                  10
#endif

#if (LOSCFG_BASE_CORE_SWTMR == 1)
    #if (LOSCFG_BASE_IPC_QUEUE == 0)
        #error "if LOSCFG_BASE_CORE_SWTMR is set to 1, then LOSCFG_BASE_IPC_QUEUE must also be set to 1"
    #endif
#endif
/* =============================================================================
                                       Memory module configuration ---- to be refactored
============================================================================= */
extern UINT8 *m_aucSysMem0;

/**
 * @ingroup los_config
 * Configure whether the kernel uses external heap memory
 */
#ifndef LOSCFG_SYS_EXTERNAL_HEAP
#define LOSCFG_SYS_EXTERNAL_HEAP                            0
#endif

/**
 * @ingroup los_config
 * Starting address of the memory
 */
#ifndef LOSCFG_SYS_HEAP_ADDR
#define LOSCFG_SYS_HEAP_ADDR                                (&m_aucSysMem0[0])
#endif

/**
 * @ingroup los_config
 * Starting address of the task stack
 */
#ifndef OS_TASK_STACK_ADDR
#define OS_TASK_STACK_ADDR                                  LOSCFG_SYS_HEAP_ADDR
#endif

/**
 * @ingroup los_config
 * Memory size
 */
#ifndef LOSCFG_SYS_HEAP_SIZE
#define LOSCFG_SYS_HEAP_SIZE                                0x10000UL
#endif

/**
 * @ingroup los_config
 * Configuration module tailoring of more memory pool checking
 */
#ifndef LOSCFG_MEM_MUL_POOL
#define LOSCFG_MEM_MUL_POOL                                 1
#endif

/**
 * @ingroup los_config
 * Configuration module tailoring of memory released by task id
 */
#ifndef LOSCFG_MEM_FREE_BY_TASKID
#define LOSCFG_MEM_FREE_BY_TASKID                           0
#endif

/**
 * @ingroup los_config
 * Configuration module tailoring of mem node integrity checking
 */
#ifndef LOSCFG_BASE_MEM_NODE_INTEGRITY_CHECK
#define LOSCFG_BASE_MEM_NODE_INTEGRITY_CHECK                0
#endif

/**
 * @ingroup los_config
 * The default is 4, which means that the function call stack is recorded from the kernel interface,
 * such as LOS_MemAlloc/LOS_MemAllocAlign/LOS_MemRealloc/LOS_MemFree. If you want to further ignore
 * the number of function call layers, you can increase this value appropriately.
 * @attention
 * The default is in the IAR tool. Under different compilation environments, this value needs to be adjusted.
 */
#ifndef LOSCFG_MEM_OMIT_LR_CNT
#define LOSCFG_MEM_OMIT_LR_CNT                              4
#endif

/**
 * @ingroup los_config
 * The record number of layers of the function call relationship starting from the number of
 * ignored layers(LOSCFG_MEM_OMIT_LR_CNT).
 */
#ifndef LOSCFG_MEM_RECORD_LR_CNT
#define LOSCFG_MEM_RECORD_LR_CNT                            3
#endif

/**
 * @ingroup los_config
 * Configuration memory leak recorded num
 */
#ifndef LOSCFG_MEM_LEAKCHECK_RECORD_MAX_NUM
#define LOSCFG_MEM_LEAKCHECK_RECORD_MAX_NUM                 1024
#endif

/**
 * @ingroup los_config
 * Configuration of memory pool record memory consumption waterline
 */
#ifndef LOSCFG_MEM_WATERLINE
#define LOSCFG_MEM_WATERLINE                                1
#endif

/**
 * @ingroup los_config
 * Number of memory checking blocks
 */
#ifndef OS_SYS_MEM_NUM
#define OS_SYS_MEM_NUM                                      20
#endif

/**
 * @ingroup los_config
 * Size of unaligned memory
 */
#ifndef OS_SYS_NOCACHEMEM_SIZE
#define OS_SYS_NOCACHEMEM_SIZE                              0x0UL
#endif

/**
 * @ingroup los_config
 * Starting address of the unaligned memory
 */
#if (OS_SYS_NOCACHEMEM_SIZE > 0)
#define OS_SYS_NOCACHEMEM_ADDR                              (&g_sysNoCacheMem0[0])
#endif

/**
 * @ingroup los_config
 * Configuration of multiple non-continuous memory regions as one memory pool
 */
#ifndef LOSCFG_MEM_MUL_REGIONS
#define LOSCFG_MEM_MUL_REGIONS                              0
#endif

/* =============================================================================
                                        Exception module configuration
============================================================================= */
/**
 * @ingroup los_config
 * Configuration of hardware stack protection
 */
#ifndef LOSCFG_EXC_HARDWARE_STACK_PROTECTION
#define LOSCFG_EXC_HARDWARE_STACK_PROTECTION                0
#endif

/* =============================================================================
                                       KAL module configuration
============================================================================= */
/**
 * @ingroup los_config
 * Configuration CMSIS_OS_VER
 */
#ifndef CMSIS_OS_VER
#define CMSIS_OS_VER                                        2
#endif

/* =============================================================================
                                       Trace module configuration
============================================================================= */
/**
 * @ingroup los_config
 * Configuration trace tool
 */

#if (LOSCFG_KERNEL_TRACE == 1)

#ifndef NUM_HAL_INTERRUPT_UART
#define NUM_HAL_INTERRUPT_UART                              0xff
#endif

#ifndef OS_TICK_INT_NUM
#define OS_TICK_INT_NUM                                     0xff
#endif

#endif

/* =============================================================================
                                       printf configuration
============================================================================= */
/**
 * @ingroup los_config
 * Configuration liteos printf
 */
#ifndef LOSCFG_KERNEL_PRINTF
#define LOSCFG_KERNEL_PRINTF                                1
#endif

/* =============================================================================
                                       misc configuration
============================================================================= */
/**
 * @ingroup los_config
 * Configuration item for mpu.
 */
#ifndef LOSCFG_MPU_ENABLE
#define LOSCFG_MPU_ENABLE                                    0
#endif

#if (LOSCFG_EXC_HARDWARE_STACK_PROTECTION == 1) && (LOSCFG_MPU_ENABLE == 0)
#error "if hardware stack protection is enabled, then MPU should be supported and enabled"
#endif

/**
 * @ingroup los_config
 * Configuration item to get task used memory.
 */
#ifndef LOSCFG_TASK_MEM_USED
#define LOSCFG_TASK_MEM_USED                                 0
#endif

/* *
 * @ingroup los_interrupt
 * Configuration item for interrupt with argument
 */
#ifndef LOSCFG_PLATFORM_HWI_WITH_ARG
#define LOSCFG_PLATFORM_HWI_WITH_ARG                       0
#endif

/**
 * @ingroup los_config
 * Configuration item to set interrupt vector align size.
 */
#ifndef LOSCFG_ARCH_HWI_VECTOR_ALIGN
#define LOSCFG_ARCH_HWI_VECTOR_ALIGN                         0x100
#endif

/**
 * @ingroup los_config
 * Task extension field additional functions, such as IAR TLS.
 * Please Use the LOSCFG_TASK_STRUCT_EXTENSION macro to define your task extended fields in target_config.h
 */
#ifndef LOSCFG_TASK_CREATE_EXTENSION_HOOK
#define LOSCFG_TASK_CREATE_EXTENSION_HOOK(taskCB)
#endif
#ifndef LOSCFG_TASK_DELETE_EXTENSION_HOOK
#define LOSCFG_TASK_DELETE_EXTENSION_HOOK(taskCB)
#endif

/**
 * @ingroup los_config
 * Configuration item to enable kernel signal.
 */
#ifndef LOSCFG_KERNEL_SIGNAL
#define LOSCFG_KERNEL_SIGNAL                          0
#endif

/**
 * @ingroup los_config
 * Configuration item to enable kernel power module.
 */
#ifndef LOSCFG_KERNEL_PM
#define LOSCFG_KERNEL_PM                              0
#endif

/**
 * @ingroup los_config
 * Configuration item to enable kernel power module in idle task.
 */
#ifndef LOSCFG_KERNEL_PM_IDLE
#define LOSCFG_KERNEL_PM_IDLE                         0
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */


#endif /* _LOS_CONFIG_H */

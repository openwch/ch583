/*
 * File      : board.h
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2009, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-09-22     Bernard      add board.h to this bsp
 * 2017-10-20     ZYH          emmm...setup for HAL Libraries
 */

#ifndef __BOARD_H__
#define __BOARD_H__

#include "CH58x_common.h"
/* board configuration */
#define SRAM_SIZE  32
#define SRAM_END (0x20000000 + SRAM_SIZE * 1024)

extern int _ebss;
#define HEAP_BEGIN  ((void *)&_ebss)
#define HEAP_END    (SRAM_END-2048) //reserved for IRQ

rt_uint32_t _SysTick_Config(rt_uint32_t ticks);
void rt_hw_board_init(void);

#endif /* __BOARD_H__ */

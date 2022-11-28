 /********************************** (C) COPYRIGHT  *******************************
 * File Name          : context_gcc.S
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2022/05/12
 * Description        : WCH Qingke V4A context gcc for rt-thread
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/
#include "cpuport.h"

.global rt_hw_interrupt_disable
.global rt_hw_interrupt_enable
.global rt_hw_context_switch_to
.global rt_hw_context_switch
.global rt_hw_context_switch_exit

/*
 * rt_base_t rt_hw_interrupt_disable(void);
 */
.section	.highcode,"ax",@progbits
.align 2
.func
rt_hw_interrupt_disable:
    csrrci a0, mstatus, 8
    nop						/* wait for 3 grade pipeline. */
    nop
    ret
	.endfunc

/*
 * void rt_hw_interrupt_enable(rt_base_t level);
 */
.section	.highcode,"ax",@progbits
.align 2
.func
rt_hw_interrupt_enable:
    csrw mstatus, a0
    ret
    .endfunc

.section	.highcode,"ax",@progbits
.align 2
.func
rt_hw_context_switch_to:

    lw sp, (a0)
    li t0, 0x1880
    csrw mstatus, t0
    j    rt_hw_context_switch_exit
    .endfunc



/*
 * void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to);
 * a0 --> from
 * a1 --> to
 */
.section	.highcode,"ax",@progbits
.align 2
.func
rt_hw_context_switch:
	/* switch in thread */
	addi sp, sp, -29 * 4
	/* save from sp */
	sw sp, 	(a0)
	/* save ra to epc */
	sw x1, 0 * 4(sp)
	sw x1, 1 * 4( sp )
	sw x5, 2 * 4( sp )
	sw x6, 3 * 4( sp )
	sw x7, 4 * 4( sp )
	sw x8, 5 * 4( sp )
	sw x9, 6 * 4( sp )
	sw x10, 7 * 4( sp )
	sw x11, 8 * 4( sp )
	sw x12, 9 * 4( sp )
	sw x13, 10 * 4( sp )
	sw x14, 11 * 4( sp )
	sw x15, 12 * 4( sp )
	sw x16, 13 * 4( sp )
	sw x17, 14 * 4( sp )
	sw x18, 15 * 4( sp )
	sw x19, 16 * 4( sp )
	sw x20, 17 * 4( sp )
	sw x21, 18 * 4( sp )
	sw x22, 19 * 4( sp )
	sw x23, 20 * 4( sp )
	sw x24, 21 * 4( sp )
	sw x25, 22 * 4( sp )
	sw x26, 23 * 4( sp )
	sw x27, 24 * 4( sp )
	sw x28, 25 * 4( sp )
	sw x29, 26 * 4( sp )
	sw x30, 27 * 4( sp )
	sw x31, 28 * 4( sp )

	/* get "to" thread sp */
	lw  sp,  0(a1)
	j  rt_hw_context_switch_exit
	.endfunc


.section	.highcode,"ax",@progbits
.align 2
.func
rt_hw_context_switch_exit:

    /* re-sw ra to mepc */
    lw a0,   0 * 4(sp)
    csrw mepc, a0

	lw x1, 1 * 4( sp )
	lw x5, 2 * 4( sp )
	lw x6, 3 * 4( sp )
	lw x7, 4 * 4( sp )
	lw x8, 5 * 4( sp )
	lw x9, 6 * 4( sp )
	lw x10, 7 * 4( sp )
	lw x11, 8 * 4( sp )
	lw x12, 9 * 4( sp )
	lw x13, 10 * 4( sp )
	lw x14, 11 * 4( sp )
	lw x15, 12 * 4( sp )
	lw x16, 13 * 4( sp )
	lw x17, 14 * 4( sp )
	lw x18, 15 * 4( sp )
	lw x19, 16 * 4( sp )
	lw x20, 17 * 4( sp )
	lw x21, 18 * 4( sp )
	lw x22, 19 * 4( sp )
	lw x23, 20 * 4( sp )
	lw x24, 21 * 4( sp )
	lw x25, 22 * 4( sp )
	lw x26, 23 * 4( sp )
	lw x27, 24 * 4( sp )
	lw x28, 25 * 4( sp )
	lw x29, 26 * 4( sp )
	lw x30, 27 * 4( sp )
	lw x31, 28 * 4( sp )

    addi sp,  sp, 29 * 4

    mret

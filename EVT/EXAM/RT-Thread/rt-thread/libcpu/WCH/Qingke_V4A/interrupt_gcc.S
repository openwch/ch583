 /********************************** (C) COPYRIGHT  *******************************
 * File Name          : interrupt_gcc.S
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2022/05/12
 * Description        : WCH Qingke V4A interrupt gcc for rt-thread
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/
#include "cpuport.h"

.global unified_interrupt_handler
.global SW_Handler
.extern user_interrupt_handler

.section	.highcode,"ax",@progbits
.align 2
.func
unified_interrupt_handler:

#if ENABLE_INTERRUPT_NEST
	csrr t0, 0x804						/* justice whether it's in interrupt nesting. */
	andi t1, t0, 0x02
	bnez t1, interrupt_nesting			/* if it's not zero, it's in interrupt nesting. */
#endif

	csrw mscratch, sp
	la sp, _eusrstack					/* Switch to ISR stack before function call. */
	csrr a0, mcause

#if ENABLE_INTERRUPT_NEST
	csrs 0x804, 0x02					/* enable interrupt nests. */
#endif

	jal user_interrupt_handler

#if ENABLE_INTERRUPT_NEST
	csrc mstatus, 8						/* disable interrupt in machine mode. */
	nop									/* wait for 3 grade pipeline. */
	nop
	nop
	csrc 0x804, 0x02					/* disable interrupt nests. */
	nop
#endif

	csrr sp, mscratch					/* resume sp from mscratch. */
	mret

#if ENABLE_INTERRUPT_NEST
interrupt_nesting:

	csrr a0, mcause						/* if it's in interrupt nesting, don't need to change sp. */
	jal user_interrupt_handler			/* handle interrupt directly. */
	mret
#endif

	.endfunc


.section	.highcode,"ax",@progbits
.align 2
.func
SW_Handler_Switch:

	li t0, 0x1880					/* set MPIE to 1, make interrupt enabled after this function. */
	csrw mstatus, t0

    la    s0, rt_interrupt_to_thread
    lw  s1, 0(s0)
    lw  sp, 0(s1)

    lw  a0,  0 * 4(sp)
    csrw  mepc, a0

	lw  x1, 1 * 4( sp )
	lw  x5, 2 * 4( sp )				/* t0 */
	lw  x6, 3 * 4( sp )				/* t1 */
	lw  x7, 4 * 4( sp )				/* t2 */
	lw  x8, 5 * 4( sp )				/* s0/fp */
	lw  x9, 6 * 4( sp )				/* s1 */
	lw  x10, 7 * 4( sp )			/* a0 */
	lw  x11, 8 * 4( sp )			/* a1 */
	lw  x12, 9 * 4( sp )			/* a2 */
	lw  x13, 10 * 4( sp )			/* a3 */
	lw  x14, 11 * 4( sp )			/* a4 */
	lw  x15, 12 * 4( sp )			/* a5 */
	lw  x16, 13 * 4( sp )			/* a6 */
	lw  x17, 14 * 4( sp )			/* a7 */
	lw  x18, 15 * 4( sp )			/* s2 */
	lw  x19, 16 * 4( sp )			/* s3 */
	lw  x20, 17 * 4( sp )			/* s4 */
	lw  x21, 18 * 4( sp )			/* s5 */
	lw  x22, 19 * 4( sp )			/* s6 */
	lw  x23, 20 * 4( sp )			/* s7 */
	lw  x24, 21 * 4( sp )			/* s8 */
	lw  x25, 22 * 4( sp )			/* s9 */
	lw  x26, 23 * 4( sp )			/* s10 */
	lw  x27, 24 * 4( sp )			/* s11 */
	lw  x28, 25 * 4( sp )			/* t3 */
	lw  x29, 26 * 4( sp )			/* t4 */
	lw  x30, 27 * 4( sp )			/* t5 */
	lw  x31, 28 * 4( sp )			/* t6 */
	addi sp, sp, 29 * 4
	mret							/* must use mret, to make mie enabled and pc changed. */
	.endfunc

.section	.highcode,"ax",@progbits
.align 2
.func
SW_Handler:

    addi sp, sp, -29 * 4

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

    /* clear rt_thread_switch_interrupt_flag */
    la    s0, rt_thread_switch_interrupt_flag
    sw    zero, 0(s0)

    csrr  a0, mepc
    sw a0, 0 * 4(sp)

    la    s0, rt_interrupt_from_thread
    lw  s1, 0(s0)
    sw sp, 0(s1)

	li t0, 0x1800					/* set MPIE to 0, make it safe when switch tasks. */
	csrw mstatus, t0

	/* Load mret with the address of the next instruction in the task to run next. */
	la t0, SW_Handler_Switch
	csrw mepc, t0

    mret
	.endfunc

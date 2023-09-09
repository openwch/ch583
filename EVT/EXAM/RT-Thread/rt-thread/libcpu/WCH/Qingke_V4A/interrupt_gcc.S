 /********************************** (C) COPYRIGHT  *******************************
 * File Name          : interrupt_gcc.S
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2023/08/16
 * Description        : WCH Qingke V4A interrupt gcc for rt-thread
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "cpuport.h"

.global SW_Handler

.section	.highcode.SW_Handler_Switch,"ax",@progbits
.align 2
.func
SW_Handler_Switch:

	li t0, 0x1880					/* set MPIE to 1, make interrupt enabled after this function. */
	csrw mstatus, t0

    la  s0, rt_interrupt_to_thread
    lw  s1, 0(s0)
    lw  sp, 0(s1)

    lw  a0,  0 * 4(sp)
    csrw  mepc, a0

	lw x1, 1 * 4( sp )
	lw x4, 2 * 4( sp )
	lw x5, 3 * 4( sp )
	lw x6, 4 * 4( sp )
	lw x7, 5 * 4( sp )
	lw x8, 6 * 4( sp )
	lw x9, 7 * 4( sp )
	lw x10, 8 * 4( sp )
	lw x11, 9 * 4( sp )
	lw x12, 10 * 4( sp )
	lw x13, 11 * 4( sp )
	lw x14, 12 * 4( sp )
	lw x15, 13 * 4( sp )
	lw x16, 14 * 4( sp )
	lw x17, 15 * 4( sp )
	lw x18, 16 * 4( sp )
	lw x19, 17 * 4( sp )
	lw x20, 18 * 4( sp )
	lw x21, 19 * 4( sp )
	lw x22, 20 * 4( sp )
	lw x23, 21 * 4( sp )
	lw x24, 22 * 4( sp )
	lw x25, 23 * 4( sp )
	lw x26, 24 * 4( sp )
	lw x27, 25 * 4( sp )
	lw x28, 26 * 4( sp )
	lw x29, 27 * 4( sp )
	lw x30, 28 * 4( sp )
	lw x31, 29 * 4( sp )

	addi sp, sp, 32 * 4
	mret							/* must use mret, to make mie enabled and pc changed. */
	.endfunc

.section	.highcode.SW_Handler,"ax",@progbits
.align 2
.func
SW_Handler:

    addi sp, sp, -32 * 4

	sw x1, 1 * 4( sp )
	sw x4, 2 * 4( sp )
	sw x5, 3 * 4( sp )
	sw x6, 4 * 4( sp )
	sw x7, 5 * 4( sp )
	sw x8, 6 * 4( sp )
	sw x9, 7 * 4( sp )
	sw x10, 8 * 4( sp )
	sw x11, 9 * 4( sp )
	sw x12, 10 * 4( sp )
	sw x13, 11 * 4( sp )
	sw x14, 12 * 4( sp )
	sw x15, 13 * 4( sp )
	sw x16, 14 * 4( sp )
	sw x17, 15 * 4( sp )
	sw x18, 16 * 4( sp )
	sw x19, 17 * 4( sp )
	sw x20, 18 * 4( sp )
	sw x21, 19 * 4( sp )
	sw x22, 20 * 4( sp )
	sw x23, 21 * 4( sp )
	sw x24, 22 * 4( sp )
	sw x25, 23 * 4( sp )
	sw x26, 24 * 4( sp )
	sw x27, 25 * 4( sp )
	sw x28, 26 * 4( sp )
	sw x29, 27 * 4( sp )
	sw x30, 28 * 4( sp )
	sw x31, 29 * 4( sp )

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

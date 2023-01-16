/********************************** (C) COPYRIGHT *******************************
 * File Name          : portASM.S
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2022/05/10
 * Description        : WCH Qingke V4A FreeRTOSÒÆÖ²»ã±à½Ó¿Ú
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#define portCONTEXT_SIZE ( 29 * 4 )

.global xPortStartFirstTask
.global unified_interrupt_handler
.global pxPortInitialiseStack
.global SW_Handler
.global SW_Handler_Switch
.extern pxCurrentTCB
.extern vTaskSwitchContext
.extern xISRStackTop
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
	lw sp, xISRStackTop					/* Switch to ISR stack before function call. */
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
/*-----------------------------------------------------------*/

.section	.highcode,"ax",@progbits
.align 2
.func
SW_Handler_Switch:

	li t0, 0x1880					/* set MPIE to 1, make interrupt enabled after this function. */
	csrw mstatus, t0

	lw  t1, pxCurrentTCB			/* Load pxCurrentTCB. */
	lw  sp, 0( t1 )				 	/* Read sp from first TCB member. */

	lw t0, 0(sp)					/* Load mret with the address of the next instruction in the task to run next. */
	csrw mepc, t0

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
	addi sp, sp, portCONTEXT_SIZE
	mret							/* must use mret, to make mie enabled and pc changed. */
	.endfunc
/*-----------------------------------------------------------*/

.section	.highcode,"ax",@progbits
.align 2
.func
SW_Handler:

	addi sp, sp, -portCONTEXT_SIZE
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

	lw  t0, pxCurrentTCB			/* Load pxCurrentTCB. */
	sw  sp, 0( t0 )					/* Write sp to first TCB member. */

	csrr a1, mepc
	sw a1, 0( sp )					/* Save updated exception return address. */

	jal vTaskSwitchContext

	li t0, 0x1800					/* set MPIE to 0, make it safe when switch tasks. */
	csrw mstatus, t0

	/* Load mret with the address of the next instruction in the task to run next. */
	la t0, SW_Handler_Switch
	csrw mepc, t0

	mret
	.endfunc
/*-----------------------------------------------------------*/

.section	.text,"ax",@progbits
.align 2
.func
xPortStartFirstTask:

	lw  sp, pxCurrentTCB			/* Load pxCurrentTCB. */
	lw  sp, 0( sp )				 	/* Read sp from first TCB member. */

	lw  x1, 0( sp ) /* Note for starting the scheduler the exception return address is used as the function return address. */

	lw  x6, 3 * 4( sp )		/* t1 */
	lw  x7, 4 * 4( sp )		/* t2 */
	lw  x8, 5 * 4( sp )		/* s0/fp */
	lw  x9, 6 * 4( sp )		/* s1 */
	lw  x10, 7 * 4( sp )	/* a0 */
	lw  x11, 8 * 4( sp )	/* a1 */
	lw  x12, 9 * 4( sp )	/* a2 */
	lw  x13, 10 * 4( sp )	/* a3 */
	lw  x14, 11 * 4( sp )	/* a4 */
	lw  x15, 12 * 4( sp )	/* a5 */
	lw  x16, 13 * 4( sp )	/* a6 */
	lw  x17, 14 * 4( sp )	/* a7 */
	lw  x18, 15 * 4( sp )	/* s2 */
	lw  x19, 16 * 4( sp )	/* s3 */
	lw  x20, 17 * 4( sp )	/* s4 */
	lw  x21, 18 * 4( sp )	/* s5 */
	lw  x22, 19 * 4( sp )	/* s6 */
	lw  x23, 20 * 4( sp )	/* s7 */
	lw  x24, 21 * 4( sp )	/* s8 */
	lw  x25, 22 * 4( sp )	/* s9 */
	lw  x26, 23 * 4( sp )	/* s10 */
	lw  x27, 24 * 4( sp )	/* s11 */
	lw  x28, 25 * 4( sp )	/* t3 */
	lw  x29, 26 * 4( sp )	/* t4 */
	lw  x30, 27 * 4( sp )	/* t5 */
	lw  x31, 28 * 4( sp )	/* t6 */

	lw  x5, 29 * 4( sp )	/* Initial mstatus into x5 (t0) */
	addi x5, x5, 0x08						/* Set MIE bit so the first task starts with interrupts enabled - required as returns with ret not eret. */
	csrrw  x0, mstatus, x5					/* Interrupts enabled from here! */
	lw  x5, 2 * 4( sp )		/* Initial x5 (t0) value. */

	addi	sp, sp, portCONTEXT_SIZE
	ret
	.endfunc
/*-----------------------------------------------------------*/

.section	.text,"ax",@progbits
.align 2
.func
pxPortInitialiseStack:

	csrr t0, mstatus					/* Obtain current mstatus value. */
	andi t0, t0, ~0x8					/* Ensure interrupts are disabled when the stack is restored within an ISR.  Required when a task is created after the schedulre has been started, otherwise interrupts would be disabled anyway. */
	addi t1, x0, 0x188					/* Generate the value 0x1880, which are the MPIE and MPP bits to set in mstatus. */
	slli t1, t1, 4
	or t0, t0, t1						/* Set MPIE and MPP bits in mstatus value. */

	addi a0, a0, -4
	sw t0, 0(a0)						/* mstatus onto the stack. */
	addi a0, a0, -(22 * 4)				/* Space for registers x11-x31. */
	sw a2, 0(a0)						/* Task parameters (pvParameters parameter) goes into register X10/a0 on the stack. */
	addi a0, a0, -(6 * 4)				/* Space for registers x5-x9. */
	sw x0, 0(a0)						/* Return address onto the stack, could be portTASK_RETURN_ADDRESS */
	addi t0, x0, 0 						/* The number of chip specific additional registers. */

chip_specific_stack_frame:				/* First add any chip specific registers to the stack frame being created. */
	beq t0, x0, 1f						/* No more chip specific registers to save. */
	addi a0, a0, -4						/* Make space for chip specific register. */
	sw x0, 0(a0)						/* Give the chip specific register an initial value of zero. */
	addi t0, t0, -1						/* Decrement the count of chip specific registers remaining. */
	j chip_specific_stack_frame			/* Until no more chip specific registers. */
1:
	addi a0, a0, -4
	sw a1, 0(a0)						/* mret value (pxCode parameter) onto the stack. */
	ret
	.endfunc
/*-----------------------------------------------------------*/

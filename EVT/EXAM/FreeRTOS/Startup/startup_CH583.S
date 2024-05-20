/********************************** (C) COPYRIGHT *******************************
 * File Name          : startup_CH58x.S
 * Author             : WCH
 * Version            : V1.0.1
 * Date               : 2023/08/16
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

	.section	.init,"ax",@progbits
	.global	_start
	.align	1
_start:
	j	handle_reset

    .section    .vector,"ax",@progbits
    .align  1
_vector_base:
    .option norvc;

    .word   0
    .word   0
    .word   NMI_Handler                 			/* NMI Handler */
    .word   HardFault_Handler           			/* Hard Fault Handler */
    .word   0xF3F9BDA9								/* boot option, can't modify */
    .word   Ecall_M_Mode_Handler        			/* 5 */
    .word   0
    .word   0
    .word   Ecall_U_Mode_Handler					/* 8 */
    .word   Break_Point_Handler						/* 9 */
    .word   0
    .word   0
    .word   SysTick_Handler         				/* SysTick Handler */
    .word   0
    .word   SW_Handler                 				/* SW Handler */
    .word   0

    /* External Interrupts */
    .word   unified_interrupt_entry           		/* TMR0 */
    .word   unified_interrupt_entry         		/* GPIOA */
    .word   unified_interrupt_entry           		/* GPIOB */
    .word   unified_interrupt_entry            		/* SPI0 */

_user_vector_base:
	.option norvc;

    .word   unified_interrupt_entry              	/* BLEB */
    .word   unified_interrupt_entry             	/* BLEL */
    .word   unified_interrupt_entry             	/* USB */
    .word   unified_interrupt_entry					/* USB2 */
    .word   unified_interrupt_entry          		/* TMR1 */
    .word   unified_interrupt_entry            		/* TMR2 */
    .word   unified_interrupt_entry           		/* UART0 */
    .word   unified_interrupt_entry           		/* UART1 */
    .word   unified_interrupt_entry             	/* RTC */
    .word   unified_interrupt_entry             	/* ADC */
    .word   unified_interrupt_entry 			 	/* I2C */
    .word   unified_interrupt_entry            		/* PWMX */
    .word   unified_interrupt_entry            		/* TMR3 */
    .word   unified_interrupt_entry           		/* UART2 */
    .word   unified_interrupt_entry           		/* UART3 */
    .word   unified_interrupt_entry        			/* WDOG_BAT */

_real_user_vector_base:
	.option norvc;

    /* External Interrupts */
    .word   TMR0_IRQHandler            				/* TMR0 */
    .word   GPIOA_IRQHandler           				/* GPIOA */
    .word   GPIOB_IRQHandler           				/* GPIOB */
    .word   SPI0_IRQHandler            				/* SPI0 */
    .word   BB_IRQLibHandler      					/* BLEB BB_IRQHandler */
    .word   LLE_IRQLibHandler       				/* BLEL LLE_IRQHandler */
    .word   USB_IRQHandler             				/* USB */
    .word   USB2_IRQHandler			   				/* USB2 */
    .word   TMR1_IRQHandler            				/* TMR1 */
    .word   TMR2_IRQHandler            				/* TMR2 */
    .word   UART0_IRQHandler           				/* UART0 */
    .word   UART1_IRQHandler           				/* UART1 */
    .word   RTC_IRQHandler             				/* RTC */
    .word   ADC_IRQHandler             				/* ADC */
    .word   I2C_IRQHandler 			   				/* I2C */
    .word   PWMX_IRQHandler            				/* PWMX */
    .word   TMR3_IRQHandler            				/* TMR3 */
    .word   UART2_IRQHandler           				/* UART2 */
    .word   UART3_IRQHandler           				/* UART3 */
    .word   WDOG_BAT_IRQHandler        				/* WDOG_BAT */

    .section    .vector_handler, "ax", @progbits
    .weak   NMI_Handler
    .weak   HardFault_Handler
    .weak   Ecall_M_Mode_Handler
    .weak   Ecall_U_Mode_Handler
    .weak   Break_Point_Handler
    .weak   SysTick_Handler
    .weak   SW_Handler
    .weak   TMR0_IRQHandler
    .weak   GPIOA_IRQHandler
    .weak   GPIOB_IRQHandler
    .weak   SPI0_IRQHandler
    .weak   BB_IRQHandler
    .weak   LLE_IRQHandler
  	.weak   BB_IRQLibHandler
    .weak   LLE_IRQLibHandler
    .weak   USB_IRQHandler
    .weak   USB2_IRQHandler
    .weak   TMR1_IRQHandler
    .weak   TMR2_IRQHandler
    .weak   UART0_IRQHandler
    .weak   UART1_IRQHandler
    .weak   RTC_IRQHandler
    .weak   ADC_IRQHandler
    .weak   I2C_IRQHandler
    .weak   PWMX_IRQHandler
    .weak   TMR3_IRQHandler
    .weak   UART2_IRQHandler
    .weak   UART3_IRQHandler
    .weak   WDOG_BAT_IRQHandler

NMI_Handler:
HardFault_Handler:
Ecall_M_Mode_Handler:
Ecall_U_Mode_Handler:
Break_Point_Handler:
SysTick_Handler:
SW_Handler:
TMR0_IRQHandler:
GPIOA_IRQHandler:
GPIOB_IRQHandler:
SPI0_IRQHandler:
BB_IRQHandler:
LLE_IRQHandler:
BB_IRQLibHandler:
LLE_IRQLibHandler:
USB_IRQHandler:
USB2_IRQHandler:
TMR1_IRQHandler:
TMR2_IRQHandler:
UART0_IRQHandler:
UART1_IRQHandler:
RTC_IRQHandler:
ADC_IRQHandler:
I2C_IRQHandler:
PWMX_IRQHandler:
TMR3_IRQHandler:
UART2_IRQHandler:
UART3_IRQHandler:
WDOG_BAT_IRQHandler:
1:
	j 1b

	.section	.handle_reset,"ax",@progbits
	.weak	handle_reset
	.align	1
handle_reset:
.option push
.option	norelax
	la gp, __global_pointer$
.option	pop
1:
	la sp, _eusrstack

/* Load highcode code  section from flash to RAM */
2:
    la a0, _highcode_lma
    la a1, _highcode_vma_start
    la a2, _highcode_vma_end
    bgeu a1, a2, 2f
1:
    lw t0, (a0)
    sw t0, (a1)
    addi a0, a0, 4
    addi a1, a1, 4
    bltu a1, a2, 1b

/* Load data section from flash to RAM */
2:
	la a0, _data_lma
	la a1, _data_vma
	la a2, _edata
	bgeu a1, a2, 2f
1:
	lw t0, (a0)
	sw t0, (a1)
	addi a0, a0, 4
	addi a1, a1, 4
	bltu a1, a2, 1b
2:
	/* clear bss section */
	la a0, _sbss
	la a1, _ebss
	bgeu a0, a1, 2f
1:
	sw zero, (a0)
	addi a0, a0, 4
	bltu a0, a1, 1b
2:
	/* 流水线控制位 & 动态预测控制位 */
	li t0, 0x1f
	csrw 0xbc0, t0

	/* 打开硬件压栈，关闭中断嵌套，在中断函数入口处再打开中断嵌套 */
	li t0, 0x01
	csrw 0x804, t0

	/* 使用机器模式，在freertos中打开中断 */
    li t0, 0x1800
    csrs mstatus, t0

	/* 配置系统异常及中断向量表，用户真正使用的向量表在统一入口 unified_interrupt_entry 中处理 */
	la t0, _vector_base
    ori t0, t0, 3
	csrw mtvec, t0

	la t0, main
	csrw mepc, t0

	mret

.extern xISRStackTop

.section	.highcode.unified_interrupt_entry,"ax",@progbits
.align 2
.func
unified_interrupt_entry:
	csrr a1, mcause
	slli a1, a1, 2
	la a0, _user_vector_base
	add a0, a0, a1
	lw a0, 0(a0)						/* read isr functions addr from _vector_base. */

#if ENABLE_INTERRUPT_NEST
	csrr t0, 0x804						/* justice whether it's in interrupt nesting. */
	andi t1, t0, 0x02
	bnez t1, interrupt_nesting			/* if it's not zero, it's in interrupt nesting. */
#endif

	csrw mscratch, sp
	lw sp, xISRStackTop					/* Switch to ISR stack before function call. */

#if ENABLE_INTERRUPT_NEST
	csrs 0x804, 0x02					/* enable interrupt nests. */
#endif

	jalr x1, 0(a0)

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
	jalr x1, 0(a0)						/* if it's in interrupt nesting, don't need to change sp. */
	mret
#endif

	.endfunc

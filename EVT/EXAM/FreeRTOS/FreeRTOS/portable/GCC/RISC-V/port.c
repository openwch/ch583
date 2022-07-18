/********************************** (C) COPYRIGHT *******************************
 * File Name          : port.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2022/05/10
 * Description        : WCH Qingke V4A FreeRTOSÒÆÖ²½Ó¿Ú
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/*-----------------------------------------------------------
 * Implementation of functions defined in portable.h for the RISC-V RV32 port.
 *----------------------------------------------------------*/

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "portmacro.h"
#include "core_riscv.h"

/* Standard includes. */
#include "string.h"

/* Let the user override the pre-loading of the initial LR with the address of
prvTaskExitError() in case it messes up unwinding of the stack in the
debugger. */
#ifdef configTASK_RETURN_ADDRESS
	#define portTASK_RETURN_ADDRESS	configTASK_RETURN_ADDRESS
#else
	#define portTASK_RETURN_ADDRESS	prvTaskExitError
#endif

/* The stack used by interrupt service routines.  Set configISR_STACK_SIZE_WORDS
to use a statically allocated array as the interrupt stack.  Alternative leave
configISR_STACK_SIZE_WORDS undefined and update the linker script so that a
linker variable names __freertos_irq_stack_top has the same value as the top
of the stack used by main.  Using the linker script method will repurpose the
stack that was used by main before the scheduler was started for use as the
interrupt stack after the scheduler has started. */
#ifdef configISR_STACK_SIZE_WORDS
	static __attribute__ ((aligned(16))) StackType_t xISRStack[ configISR_STACK_SIZE_WORDS ] = { 0 };
	const StackType_t xISRStackTop = ( StackType_t ) &( xISRStack[ configISR_STACK_SIZE_WORDS & ~portBYTE_ALIGNMENT_MASK ] );

	/* Don't use 0xa5 as the stack fill bytes as that is used by the kernerl for
	the task stacks, and so will legitimately appear in many positions within
	the ISR stack. */
	#define portISR_STACK_FILL_BYTE	0xee
#else
	/* __freertos_irq_stack_top define by .ld file */
	extern const uint32_t __freertos_irq_stack_top[];
	const StackType_t xISRStackTop = ( StackType_t ) __freertos_irq_stack_top;
#endif

/*
 * Setup the timer to generate the tick interrupts.  The implementation in this
 * file is weak to allow application writers to change the timer used to
 * generate the tick interrupt.
 */
void vPortSetupTimerInterrupt( void ) __attribute__(( weak ));

/* Set configCHECK_FOR_STACK_OVERFLOW to 3 to add ISR stack checking to task
stack checking.  A problem in the ISR stack will trigger an assert, not call the
stack overflow hook function (because the stack overflow hook is specific to a
task stack, not the ISR stack). */
#if defined( configISR_STACK_SIZE_WORDS ) && ( configCHECK_FOR_STACK_OVERFLOW > 2 )
	#warning This path not tested, or even compiled yet.

	static const uint8_t ucExpectedStackBytes[] = {
									portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE,		\
									portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE,		\
									portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE,		\
									portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE,		\
									portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE, portISR_STACK_FILL_BYTE };	\

	#define portCHECK_ISR_STACK() configASSERT( ( memcmp( ( void * ) xISRStack, ( void * ) ucExpectedStackBytes, sizeof( ucExpectedStackBytes ) ) == 0 ) )
#else
	/* Define the function away. */
	#define portCHECK_ISR_STACK()
#endif /* configCHECK_FOR_STACK_OVERFLOW > 2 */

/*-----------------------------------------------------------*/


/* just for wch's systick, don't have mtime */
void vPortSetupTimerInterrupt( void )
{
    /* set software is lowest priority */
    PFIC_SetPriority(SWI_IRQn, 0xf0);
    PFIC_EnableIRQ(SWI_IRQn);
    /* set systick is lowest priority */
    PFIC_SetPriority(SysTick_IRQn, 0xf0);
    SysTick_Config(configCPU_CLOCK_HZ / configTICK_RATE_HZ);

}

/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
extern void xPortStartFirstTask( void );

	#if( configASSERT_DEFINED == 1 )
	{
		volatile uint32_t mtvec = 0;

		/* Check the least significant two bits of mtvec are 0b11 - indicating
		multiply vector mode. */
		__asm volatile( "csrr %0, mtvec" : "=r"( mtvec ) );
		configASSERT( ( mtvec & 0x03UL ) == 0x3 );

		/* Check alignment of the interrupt stack - which is the same as the
		stack that was being used by main() prior to the scheduler being
		started. */
		configASSERT( ( xISRStackTop & portBYTE_ALIGNMENT_MASK ) == 0 );

		#ifdef configISR_STACK_SIZE_WORDS
		{
			memset( ( void * ) xISRStack, portISR_STACK_FILL_BYTE, sizeof( xISRStack ) );
		}
		#endif	 /* configISR_STACK_SIZE_WORDS */
	}
	#endif /* configASSERT_DEFINED */

	/* If there is a CLINT then it is ok to use the default implementation
	in this file, otherwise vPortSetupTimerInterrupt() must be implemented to
	configure whichever clock is to be used to generate the tick interrupt. */
	vPortSetupTimerInterrupt();

	{
		/* Enable external interrupts. */
	}

    /* Initialise the critical nesting count ready for the first task. */
	xPortStartFirstTask();

	/* Should not get here as after calling xPortStartFirstTask() only tasks
	should be executing. */
	return pdFAIL;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
	/* Not implemented. */
	for( ;; );
}

/*-----------------------------------------------------------*/

__attribute__((section(".highcode")))
void SysTick_Handler( void )
{
    if( xTaskIncrementTick() != pdFALSE )
    {
        portYIELD();
    }
    SysTick->SR &= (~(1<<0));
}

/*-----------------------------------------------------------*/
__HIGH_CODE
void vPortEnterCritical( void )
{
    portDISABLE_INTERRUPTS();
}

/*-----------------------------------------------------------*/
__HIGH_CODE
void vPortExitCritical( void )
{
    portENABLE_INTERRUPTS();
}

extern void SysTick_Handler(void);
extern void TMR0_IRQHandler(void);
extern void GPIOA_IRQHandler(void);
extern void GPIOB_IRQHandler(void);
extern void SPI0_IRQHandler(void);
extern void BB_IRQHandler(void);
extern void LLE_IRQHandler(void);
extern void USB_IRQHandler(void);
extern void TMR1_IRQHandler(void);
extern void TMR2_IRQHandler(void);
extern void UART0_IRQHandler(void);
extern void UART1_IRQHandler(void);
extern void RTC_IRQHandler(void);
extern void ADC_IRQHandler(void);
extern void PWMX_IRQHandler(void);
extern void TMR3_IRQHandler(void);
extern void UART2_IRQHandler(void);
extern void UART3_IRQHandler(void);
extern void WDOG_BAT_IRQHandler(void);

typedef void (*user_irq_handler_t)(void);

__attribute__((section("wch_user_vectors")))
user_irq_handler_t wch_user_irq_table[] =
{
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   SysTick_Handler,             /* SysTick Handler */
   0,
   0,                           /* SW Handler */
   0,
  /* External Interrupts */
   TMR0_IRQHandler   ,          /* 0:  TMR0 */
   GPIOA_IRQHandler,            /* GPIOA */
   GPIOB_IRQHandler ,           /* GPIOB */
   SPI0_IRQHandler  ,           /* SPI0 */
   BB_IRQHandler   ,            /* BLEB */
   LLE_IRQHandler ,             /* BLEL */
   USB_IRQHandler  ,            /* USB */
   0,
   TMR1_IRQHandler ,            /* TMR1 */
   TMR2_IRQHandler,             /* TMR2 */
   UART0_IRQHandler ,           /* UART0 */
   UART1_IRQHandler,            /* UART1 */
   RTC_IRQHandler,              /* RTC */
   ADC_IRQHandler,              /* ADC */
   0,
   PWMX_IRQHandler,             /* PWMX */
   TMR3_IRQHandler,             /* TMR3 */
   UART2_IRQHandler,            /* UART2 */
   UART3_IRQHandler,            /* UART3 */
   WDOG_BAT_IRQHandler,         /* WDOG_BAT */
};

__attribute__((section(".highcode")))
void user_interrupt_handler(uint32_t mcause)
{
    uint32_t irq_num;
    irq_num = mcause & 0x7f;
    if(wch_user_irq_table[irq_num] != NULL)
    {
        wch_user_irq_table[irq_num]();
    }
}

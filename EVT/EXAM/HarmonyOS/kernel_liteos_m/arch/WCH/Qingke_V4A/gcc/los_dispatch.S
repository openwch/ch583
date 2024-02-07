/*
 * Copyright (c) 2023, smartmx <smartmx@qq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-08-28     smartmx      the first version
 *
 */


#include "riscv_encoding.h"

    .section .text,"ax",@progbits
    .align 2
    .type ArchIntLock, %function
    .global ArchIntLock
ArchIntLock:
    csrr    a0, mstatus  /* return value */
    li      t0, (MSTATUS_MIE|MSTATUS_MPIE) /* mie|mpie */
    csrrc   zero, mstatus, t0
    ret

    .type ArchIntUnLock, %function
    .global ArchIntUnLock
ArchIntUnLock:
    csrr    a0, mstatus  /* return value*/
    li      t0, (MSTATUS_MIE|MSTATUS_MPIE) /* mie|mpie */
    csrrs   zero, mstatus, t0
    ret

    .type ArchIntRestore, %function
    .global ArchIntRestore
ArchIntRestore:
    csrw mstatus, a0
    ret


/* Start the first task.  This also clears the bit that indicates the FPU is
    in use in case the FPU was used before the scheduler was started - which
    would otherwise result in the unnecessary leaving of space in the stack
    for lazy saving of FPU registers. */
    .type HalStartToRun, %function
    .global HalStartToRun
    .align 2
HalStartToRun:
    /* get stack pointer */
    la t0, g_losTask
    lw t1, 0(t0)
    lw sp, 0(t1)    /* TCB sp->sp */

    /* Pop PC from stack and set MEPC */
    lw t0,  0  * 4(sp)
    csrw mepc, t0

    li t0, 0x1880
    csrw mstatus, t0

    /* Interrupt still disable here */
    /* Restore Registers from Stack */
    lw x1,  1  * 4(sp)    /* RA */
    lw x5,  2  * 4(sp)
    lw x6,  3  * 4(sp)
    lw x7,  4  * 4(sp)
    lw x8,  5  * 4(sp)
    lw x9,  6  * 4(sp)
    lw x10, 7  * 4(sp)
    lw x11, 8  * 4(sp)
    lw x12, 9  * 4(sp)
    lw x13, 10 * 4(sp)
    lw x14, 11 * 4(sp)
    lw x15, 12 * 4(sp)
    lw x16, 13 * 4(sp)
    lw x17, 14 * 4(sp)
    lw x18, 15 * 4(sp)
    lw x19, 16 * 4(sp)
    lw x20, 17 * 4(sp)
    lw x21, 18 * 4(sp)
    lw x22, 19 * 4(sp)
    lw x23, 20 * 4(sp)
    lw x24, 21 * 4(sp)
    lw x25, 22 * 4(sp)
    lw x26, 23 * 4(sp)
    lw x27, 24 * 4(sp)
    lw x28, 25 * 4(sp)
    lw x29, 26 * 4(sp)
    lw x30, 27 * 4(sp)
    lw x31, 28 * 4(sp)

    lw  x4, 29 * 4( sp )			/* tp - not sure it is useful */

    addi sp, sp, ( 32 * 4 )

    mret


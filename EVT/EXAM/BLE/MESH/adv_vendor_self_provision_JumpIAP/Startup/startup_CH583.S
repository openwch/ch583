/********************************** (C) COPYRIGHT *******************************
* File Name          : startup_ch58x.s
* Author             : WCH
* Version            : V1.0.0
* Date               : 2020/04/30
* Description        : 
*******************************************************************************/

	.section	.init,"ax",@progbits
	.global	_start
	.align	1
_start:
	j	0x0004D000


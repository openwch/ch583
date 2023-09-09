本工程为 FreeRTOS-KernelV10.5.1 在Qingke V4A RISC-V内核的CH583/2/1上的移植。

需要注意以下几点：

	1.配置文件路径为工程目录下FreeRTOS/FreeRTOSConfig.h
	
	2.本移植例程默认使用了硬件压栈（不可关闭），未使用中断嵌套，用户使用的外部中断函数最好使用__attribute__((section(".highcode")))修饰，保证运行速度。
	
	3.使能中断嵌套会导致每个中断执行会多约10个指令周期，中断嵌套使能通过工程右键 -> properties -> c/c++ Build -> settings -> tool settings -> GNU RISC-V Cross Assembler -> Preprocessor 右边输入框Defined symbols中的 ENABLE_INTERRUPT_NEST=0 修改为 ENABLE_INTERRUPT_NEST=1 即可。
	
	4.用户中断函数不需要再使用__attribute__((interrupt("WCH-Interrupt-fast")))或者__attribute__((interrupt()))修饰，中断入口已统一为汇编函数，在汇编函数中调用用户中断函数。库自带的HardFault中断除外，用户只需要修改自己编写的中断函数即可。
	
	5.CH583系列上电运行默认的栈为编译后剩余的RAM空间。所有统一入口的中断会把栈修改为LD文件中提供的__freertos_irq_stack_top，所以中断中可以使用的最大栈空间为RAM剩余空间。所以请一定要预留RAM空间给栈使用。
		
	6.因为Systick中断经常处理，所以Systick中断不从统一中断入口处理，并使用了__attribute__((interrupt("WCH-Interrupt-fast")))修饰，Systick中调用的函数栈不大，所以未切换为中断栈，提升速度。
	
	7.中断中使用了CSR寄存器mscratch作为sp指针的临时寄存器，所以用户不可以再自行使用mscratch寄存器。
	
	8.不建议使用蓝牙，使用蓝牙的话可以创建一个优先级仅比IDLE TASK高一级的任务，循环运行TMOS_SystemProcess，不主动退出任务，由其他更高优先级任务抢断。由于蓝牙中断为库内中断函数，使用了免表中断方式，所以初始化蓝牙时需要关闭中断，等初始化完成后失能免表中断。在StartUP.S文件中，已经将LIB中真正的中断函数地址BB_IRQLibHandler和LLE_IRQLibHandler放入中断向量表中。
	
	9.本工程的Startup.S文件和Ld文件都是单独的，用户中断向量表可见Startup.S，用户中断总入口unified_interrupt_entry和中断向量表_real_user_vector_base就在该文件中。
	
	10.FreeRTOS任务中函数的打印最好使用main.c中提供的APP_Printf进行，不然可能会导致HardFault。
	
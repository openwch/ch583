/********************************** (C) COPYRIGHT *******************************
* File Name          : CH57x_SYS.c
* Author             : WCH
* Version            : V1.0
* Date               : 2018/12/15
* Description 
*******************************************************************************/

#include "CH58x_common.h"
#include "HAL/config.h"

typedef enum
{
  CLK_FLASH_4 = 0X01,
  CLK_FLASH_5 = 0X05,
  CLK_FLASH_6 = 0X02,
  CLK_FLASH_7 = 0X06,
  CLK_FLASH_8 = 0X03,
  CLK_FLASH_9 = 0X07,

  AHB_READY_SHORT = 0X00,
  AHB_READY_NORMAL = 0X40,
  AHB_READY_LONG = 0X80,
  AHB_READY_LONGER = 0XC0,

  AHB_SAMPLE_NORMAL = 0X00,
  AHB_SAMPLE_DELAY = 0X10,
  AHB_SAMPLE_BEFORE = 0X20,

  AHB_SCSWIDTH_3 = 0X00,
  AHB_SCSWIDTH_2 = 0X08,

}FLASH_CLKTypeDef;

extern uint32_t _highcode_lma;
extern uint32_t _highcode_vma_start;
extern uint32_t _highcode_vma_end;

extern uint32_t _data_lma;
extern uint32_t _data_vma;
extern uint32_t _edata;

extern uint32_t _sbss;
extern uint32_t _ebss;


__attribute__((section(".highcode_copy")))
static void __attribute__((noinline)) copy_section(uint32_t * p_load, uint32_t * p_vma, uint32_t * p_vma_end)
{
    while(p_vma <= p_vma_end)
    {
        *p_vma = *p_load;
        ++p_load;
        ++p_vma;
    }
}

__attribute__((section(".highcode_copy")))
static void __attribute__((noinline)) zero_section(uint32_t * start, uint32_t * end)
{
    uint32_t * p_zero = start;

    while(p_zero <= end)
    {
        *p_zero = 0;
        ++p_zero;
    }
}

/*******************************************************************************
* Function Name  : SetSysClock
* Description    : 配置系统运行时钟
* Input          : sc: 系统时钟源选择
          refer to SYS_CLKTypeDef
* Return         : None
*******************************************************************************/
__attribute__((section(".highcode")))
void SetSysClock( SYS_CLKTypeDef sc)
{
  UINT32 i;
  R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;
  R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;
  SAFEOPERATE;
  R8_PLL_CONFIG &= ~(1<<5);   //
  R8_SAFE_ACCESS_SIG = 0;
  if ( sc & 0x20 ){    // HSE div
    if ( !( R8_HFCK_PWR_CTRL & RB_CLK_XT32M_PON ) ){
      R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;
      R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;
      SAFEOPERATE;
      R8_HFCK_PWR_CTRL |= RB_CLK_XT32M_PON;    // HSE power on
      for(i=0;i<1200;i++){  __nop();__nop();  }
    }

    R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;
    R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;
    R16_CLK_SYS_CFG = ( 0 << 6 ) | ( sc & 0x1f );
    __nop();__nop();__nop();__nop();
    R8_SAFE_ACCESS_SIG = 0;
    R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;
    R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;
    SAFEOPERATE;
    R8_FLASH_CFG = 0X01;
    R8_SAFE_ACCESS_SIG = 0;
  }

  else if ( sc & 0x40 ){    // PLL div
    if ( !( R8_HFCK_PWR_CTRL & RB_CLK_PLL_PON ) ){
        R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;
        R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;
        R8_HFCK_PWR_CTRL |= RB_CLK_PLL_PON;    // PLL power on
        SAFEOPERATE;
        for(i=0;i<2000;i++){  __nop();__nop();  }
    }
    R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;
    R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;
    SAFEOPERATE;
    R16_CLK_SYS_CFG = ( 1 << 6 ) | ( sc & 0x1f );
    __nop();__nop();__nop();__nop();
    R8_SAFE_ACCESS_SIG = 0;
    R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;
    R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;
    SAFEOPERATE;
    R8_FLASH_CFG = 0X03;
    R8_SAFE_ACCESS_SIG = 0;
  }
}

__attribute__((section(".highcode_copy")))
void mySysInit(void) {
  uint32_t i;
  R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;
  R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;
  SAFEOPERATE;
  R8_PLL_CONFIG &= ~(1<<5);   //
  R8_SAFE_ACCESS_SIG = 0;
  if ( !( R8_HFCK_PWR_CTRL & RB_CLK_PLL_PON ) ){
        R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;
        R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;
        SAFEOPERATE;
        R8_HFCK_PWR_CTRL |= RB_CLK_PLL_PON;    // PLL power on
        for(i=0;i<2000;i++){  __nop();__nop();  }
    }
    R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;
    R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;
    SAFEOPERATE;
    R16_CLK_SYS_CFG = ( 1 << 6 ) | ( CLK_SOURCE_PLL_60MHz & 0x1f );
    __nop();__nop();__nop();__nop();
    R8_SAFE_ACCESS_SIG = 0;
    R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;
    R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;
    SAFEOPERATE;
    R8_FLASH_CFG = CLK_FLASH_6;
    R8_SAFE_ACCESS_SIG = 0;

    R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;
    R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;__nop();__nop();
    R8_PLL_CONFIG |= 1<<7;
    R8_SAFE_ACCESS_SIG = 0;

    copy_section(&_highcode_lma, &_highcode_vma_start, &_highcode_vma_end);
    copy_section(&_data_lma, &_data_vma, &_edata);
    zero_section(&_sbss, &_ebss);
}

void SystemInit(void){
    mySysInit();
}
/*******************************************************************************
* Function Name  : GetSysClock
* Description    : 获取当前系统时钟
* Input          : None
* Return         : Hz
*******************************************************************************/
UINT32 GetSysClock( void )
{
  UINT16  rev;

  rev = R16_CLK_SYS_CFG & 0xff;
  if( (rev & 0x40) == (0<<6) ){       // 32M进行分频
      return (32000000/(rev&0x1f));
  }
  else if( (rev & RB_CLK_SYS_MOD) == (1<<6) ){    // PLL进行分频
      return (480000000/(rev&0x1f));
  }
  else {                        // 32K做主频
    return (32000);
  }
}

/*******************************************************************************
* Function Name  : SYS_GetInfoSta
* Description    : 获取当前系统信息状态
* Input          : i: 
					refer to SYS_InfoStaTypeDef
* Return         : DISABLE  -  关闭
				   ENABLE   -  开启
*******************************************************************************/
UINT8 SYS_GetInfoSta( SYS_InfoStaTypeDef i )
{
  if (i == STA_SAFEACC_ACT)
    return (R8_SAFE_ACCESS_SIG & RB_SAFE_ACC_ACT);
  else
    return (R8_GLOB_CFG_INFO & (1 << i));
}

/*******************************************************************************
* Function Name  : SYS_ResetExecute
* Description    : 执行系统软件复位
* Input          : None
* Return         : None
*******************************************************************************/
__attribute__((section(".highcode")))
void SYS_ResetExecute( void )
{
#if( DEBUG == Debug_UART1 )  // 使用其他串口输出打印信息需要修改这行代码
  while( ( R8_UART1_LSR & RB_LSR_TX_ALL_EMP ) == 0 )
    __nop();
#endif
  FLASH_ROM_SW_RESET();
  R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;
  R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;
  SAFEOPERATE;
  R8_RST_WDOG_CTRL |= RB_SOFTWARE_RESET;
  R8_SAFE_ACCESS_SIG = 0;
}

/*******************************************************************************
* Function Name  : SYS_DisableAllIrq
* Description    : 关闭所有中断，并保留当前中断值
* Input          : pirqv：当前保留中断值
* Return         : None
*******************************************************************************/
void SYS_DisableAllIrq( PUINT32 pirqv )
{
  *pirqv = (PFIC->ISR[0] >> 8) | (PFIC->ISR[1] << 24);
  PFIC->IRER[0] = 0xffffffff;
  PFIC->IRER[1] = 0xffffffff;
}

/*******************************************************************************
 * Function Name  : SYS_RecoverIrq
 * Description    : 恢复之前关闭的中断值
 * Input          : irq_status：当前保留中断值
 * Return         : None
 *******************************************************************************/
void SYS_RecoverIrq( UINT32 irq_status )
{
  PFIC->IENR[0] = (irq_status << 8);
  PFIC->IENR[1] = (irq_status >> 24);
}

/*******************************************************************************
* Function Name  : SYS_GetSysTickCnt
* Description    : 获取当前系统(SYSTICK)计数值
* Input          : None
* Return         : 当前计数值
*******************************************************************************/
UINT32 SYS_GetSysTickCnt( void )
{
	UINT32 val;

	val = SysTick->CNT;
	return( val );
}

/*******************************************************************************
* Function Name  : WWDG_ITCfg
* Description    : 看门狗定时器溢出中断使能
* Input          : DISABLE-溢出不中断      ENABLE-溢出中断
* Return         : None
*******************************************************************************/
void  WWDG_ITCfg( FunctionalState s )
{
	R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;		
	R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;SAFEOPERATE;
	if(s == DISABLE)		R8_RST_WDOG_CTRL&=~RB_WDOG_INT_EN;
	else 					R8_RST_WDOG_CTRL|=RB_WDOG_INT_EN;
	R8_SAFE_ACCESS_SIG = 0;	
}

/*******************************************************************************
* Function Name  : WWDG_ResetCfg
* Description    : 看门狗定时器复位功能
* Input          : DISABLE-溢出不复位      ENABLE-溢出系统复位
* Return         : None
*******************************************************************************/
void WWDG_ResetCfg( FunctionalState s )
{
	R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;		
	R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;SAFEOPERATE;
	if(s == DISABLE)		R8_RST_WDOG_CTRL&=~RB_WDOG_RST_EN;
	else 					R8_RST_WDOG_CTRL|=RB_WDOG_RST_EN;
	R8_SAFE_ACCESS_SIG = 0;	
}

/*******************************************************************************
* Function Name  : WWDG_ClearFlag
* Description    : 清除看门狗中断标志，重新加载计数值也可清除
* Input          : None
* Return         : None
*******************************************************************************/
void WWDG_ClearFlag( void )
{
	R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;		
	R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;SAFEOPERATE;
	R8_RST_WDOG_CTRL |= RB_WDOG_INT_FLAG;
	R8_SAFE_ACCESS_SIG = 0;	
}

/*******************************************************************************
* Function Name  : HardFault_Handler
* Description    : 硬件错误中断，进入后执行复位，复位类型为上电复位
* Input          : None
* Return         : None
*******************************************************************************/
__attribute__((section(".highcode")))
void HardFault_Handler( void )
{

  LOG_INFO("hardfault");

  LOG_INFO("MCAUSE:%#x",__get_MCAUSE());
  LOG_INFO("MEPC:%#x",__get_MEPC());
  LOG_INFO("MTVAL:%#x",__get_MTVAL());
  while(1);
  SYS_ResetExecute();
  CODE_UNREACHABLE;
}

__attribute__((section(".highcode")))
void NMI_Handler( void )
{

  LOG_INFO("NMI");

  LOG_INFO("MCAUSE:%#x",__get_MCAUSE());
  LOG_INFO("MEPC:%#x",__get_MEPC());
  LOG_INFO("MTVAL:%#x",__get_MTVAL());
  while(1);
  SYS_ResetExecute();
  CODE_UNREACHABLE;
}

volatile static uint32_t systick_millis = 0;

__attribute__((section(".highcode")))
void SysTick_Handler(void) {
    systick_millis++;
    SysTick->SR &= ~(1 << 0);
}


unsigned long millis(void)
{
    return systick_millis;
}

/*******************************************************************************
* Function Name  : mDelayuS
* Description    : uS 延时
* Input          : t: 时间参数
* Return         : None
*******************************************************************************/
__attribute__((section(".highcode")))
void mDelayuS( UINT16 t )
{
    UINT32 i;

#if     (FREQ_SYS == 80000000)
    i = t*20;
#elif (FREQ_SYS == 60000000)
    i = t*15;
#elif (FREQ_SYS == 48000000)
    i = t*12;
#elif (FREQ_SYS == 40000000)
    i = t*10;
#elif	(FREQ_SYS == 32000000)
    i = t<<3;
#elif	(FREQ_SYS == 24000000)
    i = t*6;
#elif	(FREQ_SYS == 16000000)
    i = t<<2;
#elif	(FREQ_SYS == 8000000)
    i = t<<1;
#elif	(FREQ_SYS == 4000000)
    i = t;
#elif	(FREQ_SYS == 2000000)
    i = t>>1;
#elif	(FREQ_SYS == 1000000)
    i = t>>2;
#endif
    do
    {
    	__nop();
    }while(--i);
}

/*******************************************************************************
* Function Name  : mDelaymS
* Description    : mS 延时
* Input          : t: 时间参数
* Return         : None
*******************************************************************************/
__attribute__((section(".highcode")))
void mDelaymS( UINT16 t )
{
    UINT16 i;

    for(i=0; i<t; i++)
        mDelayuS(1000);
}


#ifdef DEBUG
int _write(int fd, char *buf, int size)
{
	int i;
	for(i=0; i<size; i++)
	{
#if  DEBUG == Debug_UART0
		while( R8_UART0_TFC == UART_FIFO_SIZE );                        /* 等待数据发送 */
		R8_UART0_THR = *buf++;                                               /* 发送数据 */
#elif DEBUG == Debug_UART1       
		while( R8_UART1_TFC == UART_FIFO_SIZE );                        /* 等待数据发送 */
		R8_UART1_THR = *buf++;                                               /* 发送数据 */
#elif DEBUG == Debug_UART2       
		while( R8_UART2_TFC == UART_FIFO_SIZE );                        /* 等待数据发送 */
		R8_UART2_THR = *buf++;                                               /* 发送数据 */
#elif DEBUG == Debug_UART3       
		while( R8_UART3_TFC == UART_FIFO_SIZE );                        /* 等待数据发送 */
		R8_UART3_THR = *buf++;                                               /* 发送数据 */
#endif
	}
	return size;
}

#endif


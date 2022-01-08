/********************************** (C) COPYRIGHT *******************************
 * File Name          : Main.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2020/08/06
 * Description 		    : 串口1收发演示
 *******************************************************************************/

#include "CH58x_common.h"

UINT8 TxBuff[] = "This is a tx exam\r\n";
UINT8 RxBuff[100];
UINT8 trigB;

int main()
{
  UINT8 len;

  SetSysClock( CLK_SOURCE_PLL_60MHz );

  /* 配置串口1：先配置IO口模式，再配置串口 */
  GPIOA_SetBits( GPIO_Pin_9 );
  GPIOA_ModeCfg( GPIO_Pin_8, GPIO_ModeIN_PU );			// RXD-配置上拉输入
  GPIOA_ModeCfg( GPIO_Pin_9, GPIO_ModeOut_PP_5mA );    // TXD-配置推挽输出，注意先让IO口输出高电平
  UART1_DefInit();

#if 1       // 测试串口发送字符串
  UART1_SendString( TxBuff, sizeof( TxBuff ) );

#endif   

#if 1       // 查询方式：接收数据后发送出去
  while( 1 )
  {
    len = UART1_RecvString( RxBuff );
    if ( len )
    {
      UART1_SendString( RxBuff, len );
    }
  }

#endif    

#if 0      // 中断方式：接收数据后发送出去
  UART1_ByteTrigCfg( UART_7BYTE_TRIG );
  trigB = 7;
  UART1_INTCfg( ENABLE, RB_IER_RECV_RDY|RB_IER_LINE_STAT );
  PFIC_EnableIRQ( UART1_IRQn );
#endif    

  while( 1 )
    ;
}

__INTERRUPT
__HIGH_CODE
void UART1_IRQHandler( void )
{
  UINT8V i;

  switch ( UART1_GetITFlag() )
  {
    case UART_II_LINE_STAT :        // 线路状态错误
    {
      UART1_GetLinSTA();
      break;
    }

    case UART_II_RECV_RDY :          // 数据达到设置触发点
      for ( i = 0; i != trigB; i++ )
      {
        RxBuff[i] = UART1_RecvByte();
        UART1_SendByte( RxBuff[i] );
      }
      break;

    case UART_II_RECV_TOUT :         // 接收超时，暂时一帧数据接收完成
      i = UART1_RecvString( RxBuff );
      UART1_SendString( RxBuff, i );
      break;

    case UART_II_THR_EMPTY :         // 发送缓存区空，可继续发送
      break;

    case UART_II_MODEM_CHG :         // 只支持串口0
      break;

    default :
      break;
  }

}


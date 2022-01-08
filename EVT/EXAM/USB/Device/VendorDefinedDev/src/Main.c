/********************************** (C) COPYRIGHT *******************************
 * File Name          : Main.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2020/08/06
 * Description        : 自定义USB设备（CH372设备），提供8个非0通道(上传+下传)，实现数据先下传，然后数据内容取反上传
 *******************************************************************************/

#include "CH58x_common.h"

#define DevEP0SIZE	0x40
// 设备描述符
const UINT8 MyDevDescr[] = { 0x12, 0x01, 0x10, 0x01, 0xFF, 0x80, 0x55, DevEP0SIZE, 0x48, 0x43, 0x37, 0x55,  	// 厂商ID和产品ID
                             0x00, 0x01, 0x01, 0x02, 0x00, 0x01, };
// 配置描述符
const UINT8 MyCfgDescr[] = { 0x09, 0x02, 0x4A, 0x00, 0x01, 0x01, 0x00, 0x80, 0x32, 0x09, 0x04, 0x00, 0x00, 0x08, 0xFF,
                             0x80, 0x55, 0x00, 0x07, 0x05, 0x84, 0x02, 0x40, 0x00, 0x00, 0x07, 0x05, 0x04, 0x02, 0x40,
                             0x00, 0x00, 0x07, 0x05, 0x83, 0x02, 0x40, 0x00, 0x00, 0x07, 0x05, 0x03, 0x02, 0x40, 0x00,
                             0x00, 0x07, 0x05, 0x82, 0x02, 0x40, 0x00, 0x00, 0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00,
                             0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00, 0x07, 0x05, 0x01, 0x02, 0x40, 0x00, 0x00, };
// 语言描述符
const UINT8 MyLangDescr[] = { 0x04, 0x03, 0x09, 0x04 };
// 厂家信息
const UINT8 MyManuInfo[] = { 0x0E, 0x03, 'w', 0, 'c', 0, 'h', 0, '.', 0, 'c', 0, 'n', 0 };
// 产品信息
const UINT8 MyProdInfo[] = { 0x0C, 0x03, 'C', 0, 'H', 0, '5', 0, '7', 0, 'x', 0 };

/**********************************************************/
UINT8 DevConfig;
UINT8 SetupReqCode;
UINT16 SetupReqLen;
const UINT8 *pDescr;

/******** 用户自定义分配端点RAM ****************************************/
__attribute__((aligned(4)))  UINT8 EP0_Databuf[64 + 64 + 64];    //ep0(64)+ep4_out(64)+ep4_in(64)
__attribute__((aligned(4)))  UINT8 EP1_Databuf[64 + 64];    //ep1_out(64)+ep1_in(64)
__attribute__((aligned(4)))  UINT8 EP2_Databuf[64 + 64];    //ep2_out(64)+ep2_in(64)
__attribute__((aligned(4)))  UINT8 EP3_Databuf[64 + 64];    //ep3_out(64)+ep3_in(64)

void USB_DevTransProcess( void )
{
  UINT8 len, chtype;
  UINT8 intflag, errflag = 0;

  intflag = R8_USB_INT_FG;
  if ( intflag & RB_UIF_TRANSFER )
  {
    if ( ( R8_USB_INT_ST & MASK_UIS_TOKEN ) != MASK_UIS_TOKEN )     // 非空闲
    {
      switch ( R8_USB_INT_ST & ( MASK_UIS_TOKEN | MASK_UIS_ENDP ) )
      // 分析操作令牌和端点号
      {
        case UIS_TOKEN_IN :
        {
          switch ( SetupReqCode )
          {
            case USB_GET_DESCRIPTOR :
              len = SetupReqLen >= DevEP0SIZE ?
                  DevEP0SIZE : SetupReqLen;    // 本次传输长度
              memcpy( pEP0_DataBuf, pDescr, len ); /* 加载上传数据 */
              SetupReqLen -= len;
              pDescr += len;
              R8_UEP0_T_LEN = len;
              R8_UEP0_CTRL ^= RB_UEP_T_TOG;                             // 翻转
              break;
            case USB_SET_ADDRESS :
              R8_USB_DEV_AD = ( R8_USB_DEV_AD & RB_UDA_GP_BIT ) | SetupReqLen;
              R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
              break;
            default :
              R8_UEP0_T_LEN = 0;                                      // 状态阶段完成中断或者是强制上传0长度数据包结束控制传输
              R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
              break;
          }
        }
          break;

        case UIS_TOKEN_OUT :
          len = R8_USB_RX_LEN;
          break;

        case UIS_TOKEN_OUT | 1 :
        {
          if ( R8_USB_INT_ST & RB_UIS_TOG_OK )
          {                       // 不同步的数据包将丢弃
            len = R8_USB_RX_LEN;
            DevEP1_OUT_Deal( len );
          }
        }
          break;

        case UIS_TOKEN_IN | 1 :
          R8_UEP1_CTRL = ( R8_UEP1_CTRL & ~MASK_UEP_T_RES ) | UEP_T_RES_NAK;
          break;

        case UIS_TOKEN_OUT | 2 :
        {
          if ( R8_USB_INT_ST & RB_UIS_TOG_OK )
          {                       // 不同步的数据包将丢弃
            len = R8_USB_RX_LEN;
            DevEP2_OUT_Deal( len );
          }
        }
          break;

        case UIS_TOKEN_IN | 2 :
          R8_UEP2_CTRL = ( R8_UEP2_CTRL & ~MASK_UEP_T_RES ) | UEP_T_RES_NAK;
          break;

        case UIS_TOKEN_OUT | 3 :
        {
          if ( R8_USB_INT_ST & RB_UIS_TOG_OK )
          {                       // 不同步的数据包将丢弃
            len = R8_USB_RX_LEN;
            DevEP3_OUT_Deal( len );
          }
        }
          break;

        case UIS_TOKEN_IN | 3 :
          R8_UEP3_CTRL = ( R8_UEP3_CTRL & ~MASK_UEP_T_RES ) | UEP_T_RES_NAK;
          break;

        case UIS_TOKEN_OUT | 4 :
        {
          if ( R8_USB_INT_ST & RB_UIS_TOG_OK )
          {
            R8_UEP4_CTRL ^= RB_UEP_R_TOG;
            len = R8_USB_RX_LEN;
            DevEP4_OUT_Deal( len );
          }
        }
          break;

        case UIS_TOKEN_IN | 4 :
          R8_UEP4_CTRL ^= RB_UEP_T_TOG;
          R8_UEP4_CTRL = ( R8_UEP4_CTRL & ~MASK_UEP_T_RES ) | UEP_T_RES_NAK;
          break;

        default :
          break;
      }
      R8_USB_INT_FG = RB_UIF_TRANSFER;
    }

    if ( R8_USB_INT_ST & RB_UIS_SETUP_ACT )                  // Setup包处理
    {
      R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
      SetupReqLen = pSetupReqPak->wLength;
      SetupReqCode = pSetupReqPak->bRequest;
      chtype = pSetupReqPak->bRequestType;

      len = 0;
      errflag = 0;
      if ( ( pSetupReqPak->bRequestType & USB_REQ_TYP_MASK ) != USB_REQ_TYP_STANDARD )
      {
        errflag = 0xFF; /* 非标准请求 */
      }
      else /* 标准请求 */
      {
        switch ( SetupReqCode )
        {
          case USB_GET_DESCRIPTOR :
          {
            switch ( ( ( pSetupReqPak->wValue ) >> 8 ) )
            {
              case USB_DESCR_TYP_DEVICE :
              {
                pDescr = MyDevDescr;
                len = MyDevDescr[0];
              }
                break;

              case USB_DESCR_TYP_CONFIG :
              {
                pDescr = MyCfgDescr;
                len = MyCfgDescr[2];
              }
                break;

              case USB_DESCR_TYP_STRING :
              {
                switch ( ( pSetupReqPak->wValue ) & 0xff )
                {
                  case 1 :
                    pDescr = MyManuInfo;
                    len = MyManuInfo[0];
                    break;
                  case 2 :
                    pDescr = MyProdInfo;
                    len = MyProdInfo[0];
                    break;
                  case 0 :
                    pDescr = MyLangDescr;
                    len = MyLangDescr[0];
                    break;
                  default :
                    errflag = 0xFF;                               // 不支持的字符串描述符
                    break;
                }
              }
                break;

              default :
                errflag = 0xff;
                break;
            }
            if ( SetupReqLen > len )
              SetupReqLen = len;      //实际需上传总长度
            len = ( SetupReqLen >= DevEP0SIZE ) ?
                DevEP0SIZE : SetupReqLen;
            memcpy( pEP0_DataBuf, pDescr, len );
            pDescr += len;
          }
            break;

          case USB_SET_ADDRESS :
            SetupReqLen = ( pSetupReqPak->wValue ) & 0xff;
            break;

          case USB_GET_CONFIGURATION :
            pEP0_DataBuf[0] = DevConfig;
            if ( SetupReqLen > 1 )
              SetupReqLen = 1;
            break;

          case USB_SET_CONFIGURATION :
            DevConfig = ( pSetupReqPak->wValue ) & 0xff;
            break;

          case USB_CLEAR_FEATURE :
          {
            if ( ( pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP )    // 端点
            {
              switch ( ( pSetupReqPak->wIndex ) & 0xff )
              {
                case 0x82 :
                  R8_UEP2_CTRL = ( R8_UEP2_CTRL & ~( RB_UEP_T_TOG | MASK_UEP_T_RES ) ) | UEP_T_RES_NAK;
                  break;
                case 0x02 :
                  R8_UEP2_CTRL = ( R8_UEP2_CTRL & ~( RB_UEP_R_TOG | MASK_UEP_R_RES ) ) | UEP_R_RES_ACK;
                  break;
                case 0x81 :
                  R8_UEP1_CTRL = ( R8_UEP1_CTRL & ~( RB_UEP_T_TOG | MASK_UEP_T_RES ) ) | UEP_T_RES_NAK;
                  break;
                case 0x01 :
                  R8_UEP1_CTRL = ( R8_UEP1_CTRL & ~( RB_UEP_R_TOG | MASK_UEP_R_RES ) ) | UEP_R_RES_ACK;
                  break;
                default :
                  errflag = 0xFF;                                 // 不支持的端点
                  break;
              }
            }
            else
              errflag = 0xFF;
          }
            break;

          case USB_GET_INTERFACE :
            pEP0_DataBuf[0] = 0x00;
            if ( SetupReqLen > 1 )
              SetupReqLen = 1;
            break;

          case USB_GET_STATUS :
            pEP0_DataBuf[0] = 0x00;
            pEP0_DataBuf[1] = 0x00;
            if ( SetupReqLen > 2 )
              SetupReqLen = 2;
            break;

          default :
            errflag = 0xff;
            break;
        }
      }
      if ( errflag == 0xff )        // 错误或不支持
      {
//                  SetupReqCode = 0xFF;
        R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;    // STALL
      }
      else
      {
        if ( chtype & 0x80 )     // 上传
        {
          len = ( SetupReqLen > DevEP0SIZE ) ?
              DevEP0SIZE : SetupReqLen;
          SetupReqLen -= len;
        }
        else
          len = 0;        // 下传
        R8_UEP0_T_LEN = len;
        R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;    // 默认数据包是DATA1
      }

      R8_USB_INT_FG = RB_UIF_TRANSFER;
    }
  }
  else if ( intflag & RB_UIF_BUS_RST )
  {
    R8_USB_DEV_AD = 0;
    R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
    R8_UEP1_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
    R8_UEP2_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
    R8_UEP3_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
    R8_USB_INT_FG = RB_UIF_BUS_RST;
  }
  else if ( intflag & RB_UIF_SUSPEND )
  {
    if ( R8_USB_MIS_ST & RB_UMS_SUSPEND )
    {
      ;
    }    // 挂起
    else
    {
      ;
    }								// 唤醒
    R8_USB_INT_FG = RB_UIF_SUSPEND;
  }
  else
  {
    R8_USB_INT_FG = intflag;
  }
}

void DebugInit( void )
{
  GPIOA_SetBits( GPIO_Pin_9 );
  GPIOA_ModeCfg( GPIO_Pin_8, GPIO_ModeIN_PU );
  GPIOA_ModeCfg( GPIO_Pin_9, GPIO_ModeOut_PP_5mA );
  UART1_DefInit();
}

int main()
{
  SetSysClock( CLK_SOURCE_PLL_60MHz );

  DebugInit();
  printf( "start\n" );

  pEP0_RAM_Addr = EP0_Databuf;
  pEP1_RAM_Addr = EP1_Databuf;
  pEP2_RAM_Addr = EP2_Databuf;
  pEP3_RAM_Addr = EP3_Databuf;
  USB_DeviceInit();
  PFIC_EnableIRQ( USB_IRQn );

  while( 1 )
    ;
}

void DevEP1_OUT_Deal( UINT8 l )
{ /* 用户可自定义 */
  UINT8 i;

  for ( i = 0; i < l; i++ )
  {
    pEP1_IN_DataBuf[i] = ~pEP1_OUT_DataBuf[i];
  }
  DevEP1_IN_Deal( l );
}

void DevEP2_OUT_Deal( UINT8 l )
{ /* 用户可自定义 */
  UINT8 i;

  for ( i = 0; i < l; i++ )
  {
    pEP2_IN_DataBuf[i] = ~pEP2_OUT_DataBuf[i];
  }
  DevEP2_IN_Deal( l );
}

void DevEP3_OUT_Deal( UINT8 l )
{ /* 用户可自定义 */
  UINT8 i;

  for ( i = 0; i < l; i++ )
  {
    pEP3_IN_DataBuf[i] = ~pEP3_OUT_DataBuf[i];
  }
  DevEP3_IN_Deal( l );
}

void DevEP4_OUT_Deal( UINT8 l )
{ /* 用户可自定义 */
  UINT8 i;

  for ( i = 0; i < l; i++ )
  {
    pEP4_IN_DataBuf[i] = ~pEP4_OUT_DataBuf[i];
  }
  DevEP4_IN_Deal( l );
}

__INTERRUPT
__HIGH_CODE
void USB_IRQHandler( void ) /* USB中断服务程序,使用寄存器组1 */
{
  USB_DevTransProcess();
}


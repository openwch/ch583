/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : WCH
* Version            : V1.0
* Date               : 2020/08/06
* Description        : 
*******************************************************************************/



/******************************************************************************/
/* 头文件包含 */
#include "CONFIG.h"
#include "CH58x_common.h"
#include "RF_PHY.h"

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
uint8 taskID;
uint8 TX_DATA[300] ={1,2,3,4,5,6,7,8,9,0};


/*******************************************************************************
* Function Name  : RF_2G4StatusCallBack
* Description    : RF 状态回调，注意：不可在此函数中直接调用RF接收或者发送API，需要使用事件的方式调用
* Input          : sta - 状态类型
*                   crc - crc校验结果
*                   rxBuf - 数据buf指针
* Output         : None
* Return         : None
*******************************************************************************/
void RF_2G4StatusCallBack( uint8 sta , uint8 crc, uint8 *rxBuf )
{
  switch( sta )
	{
    case TX_MODE_TX_FINISH:
    {
      break;
    }
    case TX_MODE_TX_FAIL:
    {
      break;
    }		
    case TX_MODE_RX_DATA:
    {
      RF_Shut();
      if( crc == 1 )
			{
        PRINT("crc error\n");
      }
			else if( crc == 2 )
			{
        PRINT("match type error\n");
      }
 			else
			{
        uint8 i;      
        PRINT("tx recv,rssi:%d\n",(s8)rxBuf[0]);
        PRINT("len:%d-",rxBuf[1]);
        for(i=0;i<rxBuf[1];i++) PRINT("%x ",rxBuf[i+2]);
        PRINT("\n");
      }
      break;
    }
    case TX_MODE_RX_TIMEOUT:		// Timeout is about 200us
    {
      break;
    }		
    case RX_MODE_RX_DATA:
    {
      if( crc == 1 )
			{
        PRINT("crc error\n");
      }
			else if( crc == 2 )
			{
        PRINT("match type error\n");
      }
			else
      {
        uint8 i;      
        PRINT("rx recv, rssi: %d\n",(s8)rxBuf[0]);
        PRINT("len: %d-",rxBuf[1]);
        for(i=0;i<rxBuf[1];i++) PRINT("%x ",rxBuf[i+2]);
        PRINT("\n");
      }
      tmos_set_event(taskID, SBP_RF_RF_RX_EVT);
      break;
    }
    case RX_MODE_TX_FINISH:
    {
      tmos_set_event(taskID, SBP_RF_RF_RX_EVT);
      break;
    }
    case RX_MODE_TX_FAIL:
    {
      break;
    }		
  }
  PRINT("STA: %x\n",sta);
}


/*******************************************************************************
* Function Name  : RF_ProcessEvent
* Description    : RF 事件处理
* Input          : task_id - 任务ID
*                   events - 事件标志
* Output         : None
* Return         : None
*******************************************************************************/
uint16 RF_ProcessEvent( uint8 task_id, uint16 events )
{
  if ( events & SYS_EVENT_MSG )
  {
    uint8 *pMsg;

    if ( (pMsg = tmos_msg_receive( task_id )) != NULL )
    {
      // Release the TMOS message
      tmos_msg_deallocate( pMsg );
    }
    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }
  if( events & SBP_RF_START_DEVICE_EVT )
	{
    tmos_start_task( taskID , SBP_RF_PERIODIC_EVT ,1000 );
    return events^SBP_RF_START_DEVICE_EVT;
  }
  if ( events & SBP_RF_PERIODIC_EVT )
	{
    RF_Shut( );
    TX_DATA[0]++;

    RF_Tx( TX_DATA,TX_DATA[0], 0xFF, 0xFF );
    tmos_start_task( taskID , SBP_RF_PERIODIC_EVT ,160 );
    return events^SBP_RF_PERIODIC_EVT;
  }
  if( events & SBP_RF_RF_RX_EVT )
  {
    uint8 state;
    RF_Shut();
    TX_DATA[0]=100;
    state = RF_Rx( TX_DATA,TX_DATA[0], 0xFF, 0xFF );
    PRINT("RX mode.state = %x\n",state);
    return events^SBP_RF_RF_RX_EVT;
  }
  return 0;
}

/*******************************************************************************
* Function Name  : RF_Init
* Description    : RF 初始化
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void RF_Init( void )
{
  uint8 state;
  rfConfig_t rfConfig;

  tmos_memset( &rfConfig, 0, sizeof(rfConfig_t) );
  taskID = TMOS_ProcessEventRegister( RF_ProcessEvent );
  rfConfig.accessAddress = 0x8E89bed6;	// 禁止使用0x55555555以及0xAAAAAAAA ( 建议不超过24次位反转，且不超过连续的6个0或1 )
  rfConfig.CRCInit = 0x555555;
  rfConfig.Channel = 39;
  rfConfig.Frequency = 2480000;
  rfConfig.LLEMode = LLE_MODE_BASIC; // 使能 LLE_MODE_EX_CHANNEL 表示 选择 rfConfig.Frequency 作为通信频点
  rfConfig.rfStatusCB = RF_2G4StatusCallBack;
  state = RF_Config( &rfConfig );
  PRINT("rf 2.4g init: %x\n",state);
	{ // RX mode
		state = RF_Rx( TX_DATA,10, 0xFF, 0xFF );
		PRINT("RX mode.state = %x\n",state);
	}

//	{ // TX mode
//		tmos_set_event( taskID , SBP_RF_PERIODIC_EVT );
//	}
}

/******************************** endfile @ main ******************************/

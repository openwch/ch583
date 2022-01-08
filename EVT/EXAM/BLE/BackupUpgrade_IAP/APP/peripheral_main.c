/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2019/11/05
 * Description        : 外设从机应用主函数及任务系统初始化
 *******************************************************************************/

/******************************************************************************/
/* 头文件包含 */
#include "CH58x_common.h"
#include "OTA.h"

/* 记录当前的Image */
unsigned char CurrImageFlag = 0xff;

/* flash的数据临时存储 */
__attribute__((aligned(8))) UINT8 block_buf[16];


#define    jumpApp  ((  void  (*)  ( void ))  ((int*)IMAGE_A_START_ADD))



/*********************************************************************
 * GLOBAL TYPEDEFS
 */

/*******************************************************************************
* Function Name  : SwitchImageFlag
* Description    : 切换dataflash里的ImageFlag
* Input          : new_flag：切换的ImageFlag
* Output         : none
* Return         : none
*******************************************************************************/
void SwitchImageFlag(UINT8 new_flag)
{
  UINT16 i;
  UINT32  ver_flag;

  /* 读取第一块 */
  EEPROM_READ( OTA_DATAFLASH_ADD, (PUINT32) &block_buf[0], 4 );

  /* 擦除第一块 */
  EEPROM_ERASE(OTA_DATAFLASH_ADD,EEPROM_PAGE_SIZE);

  /* 更新Image信息 */
  block_buf[0] = new_flag;

  /* 编程DataFlash */
  EEPROM_WRITE(OTA_DATAFLASH_ADD, (PUINT32) &block_buf[0], 4);
}

/*******************************************************************************
 * Function Name  : jump_APP
 * Description    :
 * Input          : None
 * Return         : None
 *******************************************************************************/
void jump_APP( void )
{
  if( CurrImageFlag==IMAGE_IAP_FLAG )
  {
    __attribute__((aligned(8))) UINT8 flash_Data[1024];
    UINT8 i;
    FLASH_ROM_LOCK( 0 );                    // 解锁flash
    FLASH_ROM_ERASE( IMAGE_A_START_ADD, IMAGE_A_SIZE );
    for(i=0; i<IMAGE_A_SIZE/1024; i++)
    {
      FLASH_ROM_READ( IMAGE_B_START_ADD+(i*1024), flash_Data, 1024 );
      FLASH_ROM_WRITE(IMAGE_A_START_ADD+(i*1024),flash_Data,1024);
    }
    SwitchImageFlag( IMAGE_A_FLAG );
    // 销毁备份代码
    FLASH_ROM_ERASE( IMAGE_B_START_ADD, IMAGE_A_SIZE );
  }
  jumpApp();
}

/*******************************************************************************
 * Function Name  : ReadImageFlag
 * Description    : 读取当前的程序的Image标志，DataFlash如果为空，就默认是ImageA
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
void ReadImageFlag( void )
{
  OTADataFlashInfo_t p_image_flash;

  EEPROM_READ( OTA_DATAFLASH_ADD, &p_image_flash, 4 );
  CurrImageFlag = p_image_flash.ImageFlag;

  /* 程序第一次执行，或者没有更新过，以后更新后在擦除DataFlash */
  if ( (CurrImageFlag != IMAGE_A_FLAG) && (CurrImageFlag != IMAGE_B_FLAG) && (CurrImageFlag != IMAGE_IAP_FLAG) )
  {
    CurrImageFlag = IMAGE_A_FLAG;
  }

  PRINT( "Image Flag %02x\n", CurrImageFlag );
}

/*******************************************************************************
 * Function Name  : main
 * Description    : 主函数
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
int main( void )
{
#if (defined (DCDC_ENABLE)) && (DCDC_ENABLE == TRUE)
  PWR_DCDCCfg( ENABLE );
#endif
  SetSysClock( CLK_SOURCE_PLL_60MHz );
#if (defined (HAL_SLEEP)) && (HAL_SLEEP == TRUE)
  GPIOA_ModeCfg( GPIO_Pin_All, GPIO_ModeIN_PU );
  GPIOB_ModeCfg( GPIO_Pin_All, GPIO_ModeIN_PU );
#endif
#ifdef DEBUG
  GPIOA_SetBits( bTXD1 );
  GPIOA_ModeCfg( bTXD1, GPIO_ModeOut_PP_5mA );
  UART1_DefInit();
#endif   
  FLASH_ROM_LOCK(0);
  ReadImageFlag();
//  while(1);
  jump_APP();
}

/******************************** endfile @ main ******************************/

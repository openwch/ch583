#include "dfu_port.h"
#include "config.h"
#include "HAL_FLASH/include/easyflash.h"
#include "RingBuffer/lwrb.h"

static uint32_t currimageflag = 0;
__attribute__((aligned(4))) uint8_t buf[256];

static uint8_t DFU_state = 0;
static uint32_t irq_status;

/**
 * lock the DFU data ram cache
 */
void dfu_port_data_lock(void) {

    /* You can add your code under here. */
    SYS_DisableAllIrq(&irq_status);
}

/**
 * unlock the DFU data ram cache
 */
void dfu_port_data_unlock(void) {

    /* You can add your code under here. */
    SYS_RecoverIrq(irq_status);
}

bool get_currimageflag( void )
{
  bool ret = SUCCESS;

  if( ef_get_env_blob("image_flag", &currimageflag, sizeof(currimageflag), NULL ) == 0 )
  {
    ret = FAILURE;
  }
  LOG_INFO("get current image flag: %#x",currimageflag);
  return ret;
}

bool dfu_erase_port( uint32_t imagflag, uint32_t *param )
{
  bool ret = SUCCESS;

  switch( ~imagflag )  //擦除另一块
  {
    case IMAGE_A_FLAG:
    {
      dfu_port_data_lock();
      for( uint8_t i = 0; i<(IMAGE_SIZE/EEPROM_BLOCK_SIZE);i++ )
      {
        if( FLASH_ROM_ERASE( IMAGE_A_START_ADD+(i*4096), EEPROM_BLOCK_SIZE)!= SUCCESS)
        {
          *param = IMAGE_A_START_ADD+(i*4096);
          ret = FAILURE;
          break;
        }
      }
      dfu_port_data_unlock();

    }break;

    case IMAGE_B_FLAG:
    {
      dfu_port_data_lock();
      for( uint8_t i = 0; i<(IMAGE_SIZE/EEPROM_BLOCK_SIZE);i++ )
      {
        if( FLASH_ROM_ERASE( IMAGE_B_START_ADD+(i*4096), EEPROM_BLOCK_SIZE)!= SUCCESS)
        {
          *param = IMAGE_A_START_ADD+(i*4096);
          ret = FAILURE;
          break;
        }
      }
      dfu_port_data_unlock();
    }break;

    default:
    {
      ret = FAILURE;
    }break;
  }

  return ret;
}

bool dfu_wrtie_port( uint8_t *pdata, size_t len, bool enforce , uint32_t *param )
{
  static bool init_ok = false;
  static uint32_t curraddr = 0;
  bool ret = SUCCESS;
  size_t readlen;

  if( unlikely(!init_ok) )
  {
    lwrb_reset(&RF_OTAbuff );
    if(currimageflag == IMAGE_A_FLAG)
    {
      curraddr = IMAGE_B_START_ADD;
    }
    else {
      curraddr = IMAGE_A_START_ADD;
    }
    LOG_INFO("curraddr:%#x",curraddr);
    init_ok = true;
  }

  if( unlikely(enforce) )
  {
    readlen = lwrb_get_full(&RF_OTAbuff);
    if( readlen%4 )
    {
      *param = curraddr;
      ret = FAILURE;
    }
    lwrb_read(&RF_OTAbuff, buf, readlen);
    dfu_port_data_lock();
    if( unlikely(FLASH_ROM_WRITE(curraddr, (PVOID)buf, readlen) != SUCCESS) )
    {
      *param = curraddr;
      ret = FAILURE;
    }
    dfu_port_data_unlock();
    LOG_INFO("write@%#x: %d", curraddr, readlen );
    curraddr += readlen;
    return ret;
  }

  lwrb_write(&RF_OTAbuff, pdata, len);
  readlen = lwrb_get_full(&RF_OTAbuff);
  if( readlen >= 256 )
  {
    lwrb_read(&RF_OTAbuff, buf, 256);
    dfu_port_data_lock();
    if(FLASH_ROM_WRITE(curraddr, (PVOID)buf, 256) != SUCCESS )
    {
      *param = curraddr;
      ret = FAILURE;
    }
    dfu_port_data_unlock();
    LOG_INFO("write@%#x: %d", curraddr, readlen );
    curraddr += 256;
  }
  return ret;
}

bool dfu_verify_port( uint8_t *pdata, size_t len, bool enforce, uint32_t *param )
{
  static bool init_ok = false;
  static uint32_t curraddr = 0;
  bool ret = SUCCESS;
  size_t readlen;

  if( unlikely(!init_ok) )
  {
    lwrb_reset(&RF_OTAbuff );
    if(currimageflag == IMAGE_A_FLAG)
    {
      curraddr = IMAGE_B_START_ADD;
    }
    else {
      curraddr = IMAGE_A_START_ADD;
    }
    init_ok = true;
  }

  if( unlikely(enforce) )
  {
    readlen = lwrb_get_full(&RF_OTAbuff);
    if( readlen%4 )
    {
      *param = curraddr;
      ret = FAILURE;
    }
    lwrb_read(&RF_OTAbuff, buf, readlen);
    if( unlikely(FLASH_ROM_VERIFY(curraddr, (PVOID)buf, readlen) != SUCCESS) )
    {
      *param = curraddr;
      ret = FAILURE;
    }
    LOG_INFO("verify@%#x: %d", curraddr, readlen );
    curraddr += readlen;
    return ret;
  }

  lwrb_write(&RF_OTAbuff, pdata, len);
  readlen = lwrb_get_full(&RF_OTAbuff);
  if( readlen >= 256 )
  {
    lwrb_read(&RF_OTAbuff, buf, 256);
    dfu_port_data_lock();
    if(FLASH_ROM_VERIFY(curraddr, (PVOID)buf, 256) != SUCCESS )
    {
      *param = curraddr;
      ret = FAILURE;
    }
    dfu_port_data_unlock();
    LOG_INFO("verify@%#x: %d", curraddr, readlen );
    curraddr += 256;
  }
  return ret;
}


DFUErrCode dfu_data_deal( DFU_DATA_t *pdata ,uint32_t *value )
{
  DFUErrCode ret = DFU_NO_ERR;

  switch( pdata->cmd )
  {
    case DFU_READ:
    {
      if( (pdata->len != DFU_CMD_LEN) || (pdata->idex != 0) )
      {
        ret = DFU_READ_ERR;
        DFU_state = 0;
        break;
      }
      LOG_INFO("DFU READ CMD");
      if( get_currimageflag() != SUCCESS )
      {
        ret = DFU_READ_ERR;
        DFU_state = 0;
        break;
      }
      DFU_state = ISREADIMAGE;        //读image清零
      *value = currimageflag;

    }break;

    case DFU_ERASE:
    {
      if( (pdata->len != DFU_CMD_LEN) || (pdata->idex != 0) )
      {
        ret = DFU_ERASE_ERR;
        DFU_state = 0;
        break;
      }
      LOG_INFO("DFU ERASE CMD");
      if( unlikely(dfu_erase_port( currimageflag , value ) != SUCCESS))
      {
        ret = DFU_ERASE_ERR;
        DFU_state = 0;
        break;
      }
      DFU_state |= ISERASE;
    }break;

    case DFU_WRITE:
    {
      static uint16_t index_old = 0;

      if( unlikely((pdata->len <= 4) || (pdata->idex == 0)) )
      {
        LOG_INFO("write len error");
        DFU_state = 0;
        ret = DFU_WRITE_ERR;
        break;
      }
      LOG_INFO("DFU WEITE CMD");
      if( unlikely(pdata->idex - index_old != 1) )
      {
        LOG_INFO("write index error");
        DFU_state = 0;
        ret = DFU_WRITE_ERR;
        break;
      }
      if( unlikely( dfu_wrtie_port( pdata->data, (pdata->len - DFU_CMD_LEN), 0, value) != SUCCESS) )
      {
        LOG_INFO("write prot error");
        DFU_state = 0;
        ret = DFU_WRITE_ERR;
        break;
      }
      DFU_state |= ISWRITE;
      index_old = pdata->idex;

    }break;

    case DFU_WRITE_END:
    {
      if( unlikely((pdata->len != DFU_CMD_LEN) || (pdata->idex != 0)) )
      {
        ret = DFU_WRITE_END_ERR;
        DFU_state = 0;
        break;
      }
      LOG_INFO("DFU WRITE END CMD");
      if( unlikely( dfu_wrtie_port( 0, 0, 1, value) != SUCCESS) )
      {
        ret = DFU_WRITE_END_ERR;
        DFU_state = 0;
        break;
      }
      DFU_state |= ISWRITEEND;

    }break;

    case DFU_VERIFY:
    {
      static uint16_t index_old = 0;
      if( unlikely((pdata->len < 4) || (pdata->idex == 0)) )
      {
        ret = DFU_VERIFY_ERR;
        DFU_state = 0;
        break;
      }
      LOG_INFO("DFU VERIFY CMD");
      if( unlikely(pdata->idex - index_old != 1) )
      {
        LOG_INFO("verify index error");
        DFU_state = 0;
        ret = DFU_VERIFY_ERR;
        break;
      }
      index_old = pdata->idex;
      if( unlikely( dfu_verify_port( pdata->data, (pdata->len - DFU_CMD_LEN), 0, value) != SUCCESS) )
      {
        LOG_INFO("verify port error");
        DFU_state = 0;
        ret = DFU_VERIFY_ERR;
        break;
      }
      DFU_state |= ISVERIFY;

    }break;

    case DFU_VERIFY_END:
    {
      if( unlikely((pdata->len != DFU_CMD_LEN) || (pdata->idex != 0)) )
      {
        ret = DFU_VERIFY_END_ERR;
        DFU_state = 0;
        break;
      }
      LOG_INFO("DFU VERIFY END CMD");
      if( unlikely( dfu_verify_port( 0, 0, 1, value) != SUCCESS) )
      {
        ret = DFU_VERIFY_END_ERR;
        DFU_state = 0;
        break;
      }
      DFU_state |= ISVERIFYEND;
      if( DFU_state != 0x3f )  // 是否经过全步骤
      {
        ret = DFU_VERIFY_END_ERR;
        DFU_state = 0;
        break;
      }
      uint32_t image_flag = ~currimageflag;
      if( ef_set_env_blob("image_flag", &(image_flag), sizeof(image_flag)) != SUCCESS)
      {
        ret = DFU_VERIFY_END_ERR;
        DFU_state = 0;
        break;
      }
      DFU_state = 0;
      SYS_ResetExecute();

    }break;

    default:
    {
      ret = DFU_CMD_ERR;
    }break;
  }
  return ret;

}

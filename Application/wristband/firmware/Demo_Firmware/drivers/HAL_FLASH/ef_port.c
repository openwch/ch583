/*
 * This file is part of the EasyFlash Library.
 *
 * Copyright (c) 2015-2019, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2015-01-16
 */

#include <HAL_FLASH/include/easyflash.h>
#include <stdarg.h>
#include "stdio.h"
#include "string.h"
#include "CH58x_common.h"
#include "HAL/config.h"

static char log_buf[128];
static uint32_t irq_status;


/* default environment variables set for user */
 static  ef_env default_env_set[] = {
        {"version","v0.1"},
        {"boot_times","0"},
        {"MAC",&MacAddr,sizeof(MacAddr)},

};

/**
 * Flash port for hardware initialize.
 *
 * @param default_env default ENV set for user
 * @param default_env_size default ENV size
 *
 * @return result
 */
EfErrCode ef_port_init(ef_env  **default_env, size_t *default_env_size) {
    EfErrCode result = EF_NO_ERR;

    *default_env = default_env_set;
    *default_env_size = sizeof(default_env_set) / sizeof(default_env_set[0]);

    return result;
}

/**
 * Read data from flash.
 * @note This operation's units is word.
 *
 * @param addr flash address
 * @param buf buffer to store read data
 * @param size read bytes size
 *
 * @return result
 */
EfErrCode ef_port_read(uint32_t addr, uint32_t *buf, size_t size) {
    EfErrCode read_result = EF_NO_ERR;

    read_result = EEPROM_READ(addr-0x070000, buf, size);
    EF_ASSERT( read_result == 0 );

    return read_result;
}

/**
 * Erase data on flash.
 * @note This operation is irreversible.
 * @note This operation's units is different which on many chips.
 *
 * @param addr flash address
 * @param size erase bytes size
 *
 * @return result
 */
EfErrCode ef_port_erase(uint32_t addr, size_t size) {
  EfErrCode ef_port_erase_result = EF_NO_ERR;
  size_t erase_pages, i;

  /* make sure the start address is a multiple of FLASH_ERASE_MIN_SIZE */
  EF_ASSERT(addr % EF_ERASE_MIN_SIZE == 0);

  /* calculate pages */
  erase_pages = size / PAGE_SIZE;
  if (size % PAGE_SIZE != 0)
  {
      erase_pages++;
  }

  /* start erase */
//    FLASH_Unlock();
  for(i = 0; i< erase_pages; i++)
  {
    ef_port_erase_result = EEPROM_ERASE(addr - 0x00070000 + (PAGE_SIZE * i), EF_ERASE_MIN_SIZE);
    EF_ASSERT(ef_port_erase_result == EF_NO_ERR);
    if( ef_port_erase_result != EF_NO_ERR )
    {
      ef_port_erase_result = EF_ERASE_ERR;
      break;
    }
  }

    return ef_port_erase_result;
}
/**
 * Write data to flash.
 * @note This operation's units is word.
 * @note This operation must after erase. @see flash_erase.
 *
 * @param addr flash address
 * @param buf the write data buffer
 * @param size write bytes size
 *
 * @return result
 */
EfErrCode ef_port_write(uint32_t addr, const uint32_t *buf, size_t size) {
  EfErrCode result = EF_NO_ERR;
  EfErrCode write_result = EF_NO_ERR;
  EfErrCode verify_result = EF_NO_ERR;

  __attribute__((aligned(4))) uint8_t read_data[4];
  __attribute__((aligned(4))) uint8_t write_data[4];
  uint8_t i;
  uint32_t write_addr;

  write_addr = addr-0x070000;

  if((uint32_t)buf >= 0x20000000) {
      write_result = EEPROM_WRITE(write_addr, (PVOID)buf, size);
      EF_ASSERT(write_result == 0);

      for(i = 0; i<size; i+=4, write_addr+=4, buf++ )
      {
        EEPROM_READ(write_addr, read_data, 4 );
        verify_result = memcmp(read_data, buf, 4);
        EF_ASSERT(verify_result == 0);
      }
  } else {
      for(i = 0; i<size; i+=4, write_addr+=4, buf++ )
      {
        memcpy(write_data, buf, 4);
        write_result = EEPROM_WRITE(write_addr, (PVOID)write_data, 4);
        EF_ASSERT(write_result == 0);

        EEPROM_READ(write_addr, read_data, 4 );
        verify_result = memcmp(read_data, write_data, 4);
        EF_ASSERT(verify_result == 0);
      }
  }

  if( verify_result != EF_NO_ERR || write_result != EF_NO_ERR )
  {
    result = EF_WRITE_ERR;
  }
  return result;
}

/**
 * lock the ENV ram cache
 */
void ef_port_env_lock(void) {
    
    /* You can add your code under here. */
    SYS_DisableAllIrq(&irq_status);
}

/**
 * unlock the ENV ram cache
 */
void ef_port_env_unlock(void) {
    
    /* You can add your code under here. */
    SYS_RecoverIrq(irq_status);
}


/**
 * This function is print flash debug info.
 *
 * @param file the file which has call this function
 * @param line the line number which has call this function
 * @param format output format
 * @param ... args
 *
 */
void ef_log_debug(const char *file, const long line, const char *format, ...) {

#ifdef DEBUG

    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    ef_print("[LOG](%s:%ld) ", file, line);
    /* must use vprintf to print */
    vsprintf(log_buf, format, args);
    ef_print("%s", log_buf);
    printf("\n");
    va_end(args);

#endif

}

/**
 * This function is print flash routine info.
 *
 * @param format output format
 * @param ... args
 */
void ef_log_info(const char *format, ...) {
  va_list args;

  /* args point to the first variable parameter */
  va_start(args, format);
  ef_print("[LOG]");
  /* must use vprintf to print */
  vsprintf(log_buf, format, args);
  ef_print("%s", log_buf);
  printf("\n");
  va_end(args);
}
/**
 * This function is print flash non-package info.
 *
 * @param format output format
 * @param ... args
 */
void ef_print(const char *format, ...) {
  va_list args;

  /* args point to the first variable parameter */
  va_start(args, format);
  /* must use vprintf to print */
  vsprintf(log_buf, format, args);
  printf("%s", log_buf);
  va_end(args);
}

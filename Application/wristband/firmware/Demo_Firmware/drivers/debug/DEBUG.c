#include <stdlib.h>
#include <stdarg.h>
#include "DEBUG.h"
#include "CH58x_common.h"
#include "HAL/config.h"

static char log_buf[128];
/**
 * This function is print flash debug info.
 *
 * @param file the file which has call this function
 * @param line the line number which has call this function
 * @param format output format
 * @param ... args
 *
 */
void log_debug(const char *file, const long line, const char *format, ...)
{

#ifdef DEBUG

    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    log_print("[LOG](%s:%ld) ", file, line);
    /* must use vprintf to print */
    vsprintf(log_buf, format, args);
    log_print("%s", log_buf);
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
void log_info(const char *format, ...)
{
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    log_print("[LOG]");
    /* must use vprintf to print */
    vsprintf(log_buf, format, args);
    log_print("%s", log_buf);
    printf("\n");
    va_end(args);
}
/**
 * This function is print flash non-package info.
 *
 * @param format output format
 * @param ... args
 */
void log_print(const char *format, ...)
{
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    /* must use vprintf to print */
    vsprintf(log_buf, format, args);
    printf("%s", log_buf);
    va_end(args);
}

void Device_Info( void )
{

  LOG_INFO( "R8_RESET_STATUS:0x%02x",R8_RESET_STATUS );
  LOG_INFO("FREQ_SYS:%d",GetSysClock());
#ifdef __CONFIG_H
#if ( BLE_MAC == 0 )
  __attribute__((aligned(4))) uint8_t MACADDR[6];
  uint8_t i;
  GetMACAddress( MACADDR );
  LOG_INFO("BLE_MAC:");
  for(i = 0; i<6; i++)
  {
      PRINT(" %02x",MACADDR[i]);
  }PRINT("\n");
#else
  uint8_t i;
  LOG_INFO("BLE_MAC:");
  for(i = 0;i<(sizeof(MacAddr)); i++)
  {
    PRINT(" %02x",MacAddr[i]);
  }PRINT("\n");
#endif

  LOG_INFO("%s",VER_LIB);

#ifdef DCDC_ENABLE
  LOG_INFO("DCDC_ENABLE:%d",DCDC_ENABLE);
#else
  LOG_INFO("DCDC_ENABLE ERROR");
#endif

#ifdef HAL_SLEEP
  LOG_INFO("HAL_SLEEP:%d",HAL_SLEEP);
#else
  LOG_INFO("HAL_SLEEP ERROR");
#endif

#ifdef CLK_OSC32K
  LOG_INFO("CLK_OSC32K:%d",CLK_OSC32K);
#else
  LOG_INFO("CLK_OSC32K ERROR");
#endif

#ifdef BLE_SNV
  LOG_INFO("BLE_SNV:%d",BLE_SNV);
#else
  LOG_INFO("BLE_SNV ERROR");
#endif

#ifdef BLE_SNV_ADDR
  LOG_INFO("BLE_SNV_ADDR:0x%08x",BLE_SNV_ADDR);
#else
  LOG_INFO("BLE_SNV_ADDR ERROR");
#endif

#ifdef BLE_TX_POWER
  LOG_INFO("BLE_TX_POWER:0x%02x",BLE_TX_POWER);
#else
  LOG_INFO("BLE_TX_POWER ERROR");
#endif

  LOG_ASSERT( MEM_BUF );

#if BLE_MEMHEAP_SIZE < 4 * 1024
#error "BLE_MEMHE6AP_SIZE config error!"
#endif

#endif
}





void DEBUG_Init( void )
{
  LOG_INFO("=======================Device Info start=====================");

  Device_Info();

  LOG_INFO("=======================Device Info end=======================");
}

void _init(void) {

}
void _fini(void) {

}

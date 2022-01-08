#include "ADC.h"
#include "config.h"
#include "CH58x_common.h"

/* ------------------------------------------------------------------------------------------------
 *                                       Local Variables
 * ------------------------------------------------------------------------------------------------
 */

#if (HAL_ADC == TRUE)
const uint32_t adc_channel2pin[] = {
        GPIO_Pin_4, GPIO_Pin_5, GPIO_Pin_12, GPIO_Pin_13, GPIO_Pin_14, /*AIN0~AIN4*/
        GPIO_Pin_15, GPIO_Pin_3, GPIO_Pin_2, GPIO_Pin_1, GPIO_Pin_0,   /*AIN5~AIN9*/
        GPIO_Pin_6, GPIO_Pin_7, GPIO_Pin_8, GPIO_Pin_9                 /*AIN10~AIN13*/
};
#endif

/**************************************************************************************************
 * @fn      HalAdcInit
 *
 * @brief   Initialize ADC Service
 *
 * @param   None
 *
 * @return  None
 **************************************************************************************************/
void HalAdcInit (void)
{
#if (HAL_ADC == TRUE)

  ADC_ExtSingleChSampInit( SampleFreq_8, ADC_PGA_1_2 );

#endif
}

/**************************************************************************************************
 * @fn      HalAdcRead
 *
 * @brief   Read the ADC based on given channel and resolution
 *
 * @param   channel - channel where ADC will be read
 * @param   gain - the gain of adc
 *
 * @return  16 bit value of the ADC in offset binary format.
 *
 **************************************************************************************************/
uint16_t HalAdcRead (uint8_t channel, ADC_SignalPGATypeDef gain)
{
    uint16_t  reading = 0;

#if (HAL_ADC == TRUE)

  /*
   * If Analog input channel is AIN0..AIN7, make sure corresponing P0 I/O pin is enabled.  The code
   * does NOT disable the pin at the end of this function.  I think it is better to leave the pin
   * enabled because the results will be more accurate.  Because of the inherent capacitance on the
   * pin, it takes time for the voltage on the pin to charge up to its steady-state level.  If
   * HalAdcRead() has to turn on the pin for every conversion, the results may show a lower voltage
   * than actuality because the pin did not have time to fully charge.
   */
  GPIOA_ModeCfg(adc_channel2pin[channel], GPIO_ModeIN_Floating);

  /* Adjust gain */
  R8_ADC_CFG |= gain<<4;

  /* Enable channel */
  R8_ADC_CHANNEL = channel;

uint32_t readingsum = 0;
  for(int i = 0; i < 16; i++){
     readingsum += ADC_ExcutSingleConver();
  }
  reading = (readingsum+8)>>4;

  GPIOA_ModeCfg(adc_channel2pin[channel], GPIO_ModeIN_PU);
#else
  // unused arguments
  (void) channel;
  (void) resolution;
#endif

  return ((uint16_t)reading);
}



/**************************************************************************************************
**************************************************************************************************/

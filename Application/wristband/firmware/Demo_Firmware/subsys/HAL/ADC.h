#ifndef _ADC_H
#define _ADC_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 * INCLUDES
 **************************************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "CH58x_common.h"
/**************************************************************************************************
 * CONSTANTS
 **************************************************************************************************/

/* Resolution */

/* Channels */
#define HAL_ADC_CHANNEL_0          0x00
#define HAL_ADC_CHANNEL_1          0x01
#define HAL_ADC_CHANNEL_2          0x02
#define HAL_ADC_CHANNEL_3          0x03
#define HAL_ADC_CHANNEL_4          0x04
#define HAL_ADC_CHANNEL_5          0x05
#define HAL_ADC_CHANNEL_6          0x06
#define HAL_ADC_CHANNEL_7          0x07
#define HAL_ADC_CHANNEL_8          0x08
#define HAL_ADC_CHANNEL_9          0x09
#define HAL_ADC_CHANNEL_10         0x0a
#define HAL_ADC_CHANNEL_11         0x0b
#define HAL_ADC_CHANNEL_12         0x0c
#define HAL_ADC_CHANNEL_13         0x0d

#define HAL_ADC_CHANNEL_BAT        0x0e
#define HAL_ADC_CHANNEL_TEMP       0x0f


/**************************************************************************************************
 *                                        FUNCTIONS - API
 **************************************************************************************************/

/*
 * Initialize ADC Service with reference set to default value
 */
extern void HalAdcInit ( void );

/*
 * Read value from a specified ADC Channel at the given gain
 */
extern uint16_t HalAdcRead ( uint8_t channel, ADC_SignalPGATypeDef gain );


/**************************************************************************************************
**************************************************************************************************/

#ifdef __cplusplus
}
#endif

#endif

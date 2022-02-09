/********************************** (C) COPYRIGHT *******************************
 * File Name          : app_drv_fifo.h
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2022/01/19
 * Description        :
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

#ifndef __APP_DRV_FIFO_H__
#define __APP_DRV_FIFO_H__

#include <stdbool.h>
#include <stdint.h>

#ifndef BV
  #define BV(n)    (1 << (n))
#endif

#ifndef BF
  #define BF(x, b, s)    (((x) & (b)) >> (s))
#endif

#ifndef MIN
  #define MIN(n, m)    (((n) < (m)) ? (n) : (m))
#endif

#ifndef MAX
  #define MAX(n, m)    (((n) < (m)) ? (m) : (n))
#endif

#ifndef ABS
  #define ABS(n)    (((n) < 0) ? -(n) : (n))
#endif

typedef enum
{
    APP_DRV_FIFO_RESULT_SUCCESS = 0,
    APP_DRV_FIFO_RESULT_LENGTH_ERROR,
    APP_DRV_FIFO_RESULT_NOT_FOUND,
    APP_DRV_FIFO_RESULT_NOT_MEM,
    APP_DRV_FIFO_RESULT_NULL,

} app_drv_fifo_result_t;

#ifndef NULL
  #define NULL    0
#endif

/*!
 * FIFO structure
 */
typedef struct Fifo_s
{
    uint16_t begin;
    uint16_t end;
    uint8_t *data;
    uint16_t size;
    uint16_t size_mask;
} app_drv_fifo_t;

//__inline uint16_t app_drv_fifo_length(app_drv_fifo_t *fifo);

uint16_t app_drv_fifo_length(app_drv_fifo_t *fifo);

/*!
 * Initializes the FIFO structure
 *
 * \param [IN] fifo   Pointer to the FIFO object
 * \param [IN] buffer Buffer to be used as FIFO
 * \param [IN] size   size of the buffer
 */
app_drv_fifo_result_t
app_drv_fifo_init(app_drv_fifo_t *fifo, uint8_t *buffer, uint16_t buffer_size);

/*!
 * Pushes data to the FIFO
 *
 * \param [IN] fifo Pointer to the FIFO object
 * \param [IN] data data to be pushed into the FIFO
 */
void app_drv_fifo_push(app_drv_fifo_t *fifo, uint8_t data);

/*!
 * Pops data from the FIFO
 *
 * \param [IN] fifo Pointer to the FIFO object
 * \retval data     data popped from the FIFO
 */
uint8_t app_drv_fifo_pop(app_drv_fifo_t *fifo);

/*!
 * Flushes the FIFO
 *
 * \param [IN] fifo   Pointer to the FIFO object
 */
void app_drv_fifo_flush(app_drv_fifo_t *fifo);

/*!
 * Checks if the FIFO is empty
 *
 * \param [IN] fifo   Pointer to the FIFO object
 * \retval isEmpty    true: FIFO is empty, false FIFO is not empty
 */
bool app_drv_fifo_is_empty(app_drv_fifo_t *fifo);

/*!
 * Checks if the FIFO is full
 *
 * \param [IN] fifo   Pointer to the FIFO object
 * \retval isFull     true: FIFO is full, false FIFO is not full
 */
bool app_drv_fifo_is_full(app_drv_fifo_t *fifo);

app_drv_fifo_result_t
app_drv_fifo_write(app_drv_fifo_t *fifo, uint8_t *data,
                   uint16_t *p_write_length);

app_drv_fifo_result_t
app_drv_fifo_write_from_same_addr(app_drv_fifo_t *fifo, uint8_t *data,
                                  uint16_t write_length);

app_drv_fifo_result_t
app_drv_fifo_read(app_drv_fifo_t *fifo, uint8_t *data, uint16_t *p_read_length);

app_drv_fifo_result_t
app_drv_fifo_read_to_same_addr(app_drv_fifo_t *fifo, uint8_t *data,
                               uint16_t read_length);

#endif // __APP_DRV_FIFO_H__

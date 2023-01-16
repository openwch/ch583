/********************************** (C) COPYRIGHT *******************************
 * File Name          : buf.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2022/06/30
 * Description        : 
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef BLE_DIRECTTEST_APP_INCLUDE_BUF_H
#define BLE_DIRECTTEST_APP_INCLUDE_BUF_H

#include <stdint.h>
#include <stddef.h>

struct simple_buf {
	/** Pointer to the start of data in the buffer. */
	uint8_t *data;

	/**
	 * Length of the data behind the data pointer.
	 *
	 */
	uint16_t len;

	/** Amount of data that simple buf can store. */
	uint16_t size;

	/** Start of the data storage. Not to be accessed directly
	 *  (the data pointer should be used instead).
	 */
	uint8_t *__buf;
};

/**
 * @brief create a simple_buf object.
 * 
 * @param buf simple_buf object
 * @param buf_pool simple_buf buffer
 * @param pool_size simple_buf size
 */
struct simple_buf *simple_buf_create(struct simple_buf *buf, 
			uint8_t *buf_pool, uint16_t pool_size);

/**
 * @brief Prepare data to be added at the end of the buffer
 *
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param buf Buffer to update.
 * @param len Number of bytes to increment the length with.
 *
 * @return The original tail of the buffer.
 */
void *simple_buf_add(struct simple_buf *buf, size_t len);

/**
 * @brief Prepare data to be added to the start of the buffer
 *
 * Modifies the data pointer and buffer length to account for more data
 * in the beginning of the buffer.
 *
 * @param buf Buffer to update.
 * @param len Number of bytes to add to the beginning.
 *
 * @return The new beginning of the buffer data.
 */
void *simple_buf_pull(struct simple_buf *buf, size_t len);

/**
 * @brief Copy given number of bytes from memory to the end of the buffer
 *
 * Increments the data length of the  buffer to account for more data at the
 * end.
 *
 * @param buf Buffer to update.
 * @param mem Location of data to be added.
 * @param len Length of data to be added
 *
 * @return The original tail of the buffer.
 */
void *simple_buf_add_mem(struct simple_buf *buf, const void *mem,
			     size_t len);

/**
 * @brief Add (8-bit) byte at the end of the buffer
 *
 * Increments the data length of the  buffer to account for more data at the
 * end.
 *
 * @param buf Buffer to update.
 * @param val byte value to be added.
 *
 * @return Pointer to the value added
 */
uint8_t *simple_buf_add_u8(struct simple_buf *buf, uint8_t val);

#endif /* BLE_DIRECTTEST_APP_INCLUDE_BUF_H */

/********************************** (C) COPYRIGHT *******************************
 * File Name          : buf.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2022/06/30
 * Description        : 
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#include "buf.h"
#include "CH58x_common.h"

#ifdef CONFIG_SIMPLE_BUF_DBG
#define SIMPLE_BUF_DBG(...)         printf(__VA_ARGS__)
#else
#define SIMPLE_BUF_DBG(...)
#endif

/*********************************************************************
 * @fn      simple_buf_create
 *
 * @brief   Create a simple_buf structure object.
 *
 * @param   buf    -   defined simple_buf structural object.
 *          buf_pool  - a pointer to the buffer where data is stored.
 *          pool_size - the size of the buffer.
 *
 * @return  none
 */
struct simple_buf *simple_buf_create(struct simple_buf *buf, 
            uint8_t *buf_pool, uint16_t pool_size)
{
    buf->data = buf_pool;
    buf->len = 0;
    buf->size = pool_size;
    buf->__buf = buf_pool;

    return buf;
}

/*********************************************************************
 * @fn      simple_buf_add
 *
 * @brief   Add data to the buf object.
 *
 * @param   buf  -  defined simple_buf structural object.
 *          len  -  length of the data to be added.
 *
 * @return  none
 */
void *simple_buf_add(struct simple_buf *buf, size_t len)
{
	uint8_t *tail = buf->data + buf->len;

    SIMPLE_BUF_DBG("buf %p add len %ld\n", buf, len);

    if((buf->size - (buf->data - buf->__buf) - buf->len) < len)
    {
        SIMPLE_BUF_DBG("simple_buf_add: no memory\n");
        while(1);
    }

    buf->len += len;

    return tail;
}

/*********************************************************************
 * @fn      simple_buf_pull
 *
 * @brief   pull data from the buf object.
 *
 * @param   buf  -  defined simple_buf structural object.
 *          len  -  The length of data to be retrieved.
 *
 * @return  none
 */
void *simple_buf_pull(struct simple_buf *buf, size_t len)
{
	void *data = buf->data;

    SIMPLE_BUF_DBG("buf %p pull len %ld\n", buf, len);

    if(buf->len < len)
    {
        SIMPLE_BUF_DBG("simple_buf_pull: no data\n");
        while(1);
    }

	buf->len -= len;
	buf->data += len;

	return data;
}

/*********************************************************************
 * @fn      simple_buf_add_mem
 *
 * @brief   Add a segment of memory data to the buf object.
 *
 * @param   buf  -  defined simple_buf structural object.
 *          mem  -  a pointer to the memory data to be added.
 *          len  -  the length of the data to be added.
 *
 * @return  The function returns a pointer to the starting position of the added data.
 */
void *simple_buf_add_mem(struct simple_buf *buf, const void *mem,
			     size_t len)
{
    SIMPLE_BUF_DBG("buf %p add len %ld\n", buf, len);

	return memcpy(simple_buf_add(buf, len), mem, len);
}

/*********************************************************************
 * @fn      simple_buf_add_u8
 *
 * @brief   Add an 8-bit unsigned integer to the buf object.
 *
 * @param   buf  -  defined simple_buf structural object.
 *          val  -  the 8-bit unsigned integer to be added.
 *
 * @return  The function returns a pointer to the starting position of the added data.
 */
uint8_t *simple_buf_add_u8(struct simple_buf *buf, uint8_t val)
{
	uint8_t *u8;

    SIMPLE_BUF_DBG("buf %p add val 0x%02x\n", buf, val);

	u8 = simple_buf_add(buf, 1);
	*u8 = val;

	return u8;
}

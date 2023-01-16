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
struct simple_buf *simple_buf_create(struct simple_buf *buf, 
            uint8_t *buf_pool, uint16_t pool_size)
{
    buf->data = buf_pool;
    buf->len = 0;
    buf->size = pool_size;
    buf->__buf = buf_pool;

    return buf;
}

void *simple_buf_add(struct simple_buf *buf, size_t len)
{
	uint8_t *tail = buf->data + buf->len;

    SIMPLE_BUF_DBG("buf %p add len %ld\n", buf, len);

    if((buf->size - (buf->data - buf->__buf) - buf->len) < len) {
        SIMPLE_BUF_DBG("simple_buf_add: no memory\n");
        while(1);
    }

    buf->len += len;

    return tail;
}

void *simple_buf_pull(struct simple_buf *buf, size_t len)
{
	void *data = buf->data;

    SIMPLE_BUF_DBG("buf %p pull len %ld\n", buf, len);

    if(buf->len < len) {
        SIMPLE_BUF_DBG("simple_buf_pull: no data\n");
        while(1);
    }

	buf->len -= len;
	buf->data += len;

	return data;
}

void *simple_buf_add_mem(struct simple_buf *buf, const void *mem,
			     size_t len)
{
    SIMPLE_BUF_DBG("buf %p add len %ld\n", buf, len);

	return memcpy(simple_buf_add(buf, len), mem, len);
}

uint8_t *simple_buf_add_u8(struct simple_buf *buf, uint8_t val)
{
	uint8_t *u8;

    SIMPLE_BUF_DBG("buf %p add val 0x%02x\n", buf, val);

	u8 = simple_buf_add(buf, 1);
	*u8 = val;

	return u8;
}

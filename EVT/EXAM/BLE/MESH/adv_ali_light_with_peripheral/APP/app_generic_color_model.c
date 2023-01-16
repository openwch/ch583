/********************************** (C) COPYRIGHT *******************************
 * File Name          : app_generic_color_model.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2021/03/24
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
#include "CONFIG.h"
#include "MESH_LIB.h"
#include "app_mesh_config.h"
#include "app_generic_onoff_model.h"
#include "app_generic_color_model.h"
#include "app_vendor_model.h"

/*********************************************************************
 * GLOBAL TYPEDEFS
 */

#define ALI_DEF_TTL    (10)

// 模拟色温值
uint16_t led_color = 20000;

/*********************************************************************
 * @fn      read_led_color
 *
 * @brief   获取当前灯色温
 *
 * @param   led_pin     - LED引脚.
 *
 * @return  色温
 */
uint16_t read_led_color(uint32_t led_pin)
{
    APP_DBG("led_color: %d ", led_color);
    return led_color;
}

/*********************************************************************
 * @fn      set_led_color
 *
 * @brief   设置当前灯色温
 *
 * @param   led_pin     - LED引脚.
 * @param   color   - 色温.
 *
 * @return  none
 */
void set_led_color(uint32_t led_pin, uint16_t color)
{
    led_color = color;
}

/*********************************************************************
 * @fn      gen_color_status
 *
 * @brief   回复天猫精灵色温
 *
 * @param   model       - 模型参数
 * @param   ctx         - 数据参数
 *
 * @return  none
 */
static void gen_color_status(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx)
{
    NET_BUF_SIMPLE_DEFINE(msg, 32);
    int err;

    bt_mesh_model_msg_init(&msg, BLE_MESH_MODEL_OP_LIGHT_CTL_STATUS );  //接收状态值
    net_buf_simple_add_le16(&msg, read_led_color(MSG_PIN));

    APP_DBG("ttl: 0x%02x dst: 0x%04x", ctx->recv_ttl, ctx->recv_dst);

    if(ctx->recv_ttl != ALI_DEF_TTL)
    {
        ctx->send_ttl = BLE_MESH_TTL_DEFAULT;
    }
    else
    {
        ctx->send_ttl = 0;
    }

    err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
    if(err)
    {
        APP_DBG("send status failed: %d", err);
    }
}

/*********************************************************************
 * @fn      gen_onoff_get
 *
 * @brief   天猫精灵下发的获取色温命令
 *
 * @param   model       - 模型参数
 * @param   ctx         - 数据参数
 * @param   buf         - 数据内容
 *
 * @return  none
 */
static void gen_color_get(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    APP_DBG(" ");

    gen_color_status(model, ctx);
}

/*********************************************************************
 * @fn      gen_color_set
 *
 * @brief   天猫精灵下发的设置色温命令,如果与当前色温不同,还需要发送ind给天猫
 *
 * @param   model       - 模型参数
 * @param   ctx         - 数据参数
 * @param   buf         - 数据内容
 *
 * @return  none
 */
static void gen_color_set(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    u8 i;
    struct indicate_param param = {
        .trans_cnt = 0x09,
        .period = K_MSEC(300),
        .rand = (tmos_rand() % 50),
        .tid = als_avail_tid_get(),
    };

    APP_DBG("ttl: 0x%02x dst: 0x%04x rssi: %d color: %d",
            ctx->recv_ttl, ctx->recv_dst, ctx->recv_rssi, buf->data[2] | (buf->data[3] << 8));
    for(i = 0; i<5; i++)
       {
        PRINT("BUF: %d\t", buf->data[i]);
       }

    set_led_color(MSG_PIN, buf->data[2] | (buf->data[3] << 8));

    if(ctx->recv_ttl != ALI_DEF_TTL)
    {
        param.send_ttl = BLE_MESH_TTL_DEFAULT;
    }

    /* Overwrite default configuration */
    if(BLE_MESH_ADDR_IS_UNICAST(ctx->recv_dst))
    {
        param.rand = 0;
        param.send_ttl = BLE_MESH_TTL_DEFAULT;
        param.period = K_MSEC(100);
    }

    send_color_indicate(&param);

    gen_color_status(model, ctx);
}

/*********************************************************************
 * @fn      gen_color_set_unack
 *
 * @brief   天猫精灵下发的设置色温命令(无应答),如果与当前色温不同,还需要发送ind给天猫
 *
 * @param   model       - 模型参数
 * @param   ctx         - 数据参数
 * @param   buf         - 数据内容
 *
 * @return  none
 */
static void gen_color_set_unack(struct bt_mesh_model   *model,
                                    struct bt_mesh_msg_ctx *ctx,
                                    struct net_buf_simple  *buf)
{
    uint16_t color;

    APP_DBG(" ");

    color = read_led_color(MSG_PIN);
    if(color != (buf->data[0] | (buf->data[1] << 8)))
    {
        set_led_color(MSG_PIN, buf->data[0] | (buf->data[1] << 8));
    }
}

const struct bt_mesh_model_op gen_color_op[] = {
    {BLE_MESH_MODEL_OP_LIGHT_CTL_GET, 0, gen_color_get},
    {BLE_MESH_MODEL_OP_LIGHT_CTL_SET, 4, gen_color_set},
    {BLE_MESH_MODEL_OP_LIGHT_CTL_SET_UNACK, 4, gen_color_set_unack},
    BLE_MESH_MODEL_OP_END,
};

/******************************** endfile @ main ******************************/

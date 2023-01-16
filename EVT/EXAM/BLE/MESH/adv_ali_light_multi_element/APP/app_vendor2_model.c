/********************************** (C) COPYRIGHT *******************************
 * File Name          : app_vendor_model.c
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
#include "app_mesh_config.h"
#include "MESH_LIB.h"
#include "CONFIG.h"
#include "app_vendor_model.h"
#include "app_generic_onoff2_model.h"
#include "app_als_fan_attr.h"
#include "app_vendor2_model.h"

/*********************************************************************
 * GLOBAL TYPEDEFS
 */

/*********************************************************************
 * GLOBAL TYPEDEFS
 */

static uint8_t als_vendor2_model_TaskID = 0; // Task ID for internal task/event processing

static struct net_buf          ind2_buf[CONFIG_INDICATE_NUM] = {0};
static struct bt_mesh_indicate indicate2[CONFIG_INDICATE_NUM] = {0};

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
static void     ind2_reset(struct bt_mesh_indicate *ind, int err);
static uint16_t als_vendor2_model_ProcessEvent(uint8_t task_id, uint16_t events);

/*********************************************************************
 * @fn      tm2_attr_get
 *
 * @brief   收到天猫精灵发出的获取属性值命令
 *
 * @param   model   - 模型参数.
 * @param   ctx     - 数据参数.
 * @param   buf     - 数据内容.
 *
 * @return  none
 */
static void tm2_attr_get(struct bt_mesh_model   *model,
                         struct bt_mesh_msg_ctx *ctx,
                         struct net_buf_simple  *buf)
{
    uint16_t attr_type;
    if((buf->data[1] | (buf->data[2] << 8)) < ALI_GEN_ATTR_TYPE_WORK_STATUS)
    {
        attr_type = buf->data[1] | (buf->data[2] << 8);
    }
    else
    {
        attr_type = buf->data[3] | (buf->data[4] << 8);
    }

    APP_DBG("attr_type：0x%04x ", attr_type);

    switch(attr_type)
    {
        case ALI_GEN_ATTR_TYPE_ANGLEAUTO_LR_ONOFF:
        {
            gen_angle_auto_LR_get(model, ctx, buf);
            break;
        }

        default:
            break;
    }
}

/*********************************************************************
 * @fn      tm2_attr_set
 *
 * @brief   收到天猫精灵发出的设置属性值命令
 *
 * @param   model   - 模型参数.
 * @param   ctx     - 数据参数.
 * @param   buf     - 数据内容.
 *
 * @return  none
 */
static void tm2_attr_set(struct bt_mesh_model   *model,
                         struct bt_mesh_msg_ctx *ctx,
                         struct net_buf_simple  *buf)
{
    uint16_t attr_type;

    if((buf->data[1] | (buf->data[2] << 8)) < ALI_GEN_ATTR_TYPE_WORK_STATUS)
    {
        attr_type = buf->data[1] | (buf->data[2] << 8);
    }
    else
    {
        attr_type = buf->data[3] | (buf->data[4] << 8);
    }

    APP_DBG("attr_type：0x%04x ", attr_type);

    gen_fan_set(model, ctx, buf);
}

/*********************************************************************
 * @fn      tm2_attr_set_unack
 *
 * @brief   收到天猫精灵发出的设置属性值命令（无应答）
 *
 * @param   model   - 模型参数.
 * @param   ctx     - 数据参数.
 * @param   buf     - 数据内容.
 *
 * @return  none
 */
static void tm2_attr_set_unack(struct bt_mesh_model   *model,
                               struct bt_mesh_msg_ctx *ctx,
                               struct net_buf_simple  *buf)
{
    uint16_t attr_type;
    if((buf->data[1] | (buf->data[2] << 8)) < ALI_GEN_ATTR_TYPE_WORK_STATUS)
    {
        attr_type = buf->data[1] | (buf->data[2] << 8);
    }
    else
    {
        attr_type = buf->data[3] | (buf->data[4] << 8);
    }

    APP_DBG("attr_type：0x%04x ", attr_type);

    gen_fan_set_unack(model, ctx, buf);
}

/*********************************************************************
 * @fn      tm2_attr_confirm
 *
 * @brief   收到天猫精灵发出的confirm 该消息用于Vendor Model Client回复给Vendor Model Server，
 *          用于表示已收到Vendor Model Server发出的Indication
 *
 * @param   model   - 模型参数.
 * @param   ctx     - 数据参数.
 * @param   buf     - 数据内容.
 *
 * @return  none
 */
static void tm2_attr_confirm(struct bt_mesh_model   *model,
                             struct bt_mesh_msg_ctx *ctx,
                             struct net_buf_simple  *buf)
{
    uint8_t recv_tid;

    recv_tid = net_buf_simple_pull_u8(buf);

    APP_DBG("src: 0x%04x dst: 0x%04x tid 0x%02x rssi: %d",
            ctx->addr, ctx->recv_dst, recv_tid, ctx->recv_rssi);

    for(int i = 0; i < CONFIG_INDICATE_NUM; i++)
    {
        if(indicate2[i].param.tid == recv_tid)
        {
            ind2_reset(&indicate2[i], 0);
            tmos_stop_task(als_vendor2_model_TaskID, indicate2[i].event);
            continue;
        }
    }
}

/*********************************************************************
 * @fn      tm2_attr_trans
 *
 * @brief   该消息用于Mesh设备与天猫精灵之间透传数据
 *
 * @param   model   - 模型参数.
 * @param   ctx     - 数据参数.
 * @param   buf     - 数据内容.
 *
 * @return  none
 */
static void tm2_attr_trans(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    APP_DBG(" ");
}

// opcode 对应的处理函数
static const struct bt_mesh_model_op vnd2_model_op[] = {
    {OP_VENDOR_MESSAGE_ATTR_GET, 0, tm2_attr_get},
    {OP_VENDOR_MESSAGE_ATTR_SET, 0, tm2_attr_set},
    {OP_VENDOR_MESSAGE_ATTR_SET_UNACK, 0, tm2_attr_set_unack},
    {OP_VENDOR_MESSAGE_ATTR_CONFIRMATION, 1, tm2_attr_confirm},
    {OP_VENDOR_MESSAGE_ATTR_TRANSPARENT_MSG, 0, tm2_attr_trans},
    BLE_MESH_MODEL_OP_END,
};

uint16_t vnd2_model_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = {BLE_MESH_KEY_UNUSED};
uint16_t vnd2_model_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = {BLE_MESH_ADDR_UNASSIGNED};

struct bt_mesh_model vnd2_models[] = {
    BLE_MESH_MODEL_VND_CB(CID_ALI_GENIE, 0x0000, vnd2_model_op, NULL, vnd2_model_srv_keys, vnd2_model_srv_groups, NULL, &bt_mesh_als_vendor2_model_cb),
};

/*********************************************************************
 * @fn      ind2_reset
 *
 * @brief   移除列表，调用发送完成回调，释放缓存
 *
 * @param   ind     - 需要重置的通知
 * @param   err     - 错误码
 *
 * @return  none
 */
static void ind2_reset(struct bt_mesh_indicate *ind, int err)
{
    if(ind->param.cb && ind->param.cb->end)
    {
        ind->param.cb->end(err, ind->param.cb_data);
    }

    tmos_msg_deallocate(ind->buf->__buf);
    ind->buf->__buf = NULL;
}

/*********************************************************************
 * @fn      bt_mesh_indicate2_reset
 *
 * @brief   释放所有未发送的通知
 *
 * @param   none
 *
 * @return  none
 */
void bt_mesh_indicate2_reset(void)
{
    uint8_t i;
    for(i = 0; i < CONFIG_INDICATE_NUM; i++)
    {
        if(indicate2[i].buf->__buf != NULL)
        {
            ind2_reset(&indicate2[i], -ECANCELED);
        }
    }
}

/*********************************************************************
 * @fn      ind2_start
 *
 * @brief   发送 indicate2 开始回调
 *
 * @param   duration    - 本次发送将要持续的时长
 * @param   err         - 错误码
 * @param   cb_data     - 回调参数
 *
 * @return  none
 */
static void ind2_start(uint16_t duration, int err, void *cb_data)
{
    struct bt_mesh_indicate *ind = cb_data;

    if(ind->buf->__buf == NULL)
    {
        return;
    }

    if(err)
    {
        APP_DBG("Unable send indicate (err:%d)", err);
        tmos_start_task(als_vendor2_model_TaskID, ind->event, K_MSEC(100));
        return;
    }
}

/*********************************************************************
 * @fn      ind2_end
 *
 * @brief   发送 indicate2 结束回调
 *
 * @param   err         - 错误码
 * @param   cb_data     - 回调参数
 *
 * @return  none
 */
static void ind2_end(int err, void *cb_data)
{
    struct bt_mesh_indicate *ind = cb_data;

    if(ind->buf->__buf == NULL)
    {
        return;
    }
    tmos_start_task(als_vendor2_model_TaskID, ind->event, ind->param.period);
}

// 发送 indicate 回调结构体
const struct bt_mesh_send_cb ind2_cb =
    {
        .start = ind2_start,
        .end = ind2_end,
};

/*********************************************************************
 * @fn      adv_ind2_send
 *
 * @brief   发送 indicate2
 *
 * @param   ind - 需要发送的通知
 *
 * @return  none
 */
static void adv_ind2_send(struct bt_mesh_indicate *ind)
{
    int err;
    NET_BUF_SIMPLE_DEFINE(msg, 32);

    struct bt_mesh_msg_ctx ctx = {
        .app_idx = vnd2_models[0].keys[0],
        .addr = ALI_TM_SUB_ADDRESS,
    };

    if(ind->buf->__buf == NULL)
    {
        APP_DBG("NULL buf");
        return;
    }

    if(ind->param.trans_cnt == 0)
    {
        ind2_reset(ind, -ETIMEDOUT);
        return;
    }

    ind->param.trans_cnt--;

    ctx.send_ttl = ind->param.send_ttl;

    /** TODO */
    net_buf_simple_add_mem(&msg, ind->buf->data, ind->buf->len);

    APP_DBG("");
    err = bt_mesh_model_send(vnd2_models, &ctx, &msg, &ind2_cb, ind);
    if(err)
    {
        APP_DBG("Unable send model message (err:%d)", err);
        ind2_reset(ind, -EIO);
        return;
    }
}

/*********************************************************************
 * @fn      bt_mesh_ind2_alloc
 *
 * @brief   找一个空的indicate2，并分配内存
 *
 * @param   len - 需要分配的数据长度
 *
 * @return  indicate结构体指针
 */
struct bt_mesh_indicate *bt_mesh_ind2_alloc(uint16_t len)
{
    uint8_t i;
    for(i = 0; i < CONFIG_INDICATE_NUM; i++)
    {
        if(indicate2[i].buf->__buf == NULL)
            break;
    }
    if(i == CONFIG_INDICATE_NUM)
    {
        return NULL;
    }
    indicate2[i].buf->__buf = tmos_msg_allocate(len);
    indicate2[i].buf->size = len;

    if(indicate2[i].buf->__buf == NULL)
    {
        return NULL;
    }

    return &indicate2[i];
}

/*********************************************************************
 * @fn      bt_mesh_indicate2_send
 *
 * @brief   启动发送通知的事件
 *
 * @param   ind - indicate结构体指针
 *
 * @return  none
 */
void bt_mesh_indicate2_send(struct bt_mesh_indicate *ind)
{
    tmos_start_task(als_vendor2_model_TaskID, ind->event, ind->param.rand);
}

/*********************************************************************
 * @fn      send_led2_indicate
 *
 * @brief   发送当前LED2(风扇开关)状态，当有LED2状态更新时都需要调用此函数
 *
 * @param   param   - 发送通知的发送参数
 *
 * @return  none
 */
void send_led2_indicate(struct indicate_param *param)
{
    struct bt_mesh_indicate *ind;

    ind = bt_mesh_ind2_alloc(16);
    if(!ind)
    {
        APP_DBG("Unable allocate buffers");
        return;
    }
    memcpy(&(ind->param), param, sizeof(struct indicate_param));

    /* Init indication opcode */
    bt_mesh_model_msg_init(&(ind->buf->b), OP_VENDOR_MESSAGE_ATTR_INDICATION);

    /* Add tid field */
    net_buf_simple_add_u8(&(ind->buf->b), param->tid);

    /* Add generic onoff attrbute op */
    net_buf_simple_add_le16(&(ind->buf->b), ALI_GEN_ATTR_TYPE_POWER_STATE);

    /* Add current generic onoff status */
    net_buf_simple_add_u8(&(ind->buf->b), read_led2_state(MSG2_PIN));

    bt_mesh_indicate2_send(ind);
}

/*********************************************************************
 * @fn      send_angle_auto_LR_indicate
 *
 * @brief   发送当前angle_auto_LR，当有angle_auto_LR更新时都需要调用此函数
 *
 * @param   param   - 发送通知的发送参数
 *
 * @return  none
 */
void send_angle_auto_LR_indicate(struct indicate_param *param)
{
    struct bt_mesh_indicate *ind;

    ind = bt_mesh_ind2_alloc(16);
    if(!ind)
    {
        APP_DBG("Unable allocate buffers");
        return;
    }
    memcpy(&(ind->param), param, sizeof(struct indicate_param));

    /* Init indication opcode */
    bt_mesh_model_msg_init(&(ind->buf->b), OP_VENDOR_MESSAGE_ATTR_INDICATION);

    /* Add tid field */
    net_buf_simple_add_u8(&(ind->buf->b), param->tid);

    /* Add angle_auto_LR attrbute opcode */
    net_buf_simple_add_le16(&(ind->buf->b), ALI_GEN_ATTR_TYPE_ANGLEAUTO_LR_ONOFF);

    /* Add angle_auto_LR status  */
    net_buf_simple_add_u8(&(ind->buf->b), read_angle_auto_LR());

    bt_mesh_indicate2_send(ind);
}

/*********************************************************************
 * @fn      als_vendor_init
 *
 * @brief   阿里 厂家模型 初始化
 *
 * @param   model   - 回调模型参数
 *
 * @return  none
 */
static int als_vendor2_init(struct bt_mesh_model *model)
{
    for(int i = 0; i < CONFIG_INDICATE_NUM; i++)
    {
        indicate2[i].buf = &ind2_buf[i];
        indicate2[i].event = (1 << i);
    }

    als_vendor2_model_TaskID = TMOS_ProcessEventRegister(als_vendor2_model_ProcessEvent);
    return 0;
}

/*********************************************************************
 * @fn      als_vendor2_model_ProcessEvent
 *
 * @brief   阿里厂商模型事件处理
 *
 * @param   task_id - The TMOS assigned task ID.
 * @param   events  - events to process.  This is a bit map and can
 *                    contain more than one event.
 *
 * @return  events not processed
 */
static uint16_t als_vendor2_model_ProcessEvent(uint8_t task_id, uint16_t events)
{
    for(int i = 0; i < CONFIG_INDICATE_NUM; i++)
    {
        if(events & indicate2[i].event)
        {
            adv_ind2_send(&indicate2[i]);
            return (events ^ indicate2[i].event);
        }
    }

    // Discard unknown events
    return 0;
}

const struct bt_mesh_model_cb bt_mesh_als_vendor2_model_cb = {
    .init = als_vendor2_init,
};

/******************************** endfile @ main ******************************/

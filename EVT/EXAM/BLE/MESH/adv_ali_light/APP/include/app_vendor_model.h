/********************************** (C) COPYRIGHT *******************************
 * File Name          : app_vendor_model.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/11/12
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef app_vendor_model_H
#define app_vendor_model_H

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/

#include "MESH_LIB.h"

/******************************************************************************/

#define CID_ALI_GENIE                             0x01A8

#define OP_VENDOR_MESSAGE_ATTR_GET                BLE_MESH_MODEL_OP_3(0xD0, CID_ALI_GENIE)
#define OP_VENDOR_MESSAGE_ATTR_SET                BLE_MESH_MODEL_OP_3(0xD1, CID_ALI_GENIE)
#define OP_VENDOR_MESSAGE_ATTR_SET_UNACK          BLE_MESH_MODEL_OP_3(0xD2, CID_ALI_GENIE)
#define OP_VENDOR_MESSAGE_ATTR_STATUS             BLE_MESH_MODEL_OP_3(0xD3, CID_ALI_GENIE)
#define OP_VENDOR_MESSAGE_ATTR_INDICATION         BLE_MESH_MODEL_OP_3(0xD4, CID_ALI_GENIE)
#define OP_VENDOR_MESSAGE_ATTR_CONFIRMATION       BLE_MESH_MODEL_OP_3(0xD5, CID_ALI_GENIE)
#define OP_VENDOR_MESSAGE_ATTR_TRANSPARENT_MSG    BLE_MESH_MODEL_OP_3(0xCF, CID_ALI_GENIE)

#define ALI_SYS_ATTR_TYPE_ERROR                   0x0000
#define ALI_SYS_ATTR_TYPE_VERSION                 0xFF01
#define ALI_SYS_ATTR_TYPE_DEV_FEATURE             0xFF02
#define ALI_SYS_ATTR_TYPE_TOTAL_FLASH_SIZE        0xFF03
#define ALI_SYS_ATTR_TYPE_USED_FLASH_SIZE         0xFF04
#define ALI_SYS_ATTR_TYPE_FREE_FLASH_SIZE         0xFF05
#define ALI_SYS_ATTR_TYPE_ENGINEER_MODE           0xFF06

#define ALI_GEN_ATTR_TYPE_WORK_STATUS             0xF001
#define ALI_GEN_ATTR_TYPE_USER_ID                 0xF002
#define ALI_GEN_ATTR_TYPE_DEVICE_NAME             0xF003
#define ALI_GEN_ATTR_TYPE_MODE_NUMBER             0xF004
#define ALI_GEN_ATTR_TYPE_ONOFF_PLAN              0xF008
#define ALI_GEN_ATTR_TYPE_EVENT_TRIGGER           0xF009
#define ALI_GEN_ATTR_TYPE_EVENT_CLEAR             0xF019
#define ALI_GEN_ATTR_TYPE_SIGNAL_INTENSITY        0xF00A
#define ALI_GEN_ATTR_TYPE_DELTA_VALUE             0xF00B
#define ALI_GEN_ATTR_TYPE_ELEMENT_COUNT           0xF00C
#define ALI_GEN_ATTR_TYPE_ATTR_SWITCH             0xF00D
#define ALI_GEN_ATTR_TYPE_REMOTE_ADDRESS          0xF00E
#define ALI_GEN_ATTR_TYPE_NEARBY_SIGNAL_INTEN     0xF00F
#define ALI_GEN_ATTR_TYPE_SET_VALUE_TIMING        0xF010
#define ALI_GEN_ATTR_TYPE_SET_VALUE_PERIODIC      0xF011
#define ALI_GEN_ATTR_TYPE_DEL_TIMING              0xF012
#define ALI_GEN_ATTR_TYPE_REQ_UPDATE_TIMING       0xF013
#define ALI_GEN_ATTR_TYPE_SETTING_TIMING          0xF01D
#define ALI_GEN_ATTR_TYPE_TIME_ZONE               0xF01E
#define ALI_GEN_ATTR_TYPE_UNIX_TIMER              0xF01F
#define ALI_GEN_ATTR_TYPE_POWERDOWN_MEM           0xF021
#define ALI_GEN_ATTR_TYPE_CALORIES                0x06D5
#define ALI_GEN_ATTR_TYPE_SPORTCOUNT              0x0212
#define ALI_GEN_ATTR_TYPE_POWER_STATE             0x0100
#define ALI_GEN_ATTR_TYPE_BRIGHTNESS              0x0121
#define ALI_GEN_ATTR_TYPE_HARDWARE_RESET          0x0023

#define ALI_TM_SUB_ADDRESS                        0xF000

/** Default number of Indication */
#define CONFIG_INDICATE_NUM                       (3)

/******************************************************************************/

/**
 * @brief 发送数据的开始和结束回调函数定义
 */
struct bt_adv_ind_send_cb
{
    void (*start)(int err, void *cb_data);
    void (*end)(int err, void *cb_data);
};

/**
 * @brief 发送参数结构体
 */
struct indicate_param
{
    uint8_t tid;
    uint8_t trans_cnt;
    int32_t period;
    int32_t rand;
    uint8_t send_ttl;

    void                            *cb_data;
    const struct bt_adv_ind_send_cb *cb;
};

/**
 * @brief indicate 发送缓存
 */
struct bt_mesh_indicate
{
    struct indicate_param param;
    struct net_buf       *buf;
    uint8_t               event;
};

/**
 * @brief 天猫精灵相关信息结构体
 */
struct bt_als_cfg
{
    /** Company Identify */
    uint16_t cid;

    /** Product Identify */
    uint32_t pid;

    /** Mac Address */
    uint8_t mac[6];

    /** Secret Information */
    uint8_t secret[16];

    /** Currently Library Version */
    uint32_t version;
};

extern struct bt_mesh_model          vnd_models[1];
extern uint8_t                       static_key[16];
extern uint8_t                       tm_uuid[16];
extern const struct bt_mesh_model_cb bt_mesh_als_vendor_model_cb;

/******************************************************************************/

/**
 * @brief   TID selection method
 *
 * @return  next TID
 */
uint8_t als_avail_tid_get(void);

/**
 * @brief   找一个空的indicate，并分配内存
 *
 * @param   len - 需要分配的数据长度
 *
 * @return  indicate结构体指针
 */
struct bt_mesh_indicate *bt_mesh_ind_alloc(uint16_t len);

/**
 * @brief   启动发送通知的事件
 *
 * @param   ind - indicate结构体指针
 */
void bt_mesh_indicate_send(struct bt_mesh_indicate *ind);

/**
 * @brief   发送当前LED状态，当有LED状态更新时都需要调用此函数
 *
 * @param   param -  发送通知的发送参数
 */
void send_led_indicate(struct indicate_param *param);

/**
 * @brief   释放所有未发送的通知
 */
void bt_mesh_indicate_reset(void);

/**
 * @brief   阿里 厂家模型 初始化
 *
 * @param   model -  回调模型参数
 *
 * @return  always success
 */
int als_vendor_init(struct bt_mesh_model *model);


#ifdef __cplusplus
}
#endif

#endif

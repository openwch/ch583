/********************************** (C) COPYRIGHT *******************************
 * File Name          : app_generic_onoff_model_cli.h
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2021/11/18
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef app_generic_onoff_model_cli_H
#define app_generic_onoff_model_cli_H

#ifdef __cplusplus
extern "C" {
#endif

#include "app_vendor_model_srv.h"

#define CID_WCH                              0x07D7

#define OP_VENDOR_MESSAGE_TRANSPARENT_CFM    BLE_MESH_MODEL_OP_3(0xCB, CID_WCH)
#define OP_VENDOR_MESSAGE_TRANSPARENT_WRT    BLE_MESH_MODEL_OP_3(0xCC, CID_WCH)
#define OP_VENDOR_MESSAGE_TRANSPARENT_ACK    BLE_MESH_MODEL_OP_3(0xCD, CID_WCH)
#define OP_VENDOR_MESSAGE_TRANSPARENT_IND    BLE_MESH_MODEL_OP_3(0xCE, CID_WCH)
#define OP_VENDOR_MESSAGE_TRANSPARENT_MSG    BLE_MESH_MODEL_OP_3(0xCF, CID_WCH)

#define BLE_MESH_MODEL_ID_WCH_SRV            0x0000
#define BLE_MESH_MODEL_ID_WCH_CLI            0x0001

#define VENDOR_MODEL_CLI_TRANS_EVT           (1 << 0)
#define VENDOR_MODEL_CLI_RSP_TIMEOUT_EVT     (1 << 1)
#define VENDOR_MODEL_CLI_WRITE_EVT           (1 << 2)

/**
 * @brief indicate 回调结构体
 */
struct bt_mesh_vendor_model_ind
{
    uint8_t *pdata;
    uint16_t len;
    uint16_t addr;
};

/**
 * @brief trans 回调结构体
 */
struct bt_mesh_vendor_model_cli_trans
{
    uint8_t *pdata;
    uint16_t len;
    uint16_t addr;
};

/**
 * @brief Client event header
 */
struct vendor_model_cli_EventHdr
{
    uint8_t  status;
    uint32_t opcode;
};

/**
 * @brief Client event union
 */
union vendor_model_cli_Event_t
{
    struct bt_mesh_vendor_model_cli_trans trans;
    struct bt_mesh_vendor_model_ind       ind;
};

/**
 * @brief Client event struct
 */
typedef struct
{
    struct vendor_model_cli_EventHdr vendor_model_cli_Hdr;
    union vendor_model_cli_Event_t   vendor_model_cli_Event;
} vendor_model_cli_status_t;

typedef void (*vendor_model_cli_rsp_handler_t)(const vendor_model_cli_status_t *val);

/**
 * @brief 记录当前发送数据包的tid
 */
struct vendor_model_cli_tid
{
    uint8_t trans_tid;
    uint16_t trans_addr;
    uint8_t ind_tid;
    uint16_t ind_addr;
};

/**
 * @brief Mesh vendor_model_cli Model Context
 */
struct bt_mesh_vendor_model_cli
{
    struct bt_mesh_model       *model;
    uint32_t                    op_req;
    uint32_t                    op_pending;
    struct vendor_model_cli_tid cli_tid;

    vendor_model_cli_rsp_handler_t handler;
};

extern const struct bt_mesh_model_op vnd_model_cli_op[];
extern const struct bt_mesh_model_cb bt_mesh_vendor_model_cli_cb;

/**
 * @brief   send_trans,透传数据通道
 *
 * @param   param   - 发送参数.
 * @param   pData   - 数据指针.
 * @param   len     - 数据长度,最大为(APP_MAX_TX_SIZE).
 *
 * @return  参考Global_Error_Code
 */
int vendor_message_cli_send_trans(struct send_param *param, uint8_t *pData, uint16_t len);

/**
 * @brief   write,有应答传输数据通道
 *
 * @param   param   - 发送参数.
 * @param   pData   - 数据指针.
 * @param   len     - 数据长度,最大为(APP_MAX_TX_SIZE).
 *
 * @return  参考Global_Error_Code
 */
int vendor_message_cli_write(struct send_param *param, uint8_t *pData, uint16_t len);

/**
 * @brief   TID selection method
 *
 * @return  TID
 */
uint8_t vendor_cli_tid_get(void);

/**
 * @brief   复位厂商模型服务，取消所有正在发送的流程
 */
void vendor_message_cli_trans_reset(void);

/**
 * @brief   厂商模型初始化
 *
 * @param   model       - 指向厂商模型结构体
 *
 * @return  always SUCCESS
 */
int vendor_model_cli_init(struct bt_mesh_model *model);


#ifdef __cplusplus
}
#endif

#endif

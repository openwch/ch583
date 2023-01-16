/********************************** (C) COPYRIGHT *******************************
 * File Name          : app.h
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2021/11/18
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef app_H
#define app_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/

#define APP_NODE_EVT                   (1 << 0)
#define APP_DELETE_NODE_TIMEOUT_EVT    (1 << 1)
#define APP_DELETE_LOCAL_NODE_EVT      (1 << 2)
#define APP_DELETE_NODE_INFO_EVT       (1 << 3)

#define CMD_PROVISION                  0xA1
#define CMD_DELETE_NODE                0xA2
#define CMD_DELETE_NODE_ACK            0x82
#define CMD_DELETE_NODE_INFO           0xA3
#define CMD_LOCAL_RESET                0xAF

#define PERIPHERAL_CMD_LEN             1
#define PROVISION_NET_KEY_LEN          16
#define ADDRESS_LEN                    2

// 通过ble接收到的配网参数，包含 1字节命令码+16字节网络密钥+2字节网络地址
#define PROVISION_DATA_LEN             (PERIPHERAL_CMD_LEN + PROVISION_NET_KEY_LEN + ADDRESS_LEN)
// 删除节点命令，包含 1字节命令码+2字节需要删除的节点地址
#define DELETE_NODE_DATA_LEN           (PERIPHERAL_CMD_LEN + ADDRESS_LEN)
// 删除节点命令应答，包含 1字节命令码
#define DELETE_NODE_ACK_DATA_LEN       (PERIPHERAL_CMD_LEN)
// 删除存储的节点信息命令，包含 1字节命令码
#define DELETE_NODE_INFO_DATA_LEN      (PERIPHERAL_CMD_LEN)
// 本地复位命令，包含 1字节命令码
#define LOCAL_RESET_DATA_LEN           (PERIPHERAL_CMD_LEN)

/******************************************************************************/

typedef struct
{
    uint16_t node_addr;
    uint16_t elem_count;
    uint16_t net_idx;
    uint16_t retry_cnt : 12,
        fixed : 1,
        blocked : 1;

} node_t;

typedef union
{
    struct
    {
        uint8_t cmd;                            /* 命令码 CMD_PROVISION */
        uint8_t net_key[PROVISION_NET_KEY_LEN]; /* 后续数据长度 */
        uint8_t addr[ADDRESS_LEN];              /* 擦除地址 */
    } provision;                                /* 配网命令 */
    struct
    {
        uint8_t cmd;                /* 命令码 CMD_DELETE_NODE */
        uint8_t addr[ADDRESS_LEN];  /* 擦除地址 */
    } delete_node;                  /* 删除节点命令 */
    struct
    {
        uint8_t cmd;                /* 命令码 CMD_DELETE_NODE_ACK */
    } delete_node_ack;              /* 删除节点命令应答 */
    struct
    {
        uint8_t cmd;                /* 命令码 CMD_DELETE_NODE_INFO */
    } delete_node_info;             /* 删除存储的节点信息命令 */
    struct
    {
        uint8_t cmd;                /* 命令码 CMD_LOCAL_RESET */
    } local_reset;                  /* 本地恢复出厂设置命令 */
    struct
    {
        uint8_t buf[20]; /* 接收数据包*/
    } data;
}app_mesh_manage_t;

void App_Init(void);

/******************************************************************************/

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif

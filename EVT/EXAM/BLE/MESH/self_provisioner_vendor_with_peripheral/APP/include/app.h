/********************************** (C) COPYRIGHT *******************************
 * File Name          : app.h
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2021/11/18
 * Description        :
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

#ifndef app_H
#define app_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/

#define APP_NODE_EVT         (1 << 0)
#define APP_NODE_TEST_EVT    (1 << 1)

/******************************************************************************/

/**
 * @brief 远端节点配置流程状态
 */
typedef enum
{
    NODE_INIT = 0,
    NODE_APPKEY_ADD = 1,
    NODE_MOD_BIND_SET = 2,
    NODE_MOD_SUB_SET = 3,

    NODE_CONFIGURATIONED = 4,
} node_stage_t;

/**
 * @brief 本地节点配置流程状态
 */
typedef enum
{
    LOCAL_INIT = 0,
    LOCAL_APPKEY_ADD = 1,
    LOCAL_MOD_BIND_SET = 2,

    LOCAL_CONFIGURATIONED = 4,
} local_stage_t;

/**
 * @brief 节点配置流程组合体
 */
typedef union
{
    node_stage_t  node;
    local_stage_t local;
} stage_t;

typedef void (*cfg_rsp_handler_t)(void *node, const void *rsp);
typedef BOOL (*stage_handler_t)(void *node);

/**
 * @brief 节点配置流程回调
 */
typedef struct
{
    cfg_rsp_handler_t rsp;
    stage_handler_t   stage;
} cfg_cb_t;

/**
 * @brief 配置节点的结构体
 */
typedef struct
{
    uint16_t node_addr;
    uint16_t elem_count;
    uint16_t net_idx;
    uint16_t retry_cnt : 12,
        fixed : 1,
        blocked : 1;

    stage_t         stage;
    const cfg_cb_t *cb;
} node_t;

extern node_t         app_nodes[1 + CONFIG_MESH_PROV_NODE_COUNT_DEF];
extern const uint16_t self_prov_net_idx;
extern const uint16_t self_prov_app_idx;
extern const uint32_t self_prov_iv_index;
extern const uint16_t self_prov_addr;

/**
 * @brief   应用层初始化
 */
void App_Init(void);

/******************************************************************************/

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif

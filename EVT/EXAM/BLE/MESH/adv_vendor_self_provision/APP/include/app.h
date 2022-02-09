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
// 本地节点配置流程
typedef enum
{
    LOCAL_INIT = 0,
    LOCAL_APPKEY_ADD = 1,
    LOCAL_MOD_BIND_SET = 2,
    LOCAL_MOD_SUB_SET = 3,
    LOCAL_CONFIGURATIONED = 4,
} local_stage_t;

typedef union
{
    local_stage_t local;
} stage_t;

typedef void (*cfg_rsp_handler_t)(void *node, const void *rsp);
typedef BOOL (*stage_handler_t)(void *node);

typedef struct
{
    cfg_rsp_handler_t rsp;
    stage_handler_t   stage;
} cfg_cb_t;

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

void App_Init(void);

/******************************************************************************/

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif

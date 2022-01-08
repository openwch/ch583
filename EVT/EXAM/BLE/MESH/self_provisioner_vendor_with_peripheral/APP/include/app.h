/********************************** (C) COPYRIGHT *******************************
* File Name          : app.h
* Author             : WCH
* Version            : V1.0
* Date               : 2018/11/12
* Description        : 
*******************************************************************************/

#ifndef app_H
#define app_H

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************/

#define		APP_NODE_EVT									(1<<0)
#define		APP_NODE_TEST_EVT							(1<<1)

/******************************************************************************/
// 远端节点配置流程
typedef enum
{
	NODE_INIT				=	0,
	NODE_APPKEY_ADD			=	1,
	NODE_MOD_BIND_SET		=	2,
	NODE_MOD_SUB_SET		=	3,
	
	NODE_CONFIGURATIONED	=	4,
} node_stage_t;

// 本地节点配置流程
typedef enum
{
	LOCAL_INIT				=	0,
	LOCAL_APPKEY_ADD		=	1,
	LOCAL_MOD_BIND_SET		=	2,
	
	LOCAL_CONFIGURATIONED	=	4,
} local_stage_t;

typedef union
{
	node_stage_t node;
	local_stage_t local;
}stage_t;

typedef void (*cfg_rsp_handler_t)(void *node, const void *rsp);
typedef BOOL (*stage_handler_t)(void *node);

typedef struct
{
	cfg_rsp_handler_t rsp;
	stage_handler_t stage;
}cfg_cb_t;

typedef struct
{
	u16_t	node_addr;
	u16_t 	elem_count;
	u16_t 	net_idx;
	u16_t 	retry_cnt:12,
			fixed:1,
			blocked:1;

	stage_t stage;
	const cfg_cb_t *cb;
}node_t;

extern node_t app_nodes[ 1 + CONFIG_MESH_PROV_NODE_COUNT_DEF ];
extern const u16_t self_prov_net_idx;
extern const u16_t self_prov_app_idx;
extern const u32_t self_prov_iv_index;
extern const u16_t self_prov_addr;


void App_Init(void);

/******************************************************************************/

/******************************************************************************/


#ifdef __cplusplus
}
#endif

#endif

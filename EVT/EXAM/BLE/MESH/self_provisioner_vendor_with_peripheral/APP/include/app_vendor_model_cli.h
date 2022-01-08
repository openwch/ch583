/********************************** (C) COPYRIGHT *******************************
* File Name          : app_generic_onoff_model_cli.h
* Author             : WCH
* Version            : V1.0
* Date               : 2018/11/12
* Description        : 
*******************************************************************************/

#ifndef app_generic_onoff_model_cli_H
#define app_generic_onoff_model_cli_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "app_vendor_model_srv.h"

#define CID_WCH                               			0x07D7

#define OP_VENDOR_MESSAGE_TRANSPARENT_CFM      			BLE_MESH_MODEL_OP_3(0xCB, CID_WCH)
#define OP_VENDOR_MESSAGE_TRANSPARENT_WRT      			BLE_MESH_MODEL_OP_3(0xCC, CID_WCH)
#define OP_VENDOR_MESSAGE_TRANSPARENT_ACK     		 	BLE_MESH_MODEL_OP_3(0xCD, CID_WCH)
#define OP_VENDOR_MESSAGE_TRANSPARENT_IND      			BLE_MESH_MODEL_OP_3(0xCE, CID_WCH)
#define OP_VENDOR_MESSAGE_TRANSPARENT_MSG      			BLE_MESH_MODEL_OP_3(0xCF, CID_WCH)

#define BLE_MESH_MODEL_ID_WCH_SRV             			0x0000
#define BLE_MESH_MODEL_ID_WCH_CLI             			0x0001

#define	VENDOR_MODEL_CLI_TRANS_EVT									(1<<0)
#define	VENDOR_MODEL_CLI_RSP_TIMEOUT_EVT						(1<<1)
#define	VENDOR_MODEL_CLI_WRITE_EVT									(1<<2)

struct bt_mesh_vendor_model_ind
{
	u8_t *pdata;
	u16_t len;
};

struct bt_mesh_vendor_model_cli_trans
{
	u8_t *pdata;
	u16_t len;
};

struct vendor_model_cli_EventHdr
{
	u8_t status;
	u32_t opcode;
};

union vendor_model_cli_Event_t
{
	struct bt_mesh_vendor_model_cli_trans trans;
	struct bt_mesh_vendor_model_ind	ind;
};

typedef struct 
{
	struct vendor_model_cli_EventHdr vendor_model_cli_Hdr;
	union vendor_model_cli_Event_t vendor_model_cli_Event;
}vendor_model_cli_status_t;


typedef void (*vendor_model_cli_rsp_handler_t)(const vendor_model_cli_status_t *val);

struct vendor_model_cli_tid
{
	u8_t trans_tid;
	u8_t ind_tid;
};

/** Mesh vendor_model_cli Model Context */
struct bt_mesh_vendor_model_cli {
	struct bt_mesh_model 				*model;
	u32_t 											op_req;
	u32_t                 			op_pending;
	struct vendor_model_cli_tid					cli_tid;
		
	vendor_model_cli_rsp_handler_t			handler;

};

extern const struct bt_mesh_model_op vnd_model_cli_op[];
extern const struct bt_mesh_model_cb bt_mesh_vendor_model_cli_cb;

/*******************************************************************************
* Function Name  : vendor_message_cli_send_trans
* Description    : send_trans ,透传数据通道
* Input          : param: 发送参数
*										pData: 数据指针
*										len: 数据长度,最大为(APP_MAX_TX_SIZE)
* Return         : 参考Global_Error_Code
*******************************************************************************/
int vendor_message_cli_send_trans(struct send_param *param, u8_t *pData, u16_t len);

/*******************************************************************************
* Function Name  : vendor_message_cli_write
* Description    : write ,有应答传输数据通道
* Input          : param: 发送参数
*										pData: 数据指针
*										len: 数据长度,最大为(APP_MAX_TX_SIZE)
* Return         : 参考Global_Error_Code
*******************************************************************************/
int vendor_message_cli_write(struct send_param *param, u8_t *pData, u16_t len);

/*******************************************************************************
* Function Name  : vendor_cli_tid_get
* Description    : TODO TID selection method 每个独立消息递增循环，cli使用0~127
* Input          : None
* Return         : None
*******************************************************************************/
u8_t vendor_cli_tid_get(void);

/*******************************************************************************
* Function Name  : vendor_message_cli_trans_reset
* Description    : 取消发送trans数据的任务，释放缓存
* Input          : None
* Return         : None
*******************************************************************************/
void vendor_message_cli_trans_reset( void );


#ifdef __cplusplus
}
#endif

#endif

/********************************** (C) COPYRIGHT *******************************
* File Name          : app_vendor_model_srv.h
* Author             : WCH
* Version            : V1.0
* Date               : 2021/03/23
* Description        : 
*******************************************************************************/

#ifndef app_vendor_model_srv_H
#define app_vendor_model_srv_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "MESH_LIB.h"

#define MSG_PIN			GPIO_Pin_12

#define CID_WCH                               			0x07D7

#define OP_VENDOR_MESSAGE_TRANSPARENT_CFM      			BLE_MESH_MODEL_OP_3(0xCB, CID_WCH)
#define OP_VENDOR_MESSAGE_TRANSPARENT_WRT      			BLE_MESH_MODEL_OP_3(0xCC, CID_WCH)
#define OP_VENDOR_MESSAGE_TRANSPARENT_ACK     		 	BLE_MESH_MODEL_OP_3(0xCD, CID_WCH)
#define OP_VENDOR_MESSAGE_TRANSPARENT_IND      			BLE_MESH_MODEL_OP_3(0xCE, CID_WCH)
#define OP_VENDOR_MESSAGE_TRANSPARENT_MSG      			BLE_MESH_MODEL_OP_3(0xCF, CID_WCH)

#define BLE_MESH_MODEL_ID_WCH_SRV             			0x0000
#define BLE_MESH_MODEL_ID_WCH_CLI             			0x0001

#define	VENDOR_MODEL_SRV_TRANS_EVT									(1<<0)
#define	VENDOR_MODEL_SRV_RSP_TIMEOUT_EVT						(1<<1)
#define	VENDOR_MODEL_SRV_INDICATE_EVT								(1<<2)

struct bt_adv_trans_cb
{
    void (*start)(int err, void *cb_data);
    void (*end)(int err, void *cb_data);
};


struct send_param
{
  u16_t net_idx;
  u16_t app_idx;
  u16_t addr;
	u8_t tid;
	u8_t trans_cnt;
	s32_t period;
	s32_t rand;
	u8_t send_ttl;

	void *cb_data;
    const struct bt_adv_trans_cb *cb;
};

struct bt_mesh_indicate
{
	struct send_param param;
	struct net_buf *buf;
};

struct bt_mesh_trans
{
	struct send_param param;
	struct net_buf *buf;
};

struct bt_mesh_write
{
	struct send_param param;
	struct net_buf *buf;
};


struct bt_mesh_vendor_model_write
{
	u8_t *pdata;
	u16_t len;
};

struct bt_mesh_vendor_model_srv_trans
{
	u8_t *pdata;
	u16_t len;
};

struct vendor_model_srv_EventHdr
{
	u8_t status;
	u32_t opcode;
};

union vendor_model_srv_Event_t
{
	struct bt_mesh_vendor_model_srv_trans trans;
	struct bt_mesh_vendor_model_write write;
};

typedef struct 
{
	struct vendor_model_srv_EventHdr vendor_model_srv_Hdr;
	union vendor_model_srv_Event_t vendor_model_srv_Event;
}vendor_model_srv_status_t;


typedef void (*vendor_model_srv_rsp_handler_t)(const vendor_model_srv_status_t *val);

struct vendor_model_srv_tid
{
	u8_t trans_tid;
	u8_t write_tid;
};

/** Mesh vendor_model_srv Model Context */
struct bt_mesh_vendor_model_srv {
	struct bt_mesh_model 			*model;
	u32_t 										op_req;
	u32_t                 		op_pending;
	struct vendor_model_srv_tid					srv_tid;

	vendor_model_srv_rsp_handler_t			handler;

};



extern const struct bt_mesh_model_cb bt_mesh_vendor_model_srv_cb;

extern const struct bt_mesh_model_op vnd_model_srv_op[];


/*******************************************************************************
* Function Name  : read_led_state
* Description    : read_led_state
* Input          : None
* Return         : None
*******************************************************************************/
BOOL read_led_state(u32_t led_pin);

/*******************************************************************************
* Function Name  : set_led_state
* Description    : set_led_state
* Input          : None
* Return         : None
*******************************************************************************/
void set_led_state(u32_t led_pin, BOOL on);

/*******************************************************************************
* Function Name  : toggle_led_state
* Description    : toggle_led_state
* Input          : None
* Return         : None
*******************************************************************************/
void toggle_led_state(u32_t led_pin);

/*******************************************************************************
* Function Name  : vendor_message_srv_indicate
* Description    : indicate,有应答传输数据通道
* Input          : param: 发送参数
*										pData: 数据指针
*										len: 数据长度,最大为(APP_MAX_TX_SIZE)
* Return         : 参考Global_Error_Code
*******************************************************************************/
int vendor_message_srv_indicate(struct send_param *param, u8_t *pData, u16_t len);

/*******************************************************************************
* Function Name  : vendor_message_srv_send_trans
* Description    : send_trans,透传数据通道
* Input          : param: 发送参数
*										pData: 数据指针
*										len: 数据长度,最大为(APP_MAX_TX_SIZE)
* Return         : 参考Global_Error_Code
*******************************************************************************/
int vendor_message_srv_send_trans(struct send_param *param, u8_t *pData, u16_t len);

/*******************************************************************************
* Function Name  : vendor_srv_tid_get
* Description    : TODO TID selection method 每个独立消息递增循环，srv使用128~191
* Input          : None
* Return         : None
*******************************************************************************/
u8_t vendor_srv_tid_get(void);

/*******************************************************************************
* Function Name  : vendor_message_srv_trans_reset
* Description    : 取消发送trans数据的任务，释放缓存
* Input          : None
* Return         : None
*******************************************************************************/
void vendor_message_srv_trans_reset( void );


#ifdef __cplusplus
}
#endif

#endif

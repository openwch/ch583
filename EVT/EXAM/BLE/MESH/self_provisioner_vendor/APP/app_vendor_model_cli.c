/********************************** (C) COPYRIGHT *******************************
* File Name          : app_vendor_model_cli.c
* Author             : WCH
* Version            : V1.0
* Date               : 2021/03/22
* Description        : 
*******************************************************************************/



/******************************************************************************/
#include "CONFIG.h"
#include "app_mesh_config.h"
#include "MESH_LIB.h"
#include "CH58x_common.h"
#include "app_vendor_model_cli.h"
#include "app_vendor_model_srv.h"


/*********************************************************************
 * GLOBAL TYPEDEFS
 */
 
// 应用层最大发送长度，不分包最大为CONFIG_MESH_UNSEG_LENGTH_DEF，分包最大为CONFIG_MESH_TX_SEG_DEF*BLE_MESH_APP_SEG_SDU_MAX-8（依据RAM使用情况决定）
#define APP_MAX_TX_SIZE          MAX(CONFIG_MESH_UNSEG_LENGTH_DEF,CONFIG_MESH_TX_SEG_DEF*BLE_MESH_APP_SEG_SDU_MAX-8)

static uint8 vendor_model_cli_TaskID = 0;    // Task ID for internal task/event processing
static uint8 cli_send_tid=0;
static s32_t cli_msg_timeout = K_SECONDS(2);		//有应答的数据传输超时时间，默认2秒

static struct net_buf cli_trans_buf;
static struct bt_mesh_trans cli_trans =
{
	.buf = &cli_trans_buf,
};

static struct net_buf cli_write_buf;
static struct bt_mesh_write cli_write =
{
	.buf = &cli_write_buf,
};

static struct bt_mesh_vendor_model_cli *vendor_model_cli;

static uint16 vendor_model_cli_ProcessEvent( uint8 task_id, uint16 events );
static void write_reset(struct bt_mesh_write *write, int err);
static void adv_cli_trans_send( void );

/*********************************************************************
 * GLOBAL TYPEDEFS
 */


/*********************************************************************
 * GLOBAL TYPEDEFS
 */

/*******************************************************************************
* Function Name  : vendor_cli_tid_get
* Description    : TODO TID selection method
* Input          : None
* Return         : None
*******************************************************************************/
u8_t vendor_cli_tid_get(void)
{
	cli_send_tid++;
	if(cli_send_tid > 127)
		cli_send_tid = 0;
	return cli_send_tid;
}

/*******************************************************************************
* Function Name  : vendor_model_cli_reset
* Description    : 重置vendor_model_cli状态，清除执行中的任务
* Input          : None
* Return         : None
*******************************************************************************/
static void vendor_model_cli_reset(void)
{
	vendor_model_cli->op_pending = 0U;
	vendor_model_cli->op_req = 0U;

	tmos_stop_task(vendor_model_cli_TaskID, VENDOR_MODEL_CLI_RSP_TIMEOUT_EVT);
	tmos_stop_task(vendor_model_cli_TaskID, VENDOR_MODEL_CLI_WRITE_EVT);
}

/*******************************************************************************
* Function Name  : vendor_model_cli_rsp_recv
* Description    : 调用应用层传入的回调
* Input          : val: vendor_model_cli_status_t
*										status: status
* Return         : None
*******************************************************************************/
static void vendor_model_cli_rsp_recv(vendor_model_cli_status_t *val, u8_t status)
{
	if (vendor_model_cli == NULL || (!vendor_model_cli->op_req))
	{
		return;
	}
	
	val->vendor_model_cli_Hdr.opcode = vendor_model_cli->op_req;
	val->vendor_model_cli_Hdr.status = status;

	vendor_model_cli_reset();

	if (vendor_model_cli->handler)
	{
		vendor_model_cli->handler(val);
	}
}

/*******************************************************************************
* Function Name  : vendor_model_cli_wait
* Description    : 默认两秒超时后通知应用层
* Input          : None
* Return         : err: 参考BLE_LIB err code
*******************************************************************************/
static int vendor_model_cli_wait(void)
{
	int err;

	err = tmos_start_task(vendor_model_cli_TaskID, VENDOR_MODEL_CLI_RSP_TIMEOUT_EVT, cli_msg_timeout);
	
	return err;
}

/*******************************************************************************
* Function Name  : vendor_model_cli_prepare
* Description    : 预发送
* Input          : op_req: 发送的请求码
*										op: 期望的返回码
* Return         : None
*******************************************************************************/
static int vendor_model_cli_prepare(u32_t op_req, u32_t op)
{
	if (!vendor_model_cli)
	{
		APP_DBG("No available Configuration Client context!");
		return -EINVAL;
	}

	if (vendor_model_cli->op_pending)
	{
		APP_DBG("Another synchronous operation pending");
		return -EBUSY;
	}

	vendor_model_cli->op_req = op_req;
	vendor_model_cli->op_pending = op;

	return 0;
}

/*******************************************************************************
* Function Name  : vendor_cli_sync_handler
* Description    : 通知应用层当前op_code超时了
* Input          : None
* Return         : None
*******************************************************************************/
static void vendor_cli_sync_handler( void )
{
	vendor_model_cli_status_t vendor_model_cli_status;

	tmos_memset(&vendor_model_cli_status, 0, sizeof(vendor_model_cli_status_t));

	write_reset(&cli_write, -ETIMEDOUT);
	
	vendor_model_cli_rsp_recv(&vendor_model_cli_status, 0xFF);
}


/*******************************************************************************
* Function Name  : vendor_message_cli_ack
* Description    : 发送vendor_message_cli_ack - 该消息用于Vendor Model Client回复给Vendor Model Server，
										用于表示已收到Vendor Model Server发出的Indication
* Input          : model: 模型参数
*										ctx：数据参数
*										buf: 数据内容
* Return         : None
*******************************************************************************/
static void vendor_message_cli_ack(struct bt_mesh_model *model,
                           struct bt_mesh_msg_ctx *ctx,
                           struct net_buf_simple *buf)
{
	NET_BUF_SIMPLE_DEFINE(msg, APP_MAX_TX_SIZE+8);
	u8_t recv_tid;
	int err;

	recv_tid = net_buf_simple_pull_u8(buf);
	
	APP_DBG("tid 0x%02x ", recv_tid);
	
	/* Init indication opcode */
	bt_mesh_model_msg_init(&msg, OP_VENDOR_MESSAGE_TRANSPARENT_ACK);
   
	/* Add tid field */
	net_buf_simple_add_u8(&msg, recv_tid);
	
	err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
	if (err)
	{
			APP_DBG("#mesh-onoff STATUS: send status failed: %d", err);
	}
}

/*******************************************************************************
* Function Name  : vendor_message_cli_trans
* Description    : 透传数据
* Input          : model: 模型参数
*										ctx：数据参数
*										buf: 数据内容
* Return         : None
*******************************************************************************/
static void vendor_message_cli_trans(struct bt_mesh_model *model,
                           struct bt_mesh_msg_ctx *ctx,
                           struct net_buf_simple *buf)
{
	vendor_model_cli_status_t vendor_model_cli_status;
	u8_t *pData = buf->data;
	u16_t len = buf->len;

	if( pData[0] != vendor_model_cli->cli_tid.trans_tid )
	{
		vendor_model_cli->cli_tid.trans_tid = pData[0];
		// 开头为tid
		pData++;len--;
		vendor_model_cli_status.vendor_model_cli_Hdr.opcode = OP_VENDOR_MESSAGE_TRANSPARENT_MSG;
		vendor_model_cli_status.vendor_model_cli_Hdr.status = 0;
		vendor_model_cli_status.vendor_model_cli_Event.trans.pdata = pData;
		vendor_model_cli_status.vendor_model_cli_Event.trans.len = len;
		
		if (vendor_model_cli->handler)
		{
			vendor_model_cli->handler(&vendor_model_cli_status);
		}
	}
}

/*******************************************************************************
* Function Name  : vendor_message_cli_ind
* Description    : 收到Vendor Model Server发出的Indication
* Input          : model: 模型参数
*										ctx：数据参数
*										buf: 数据内容
* Return         : None
*******************************************************************************/
static void vendor_message_cli_ind(struct bt_mesh_model *model,
                           struct bt_mesh_msg_ctx *ctx,
                           struct net_buf_simple *buf)
{
	vendor_model_cli_status_t vendor_model_cli_status;
	u8_t *pData = buf->data;
	u16_t len = buf->len;
	APP_DBG("src: 0x%04x dst: 0x%04x rssi: %d app_idx: 0x%x", 
					ctx->addr, ctx->recv_dst, ctx->recv_rssi, ctx->app_idx);

	if( pData[0] != vendor_model_cli->cli_tid.ind_tid )
	{
		vendor_model_cli->cli_tid.ind_tid = pData[0];
		// 开头为tid
		pData++;len--;
		vendor_model_cli_status.vendor_model_cli_Hdr.opcode = OP_VENDOR_MESSAGE_TRANSPARENT_IND;
		vendor_model_cli_status.vendor_model_cli_Hdr.status = 0;
		vendor_model_cli_status.vendor_model_cli_Event.ind.pdata = pData;
		vendor_model_cli_status.vendor_model_cli_Event.ind.len = len;
	
		if (vendor_model_cli->handler)
		{
			vendor_model_cli->handler(&vendor_model_cli_status);
		}
	}
	vendor_message_cli_ack(model, ctx, buf);
}

/*******************************************************************************
* Function Name  : vendor_message_cli_cfm
* Description    : 收到 vendor_message_cli_cfm - 该消息用于Vendor Model Server回复给Vendor Model Client，
										用于表示已收到Vendor Model Client发出的 Write
* Input          : model: 模型参数
*										ctx：数据参数
*										buf: 数据内容
* Return         : None
*******************************************************************************/
static void vendor_message_cli_cfm(struct bt_mesh_model *model,
                           struct bt_mesh_msg_ctx *ctx,
                           struct net_buf_simple *buf)
{
	u8_t recv_tid;
	vendor_model_cli_status_t vendor_model_cli_status;
	
	recv_tid = net_buf_simple_pull_u8(buf);
	
	APP_DBG("src: 0x%04x dst: 0x%04x tid 0x%02x rssi: %d", 
					ctx->addr, ctx->recv_dst, recv_tid, ctx->recv_rssi);

	if (cli_write.param.tid == recv_tid)
	{
		write_reset(&cli_write, 0);
		vendor_model_cli_rsp_recv(&vendor_model_cli_status, 0);
	}
}


// opcode 对应的处理函数
const struct bt_mesh_model_op vnd_model_cli_op[] = {
	{OP_VENDOR_MESSAGE_TRANSPARENT_MSG, 	0, vendor_message_cli_trans},
	{OP_VENDOR_MESSAGE_TRANSPARENT_IND, 	0, vendor_message_cli_ind},
	{OP_VENDOR_MESSAGE_TRANSPARENT_CFM, 	0, vendor_message_cli_cfm},
  BLE_MESH_MODEL_OP_END,
};

/*******************************************************************************
* Function Name  : vendor_message_cli_send_trans
* Description    : send_trans ,透传数据通道
* Input          : param: 发送参数
*										pData: 数据指针
*										len: 数据长度,最大为(APP_MAX_TX_SIZE)
* Return         : 参考Global_Error_Code
*******************************************************************************/
int vendor_message_cli_send_trans(struct send_param *param, u8_t *pData, u16_t len)
{
	if(!param->addr)
		return -EINVAL;
	if( cli_trans.buf->__buf )
		return -EBUSY;
	if( len > (APP_MAX_TX_SIZE))
		return -EINVAL;
	
	cli_trans.buf->__buf = tmos_msg_allocate(len+8);
	if( !(cli_trans.buf->__buf) )
	{
		APP_DBG("No enough space!");
		return -ENOMEM;
	}
	cli_trans.buf->size = len+4;
	/* Init indication opcode */
	bt_mesh_model_msg_init(&(cli_trans.buf->b), OP_VENDOR_MESSAGE_TRANSPARENT_MSG);
   
	/* Add tid field */
	net_buf_simple_add_u8(&(cli_trans.buf->b), param->tid);
	
	net_buf_simple_add_mem(&(cli_trans.buf->b), pData, len);

	memcpy(&cli_trans.param, param, sizeof(struct send_param));

	if( param->rand )
	{
	  // 延迟发送
	  tmos_start_task(vendor_model_cli_TaskID, VENDOR_MODEL_CLI_TRANS_EVT, param->rand);
	}
	else
	{
    // 直接发送
	  adv_cli_trans_send();
	}
	return 0;
}

/*******************************************************************************
* Function Name  : vendor_message_cli_write
* Description    : write ,有应答传输数据通道
* Input          : param: 发送参数
*										pData: 数据指针
*										len: 数据长度,最大为(APP_MAX_TX_SIZE)
* Return         : 参考Global_Error_Code
*******************************************************************************/
int vendor_message_cli_write(struct send_param *param, u8_t *pData, u16_t len)
{
	if( !param->addr)
		return -EINVAL;
	if( cli_write.buf->__buf )
		return -EBUSY;
	if( len > (APP_MAX_TX_SIZE))
		return -EINVAL;

	cli_write.buf->__buf = tmos_msg_allocate(len+8);
	if( !(cli_write.buf->__buf) )
	{
		APP_DBG("No enough space!");
		return -ENOMEM;
	}
	cli_write.buf->size = len+4;
	/* Init indication opcode */
	bt_mesh_model_msg_init(&(cli_write.buf->b), OP_VENDOR_MESSAGE_TRANSPARENT_WRT);
   
	/* Add tid field */
	net_buf_simple_add_u8(&(cli_write.buf->b), param->tid);
	
	net_buf_simple_add_mem(&(cli_write.buf->b), pData, len);

	memcpy(&cli_write.param, param, sizeof(struct send_param));

	vendor_model_cli_prepare(OP_VENDOR_MESSAGE_TRANSPARENT_WRT, OP_VENDOR_MESSAGE_TRANSPARENT_CFM);
	
	tmos_start_task(vendor_model_cli_TaskID, VENDOR_MODEL_CLI_WRITE_EVT, param->rand);
	
	vendor_model_cli_wait();
	return 0;
}

/*******************************************************************************
* Function Name  : adv_cli_trans_send
* Description    : 发送 透传 cli_trans 
* Input          : None
* Return         : None
*******************************************************************************/
static void adv_cli_trans_send( void )
{
	int err;
	NET_BUF_SIMPLE_DEFINE(msg, APP_MAX_TX_SIZE+8);

	struct bt_mesh_msg_ctx ctx = {
		.app_idx = cli_trans.param.app_idx ? cli_trans.param.app_idx : vendor_model_cli->model->keys[0],
		.addr = cli_trans.param.addr,
	};

	if (cli_trans.buf->__buf == NULL)
	{
		APP_DBG("NULL buf");
		return;
	}
	
	if (cli_trans.param.trans_cnt == 0)
	{
//		APP_DBG("cli_trans.buf.trans_cnt over");
		tmos_msg_deallocate(cli_trans.buf->__buf);
		cli_trans.buf->__buf = NULL;
		return;
	}

	cli_trans.param.trans_cnt --;

	ctx.send_ttl = cli_trans.param.send_ttl;

	/** TODO */
	net_buf_simple_add_mem(&msg, cli_trans.buf->data, cli_trans.buf->len);

	err = bt_mesh_model_send(vendor_model_cli->model, &ctx, &msg, NULL, NULL);
	if (err)
	{
		APP_DBG("Unable send model message (err:%d)", err);
		tmos_msg_deallocate(cli_trans.buf->__buf);
		cli_trans.buf->__buf = NULL;
		return;
	}

  if (cli_trans.param.trans_cnt == 0)
  {
//    APP_DBG("cli_trans.buf.trans_cnt over");
    tmos_msg_deallocate(cli_trans.buf->__buf);
    cli_trans.buf->__buf = NULL;
    return;
  }
	// 重传
	tmos_start_task(vendor_model_cli_TaskID, VENDOR_MODEL_CLI_TRANS_EVT, cli_trans.param.period);
}

/*******************************************************************************
* Function Name  : vendor_message_cli_trans_reset
* Description    : 取消发送trans数据的任务，释放缓存
* Input          : None
* Return         : None
*******************************************************************************/
void vendor_message_cli_trans_reset( void )
{
	tmos_msg_deallocate(cli_trans.buf->__buf);
	cli_trans.buf->__buf = NULL;
	tmos_stop_task(vendor_model_cli_TaskID, VENDOR_MODEL_CLI_TRANS_EVT);
}

/*******************************************************************************
* Function Name  : write_reset
* Description    : 调用应用层write发送完成回调，释放缓存
* Input          : write: 参数
*										err: 参考Global_Error_Code
* Return         : None
*******************************************************************************/
static void write_reset(struct bt_mesh_write *write, int err)
{
	if (write->param.cb && write->param.cb->end)
	{
		write->param.cb->end(err, write->param.cb_data);
	}

	tmos_msg_deallocate(write->buf->__buf);
	write->buf->__buf = NULL;
}

/*******************************************************************************
* Function Name  : write_start
* Description    : 发送 write 开始回调
* Input          : duration: 此发送事件即将持续的事件（单位0.625ms）
*										err: 参考Global_Error_Code
*										cb_data: 回调参数
* Return         : None
*******************************************************************************/
static void write_start(u16_t duration, int err, void *cb_data)
{
	struct bt_mesh_write *write = cb_data;

	if (write->buf->__buf == NULL)
	{
		return;
	}

	if (err)
	{
		APP_DBG("Unable send indicate (err:%d)", err);
		tmos_start_task(vendor_model_cli_TaskID, VENDOR_MODEL_CLI_WRITE_EVT, K_MSEC(100));
		return;
	}
}

/*******************************************************************************
* Function Name  : write_end
* Description    : 发送 write 结束回调
* Input          : err: 参考Global_Error_Code
*										cb_data: 回调参数
* Return         : None
*******************************************************************************/
static void write_end(int err, void *cb_data)
{
	struct bt_mesh_write *write = cb_data;
  APP_DBG("write_end (err:%d)", err);

	if (write->buf->__buf == NULL)
	{
		return;
	}

	tmos_start_task(vendor_model_cli_TaskID, VENDOR_MODEL_CLI_WRITE_EVT, write->param.period);
}

// 发送 indicate 回调结构体
const struct bt_mesh_send_cb write_cb = 
{
	.start = write_start,
	.end = write_end,
};

/*******************************************************************************
* Function Name  : adv_cli_write_send
* Description    : 发送  有应答 cli_write
* Input          : None
* Return         : None
*******************************************************************************/
static void adv_cli_write_send( void )
{
	int err;
	NET_BUF_SIMPLE_DEFINE(msg, APP_MAX_TX_SIZE+8);

	struct bt_mesh_msg_ctx ctx = {
		.app_idx = cli_write.param.app_idx ? cli_write.param.app_idx : vendor_model_cli->model->keys[0],
		.addr = cli_write.param.addr,
	};

	if (cli_write.buf->__buf == NULL)
	{
		APP_DBG("NULL buf");
		return;
	}
	
	if (cli_write.param.trans_cnt == 0)
	{
//		APP_DBG("cli_write.buf.trans_cnt over");
		write_reset(&cli_write, -ETIMEDOUT);
		return;
	}

	cli_write.param.trans_cnt --;

	ctx.send_ttl = cli_write.param.send_ttl;

	/** TODO */
	net_buf_simple_add_mem(&msg, cli_write.buf->data, cli_write.buf->len);

	err = bt_mesh_model_send(vendor_model_cli->model, &ctx, &msg, &write_cb, &cli_write);
	if (err)
	{
		APP_DBG("Unable send model message (err:%d)", err);

		write_reset(&cli_write, err);
		return;
	}
}


/*******************************************************************************
* Function Name  : vendor_model_cli_init
* Description    : vendor_model_cli 初始化
* Input          : model;
* Return         : SUCCESS
*******************************************************************************/
static int vendor_model_cli_init(struct bt_mesh_model *model)
{
	vendor_model_cli = model->user_data;
	vendor_model_cli->model = model;

  vendor_model_cli_TaskID = TMOS_ProcessEventRegister( vendor_model_cli_ProcessEvent );
	return 0;
}

/*******************************************************************************
* Function Name  : vendor_model_cli_ProcessEvent
* Description    : vendor_model_cli 任务处理
* Input          : task_id, events
* Return         : events
*******************************************************************************/
static uint16 vendor_model_cli_ProcessEvent( uint8 task_id, uint16 events )
{

  if ( events & VENDOR_MODEL_CLI_TRANS_EVT )
  {
		adv_cli_trans_send();
		return ( events ^ VENDOR_MODEL_CLI_TRANS_EVT );
  }
  if ( events & VENDOR_MODEL_CLI_RSP_TIMEOUT_EVT )
  {
		vendor_cli_sync_handler();
		return ( events ^ VENDOR_MODEL_CLI_RSP_TIMEOUT_EVT );
  }
  if ( events & VENDOR_MODEL_CLI_WRITE_EVT )
  {
		adv_cli_write_send();
		return ( events ^ VENDOR_MODEL_CLI_WRITE_EVT );
  }
  // Discard unknown events
  return 0;
}


const struct bt_mesh_model_cb bt_mesh_vendor_model_cli_cb = {
	.init = vendor_model_cli_init,
};

/******************************** endfile @ main ******************************/

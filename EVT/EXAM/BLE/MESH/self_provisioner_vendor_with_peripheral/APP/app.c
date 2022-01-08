/********************************** (C) COPYRIGHT *******************************
* File Name          : app.c
* Author             : WCH
* Version            : V1.0
* Date               : 2021/03/22
* Description        : 
*******************************************************************************/



/******************************************************************************/
#include "CONFIG.h"
#include "CH58x_common.h"
#include "MESH_LIB.h"
#include "app_vendor_model_srv.h"
#include "app_vendor_model_cli.h"
#include "app.h"
#include "peripheral.h"
#include "HAL.h"

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
#define ADV_TIMEOUT		K_MINUTES(10)


#define SELENCE_ADV_ON	0x01
#define SELENCE_ADV_OF	0x00

/*********************************************************************
 * GLOBAL TYPEDEFS
 */

static u8_t MESH_MEM[1024*2+30*CONFIG_MESH_PROV_NODE_COUNT_DEF]={0};

extern const ble_mesh_cfg_t app_mesh_cfg;
extern const struct device app_dev;

static uint8 App_TaskID = 0;    // Task ID for internal task/event processing

static uint16 App_ProcessEvent( uint8 task_id, uint16 events );

static uint8_t dev_uuid[16]={0};								// 此设备的UUID
u8 MACAddr[6];																					// 此设备的mac

static const u8_t self_prov_net_key[16] 	= {
	0x00, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x00, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

static const u8_t self_prov_dev_key[16] 	= {
	0x00, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x00, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

static const u8_t self_prov_app_key[16] 	= {
	0x00, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x00, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

const u16_t self_prov_net_idx 		=	0x0000;					// 自配网所用的net key
const u16_t self_prov_app_idx 		=	0x0001;					// 自配网所用的app key
const u32_t self_prov_iv_index		=	0x00000000;			// 自配网的iv_index
const u16_t self_prov_addr 				= 0x0001;					// 自配网的自身主元素地址
const u8_t self_prov_flags				=	0x00;						// 是否处于key更新状态，默认为否
const u16_t vendor_sub_addr 			= 0xC000;					// 配置自定义模型的订阅group地址

#if (!CONFIG_BLE_MESH_PB_GATT)
NET_BUF_SIMPLE_DEFINE_STATIC(rx_buf, 65);
#endif /* !PB_GATT */

/*********************************************************************
 * LOCAL FUNCION
 */

static void link_open(bt_mesh_prov_bearer_t bearer);
static void link_close(bt_mesh_prov_bearer_t bearer, u8_t reason);
static void prov_complete(u16_t net_idx, u16_t addr, u8_t flags, u32_t iv_index);
static void unprov_recv(bt_mesh_prov_bearer_t bearer,
                        const u8_t uuid[16], bt_mesh_prov_oob_info_t oob_info,
					    const unprivison_info_t *info);
static void node_added(u16_t net_idx, u16_t addr, u8_t num_elem);
static void cfg_cli_rsp_handler(const cfg_cli_status_t *val);
static void vendor_model_cli_rsp_handler(const vendor_model_cli_status_t *val);
static void node_init(void);

static struct bt_mesh_cfg_srv cfg_srv = {
#if (CONFIG_BLE_MESH_RELAY)
	.relay = BLE_MESH_RELAY_ENABLED,
#endif
	.beacon = BLE_MESH_BEACON_DISABLED,
#if (CONFIG_BLE_MESH_FRIEND)
	.frnd = BLE_MESH_FRIEND_ENABLED,
#endif
#if (CONFIG_BLE_MESH_PROXY)
	.gatt_proxy = BLE_MESH_GATT_PROXY_ENABLED,
#endif
  /* 默认TTL为3 */
  .default_ttl = 3,
  /* 底层发送数据重试7次，每次间隔10ms（不含内部随机数） */
  .net_transmit = BLE_MESH_TRANSMIT(7, 10),
  /* 底层转发数据重试7次，每次间隔10ms（不含内部随机数） */
  .relay_retransmit = BLE_MESH_TRANSMIT(7, 10),
};

static struct bt_mesh_health_srv health_srv;

BLE_MESH_HEALTH_PUB_DEFINE(health_pub, 8);

struct bt_mesh_cfg_cli cfg_cli = {
	.handler = cfg_cli_rsp_handler,
};

u16_t cfg_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF]={ BLE_MESH_KEY_UNUSED };
u16_t cfg_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF]={	BLE_MESH_ADDR_UNASSIGNED };

u16_t cfg_cli_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF]={ BLE_MESH_KEY_UNUSED };
u16_t cfg_cli_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF]={	BLE_MESH_ADDR_UNASSIGNED };

u16_t health_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF]={ BLE_MESH_KEY_UNUSED };
u16_t health_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF]={	BLE_MESH_ADDR_UNASSIGNED };

// root模型加载
static struct bt_mesh_model root_models[] = {
	BLE_MESH_MODEL_CFG_SRV(cfg_srv_keys, cfg_srv_groups, &cfg_srv),
	BLE_MESH_MODEL_CFG_CLI(cfg_cli_keys, cfg_cli_groups, &cfg_cli),
	BLE_MESH_MODEL_HEALTH_SRV(health_srv_keys, health_srv_groups, &health_srv, &health_pub),
};

struct bt_mesh_vendor_model_cli vendor_model_cli = {
	.cli_tid.trans_tid = 0xFF,
	.cli_tid.ind_tid = 0xFF,
	.handler = vendor_model_cli_rsp_handler,
};

u16_t vnd_model_cli_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF]={ BLE_MESH_KEY_UNUSED };
u16_t vnd_model_cli_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF]={	BLE_MESH_ADDR_UNASSIGNED };

// 自定义模型加载
struct bt_mesh_model vnd_models[] = {
  BLE_MESH_MODEL_VND_CB(CID_WCH, BLE_MESH_MODEL_ID_WCH_CLI, vnd_model_cli_op, NULL, vnd_model_cli_keys, 
												vnd_model_cli_groups, &vendor_model_cli, &bt_mesh_vendor_model_cli_cb),
};

// 模型组成 elements
static struct bt_mesh_elem elements[] = {
	{
    /* Location Descriptor (GATT Bluetooth Namespace Descriptors) */
		.loc              = (0),
		.model_count      = ARRAY_SIZE(root_models),
		.models           = (root_models),
		.vnd_model_count 	= ARRAY_SIZE(vnd_models),
		.vnd_models       = (vnd_models),
	}
};

// elements 构成 Node Composition
const struct bt_mesh_comp app_comp = {
	.cid = 0x07D7,			// WCH 公司id
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

// 配网参数和回调
static const struct bt_mesh_prov app_prov = {
	.uuid 			= dev_uuid,
	.link_open		= link_open,
	.link_close		= link_close,
	.complete 		= prov_complete,
	.unprovisioned_beacon = unprov_recv,
	.node_added = node_added,
};

// 配网者管理的节点，第0个为自己，第1，2依次为配网顺序的节点
node_t app_nodes[ 1 + CONFIG_MESH_PROV_NODE_COUNT_DEF ]={0};


/*********************************************************************
 * GLOBAL TYPEDEFS
 */


/*******************************************************************************
* Function Name  : link_open
* Description    : 配网前的link建立回调，停止广播
* Input          : bearer
* Return         : None
*******************************************************************************/
static void link_open(bt_mesh_prov_bearer_t bearer)
{
	APP_DBG("");
}

/*******************************************************************************
* Function Name  : link_close
* Description    : 配网后的link关闭回调
* Input          : reason : link close reason 见头文件相关描述
* Return         : None
*******************************************************************************/
static void link_close(bt_mesh_prov_bearer_t bearer, u8_t reason)
{
	APP_DBG("reason %x",reason);
	if( reason == CLOSE_REASON_RESOURCES )
	{
		// 存储的节点已满，可选择 停止发起配网 或 清除全部节点 或 按地址清除节点(注意应用层管理的节点也需要对应清除)
		bt_mesh_provisioner_disable(BLE_MESH_PROV_ADV, TRUE);
		//bt_mesh_node_clear();node_init();
		//bt_mesh_node_del_by_addr(app_nodes[1].node_addr);
	}
	else
		bt_mesh_provisioner_enable(BLE_MESH_PROV_ADV);
}

/*******************************************************************************
* Function Name  : node_unblock_get
* Description    : 获取 unblocked node
* Input          : None
* Return         : node_t / NULL
*******************************************************************************/
static node_t * node_unblock_get(void)
{
	for (int i = 0; i < ARRAY_SIZE(app_nodes); i++)
	{
		if (app_nodes[i].node_addr != BLE_MESH_ADDR_UNASSIGNED &&
			app_nodes[i].fixed != TRUE &&
			app_nodes[i].blocked == FALSE)
		{
			return &app_nodes[i];
		}
	}
	return NULL;
}

/*******************************************************************************
* Function Name  : node_block_get
* Description    : 获取 blocked node
* Input          : None
* Return         : node_t / NULL
*******************************************************************************/
static node_t * node_block_get(void)
{
	for (int i = 0; i < ARRAY_SIZE(app_nodes); i++)
	{
		if (app_nodes[i].node_addr != BLE_MESH_ADDR_UNASSIGNED &&
			app_nodes[i].fixed != TRUE &&
			app_nodes[i].blocked == TRUE)
		{
			return &app_nodes[i];
		}
	}
	return NULL;
}

/*******************************************************************************
* Function Name  : node_work_handler
* Description    : node 任务到期执行，判断是否还有未配置完成的节点，调用节点配置函数
* Input          : None
* Return         : TRUE: 继续执行配置节点
*										FALSE: 节点配置完成，停止任务
*******************************************************************************/
static BOOL node_work_handler( void )
{
	node_t *node;
	
	node = node_unblock_get();
	if (!node)
	{
		APP_DBG("Unable find Unblocked Node");
		return FALSE;
	}

	if (node->retry_cnt -- == 0)
	{
		APP_DBG("Ran Out of Retransmit");
		goto unblock;
	}
	
	if (!node->cb->stage(node))
	{
		return FALSE;
	}

unblock:
	
	node->fixed = TRUE;

	node = node_block_get();
	if (node)
	{
		node->blocked = FALSE;
		return TRUE;
	}
	return FALSE;
}

/*******************************************************************************
* Function Name  : node_init
* Description    : node 初始化
* Input          : None
* Return         : None
*******************************************************************************/
static void node_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(app_nodes); i++)
	{
		app_nodes[i].stage.node = NODE_INIT;
		app_nodes[i].node_addr = BLE_MESH_ADDR_UNASSIGNED;
	}
}

/*******************************************************************************
* Function Name  : free_node_get
* Description    : 获取一个空的node
* Input          : None
* Return         : node_t / NULL
*******************************************************************************/
static node_t * free_node_get(void)
{	
	for (int i = 0; i < ARRAY_SIZE(app_nodes); i++)
	{
		if (app_nodes[i].node_addr == BLE_MESH_ADDR_UNASSIGNED)
		{
			return &app_nodes[i];
		}
	}
	return NULL;
}

/*******************************************************************************
* Function Name  : node_get
* Description    : 获取匹配的node
* Input          : None
* Return         : node_t / NULL
*******************************************************************************/
static node_t * node_get(u16_t node_addr)
{	
	for (int i = 0; i < ARRAY_SIZE(app_nodes); i++)
	{
		if (app_nodes[i].node_addr == node_addr)
		{
			return &app_nodes[i];
		}
	}
	return NULL;
}

/*******************************************************************************
* Function Name  : node_should_blocked
* Description    : 判断是否有阻塞的node
* Input          : None
* Return         : TRUE: node未完成配置
*										FALSE: node已经完成配置或不需要配置
*******************************************************************************/
static BOOL node_should_blocked(u16_t node_addr)
{
	for (int i = 0; i < ARRAY_SIZE(app_nodes); i++)
	{
		if (app_nodes[i].node_addr != BLE_MESH_ADDR_UNASSIGNED &&
			app_nodes[i].node_addr != node_addr &&
			app_nodes[i].fixed != TRUE &&
			app_nodes[i].blocked == FALSE)
		{
			return TRUE;
		}
	}
	return FALSE;
}

/*******************************************************************************
* Function Name  : node_cfg_process
* Description    : 找一个空闲的节点，执行配置流程
* Input          : node参数
* Return         : node_t / NULL
*******************************************************************************/
static node_t * node_cfg_process(node_t *node, u16_t net_idx, u16_t addr, u8_t num_elem)
{
	if (!node)
	{
		node = free_node_get();
		if (!node)
		{
			APP_DBG("No Free Node Object Available");
			return NULL;
		}
		node->net_idx = net_idx;
		node->node_addr = addr;
		node->elem_count = num_elem;
	}

	node->blocked = node_should_blocked(addr);
		
	if (!node->blocked)
	{
		tmos_set_event(App_TaskID, APP_NODE_EVT);
	}
	return node;
}

/*******************************************************************************
* Function Name  : node_stage_set
* Description    : 设置远端node配置的下一个阶段
* Input          : new_stage: 新阶段
* Return         : None
*******************************************************************************/
static void node_stage_set(node_t *node, node_stage_t new_stage)
{
	node->retry_cnt = 3;
	node->stage.node = new_stage;
}

/*******************************************************************************
* Function Name  : local_stage_set
* Description    : 设置本地node配置的下一个阶段
* Input          : new_stage: 新阶段
* Return         : None
*******************************************************************************/
static void local_stage_set(node_t *node, local_stage_t new_stage)
{
	node->retry_cnt = 1;
	node->stage.local = new_stage;
}

/*******************************************************************************
* Function Name  : node_rsp
* Description    : 每执行一个远端节点配置流程的回调
* Input          : p1：要配置的远端node
*										p2：当前的状态
* Return         : None
*******************************************************************************/
static void node_rsp(void *p1, const void *p2)
{
	node_t *node = p1;
	const cfg_cli_status_t *val = p2;

	switch (val->cfgHdr.opcode)
	{
		case OP_APP_KEY_ADD:
			APP_DBG("node Application Key Added");
			node_stage_set(node, NODE_MOD_BIND_SET);
			break;
		case OP_MOD_APP_BIND:
			APP_DBG("node vendor Model Binded");
			node_stage_set(node, NODE_MOD_SUB_SET);
			break;
		case OP_MOD_SUB_ADD:
			APP_DBG("node vendor Model Subscription Set");
			node_stage_set(node, NODE_CONFIGURATIONED);
			break;
		default:
			APP_DBG("Unknown Opcode (0x%04x)", val->cfgHdr.opcode);
			return;
	}
}

/*******************************************************************************
* Function Name  : local_rsp
* Description    : 每执行一个本地节点配置流程的回调
* Input          : p1：要配置的本地node
*										p2：当前的状态
* Return         : None
*******************************************************************************/
static void local_rsp(void *p1, const void *p2)
{
	node_t *node = p1;
	const cfg_cli_status_t *val = p2;

	switch (val->cfgHdr.opcode)
	{
		case OP_APP_KEY_ADD:
			APP_DBG("local Application Key Added");
			local_stage_set(node, LOCAL_MOD_BIND_SET);
			break;
		case OP_MOD_APP_BIND:
			APP_DBG("local vendor Model Binded");
			local_stage_set(node, LOCAL_CONFIGURATIONED);
			break;
		default:
			APP_DBG("Unknown Opcode (0x%04x)", val->cfgHdr.opcode);
			return;
	}
}

/*******************************************************************************
* Function Name  : node_stage
* Description    : 远端节点配置，添加app key，并为自定义服务绑定app key
* Input          : p1：要配置的本地node
* Return         : TRUE: 
*******************************************************************************/
static BOOL node_stage(void *p1)
{
	int err;
	BOOL ret = FALSE;
	node_t *node = p1;

	switch (node->stage.node)
	{
		case NODE_APPKEY_ADD:
			err = bt_mesh_cfg_app_key_add(node->net_idx, node->node_addr, self_prov_net_idx, self_prov_app_idx, self_prov_app_key);
			if (err)
			{
				APP_DBG("Unable to adding Application key (err %d)", err);
				ret = TRUE;
			}
			break;

		case NODE_MOD_BIND_SET:
			err = bt_mesh_cfg_mod_app_bind_vnd(node->net_idx, node->node_addr, node->node_addr, self_prov_app_idx, BLE_MESH_MODEL_ID_WCH_SRV, CID_WCH);
			if (err)
			{
				APP_DBG("Unable to Binding vendor Model (err %d)", err);
				ret = TRUE;
			}
			break;

			// 设置模型订阅
		case NODE_MOD_SUB_SET:
			err = bt_mesh_cfg_mod_sub_add_vnd(node->net_idx, node->node_addr, node->node_addr, vendor_sub_addr, BLE_MESH_MODEL_ID_WCH_SRV, CID_WCH);
			if (err)
			{
				APP_DBG("Unable to Set vendor Model Subscription (err %d)", err);
				ret = TRUE;
			}
			break;

		default:
			ret = TRUE;
			break;
	}
	
	return ret;
}

/*******************************************************************************
* Function Name  : local_stage
* Description    : 本地节点配置，添加app key，并为自定义客户端绑定app key
* Input          : None
* Return         : None
*******************************************************************************/
static BOOL local_stage(void *p1)
{
	int err;
	BOOL ret = FALSE;
	node_t *node = p1;

	switch (node->stage.local)
	{
		case LOCAL_APPKEY_ADD:
			err = bt_mesh_cfg_app_key_add(node->net_idx, node->node_addr, self_prov_net_idx, self_prov_app_idx, self_prov_app_key);
			if (err)
			{
				APP_DBG("Unable to adding Application key (err %d)", err);
				ret = 1;
			}
			break;

		case LOCAL_MOD_BIND_SET:			
			err = bt_mesh_cfg_mod_app_bind_vnd(node->net_idx, node->node_addr, node->node_addr, self_prov_app_idx, BLE_MESH_MODEL_ID_WCH_CLI,CID_WCH);
			if (err)
			{
				APP_DBG("Unable to Binding vendor Model (err %d)", err);
				ret = 1;
			}
			break;

		default:
			ret = 1;
			break;
	}
	
	return ret;
}

static const cfg_cb_t node_cfg_cb = {
	node_rsp,
	node_stage,
};

static const cfg_cb_t local_cfg_cb = {
	local_rsp,
	local_stage,
};

/*******************************************************************************
* Function Name  : prov_complete
* Description    : 自配网完成回调，完成后开始本地节点配置流程
* Input          : 配网的节点参数
* Return         : None
*******************************************************************************/
static void prov_complete(u16_t net_idx, u16_t addr, u8_t flags, u32_t iv_index)
{
	int err;
	node_t *node;

	APP_DBG("");

	err = bt_mesh_provisioner_enable(BLE_MESH_PROV_ADV);
	if (err)
	{
		APP_DBG("Unabled Enable Provisoner (err:%d)", err);
	}

	node = node_get(addr);
	if (!node || !node->fixed)
	{
		node = node_cfg_process(node, net_idx, addr, ARRAY_SIZE(elements));
		if (!node)
		{
			APP_DBG("Unable allocate node object");
			return;
		}
		
		node->cb = &local_cfg_cb;
		local_stage_set(node, LOCAL_APPKEY_ADD);
	}
}

/*******************************************************************************
* Function Name  : unprov_recv
* Description    : 收到未配网的数据，发起配网
* Input          : uuid 收到的设备发出广播内包含的设备uuid，可用于区分不同的设备
* Return         : None
*******************************************************************************/
static void unprov_recv(bt_mesh_prov_bearer_t bearer,
                        const u8_t uuid[16], bt_mesh_prov_oob_info_t oob_info,
												const unprivison_info_t *info)
{
	APP_DBG("");
	int err;
	
	if (bearer & BLE_MESH_PROV_ADV)
	{
		err = bt_mesh_provision_adv(uuid, self_prov_net_idx, BLE_MESH_ADDR_UNASSIGNED, 5);
		if (err)
		{
			APP_DBG("Unable Open PB-ADV Session (err:%d)", err);
		}
	}
}

/*******************************************************************************
* Function Name  : node_added
* Description    : 远端节点配网成功，添加到本地节点管理，并开始远端节点配置流程
* Input          : net_idx  节点使用的网络序号
*                   addr    节点的网络地址（主元素地址）
* Return         : None
*******************************************************************************/
static void node_added(u16_t net_idx, u16_t addr, u8_t num_elem)
{
	node_t *node;
	APP_DBG("");

	node = node_get(addr);
	if (!node || !node->fixed)
	{
		node = node_cfg_process(node, net_idx, addr, num_elem);
		if (!node)
		{
			APP_DBG("Unable allocate node object");
			return;
		}

		node->cb = &node_cfg_cb;
		node_stage_set(node, NODE_APPKEY_ADD);
	}
}

/*******************************************************************************
* Function Name  : cfg_cli_rsp_handler
* Description    : 收到cfg命令的应答回调
* Input          : None
* Return         : None
*******************************************************************************/
static void cfg_cli_rsp_handler(const cfg_cli_status_t *val)
{
	node_t *node;
	APP_DBG("");

	node = node_unblock_get();
	if (!node)
	{
		APP_DBG("Unable find Unblocked Node");
		return;
	}

	if (val->cfgHdr.status == 0xFF)
	{
		APP_DBG("Opcode 0x%04x, timeout", val->cfgHdr.opcode);
		goto end;
	}

	node->cb->rsp(node, val);

end:
	tmos_start_task(App_TaskID, APP_NODE_EVT, K_SECONDS(1));
}

/*******************************************************************************
* Function Name  : vendor_model_cli_rsp_handler
* Description    : vendor_model cli 收到回调
* Input          : None
* Return         : None
*******************************************************************************/
static void vendor_model_cli_rsp_handler(const vendor_model_cli_status_t *val)
{
	if( val->vendor_model_cli_Hdr.status)
	{
		// 有应答数据传输 超时未收到应答
		APP_DBG("Timeout opcode 0x%02x",val->vendor_model_cli_Hdr.opcode);
		return;
	}
	if( val->vendor_model_cli_Hdr.opcode == OP_VENDOR_MESSAGE_TRANSPARENT_MSG )
	{
		// 收到透传数据
		APP_DBG("trans len %d, data 0x%02x",val->vendor_model_cli_Event.trans.len,
																	val->vendor_model_cli_Event.trans.pdata[0]);
		// 转发给主机(如果已连接)
		peripheralChar4Notify(val->vendor_model_cli_Event.trans.pdata, val->vendor_model_cli_Event.trans.len);
	}
	else if( val->vendor_model_cli_Hdr.opcode == OP_VENDOR_MESSAGE_TRANSPARENT_IND )
	{		
		// 收到indicate数据
		APP_DBG("ind len %d, data 0x%02x",val->vendor_model_cli_Event.ind.len,
																	val->vendor_model_cli_Event.ind.pdata[0]);
		// 转发给主机(如果已连接)
		peripheralChar4Notify(val->vendor_model_cli_Event.trans.pdata, val->vendor_model_cli_Event.trans.len);
	}
	else if( val->vendor_model_cli_Hdr.opcode == OP_VENDOR_MESSAGE_TRANSPARENT_WRT )
	{		
		// 收到write的应答
	}
	else
	{
		APP_DBG("Unknow opcode 0x%02x",val->vendor_model_cli_Hdr.opcode);
	}
}

/*******************************************************************************
* Function Name  : keyPress
* Description    : 按键回调，按下向第二个配网的节点发送透传数据
* Input          : keys: 触发的按键
* Return         : None
*******************************************************************************/
void keyPress(uint8 keys)
{
	APP_DBG("%d", keys);

	switch (keys)
	{
		default:
		{
			if(app_nodes[1].node_addr)
			{
				uint8 status;
				APP_DBG("node1_addr %x",app_nodes[1].node_addr);
				struct send_param param = {
					.app_idx = self_prov_app_idx,			// 此消息使用的app key
					.addr = app_nodes[1].node_addr,		// 此消息发往的目的地地址，此处为第1个配网的节点
					.trans_cnt = 0x03,								// 此消息的重传次数
					.period = K_MSEC(400),						// 此消息重传的间隔，建议不小于(200+50*TTL)ms，若数据较大则建议加长
					.rand = (0),											// 此消息发送的随机延迟
					.tid = vendor_cli_tid_get(),			// tid，每个独立消息递增循环，cli使用0~127
					.send_ttl = BLE_MESH_TTL_DEFAULT,	// ttl，无特定则使用默认值
				};
				uint8 data[16] = {0,1,2,3,4,5,6,7,8,9,11,12,13,14,15,16};
				status = vendor_message_cli_write(&param, data, 16);	// 调用自定义模型客户端的有应答写函数发送数据
				if( status )	APP_DBG("write failed %d",status);
			}			
			break;
		}
	}
}

/*******************************************************************************
* Function Name  : blemesh_on_sync
* Description    : 同步mesh参数，启用对应功能，不建议修改
* Input          : None
* Return         : None
*******************************************************************************/
void blemesh_on_sync(void)
{
  int err;
	mem_info_t info;
	
	if( tmos_memcmp( VER_MESH_LIB,VER_MESH_FILE,strlen(VER_MESH_FILE)) == FALSE )
	{
		PRINT("head file error...\n");
		while(1);
  }

	info.base_addr = MESH_MEM;
	info.mem_len = ARRAY_SIZE(MESH_MEM);
#if (CONFIG_BLE_MESH_FRIEND)
	friend_init_register(bt_mesh_friend_init);
#endif /* FRIEND */
#if (CONFIG_BLE_MESH_LOW_POWER)
	lpn_init_register(bt_mesh_lpn_init);
#endif /* LPN */
	GetMACAddress( MACAddr );
	tmos_memcpy(dev_uuid, MACAddr, 6);
	err = bt_mesh_cfg_set(&app_mesh_cfg, &app_dev, MACAddr, &info);
	if (err)
	{
		APP_DBG("Unable set configuration (err:%d)", err);
		return;
	}
  hal_rf_init();
  err = bt_mesh_comp_register(&app_comp);
	node_init();
	
#if (CONFIG_BLE_MESH_RELAY)
	bt_mesh_relay_init();
#endif /* RELAY  */
#if (CONFIG_BLE_MESH_PROXY || CONFIG_BLE_MESH_PB_GATT)
#if (CONFIG_BLE_MESH_PROXY )
	bt_mesh_proxy_beacon_init_register( (void*) bt_mesh_proxy_beacon_init );
	gatts_notify_register( bt_mesh_gatts_notify );
	proxy_gatt_enable_register( bt_mesh_proxy_gatt_enable );
#endif /* PROXY  */
#if (CONFIG_BLE_MESH_PB_GATT )
	proxy_prov_enable_register( bt_mesh_proxy_prov_enable );
#endif /* PB_GATT  */
	bt_mesh_proxy_init();
#endif /* PROXY || PB-GATT */

#if (CONFIG_BLE_MESH_PROXY_CLI)
  bt_mesh_proxy_client_init(cli);				//待添加
#endif /* PROXY_CLI */

	bt_mesh_prov_retransmit_init();
	
#if (!CONFIG_BLE_MESH_PB_GATT)
	adv_link_rx_buf_register(&rx_buf);
#endif /* !PB_GATT */
	err = bt_mesh_prov_init(&app_prov);

	bt_mesh_mod_init();
	bt_mesh_net_init();
	bt_mesh_trans_init();
	bt_mesh_beacon_init();
	bt_mesh_adv_init();
		
#if ((CONFIG_BLE_MESH_PB_GATT) || (CONFIG_BLE_MESH_PROXY) || (CONFIG_BLE_MESH_OTA))
	bt_mesh_conn_adv_init();
#endif /* PROXY || PB-GATT || OTA */
		
#if (CONFIG_BLE_MESH_SETTINGS)
  bt_mesh_settings_init();
#endif /* SETTINGS */
		
#if (CONFIG_BLE_MESH_PROXY_CLI)
	bt_mesh_proxy_cli_adapt_init();
#endif /* PROXY_CLI */
		
#if ((CONFIG_BLE_MESH_PROXY) || (CONFIG_BLE_MESH_PB_GATT) || \
	(CONFIG_BLE_MESH_PROXY_CLI) || (CONFIG_BLE_MESH_OTA))
  bt_mesh_adapt_init();
#endif /* PROXY || PB-GATT || PROXY_CLI || OTA */

#if (CONFIG_BLE_MESH_LOW_POWER)
	bt_mesh_lpn_set(TRUE);
#endif /* LPN */

	if (err)
	{
			APP_DBG("Initializing mesh failed (err %d)", err);
			return;
	}
	APP_DBG("Bluetooth initialized");

#if (CONFIG_BLE_MESH_SETTINGS)
	settings_load();
#endif /* SETTINGS */

	if (bt_mesh_is_provisioned())
	{
		APP_DBG("Mesh network restored from flash");
	}
	else
	{
    err = bt_mesh_provision(self_prov_net_key, self_prov_net_idx, self_prov_flags,
														self_prov_iv_index, self_prov_addr, self_prov_dev_key);
		if (err)
		{
			APP_DBG("Self Privisioning (err %d)", err);
			return;
		}
	}
	APP_DBG("Mesh initialized");
}

/*******************************************************************************
* Function Name  : App_Init
* Description    : 初始化
* Input          : None
* Return         : None
*******************************************************************************/
void App_Init()
{
	GAPRole_PeripheralInit();
	Peripheral_Init();
	
  App_TaskID = TMOS_ProcessEventRegister( App_ProcessEvent );
	
	blemesh_on_sync();
	
	HAL_KeyInit();
	HalKeyConfig( keyPress );
	
	// 添加一个测试任务，定时向第一个配网的设备发送透传数据
	tmos_start_task(App_TaskID, APP_NODE_TEST_EVT, 4800);
}

/*******************************************************************************
* Function Name  : App_ProcessEvent
* Description    : 
* Input          : None
* Return         : None
*******************************************************************************/
static uint16 App_ProcessEvent( uint8 task_id, uint16 events )
{
	// 节点配置任务事件处理
  if ( events & APP_NODE_EVT )
  {
		if (node_work_handler() )
			return ( events );
		else
			return ( events ^ APP_NODE_EVT );
  }
	
	// 测试任务事件处理
  if ( events & APP_NODE_TEST_EVT )
  {
		if(app_nodes[1].node_addr)
		{
			uint8 status;
			APP_DBG("app_nodes[1] ADDR %x",app_nodes[1].node_addr);
			struct send_param param = {
				.app_idx = self_prov_app_idx,			// 此消息使用的app key
				.addr = app_nodes[1].node_addr,		// 此消息发往的目的地地址，此处为配置远端节点1的地址
				.trans_cnt = 0x03,								// 此消息的重传次数
				.period = K_MSEC(400),						// 此消息重传的间隔，建议不小于(200+50*TTL)ms，若数据较大则建议加长
				.rand = (0),											// 此消息发送的随机延迟
				.tid = vendor_cli_tid_get(),			// tid，每个独立消息递增循环，cli使用0~127
				.send_ttl = BLE_MESH_TTL_DEFAULT,	// ttl，无特定则使用默认值
			};
			uint8 data[16] = {0,1,2,3,4,5,6,7,8,9,11,12,13,14,15,16};
			status = vendor_message_cli_send_trans(&param, data, 4);	// 调用自定义模型客户端的透传函数发送数据
			if( status )	APP_DBG("trans failed %d",status);
		}
		tmos_start_task(App_TaskID, APP_NODE_TEST_EVT, 2400);
		return ( events ^ APP_NODE_TEST_EVT );
  }
	
  // Discard unknown events
  return 0;
}

/******************************** endfile @ main ******************************/

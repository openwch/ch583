/********************************** (C) COPYRIGHT *******************************
* File Name          : app.c
* Author             : WCH
* Version            : V1.0
* Date               : 2021/03/24
* Description        : 
*******************************************************************************/



/******************************************************************************/
#include "CONFIG.h"
#include "CH58x_common.h"
#include "MESH_LIB.h"
#include "app_vendor_model_srv.h"
#include "app.h"
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

static u8_t MESH_MEM[1024 * 3]={0};

extern const ble_mesh_cfg_t app_mesh_cfg;
extern const struct device app_dev;

static uint8 App_TaskID = 0;    // Task ID for internal task/event processing

static uint16 App_ProcessEvent( uint8 task_id, uint16 events );

static uint8_t dev_uuid[16]={0};								// 此设备的UUID
u8 MACAddr[6];																						// 此设备的mac


#if (!CONFIG_BLE_MESH_PB_GATT)
NET_BUF_SIMPLE_DEFINE_STATIC(rx_buf, 65);
#endif /* !PB_GATT */

/*********************************************************************
 * LOCAL FUNCION
 */

static void link_open(bt_mesh_prov_bearer_t bearer);
static void link_close(bt_mesh_prov_bearer_t bearer, u8_t reason);
static void prov_complete(u16_t net_idx, u16_t addr, u8_t flags, u32_t iv_index);
static void vendor_model_srv_rsp_handler(const vendor_model_srv_status_t *val);

static struct bt_mesh_cfg_srv cfg_srv = {
	.relay = BLE_MESH_RELAY_ENABLED,
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

/* Attention on */
void app_prov_attn_on(struct bt_mesh_model *model)
{
	APP_DBG("app_prov_attn_on");
}

/* Attention off */
void app_prov_attn_off(struct bt_mesh_model *model)
{
	APP_DBG("app_prov_attn_off");
}

const struct bt_mesh_health_srv_cb health_srv_cb={
	.attn_on = app_prov_attn_on,
	.attn_off = app_prov_attn_off,
};

static struct bt_mesh_health_srv health_srv={
	.cb = &health_srv_cb,
};

BLE_MESH_HEALTH_PUB_DEFINE(health_pub, 8);

u16_t cfg_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF]={ BLE_MESH_KEY_UNUSED };
u16_t cfg_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF]={	BLE_MESH_ADDR_UNASSIGNED };

u16_t health_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF]={ BLE_MESH_KEY_UNUSED };
u16_t health_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF]={	BLE_MESH_ADDR_UNASSIGNED };

// root模型加载
static struct bt_mesh_model root_models[] = {
	BLE_MESH_MODEL_CFG_SRV(cfg_srv_keys, cfg_srv_groups, &cfg_srv),
	BLE_MESH_MODEL_HEALTH_SRV(health_srv_keys, health_srv_groups, &health_srv, &health_pub),
};

struct bt_mesh_vendor_model_srv vendor_model_srv = {
	.srv_tid.trans_tid = 0xFF,
	.handler = vendor_model_srv_rsp_handler,
};

u16_t vnd_model_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF]={ BLE_MESH_KEY_UNUSED };
u16_t vnd_model_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF]={	BLE_MESH_ADDR_UNASSIGNED };

// 自定义模型加载
struct bt_mesh_model vnd_models[] = {
  BLE_MESH_MODEL_VND_CB(CID_WCH, BLE_MESH_MODEL_ID_WCH_SRV, vnd_model_srv_op, NULL, vnd_model_srv_keys, 
												vnd_model_srv_groups, &vendor_model_srv, &bt_mesh_vendor_model_srv_cb),
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
};


/*********************************************************************
 * GLOBAL TYPEDEFS
 */

/*******************************************************************************
* Function Name  : prov_enable
* Description    : 使能配网功能，不建议修改
* Input          : None
* Return         : None
*******************************************************************************/
static void prov_enable(void)
{
	if (bt_mesh_is_provisioned())
	{
			return;
	}

	// Make sure we're scanning for provisioning inviations
	bt_mesh_scan_enable();
	// Enable unprovisioned beacon sending
	bt_mesh_beacon_enable();

	if ( CONFIG_BLE_MESH_PB_GATT )
	{
			bt_mesh_proxy_prov_enable();
	}
}

/*******************************************************************************
* Function Name  : link_open
* Description    : 配网前的link建立回调
* Input          : None
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
	if( reason != CLOSE_REASON_SUCCESS )
		APP_DBG("reason %x",reason);
}

/*******************************************************************************
* Function Name  : prov_complete
* Description    : 配网完成回调
* Input          : None
* Return         : None
*******************************************************************************/
static void prov_complete(u16_t net_idx, u16_t addr, u8_t flags, u32_t iv_index)
{
	APP_DBG("net_idx %x, addr %x",net_idx,addr);
}

/*******************************************************************************
* Function Name  : friend_state
* Description    : 朋友关系建立回调
* Input          : None
* Return         : None
*******************************************************************************/
static void friend_state(u16_t lpn_addr, u8_t state)
{
	if( state == FRIEND_FRIENDSHIP_ESTABLISHED )
	{
		APP_DBG("friend friendship established, lpn addr 0x%04x",lpn_addr);
	}
	else if( state == FRIEND_FRIENDSHIP_TERMINATED )
	{
		APP_DBG("friend friendship terminated, lpn addr 0x%04x",lpn_addr);
	}
	else
	{
		APP_DBG("unknow state %x",state);
	}
}

/*******************************************************************************
* Function Name  : vendor_model_srv_rsp_handler
* Description    : 自定义模型服务回调
* Input          : None
* Return         : None
*******************************************************************************/
static void vendor_model_srv_rsp_handler(const vendor_model_srv_status_t *val)
{
	if( val->vendor_model_srv_Hdr.status)
	{
		// 有应答数据传输 超时未收到应答
		APP_DBG("Timeout opcode 0x%02x",val->vendor_model_srv_Hdr.opcode);
		return;
	}
	if( val->vendor_model_srv_Hdr.opcode == OP_VENDOR_MESSAGE_TRANSPARENT_MSG )
	{
		// 收到透传数据
		APP_DBG("len %d, data 0x%02x",val->vendor_model_srv_Event.trans.len,
																	val->vendor_model_srv_Event.trans.pdata[0]);
	}
	else if( val->vendor_model_srv_Hdr.opcode == OP_VENDOR_MESSAGE_TRANSPARENT_WRT )
	{		
		// 收到write数据
		APP_DBG("len %d, data 0x%02x",val->vendor_model_srv_Event.write.len,
																	val->vendor_model_srv_Event.write.pdata[0]);
	}
	else if( val->vendor_model_srv_Hdr.opcode == OP_VENDOR_MESSAGE_TRANSPARENT_IND )
	{		
		// 发送的indicate已收到应答
	}
	else
	{
		APP_DBG("Unknow opcode 0x%02x",val->vendor_model_srv_Hdr.opcode);
	}
}

/*******************************************************************************
* Function Name  : keyPress
* Description    : 按键回调，按下发送一包数据给配网发起者
* Input          : None
* Return         : None
*******************************************************************************/
void keyPress( uint8 keys )
{
	APP_DBG("%d", keys);

	switch (keys)
	{
		default:
		{
			uint8 status;
			struct send_param param = {
				.app_idx = vnd_models[0].keys[0],				// 此消息使用的app key，如无特定则使用第0个key
				.addr = 0x01,														// 此消息发往的目的地地址，例程为发往配网发起者，默认地址为0x0001
				.trans_cnt = 0x01,											// 此消息的发送次数
				.period = K_MSEC(400),									// 此消息重传的间隔，建议不小于(200+50*TTL)ms，若数据较大则建议加长
				.rand = (0),														// 此消息发送的随机延迟
				.tid = vendor_srv_tid_get(),						// tid，每个独立消息递增循环，srv使用128~191
				.send_ttl = BLE_MESH_TTL_DEFAULT,				// ttl，无特定则使用默认值
			};
			uint8 data[8] = {0,1,2,3,4,5,6,7};
			status = vendor_message_srv_indicate(&param, data, 8);	// 调用自定义模型服务的有应答指示函数发送数据
			if( status )	APP_DBG("indicate failed %d",status);	
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
	friend_init_register(bt_mesh_friend_init, friend_state);
#endif /* FRIEND */
#if (CONFIG_BLE_MESH_LOW_POWER)
	lpn_init_register(bt_mesh_lpn_init, lpn_state);
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
		prov_enable();
	}

	APP_DBG("Mesh initialized");
}

/*******************************************************************************
* Function Name  : App_Init
* Description    : 
* Input          : None
* Return         : None
*******************************************************************************/
void App_Init()
{
  App_TaskID = TMOS_ProcessEventRegister( App_ProcessEvent );
	
	blemesh_on_sync();
	HAL_KeyInit();
	HalKeyConfig( keyPress);
	tmos_start_task(App_TaskID, APP_NODE_TEST_EVT, 1600);
}

/*******************************************************************************
* Function Name  : App_ProcessEvent
* Description    : 
* Input          : None
* Return         : None
*******************************************************************************/
static uint16 App_ProcessEvent( uint8 task_id, uint16 events )
{

  if ( events & APP_NODE_TEST_EVT )
  {
		tmos_start_task(App_TaskID, APP_NODE_TEST_EVT, 2400);
		return ( events ^ APP_NODE_TEST_EVT );
  }
	
  // Discard unknown events
  return 0;
}

/******************************** endfile @ main ******************************/

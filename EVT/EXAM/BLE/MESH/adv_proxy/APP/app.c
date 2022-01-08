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
#include "app_generic_onoff_model.h"
#include "app.h"
#include "HAL.h"


/*********************************************************************
 * GLOBAL TYPEDEFS
 */

/*********************************************************************
 * GLOBAL TYPEDEFS
 */

static u8_t MESH_MEM[1024 * 2]={0};

extern const ble_mesh_cfg_t app_mesh_cfg;
extern const struct device app_dev;

static uint8 App_TaskID = 0;    // Task ID for internal task/event processing

static uint16 App_ProcessEvent( uint8 task_id, uint16 events );

#if (!CONFIG_BLE_MESH_PB_GATT)
NET_BUF_SIMPLE_DEFINE_STATIC(rx_buf, 65);
#endif /* !PB_GATT */

/*********************************************************************
 * LOCAL FUNCION
 */

static void link_open(bt_mesh_prov_bearer_t bearer);
static void link_close(bt_mesh_prov_bearer_t bearer, u8_t reason);
static void prov_complete(u16_t net_idx, u16_t addr, u8_t flags, u32_t iv_index);
static void prov_reset(void);

static struct bt_mesh_cfg_srv cfg_srv = {
	.relay = BLE_MESH_RELAY_ENABLED,
	.beacon = BLE_MESH_BEACON_DISABLED,
#if (CONFIG_BLE_MESH_FRIEND)
	.frnd = BLE_MESH_FRIEND_ENABLED,
#endif
#if (CONFIG_BLE_MESH_PROXY)
	.gatt_proxy = BLE_MESH_GATT_PROXY_ENABLED,
#endif
	.default_ttl = 3,

	/* 8 transmissions with 10ms interval */
	.net_transmit = BLE_MESH_TRANSMIT(7, 10),
	.relay_retransmit = BLE_MESH_TRANSMIT(7, 10),
};

static struct bt_mesh_health_srv health_srv;

BLE_MESH_HEALTH_PUB_DEFINE(health_pub, 8);

u16_t cfg_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF]={ BLE_MESH_KEY_UNUSED };
u16_t cfg_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF]={	BLE_MESH_ADDR_UNASSIGNED };

u16_t health_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF]={ BLE_MESH_KEY_UNUSED };
u16_t health_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF]={	BLE_MESH_ADDR_UNASSIGNED };

u16_t gen_onoff_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF]={ BLE_MESH_KEY_UNUSED };
u16_t gen_onoff_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF]={	BLE_MESH_ADDR_UNASSIGNED };

static struct bt_mesh_model root_models[] = {
	BLE_MESH_MODEL_CFG_SRV(cfg_srv_keys, cfg_srv_groups, &cfg_srv),
	BLE_MESH_MODEL_HEALTH_SRV(health_srv_keys, health_srv_groups, &health_srv, &health_pub),
	BLE_MESH_MODEL(BLE_MESH_MODEL_ID_GEN_ONOFF_SRV, gen_onoff_op, NULL, gen_onoff_srv_keys, gen_onoff_srv_groups, NULL),
};

static struct bt_mesh_elem elements[] = {
	{
    /* Location Descriptor (GATT Bluetooth Namespace Descriptors) */
		.loc              = (0),
		.model_count      = ARRAY_SIZE(root_models),
		.models           = (root_models),
	}
};
static uint8_t dev_uuid[16];
u8 MACAddr[6];

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
	.reset			= prov_reset,
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
* Description    : 配网前的link建立回调，停止广播
* Input          : None
* Return         : None
*******************************************************************************/
static void link_open(bt_mesh_prov_bearer_t bearer)
{
	APP_DBG(" ");
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

	if (!bt_mesh_is_provisioned())
	{
		prov_enable();
	}
	else
	{
	}
}

/*******************************************************************************
* Function Name  : prov_complete
* Description    : 配网完成回调，重新开始广播
* Input          : None
* Return         : None
*******************************************************************************/
static void prov_complete(u16_t net_idx, u16_t addr, u8_t flags, u32_t iv_index)
{
}

/*******************************************************************************
* Function Name  : prov_reset
* Description    : 复位配网功能回调
* Input          : None
* Return         : None
*******************************************************************************/
static void prov_reset(void)
{
	APP_DBG("");
	
	prov_enable();
}

/*******************************************************************************
* Function Name  : keyPress
* Description    : 
* Input          : None
* Return         : None
*******************************************************************************/
void keyPress(uint8 keys)
{
	APP_DBG("keys %d ",keys);

	switch (keys)
	{
		default:
			break;
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
	u8_t i;
	
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

#if (defined (BLE_MAC)) && (BLE_MAC == TRUE)
  for ( i = 0; i < 6; i++ )
    MACAddr[i] = MacAddr[5 - i];
  tmos_memcpy(dev_uuid, MACAddr, 6);
#else
  GetMACAddress( MACAddr );
  tmos_memcpy(dev_uuid, MACAddr, 6);
#endif
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
	
	tmos_set_event(App_TaskID, APP_USER_EVT);

}

/*******************************************************************************
* Function Name  : App_ProcessEvent
* Description    : 
* Input          : None
* Return         : None
*******************************************************************************/
static uint16 App_ProcessEvent( uint8 task_id, uint16 events )
{

  if ( events & APP_USER_EVT )
  {
    return ( events ^ APP_USER_EVT );
  }
  
  // Discard unknown events
  return 0;
}

/******************************** endfile @ main ******************************/

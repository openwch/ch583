/********************************** (C) COPYRIGHT *******************************
 * File Name          : app.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2022/01/18
 * Description        :
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/******************************************************************************/
#include "CONFIG.h"
#include "MESH_LIB.h"
#include "app_vendor_model_srv.h"
#include "app.h"
#include "peripheral.h"
#include "HAL.h"
#include "app_trans_process.h"

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
#define ADV_TIMEOUT       K_MINUTES(10)

#define SELENCE_ADV_ON    0x01
#define SELENCE_ADV_OF    0x00

// 命令超时设置，如果有低功耗节点，则不能小于低功耗节点唤醒间隔（默认10s）
#define APP_CMD_TIMEOUT               1600*10

#define APP_DELETE_LOCAL_NODE_DELAY   3200
// shall not less than APP_DELETE_LOCAL_NODE_DELAY
#define APP_DELETE_NODE_INFO_DELAY    3200
/*********************************************************************
 * GLOBAL TYPEDEFS
 */

static uint8_t MESH_MEM[1024 * 3] = {0};

extern const ble_mesh_cfg_t app_mesh_cfg;
extern const struct device  app_dev;

static uint8_t App_TaskID = 0; // Task ID for internal task/event processing

static uint16_t App_ProcessEvent(uint8_t task_id, uint16_t events);

static uint8_t dev_uuid[16] = {0}; // 此设备的UUID
uint8_t        MACAddr[6];         // 此设备的mac

static uint8_t self_prov_net_key[16] = {0};

static const uint8_t self_prov_dev_key[16] = {
    0x00, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x00, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

static uint8_t self_prov_app_key[16] = {
    0x00, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x00, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

const uint16_t self_prov_net_idx = 0x0000;      // 自配网所用的net key
const uint16_t self_prov_app_idx = 0x0001;      // 自配网所用的app key
uint32_t self_prov_iv_index = 0x00000000; // 自配网的iv_index，默认为0
uint16_t self_prov_addr = 0;         // 自配网的自身主元素地址
uint8_t  self_prov_flags = 0x00;          // 是否处于key更新状态，默认为否

uint16_t delete_node_address=0;
uint16_t ask_status_node_address=0;
uint16_t ota_update_node_address=0;
uint16_t set_sub_node_address=0;

#if(!CONFIG_BLE_MESH_PB_GATT)
NET_BUF_SIMPLE_DEFINE_STATIC(rx_buf, 65);
#endif /* !PB_GATT */

/*********************************************************************
 * LOCAL FUNCION
 */

static void cfg_srv_rsp_handler( const cfg_srv_status_t *val );
static void link_open(bt_mesh_prov_bearer_t bearer);
static void link_close(bt_mesh_prov_bearer_t bearer, uint8_t reason);
static void prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index);
static void cfg_cli_rsp_handler(const cfg_cli_status_t *val);
static void vendor_model_srv_rsp_handler(const vendor_model_srv_status_t *val);
static int  vendor_model_srv_send(uint16_t addr, uint8_t *pData, uint16_t len);
static void prov_reset(void);
void App_trans_model_reveived(uint8_t *pValue, uint16_t len, uint16_t addr );


static struct bt_mesh_cfg_srv cfg_srv = {
    .relay = BLE_MESH_RELAY_ENABLED,
    .beacon = BLE_MESH_BEACON_ENABLED,
#if(CONFIG_BLE_MESH_FRIEND)
    .frnd = BLE_MESH_FRIEND_ENABLED,
#endif
#if(CONFIG_BLE_MESH_PROXY)
    .gatt_proxy = BLE_MESH_GATT_PROXY_ENABLED,
#endif
    /* 默认TTL为3 */
    .default_ttl = 3,
    /* 底层发送数据重试7次，每次间隔10ms（不含内部随机数） */
    .net_transmit = BLE_MESH_TRANSMIT(7, 10),
    /* 底层转发数据重试7次，每次间隔10ms（不含内部随机数） */
    .relay_retransmit = BLE_MESH_TRANSMIT(7, 10),
    .handler = cfg_srv_rsp_handler,
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

const struct bt_mesh_health_srv_cb health_srv_cb = {
    .attn_on = app_prov_attn_on,
    .attn_off = app_prov_attn_off,
};

static struct bt_mesh_health_srv health_srv = {
    .cb = &health_srv_cb,
};

BLE_MESH_HEALTH_PUB_DEFINE(health_pub, 8);

struct bt_mesh_cfg_cli cfg_cli = {
    .handler = cfg_cli_rsp_handler,
};

uint16_t cfg_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = {BLE_MESH_KEY_UNUSED};
uint16_t cfg_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = {BLE_MESH_ADDR_UNASSIGNED};

uint16_t cfg_cli_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = {BLE_MESH_KEY_UNUSED};
uint16_t cfg_cli_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = {BLE_MESH_ADDR_UNASSIGNED};

uint16_t health_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = {BLE_MESH_KEY_UNUSED};
uint16_t health_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = {BLE_MESH_ADDR_UNASSIGNED};

// root模型加载
static struct bt_mesh_model root_models[] = {
    BLE_MESH_MODEL_CFG_SRV(cfg_srv_keys, cfg_srv_groups, &cfg_srv),
    BLE_MESH_MODEL_CFG_CLI(cfg_cli_keys, cfg_cli_groups, &cfg_cli),
    BLE_MESH_MODEL_HEALTH_SRV(health_srv_keys, health_srv_groups, &health_srv, &health_pub),
};

struct bt_mesh_vendor_model_srv vendor_model_srv = {
    .srv_tid.trans_tid = 0xFF,
    .handler = vendor_model_srv_rsp_handler,
};

uint16_t vnd_model_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = {BLE_MESH_KEY_UNUSED};
uint16_t vnd_model_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = {BLE_MESH_ADDR_UNASSIGNED};

// 自定义模型加载
struct bt_mesh_model vnd_models[] = {
    BLE_MESH_MODEL_VND_CB(CID_WCH, BLE_MESH_MODEL_ID_WCH_SRV, vnd_model_srv_op, NULL, vnd_model_srv_keys,
                          vnd_model_srv_groups, &vendor_model_srv, NULL),
};

// 模型组成 elements
static struct bt_mesh_elem elements[] = {
    {
        /* Location Descriptor (GATT Bluetooth Namespace Descriptors) */
        .loc = (0),
        .model_count = ARRAY_SIZE(root_models),
        .models = (root_models),
        .vnd_model_count = ARRAY_SIZE(vnd_models),
        .vnd_models = (vnd_models),
    }
};

// elements 构成 Node Composition
const struct bt_mesh_comp app_comp = {
    .cid = 0x07D7, // WCH 公司id
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

// 配网参数和回调
static const struct bt_mesh_prov app_prov = {
    .uuid = dev_uuid,
    .link_open = link_open,
    .link_close = link_close,
    .complete = prov_complete,
    .reset = prov_reset,
};

// 配网者管理的节点，第0个为自己，第1，2依次为配网顺序的节点
node_t app_nodes[1] = {0};

app_mesh_manage_t app_mesh_manage;

/* flash的数据临时存储 */
__attribute__((aligned(8))) uint8_t block_buf[16];

/*********************************************************************
 * GLOBAL TYPEDEFS
 */

/*********************************************************************
 * @fn      prov_enable
 *
 * @brief   使能配网功能
 *
 * @return  none
 */
static void prov_enable(void)
{
    if(bt_mesh_is_provisioned())
    {
        return;
    }

    // Make sure we're scanning for provisioning inviations
    bt_mesh_scan_enable();
    // Enable unprovisioned beacon sending
    bt_mesh_beacon_enable();

    if(CONFIG_BLE_MESH_PB_GATT)
    {
        bt_mesh_proxy_prov_enable();
    }
}

/*********************************************************************
 * @fn      link_open
 *
 * @brief   配网时后的link打开回调
 *
 * @param   bearer  - 当前link是PB_ADV还是PB_GATT
 *
 * @return  none
 */
static void link_open(bt_mesh_prov_bearer_t bearer)
{
    APP_DBG("");
}

/*********************************************************************
 * @fn      link_close
 *
 * @brief   配网后的link关闭回调
 *
 * @param   bearer  - 当前link是PB_ADV还是PB_GATT
 * @param   reason  - link关闭原因
 *
 * @return  none
 */
static void link_close(bt_mesh_prov_bearer_t bearer, uint8_t reason)
{
    if(reason != CLOSE_REASON_SUCCESS)
        APP_DBG("reason %x", reason);
}

/*********************************************************************
 * @fn      node_cfg_process
 *
 * @brief   找一个空闲的节点，执行配置流程
 *
 * @param   node        - 空节点指针
 * @param   net_idx     - 网络key编号
 * @param   addr        - 网络地址
 * @param   num_elem    - 元素数量
 *
 * @return  node_t / NULL
 */
static node_t *node_cfg_process(node_t *node, uint16_t net_idx, uint16_t addr, uint8_t num_elem)
{
    node = app_nodes;
    node->net_idx = net_idx;
    node->node_addr = addr;
    node->elem_count = num_elem;
    return node;
}

/*********************************************************************
 * @fn      prov_complete
 *
 * @brief   配网完成回调
 *
 * @param   net_idx     - 网络key的index
 * @param   addr        - link关闭原因网络地址
 * @param   flags       - 是否处于key refresh状态
 * @param   iv_index    - 当前网络iv的index
 *
 * @return  none
 */
static void prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index)
{
    int     err;
    node_t *node;

    APP_DBG("");

    node = node_cfg_process(node, net_idx, addr, ARRAY_SIZE(elements));
    if(!node)
    {
        APP_DBG("Unable allocate node object");
        return;
    }
    set_led_state(LED_PIN, TRUE);
    Peripheral_AdvertData_Privisioned(TRUE);

    // 如果未配置过网络信息，则退出回调后，执行配置本地网络信息流程
    if( vnd_models[0].keys[0] == BLE_MESH_KEY_UNUSED )
    {
        tmos_start_task(App_TaskID, APP_NODE_EVT, 160);
    }
}

/*********************************************************************
 * @fn      prov_reset
 *
 * @brief   复位配网功能回调
 *
 * @param   none
 *
 * @return  none
 */
static void prov_reset(void)
{
    if( app_mesh_manage.local_reset.cmd == CMD_LOCAL_RESET )
    {
        app_mesh_manage.local_reset_ack.cmd = CMD_LOCAL_RESET_ACK;
        app_mesh_manage.local_reset_ack.status = STATUS_SUCCESS;
        // 通知主机(如果已连接)
        peripheralChar4Notify(app_mesh_manage.data.buf, LOCAL_RESET_ACK_DATA_LEN);
    }
    Peripheral_TerminateLink();
    self_prov_iv_index = 0x00000000;
    self_prov_addr = 0;
    self_prov_flags = 0x00;
    delete_node_address=0;
    ask_status_node_address=0;
    ota_update_node_address=0;
    set_sub_node_address=0;
    set_led_state(LED_PIN, FALSE);
    Peripheral_AdvertData_Privisioned(FALSE);
    APP_DBG("Waiting for privisioning data");
#if(CONFIG_BLE_MESH_LOW_POWER)
    bt_mesh_lpn_set(FALSE);
    APP_DBG("Low power disable");
#endif /* LPN */
}

/*********************************************************************
 * @fn      cfg_local_net_info
 *
 * @brief   配置本地网络信息，由于是自配网，所以直接设置自身的密钥和订阅地址
 *
 * @param   none
 *
 * @return  none
 */
static void cfg_local_net_info(void)
{
    uint8_t status;

    // 本地直接添加应用密钥
    status = bt_mesh_app_key_set(app_nodes->net_idx, self_prov_app_idx, self_prov_app_key, FALSE);
    if( status )
    {
        APP_DBG("Unable set app key");
    }
    APP_DBG("lcoal app key added");
    // 绑定应用密钥到沁恒自定义模型
    vnd_models[0].keys[0] = (uint16_t)self_prov_app_idx;
    bt_mesh_store_mod_bind(&vnd_models[0]);
    APP_DBG("lcoal app key binded");
#if(CONFIG_BLE_MESH_LOW_POWER)
    bt_mesh_lpn_set(TRUE);
    APP_DBG("Low power enable");
#endif /* LPN */

}

/*********************************************************************
 * @fn      App_model_find_group
 *
 * @brief   在模型的订阅地址中查询对应的地址
 *
 * @param   mod     - 指向对应模型
 * @param   addr    - 订阅地址
 *
 * @return  查到的订阅地址指针
 */
static uint16_t *App_model_find_group(struct bt_mesh_model *mod, u16_t addr)
{
    int i;

    for (i = 0; i < CONFIG_MESH_MOD_GROUP_COUNT_DEF; i++)
    {
        if (mod->groups[i] == addr)
        {
            return &mod->groups[i];
        }
    }

    return NULL;
}
/*********************************************************************
 * @fn      cfg_srv_rsp_handler
 *
 * @brief   config 模型服务回调
 *
 * @param   val     - 回调参数，包括命令类型、配置命令执行状态
 *
 * @return  none
 */
static void cfg_srv_rsp_handler( const cfg_srv_status_t *val )
{
    if(val->cfgHdr.status)
    {
        // 配置命令执行不成功
        APP_DBG("warning opcode 0x%02x", val->cfgHdr.opcode);
        return;
    }
    if(val->cfgHdr.opcode == OP_APP_KEY_ADD)
    {
        APP_DBG("App Key Added");
    }
    else if(val->cfgHdr.opcode == OP_MOD_APP_BIND)
    {
        APP_DBG("Vendor Model Binded");
    }
    else if(val->cfgHdr.opcode == OP_MOD_SUB_ADD)
    {
        APP_DBG("Vendor Model Subscription Set");
    }
    else
    {
        APP_DBG("Unknow opcode 0x%02x", val->cfgHdr.opcode);
    }
}

/*********************************************************************
 * @fn      cfg_cli_rsp_handler
 *
 * @brief   收到cfg命令的应答回调
 *
 * @param   val     - 回调参数，包含命令类型和返回数据
 *
 * @return  none
 */
static void cfg_cli_rsp_handler(const cfg_cli_status_t *val)
{
    APP_DBG("opcode 0x%04x",val->cfgHdr.opcode);

    if(val->cfgHdr.status == 0xFF)
    {
        APP_DBG("Opcode 0x%04x, timeout", val->cfgHdr.opcode);
    }
}

/*********************************************************************
 * @fn      vendor_model_srv_rsp_handler
 *
 * @brief   自定义模型服务回调
 *
 * @param   val     - 回调参数，包括消息类型、数据内容、长度、来源地址
 *
 * @return  none
 */
static void vendor_model_srv_rsp_handler(const vendor_model_srv_status_t *val)
{
    if(val->vendor_model_srv_Hdr.status)
    {
        // 有应答数据传输 超时未收到应答
        APP_DBG("Timeout opcode 0x%02x", val->vendor_model_srv_Hdr.opcode);
        return;
    }
    if(val->vendor_model_srv_Hdr.opcode == OP_VENDOR_MESSAGE_TRANSPARENT_MSG)
    {
        // 收到透传数据
        APP_DBG("len %d, data 0x%02x from 0x%04x", val->vendor_model_srv_Event.trans.len,
                val->vendor_model_srv_Event.trans.pdata[0],
                val->vendor_model_srv_Event.trans.addr);
        App_trans_model_reveived(val->vendor_model_srv_Event.trans.pdata, val->vendor_model_srv_Event.trans.len,
            val->vendor_model_srv_Event.trans.addr );
//        // 转发给主机(如果已连接)
//        peripheralChar4Notify(val->vendor_model_srv_Event.trans.pdata, val->vendor_model_srv_Event.trans.len);
    }
    else if(val->vendor_model_srv_Hdr.opcode == OP_VENDOR_MESSAGE_TRANSPARENT_WRT)
    {
        // 收到write数据
        APP_DBG("len %d, data 0x%02x from 0x%04x", val->vendor_model_srv_Event.write.len,
                val->vendor_model_srv_Event.write.pdata[0],
                val->vendor_model_srv_Event.write.addr);
//        // 转发给主机(如果已连接)
//        peripheralChar4Notify(val->vendor_model_srv_Event.write.pdata, val->vendor_model_srv_Event.write.len);
    }
    else if(val->vendor_model_srv_Hdr.opcode == OP_VENDOR_MESSAGE_TRANSPARENT_IND)
    {
        // 发送的indicate已收到应答
    }
    else
    {
        APP_DBG("Unknow opcode 0x%02x", val->vendor_model_srv_Hdr.opcode);
    }
}

/*********************************************************************
 * @fn      vendor_model_srv_send
 *
 * @brief   通过厂商自定义模型发送数据
 *
 * @param   addr    - 需要发送的目的地址
 *          pData   - 需要发送的数据指针
 *          len     - 需要发送的数据长度
 *
 * @return  参考Global_Error_Code
 */
static int vendor_model_srv_send(uint16_t addr, uint8_t *pData, uint16_t len)
{
    struct send_param param = {
        .app_idx = vnd_models[0].keys[0], // 此消息使用的app key，如无特定则使用第0个key
        .addr = addr,          // 此消息发往的目的地地址，例程为发往订阅地址，包括自己
        .trans_cnt = 0x05,                // 此消息的用户层发送次数
        .period = K_MSEC(500),            // 此消息重传的间隔，建议不小于(200+50*TTL)ms，若数据较大则建议加长
        .rand = (0),                      // 此消息发送的随机延迟
        .tid = vendor_srv_tid_get(),      // tid，每个独立消息递增循环，srv使用128~191
        .send_ttl = BLE_MESH_TTL_DEFAULT, // ttl，无特定则使用默认值
    };
//    return vendor_message_srv_indicate(&param, pData, len);  // 调用自定义模型服务的有应答指示函数发送数据，默认超时2s
    vendor_message_srv_trans_reset();
    return vendor_message_srv_send_trans(&param, pData, len); // 或者调用自定义模型服务的透传函数发送数据，只发送，无应答机制
}

/*********************************************************************
 * @fn      keyPress
 *
 * @brief   按键回调
 *
 * @param   keys    - 按键类型
 *
 * @return  none
 */
void keyPress(uint8_t keys)
{
    APP_DBG("%d", keys);

    switch(keys)
    {
        default:
        {
            uint8_t status;
            uint8_t data[8] = {0, 1, 2, 3, 4, 5, 6, 7};
            status = vendor_model_srv_send(BLE_MESH_ADDR_ALL_NODES, data, 8);
            if(status)
            {
                APP_DBG("send failed %d", status);
            }
            break;
        }
    }
}

#if(CONFIG_BLE_MESH_FRIEND)
/*********************************************************************
 * @fn      friend_state
 *
 * @brief   朋友关系建立回调
 *
 * @param   lpn_addr    - 低功耗节点的网络地址
 * @param   state       - 回调状态
 *
 * @return  none
 */
static void friend_state(uint16_t lpn_addr, uint8_t state)
{
    if(state == FRIEND_FRIENDSHIP_ESTABLISHED)
    {
        APP_DBG("friend friendship established, lpn addr 0x%04x", lpn_addr);
    }
    else if(state == FRIEND_FRIENDSHIP_TERMINATED)
    {
        APP_DBG("friend friendship terminated, lpn addr 0x%04x", lpn_addr);
    }
    else
    {
        APP_DBG("unknow state %x", state);
    }
}
#endif

#if(CONFIG_BLE_MESH_LOW_POWER)
/*********************************************************************
 * @fn      lpn_state
 *
 * @brief   朋友关系建立回调
 *
 * @param   state       - 回调状态
 *
 * @return  none
 */
static void lpn_state(uint8_t state)
{
    if(state == LPN_FRIENDSHIP_ESTABLISHED)
    {
        APP_DBG("lpn friendship established");
    }
    else if(state == LPN_FRIENDSHIP_TERMINATED)
    {
        APP_DBG("lpn friendship terminated");
    }
    else
    {
        APP_DBG("unknow state %x", state);
    }
}
#endif

/*********************************************************************
 * @fn      blemesh_on_sync
 *
 * @brief   同步mesh参数，启用对应功能，不建议修改
 *
 * @return  none
 */
void blemesh_on_sync(void)
{
    int        err;
    mem_info_t info;

    if(tmos_memcmp(VER_MESH_LIB, VER_MESH_FILE, strlen(VER_MESH_FILE)) == FALSE)
    {
        PRINT("head file error...\n");
        while(1);
    }

    info.base_addr = MESH_MEM;
    info.mem_len = ARRAY_SIZE(MESH_MEM);

#if(CONFIG_BLE_MESH_FRIEND)
    friend_init_register(bt_mesh_friend_init, friend_state);
#endif /* FRIEND */
#if(CONFIG_BLE_MESH_LOW_POWER)
    lpn_init_register(bt_mesh_lpn_init, lpn_state);
#endif /* LPN */

    GetMACAddress(MACAddr);
    tmos_memcpy(dev_uuid, MACAddr, 6);
    err = bt_mesh_cfg_set(&app_mesh_cfg, &app_dev, MACAddr, &info);
    if(err)
    {
        APP_DBG("Unable set configuration (err:%d)", err);
        return;
    }
    hal_rf_init();
    err = bt_mesh_comp_register(&app_comp);

#if(CONFIG_BLE_MESH_RELAY)
    bt_mesh_relay_init();
#endif /* RELAY  */
#if(CONFIG_BLE_MESH_PROXY || CONFIG_BLE_MESH_PB_GATT)
  #if(CONFIG_BLE_MESH_PROXY)
    bt_mesh_proxy_beacon_init_register((void *)bt_mesh_proxy_beacon_init);
    gatts_notify_register(bt_mesh_gatts_notify);
    proxy_gatt_enable_register(bt_mesh_proxy_gatt_enable);
  #endif /* PROXY  */
  #if(CONFIG_BLE_MESH_PB_GATT)
    proxy_prov_enable_register(bt_mesh_proxy_prov_enable);
  #endif /* PB_GATT  */

    bt_mesh_proxy_init();
#endif /* PROXY || PB-GATT */

#if(CONFIG_BLE_MESH_PROXY_CLI)
    bt_mesh_proxy_client_init(cli); //待添加
#endif                              /* PROXY_CLI */

    bt_mesh_prov_retransmit_init();
#if(!CONFIG_BLE_MESH_PB_GATT)
    adv_link_rx_buf_register(&rx_buf);
#endif /* !PB_GATT */
    err = bt_mesh_prov_init(&app_prov);

    bt_mesh_mod_init();
    bt_mesh_net_init();
    bt_mesh_trans_init();
    bt_mesh_beacon_init();

    bt_mesh_adv_init();

#if((CONFIG_BLE_MESH_PB_GATT) || (CONFIG_BLE_MESH_PROXY) || (CONFIG_BLE_MESH_OTA))
    bt_mesh_conn_adv_init();
#endif /* PROXY || PB-GATT || OTA */

#if(CONFIG_BLE_MESH_SETTINGS)
    bt_mesh_settings_init();
#endif /* SETTINGS */

#if(CONFIG_BLE_MESH_PROXY_CLI)
    bt_mesh_proxy_cli_adapt_init();
#endif /* PROXY_CLI */

#if((CONFIG_BLE_MESH_PROXY) || (CONFIG_BLE_MESH_PB_GATT) || \
    (CONFIG_BLE_MESH_PROXY_CLI) || (CONFIG_BLE_MESH_OTA))
    bt_mesh_adapt_init();
#endif /* PROXY || PB-GATT || PROXY_CLI || OTA */

    if(err)
    {
        APP_DBG("Initializing mesh failed (err %d)", err);
        return;
    }

    APP_DBG("Bluetooth initialized");

#if(CONFIG_BLE_MESH_SETTINGS)
    settings_load();
#endif /* SETTINGS */

    if(bt_mesh_is_provisioned())
    {
        set_led_state(LED_PIN, TRUE);
        APP_DBG("Mesh network restored from flash");
#if(CONFIG_BLE_MESH_LOW_POWER)
        bt_mesh_lpn_set(TRUE);
        APP_DBG("Low power enable");
#endif /* LPN */
    }
    else
    {
        set_led_state(LED_PIN, FALSE);
       APP_DBG("Waiting for privisioning data");
        Peripheral_AdvertData_Privisioned(FALSE);
    }

    APP_DBG("Mesh initialized");
}

/*********************************************************************
 * @fn      App_Init
 *
 * @brief   应用层初始化
 *
 * @return  none
 */
void App_Init()
{
    GAPRole_PeripheralInit();
    Peripheral_Init();

    App_TaskID = TMOS_ProcessEventRegister(App_ProcessEvent);

    vendor_model_srv_init(vnd_models);
    blemesh_on_sync();
    HAL_KeyInit();
    HalKeyConfig(keyPress);
}

/*********************************************************************
 * @fn      App_ProcessEvent
 *
 * @brief   应用层事件处理函数
 *
 * @param   task_id  - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
static uint16_t App_ProcessEvent(uint8_t task_id, uint16_t events)
{
    // 节点配置任务事件处理
    if(events & APP_NODE_EVT)
    {
        cfg_local_net_info();
        app_mesh_manage.provision_ack.cmd = CMD_PROVISION_ACK;
        app_mesh_manage.provision_ack.addr[0] = app_nodes[0].node_addr&0xFF;
        app_mesh_manage.provision_ack.addr[1] = (app_nodes[0].node_addr>>8)&0xFF;
        app_mesh_manage.provision_ack.status = STATUS_SUCCESS;
        // 通知主机(如果已连接)
        peripheralChar4Notify(app_mesh_manage.data.buf, PROVISION_ACK_DATA_LEN);
        return (events ^ APP_NODE_EVT);
    }

    if(events & APP_NODE_PROVISION_EVT)
    {
        if( self_prov_addr )
        {
            int err;
            err = bt_mesh_provision(self_prov_net_key, self_prov_net_idx, self_prov_flags,
                                    self_prov_iv_index, self_prov_addr, self_prov_dev_key);
            if(err)
            {
                APP_DBG("Self Privisioning (err %d)", err);
                self_prov_addr = 0;
                app_mesh_manage.provision_ack.cmd = CMD_PROVISION_ACK;
                app_mesh_manage.provision_ack.addr[0] = app_nodes[0].node_addr&0xFF;
                app_mesh_manage.provision_ack.addr[1] = (app_nodes[0].node_addr>>8)&0xFF;
                app_mesh_manage.provision_ack.status = STATUS_INVALID;
                // 通知主机(如果已连接)
                peripheralChar4Notify(app_mesh_manage.data.buf, PROVISION_ACK_DATA_LEN);
            }
        }
        return (events ^ APP_NODE_PROVISION_EVT);
    }

    if(events & APP_DELETE_NODE_TIMEOUT_EVT)
    {
        // 通过应用层自定协议删除超时，可添加其他流程
        APP_DBG("Delete node failed ");
        app_mesh_manage.delete_node_ack.cmd = CMD_DELETE_NODE_ACK;
        app_mesh_manage.delete_node_ack.addr[0] = delete_node_address&0xFF;
        app_mesh_manage.delete_node_ack.addr[1] = (delete_node_address>>8)&0xFF;
        app_mesh_manage.delete_node_ack.status = STATUS_TIMEOUT;
        vendor_message_srv_trans_reset();
        // 通知主机(如果已连接)
        peripheralChar4Notify(app_mesh_manage.data.buf, DELETE_NODE_ACK_DATA_LEN);
        return (events ^ APP_DELETE_NODE_TIMEOUT_EVT);
    }

    if(events & APP_DELETE_LOCAL_NODE_EVT)
    {
        // 收到删除命令，删除自身网络信息
        APP_DBG("Delete local node");
        self_prov_addr = 0;
        // 复位自身网络状态
        bt_mesh_reset();
        return (events ^ APP_DELETE_LOCAL_NODE_EVT);
    }

    if(events & APP_DELETE_NODE_INFO_EVT)
    {
        // 删除已存储的被删除节点的信息
        bt_mesh_delete_node_info(delete_node_address,app_comp.elem_count);
        APP_DBG("Delete stored node info complete");
        app_mesh_manage.delete_node_info_ack.cmd = CMD_DELETE_NODE_INFO_ACK;
        app_mesh_manage.delete_node_info_ack.addr[0] = delete_node_address&0xFF;
        app_mesh_manage.delete_node_info_ack.addr[1] = (delete_node_address>>8)&0xFF;
        // 通知主机(如果已连接)
        peripheralChar4Notify(app_mesh_manage.data.buf, DELETE_NODE_INFO_ACK_DATA_LEN);
        return (events ^ APP_DELETE_NODE_INFO_EVT);
    }

    if(events & APP_ASK_STATUS_NODE_TIMEOUT_EVT)
    {
        // 查询节点状态超时
        APP_DBG("Ask status node failed ");
        app_mesh_manage.ask_status_ack.cmd = CMD_ASK_STATUS_ACK;
        app_mesh_manage.ask_status_ack.addr[0] = ask_status_node_address&0xFF;
        app_mesh_manage.ask_status_ack.addr[1] = (ask_status_node_address>>8)&0xFF;
        app_mesh_manage.ask_status_ack.status = STATUS_TIMEOUT;
        ask_status_node_address = 0;
        vendor_message_srv_trans_reset();
        // 通知主机(如果已连接)
        peripheralChar4Notify(app_mesh_manage.data.buf, ASK_STATUS_ACK_DATA_LEN);
        return (events ^ APP_ASK_STATUS_NODE_TIMEOUT_EVT);
    }

    if(events & APP_OTA_UPDATE_TIMEOUT_EVT)
    {
        // OTA 升级超时
        APP_DBG("OTA update node failed ");
        switch(app_mesh_manage.data.buf[0])
        {
            case CMD_IMAGE_INFO:
            {
                app_mesh_manage.image_info_ack.cmd = CMD_DELETE_NODE_ACK;
                app_mesh_manage.image_info_ack.addr[0] = ota_update_node_address&0xFF;
                app_mesh_manage.image_info_ack.addr[1] = (ota_update_node_address>>8)&0xFF;
                app_mesh_manage.image_info_ack.status = STATUS_TIMEOUT;
                peripheralChar4Notify(app_mesh_manage.data.buf, IMAGE_INFO_ACK_DATA_LEN);
                break;
            }
            case CMD_UPDATE:
            {
                app_mesh_manage.update_ack.cmd = CMD_UPDATE_ACK;
                app_mesh_manage.update_ack.addr[0] = ota_update_node_address&0xFF;
                app_mesh_manage.update_ack.addr[1] = (ota_update_node_address>>8)&0xFF;
                app_mesh_manage.update_ack.status = STATUS_TIMEOUT;
                peripheralChar4Notify(app_mesh_manage.data.buf, UPDATE_ACK_DATA_LEN);
                break;
            }
            case CMD_VERIFY:
            {
                app_mesh_manage.verify_ack.cmd = CMD_VERIFY_ACK;
                app_mesh_manage.verify_ack.addr[0] = ota_update_node_address&0xFF;
                app_mesh_manage.verify_ack.addr[1] = (ota_update_node_address>>8)&0xFF;
                app_mesh_manage.verify_ack.status = STATUS_TIMEOUT;
                peripheralChar4Notify(app_mesh_manage.data.buf, VERIFY_ACK_DATA_LEN);
                break;
            }
        }
        vendor_message_srv_trans_reset();
        ota_update_node_address = 0;
        // 通知主机(如果已连接)
        return (events ^ APP_OTA_UPDATE_TIMEOUT_EVT);
    }

    if(events & APP_SET_SUB_TIMEOUT_EVT)
    {
        // 设置节点订阅地址超时
        APP_DBG("Set sub node failed ");
        app_mesh_manage.set_sub_ack.cmd = CMD_SET_SUB_ACK;
        app_mesh_manage.set_sub_ack.addr[0] = set_sub_node_address&0xFF;
        app_mesh_manage.set_sub_ack.addr[1] = (set_sub_node_address>>8)&0xFF;
        app_mesh_manage.set_sub_ack.status = STATUS_TIMEOUT;
        set_sub_node_address = 0;
        vendor_message_srv_trans_reset();
        // 通知主机(如果已连接)
        peripheralChar4Notify(app_mesh_manage.data.buf, SET_SUB_ACK_DATA_LEN);
        return (events ^ APP_SET_SUB_TIMEOUT_EVT);
    }

    // Discard unknown events
    return 0;
}

/*********************************************************************
 * @fn      SwitchImageFlag
 *
 * @brief   切换dataflash里的ImageFlag
 *
 * @param   new_flag    - 切换的ImageFlag
 *
 * @return  none
 */
void SwitchImageFlag(uint8_t new_flag)
{
    uint16_t i;
    uint32_t ver_flag;

    /* 读取第一块 */
    EEPROM_READ(OTA_DATAFLASH_ADD, (uint32_t *)&block_buf[0], 4);

    /* 擦除第一块 */
    EEPROM_ERASE(OTA_DATAFLASH_ADD, EEPROM_PAGE_SIZE);

    /* 更新Image信息 */
    block_buf[0] = new_flag;

    /* 编程DataFlash */
    EEPROM_WRITE(OTA_DATAFLASH_ADD, (uint32_t *)&block_buf[0], 4);
}

/*********************************************************************
 * @fn      App_trans_model_reveived
 *
 * @brief   透传模型收到数据
 *
 * @param   pValue - pointer to data that was changed
 *          len - length of data
 *          addr - 数据来源地址
 *
 * @return  none
 */
void App_trans_model_reveived(uint8_t *pValue, uint16_t len, uint16_t addr )
{
    tmos_memcpy(&app_mesh_manage, pValue, len);
    switch(app_mesh_manage.data.buf[0])
    {
        // 判断是否为删除命令
        case CMD_DELETE_NODE:
        {
            if(len != DELETE_NODE_DATA_LEN)
            {
                APP_DBG("Delete node data err!");
                return;
            }
            int status;
            APP_DBG("receive delete cmd, send ack and start delete node delay");
            app_mesh_manage.delete_node_ack.cmd = CMD_DELETE_NODE_ACK;
            app_mesh_manage.delete_node_ack.status = STATUS_SUCCESS;
            status = vendor_model_srv_send(addr, app_mesh_manage.data.buf, DELETE_NODE_ACK_DATA_LEN);
            if(status)
            {
                APP_DBG("send ack failed %d", status);
            }
            // 即将删除自身，先发送CMD_DELETE_NODE_INFO命令
            APP_DBG("send to all node to let them delete stored info ");
            app_mesh_manage.delete_node_info.cmd = CMD_DELETE_NODE_INFO;
            status = vendor_model_srv_send(BLE_MESH_ADDR_ALL_NODES,
                                            app_mesh_manage.data.buf, DELETE_NODE_INFO_DATA_LEN);
            if(status)
            {
                APP_DBG("send ack failed %d", status);
            }
            tmos_start_task(App_TaskID, APP_DELETE_LOCAL_NODE_EVT, APP_DELETE_LOCAL_NODE_DELAY);
            break;
        }

        // 判断是否为应用层自定义删除命令应答
        case CMD_DELETE_NODE_ACK:
        {
            if(len != DELETE_NODE_ACK_DATA_LEN)
            {
                APP_DBG("Delete node ack data err!");
                return;
            }
            tmos_stop_task(App_TaskID, APP_DELETE_NODE_TIMEOUT_EVT);
            APP_DBG("Delete node complete");
            vendor_message_srv_trans_reset();
            // 通知主机(如果已连接)
            peripheralChar4Notify(app_mesh_manage.data.buf, PROVISION_ACK_DATA_LEN);
            break;
        }

        // 判断是否为有节点被删除，需要删除存储的节点信息
        case CMD_DELETE_NODE_INFO:
        {
            if(len != DELETE_NODE_INFO_DATA_LEN)
            {
                APP_DBG("Delete node info data err!");
                return;
            }
            delete_node_address = addr;
            tmos_start_task(App_TaskID, APP_DELETE_NODE_INFO_EVT, APP_DELETE_NODE_INFO_DELAY);
            break;
        }

        // 判断是否为查询节点信息命令
        case CMD_ASK_STATUS:
        {
            if(len != ASK_STATUS_DATA_LEN)
            {
                APP_DBG("ask status data err!");
                return;
            }
            int status;
            app_mesh_manage.ask_status_ack.cmd = CMD_ASK_STATUS_ACK;
            app_mesh_manage.ask_status_ack.status = STATUS_SUCCESS; //用户自定义状态码
            status = vendor_model_srv_send(addr, app_mesh_manage.data.buf, ASK_STATUS_ACK_DATA_LEN);
            if(status)
            {
                APP_DBG("send ack failed %d", status);
            }
            break;
        }

        // 判断是否为查询节点信息命令应答
        case CMD_ASK_STATUS_ACK:
        {
            if(len != ASK_STATUS_ACK_DATA_LEN)
            {
                APP_DBG("ask status data err!");
                return;
            }
            tmos_stop_task(App_TaskID, APP_ASK_STATUS_NODE_TIMEOUT_EVT);
            APP_DBG("ask status complete");
            vendor_message_srv_trans_reset();
            // 通知主机(如果已连接)
            peripheralChar4Notify(app_mesh_manage.data.buf, ASK_STATUS_ACK_DATA_LEN);
            break;
        }

        case CMD_TRANSFER:
        {
            if(len < TRANSFER_DATA_LEN)
            {
                APP_DBG("transfer data err!");
                return;
            }
            int status;
            uint16 dst_addr;
            dst_addr = app_mesh_manage.transfer.addr[0]|(app_mesh_manage.transfer.addr[1]<<8);
            APP_DBG("Receive trans data, len: %d",len);
            app_trans_process(app_mesh_manage.transfer.data, len-TRANSFER_DATA_LEN, addr, dst_addr);
            // 判断是单播地址还是群发组地址
            if( BLE_MESH_ADDR_IS_UNICAST(dst_addr) )
            {
                // 收到单播信息，这里演示原路把收到的数据第一字节加一返回
                app_mesh_manage.transfer_receive.cmd = CMD_TRANSFER_RECEIVE;
                app_mesh_manage.transfer_receive.data[0]++;
                status = vendor_model_srv_send(addr, app_mesh_manage.data.buf, len);
                if(status)
                {
                    APP_DBG("send failed %d", status);
                }
            }
            else
            {
                // 收到群组信息
            }
            break;
        }

        case CMD_TRANSFER_RECEIVE:
        {
            if(len < TRANSFER_RECEIVE_DATA_LEN)
            {
                APP_DBG("transfer receive data err!");
                return;
            }
            vendor_message_srv_trans_reset();
            // 通知主机(如果已连接)
            peripheralChar4Notify(app_mesh_manage.data.buf, len);
            break;
        }

        case CMD_IMAGE_INFO:
        {
            if(len != IMAGE_INFO_DATA_LEN)
            {
                APP_DBG("image info data err!");
                return;
            }
            int status;
            // 清除codeflash
            status = FLASH_ROM_ERASE(IMAGE_B_START_ADD, IMAGE_B_SIZE);
            // 填充 image info
            app_mesh_manage.image_info_ack.cmd = CMD_IMAGE_INFO_ACK;
            /* IMAGE_SIZE */
            app_mesh_manage.image_info_ack.image_size[0] = (uint8_t)(IMAGE_SIZE & 0xff);
            app_mesh_manage.image_info_ack.image_size[1] = (uint8_t)((IMAGE_SIZE >> 8) & 0xff);
            app_mesh_manage.image_info_ack.image_size[2] = (uint8_t)((IMAGE_SIZE >> 16) & 0xff);
            app_mesh_manage.image_info_ack.image_size[3] = (uint8_t)((IMAGE_SIZE >> 24) & 0xff);
            /* BLOCK SIZE */
            app_mesh_manage.image_info_ack.block_size[0] = (uint8_t)(FLASH_BLOCK_SIZE & 0xff);
            app_mesh_manage.image_info_ack.block_size[1] = (uint8_t)((FLASH_BLOCK_SIZE >> 8) & 0xff);
            /* CHIP ID */
            app_mesh_manage.image_info_ack.chip_id[0] = CHIP_ID&0xFF;
            app_mesh_manage.image_info_ack.chip_id[1] = (CHIP_ID<<8)&0xFF;
            /* STATUS */
            app_mesh_manage.image_info_ack.status = status;
            status = vendor_model_srv_send(addr, app_mesh_manage.data.buf, IMAGE_INFO_ACK_DATA_LEN);
            if(status)
            {
                APP_DBG("image info ack failed %d", status);
            }
            break;
        }

        case CMD_IMAGE_INFO_ACK:
        {
            if(len != IMAGE_INFO_ACK_DATA_LEN)
            {
                APP_DBG("image info ack data err!");
                return;
            }
            tmos_stop_task(App_TaskID, APP_OTA_UPDATE_TIMEOUT_EVT);
            vendor_message_srv_trans_reset();
            // 通知主机(如果已连接)
            peripheralChar4Notify(app_mesh_manage.data.buf, IMAGE_INFO_ACK_DATA_LEN);
            break;
        }

        case CMD_UPDATE:
        {
            if(len < UPDATE_DATA_LEN)
            {
                APP_DBG("update data err!");
                return;
            }
            int status;
            uint32_t OpParaDataLen = 0;
            uint32_t OpAdd = 0;
            /* flash的数据临时存储 */
            __attribute__((aligned(8))) uint8_t flash_buf[216];
            // 写flash
            OpParaDataLen = len-UPDATE_DATA_LEN;
            OpAdd = (uint32_t)(app_mesh_manage.update.update_addr[0]);
            OpAdd |= ((uint32_t)(app_mesh_manage.update.update_addr[1]) << 8);
            OpAdd = OpAdd * 8;

            OpAdd += IMAGE_A_SIZE;

            PRINT("IAP_PROM: %08x len:%d \r\n", (int)OpAdd, (int)OpParaDataLen);
            tmos_memcpy(flash_buf, app_mesh_manage.update.data, OpParaDataLen);
            /* 当前是ImageA，直接编程 */
            status = FLASH_ROM_WRITE(OpAdd, flash_buf, (uint16_t)OpParaDataLen);
            if(status)
            {
                PRINT("IAP_PROM err \r\n");
            }

            app_mesh_manage.update_ack.cmd = CMD_UPDATE_ACK;
            app_mesh_manage.update_ack.status = status;
            status = vendor_model_srv_send(addr, app_mesh_manage.data.buf, UPDATE_ACK_DATA_LEN);
            if(status)
            {
                APP_DBG("update ack failed %d", status);
            }
            break;
        }

        case CMD_UPDATE_ACK:
        {
            if(len != UPDATE_ACK_DATA_LEN)
            {
                APP_DBG("update ack data err!");
                return;
            }
            tmos_stop_task(App_TaskID, APP_OTA_UPDATE_TIMEOUT_EVT);
            vendor_message_srv_trans_reset();
            // 通知主机(如果已连接)
            peripheralChar4Notify(app_mesh_manage.data.buf, UPDATE_ACK_DATA_LEN);
            break;
        }


        case CMD_VERIFY:
        {
            if(len < VERIFY_DATA_LEN)
            {
                APP_DBG("verify data err!");
                return;
            }
            int status;
            // 校验flash
            uint32_t OpParaDataLen = 0;
            uint32_t OpAdd = 0;
            __attribute__((aligned(8))) uint8_t flash_buf[216];
            // 写flash
            OpParaDataLen = len-VERIFY_DATA_LEN;
            OpAdd = (uint32_t)(app_mesh_manage.verify.update_addr[0]);
            OpAdd |= ((uint32_t)(app_mesh_manage.verify.update_addr[1]) << 8);
            OpAdd = OpAdd * 8;

            OpAdd += IMAGE_A_SIZE;

            PRINT("IAP_VERIFY: %08x len:%d \r\n", (int)OpAdd, (int)OpParaDataLen);
            tmos_memcpy(flash_buf, app_mesh_manage.verify.data, OpParaDataLen);

            /* 当前是ImageA，直接编程 */
            status = FLASH_ROM_VERIFY(OpAdd, flash_buf, (uint16_t)OpParaDataLen);
            if(status)
            {
                PRINT("IAP_VERIFY err \r\n");
            }

            app_mesh_manage.verify_ack.cmd = CMD_VERIFY_ACK;
            app_mesh_manage.verify_ack.status = status;
            status = vendor_model_srv_send(addr, app_mesh_manage.data.buf, VERIFY_ACK_DATA_LEN);
            if(status)
            {
                APP_DBG("verify ack failed %d", status);
            }
            break;
        }

        case CMD_VERIFY_ACK:
        {
            if(len != VERIFY_ACK_DATA_LEN)
            {
                APP_DBG("verify ack data err!");
                return;
            }
            tmos_stop_task(App_TaskID, APP_OTA_UPDATE_TIMEOUT_EVT);
            vendor_message_srv_trans_reset();
            // 通知主机(如果已连接)
            peripheralChar4Notify(app_mesh_manage.data.buf, VERIFY_ACK_DATA_LEN);
            break;
        }

        case CMD_END:
        {
            if(len != END_DATA_LEN)
            {
                APP_DBG("end data err!");
                return;
            }
            int status;
            PRINT("IAP_END \r\n");

            /* 关闭当前所有使用中断，或者方便一点直接全部关闭 */
            SYS_DisableAllIrq(NULL);

            /* 修改DataFlash，切换至ImageIAP */
            SwitchImageFlag(IMAGE_IAP_FLAG);

            /* 等待打印完成 ，复位*/
            mDelaymS(10);
            SYS_ResetExecute();
            break;
        }

        case CMD_SET_SUB:
        {
            if(len != SET_SUB_DATA_LEN)
            {
                APP_DBG("set sub data err!");
                return;
            }
            int status;
            uint16_t sub_addr;
            uint16_t *match;
            sub_addr = app_mesh_manage.set_sub.sub_addr[0]|(app_mesh_manage.set_sub.sub_addr[1]<<8);
            if( BLE_MESH_ADDR_IS_GROUP(sub_addr) )
            {
                if( app_mesh_manage.set_sub.add_flag )
                {
                    match = App_model_find_group( &vnd_models[0], BLE_MESH_ADDR_UNASSIGNED);
                    if( match )
                    {
                        // 本地添加订阅地址
                        *match = (uint16_t)sub_addr;
                        bt_mesh_store_mod_sub(&vnd_models[0]);
                        status = STATUS_SUCCESS;
                        APP_DBG("lcoal sub addr added");
                    }
                    else
                    {
                        status = STATUS_NOMEM;
                    }
                }
                else
                {
                    match = App_model_find_group( &vnd_models[0], sub_addr);
                    if( match )
                    {
                        // 本地删除订阅地址
                        *match = (uint16_t)BLE_MESH_ADDR_UNASSIGNED;
                        bt_mesh_store_mod_sub(&vnd_models[0]);
                        status = STATUS_SUCCESS;
                        APP_DBG("lcoal sub addr deleted");
                    }
                    else
                    {
                        status = STATUS_INVALID;
                    }
                }
            }
            else
            {
                status = STATUS_INVALID;
            }
            app_mesh_manage.set_sub_ack.cmd = CMD_SET_SUB_ACK;
            app_mesh_manage.set_sub_ack.status = status; //用户自定义状态码
            status = vendor_model_srv_send(addr, app_mesh_manage.data.buf, SET_SUB_ACK_DATA_LEN);
            if(status)
            {
                APP_DBG("set sub ack failed %d", status);
            }
            break;
        }

        case CMD_SET_SUB_ACK:
        {
            if(len != SET_SUB_ACK_DATA_LEN)
            {
                APP_DBG("set sub ack data err!");
                return;
            }
            tmos_stop_task(App_TaskID, APP_SET_SUB_TIMEOUT_EVT);
            vendor_message_srv_trans_reset();
            // 通知主机(如果已连接)
            peripheralChar4Notify(app_mesh_manage.data.buf, SET_SUB_ACK_DATA_LEN);
            break;
        }

        default:
        {
            APP_DBG("Invalid CMD ");
            break;
        }
    }
}

/*********************************************************************
 * @fn      App_peripheral_reveived
 *
 * @brief   ble外设从机收到数据
 *
 * @param   pValue - pointer to data that was changed
 *          len - length of data
 *
 * @return  none
 */
void App_peripheral_reveived(uint8_t *pValue, uint16_t len)
{
    tmos_memcpy(&app_mesh_manage, pValue, len);
    APP_DBG("CMD: %x", app_mesh_manage.data.buf[0]);
    switch(app_mesh_manage.data.buf[0])
    {
        // 配网信息命令
        case CMD_PROVISION_INFO:
        {
            if(len != PROVISION_INFO_DATA_LEN)
            {
                APP_DBG("Privisioning info data err!");
                return;
            }
            // 判断是设置还是查询
            if( app_mesh_manage.provision_info.set_flag )
            {
                self_prov_iv_index = app_mesh_manage.provision_info.iv_index[0]|
                    app_mesh_manage.provision_info.iv_index[1]<<8|
                    app_mesh_manage.provision_info.iv_index[2]<<16|
                    app_mesh_manage.provision_info.iv_index[3]<<24;
                self_prov_flags = app_mesh_manage.provision_info.flag;
                APP_DBG("set iv 0x%x, flag %d ",self_prov_iv_index,self_prov_flags);
                app_mesh_manage.provision_info_ack.status = STATUS_SUCCESS;
            }
            else
            {
                uint32_t iv_index = bt_mesh_iv_index_get();
                app_mesh_manage.provision_info_ack.iv_index[0] = iv_index&0xFF;
                app_mesh_manage.provision_info_ack.iv_index[1] = (iv_index>>8)&0xFF;
                app_mesh_manage.provision_info_ack.iv_index[2] = (iv_index>>16)&0xFF;
                app_mesh_manage.provision_info_ack.iv_index[3] = (iv_index>>24)&0xFF;
                app_mesh_manage.provision_info_ack.flag = bt_mesh_net_flags_get(self_prov_net_idx);
                APP_DBG("ask iv 0x%x, flag %d ",iv_index,app_mesh_manage.provision_info_ack.flag);
                app_mesh_manage.provision_info_ack.status = STATUS_SUCCESS;
            }
            app_mesh_manage.provision_info_ack.cmd = CMD_PROVISION_INFO_ACK;
            peripheralChar4Notify(app_mesh_manage.data.buf, PROVISION_INFO_ACK_DATA_LEN);
            break;
        }

        // 配网命令 包含 1字节命令码+16字节网络密钥+2字节网络地址
        case CMD_PROVISION:
        {
            if(len != PROVISION_DATA_LEN)
            {
                APP_DBG("Privisioning data err!");
                return;
            }
            tmos_memcpy(self_prov_net_key, app_mesh_manage.provision.net_key, PROVISION_NET_KEY_LEN);
            self_prov_addr = app_mesh_manage.provision.addr[0]|(app_mesh_manage.provision.addr[1]<<8);
            tmos_start_task(App_TaskID, APP_NODE_PROVISION_EVT, 160);
            break;
        }

        // 删除网络中节点(包括自己，建议使用此方式)，通过应用层自定协议删除
        // 包含 1字节命令码+2字节需要删除的节点地址
        case CMD_DELETE_NODE:
        {
            if(len != DELETE_NODE_DATA_LEN)
            {
                APP_DBG("Delete node data err!");
                return;
            }
            uint16 remote_addr;
            int status;
            remote_addr = app_mesh_manage.delete_node.addr[0]|(app_mesh_manage.delete_node.addr[1]<<8);
            APP_DBG("CMD_DELETE_NODE %x ",remote_addr);
            status = vendor_model_srv_send(remote_addr, app_mesh_manage.data.buf, DELETE_NODE_DATA_LEN);
            if(status)
            {
                APP_DBG("delete failed %d", status);
            }
            else
            {
                delete_node_address = remote_addr;
                // 定时，未收到应答就超时
                tmos_start_task(App_TaskID, APP_DELETE_NODE_TIMEOUT_EVT, APP_CMD_TIMEOUT);
            }
            break;
        }

        case CMD_ASK_STATUS:
        {
            if(len != ASK_STATUS_DATA_LEN)
            {
                APP_DBG("ask status data err!");
                return;
            }
            uint16 remote_addr;
            int status;
            remote_addr = app_mesh_manage.ask_status.addr[0]|(app_mesh_manage.ask_status.addr[1]<<8);
            APP_DBG("CMD_ASK_STATUS %x ",remote_addr);
            status = vendor_model_srv_send(remote_addr, app_mesh_manage.data.buf, ASK_STATUS_DATA_LEN);
            if(status)
            {
                APP_DBG("ask_status failed %d", status);
            }
            else
            {
                ask_status_node_address = remote_addr;
                // 定时，未收到应答就超时
                tmos_start_task(App_TaskID, APP_ASK_STATUS_NODE_TIMEOUT_EVT, APP_CMD_TIMEOUT);
            }
            break;
        }

        case CMD_TRANSFER:
        {
            if(len < TRANSFER_DATA_LEN)
            {
                APP_DBG("transfer data err!");
                return;
            }
            uint16 remote_addr;
            int status;
            remote_addr = app_mesh_manage.transfer.addr[0]|(app_mesh_manage.transfer.addr[1]<<8);
            APP_DBG("CMD_TRANSFER %x ",remote_addr);
            status = vendor_model_srv_send(remote_addr, app_mesh_manage.data.buf, len);
            if(status)
            {
                APP_DBG("transfer failed %d", status);
            }
            break;
        }

        case CMD_IMAGE_INFO:
        {
            if(len != IMAGE_INFO_DATA_LEN)
            {
                APP_DBG("image info data err!");
                return;
            }
            uint16 remote_addr;
            int status;
            remote_addr = app_mesh_manage.image_info.addr[0]|(app_mesh_manage.image_info.addr[1]<<8);
            APP_DBG("CMD_IMAGE_INFO %x ",remote_addr);
            status = vendor_model_srv_send(remote_addr, app_mesh_manage.data.buf, IMAGE_INFO_DATA_LEN);
            if(status)
            {
                APP_DBG("image info failed %d", status);
                break;
            }
            else
            {
                ota_update_node_address = remote_addr;
                tmos_start_task(App_TaskID, APP_OTA_UPDATE_TIMEOUT_EVT, APP_CMD_TIMEOUT);
            }
            break;
        }

        case CMD_UPDATE:
        {
            if(len < UPDATE_DATA_LEN)
            {
                APP_DBG("update data err!");
                return;
            }
            uint16 remote_addr;
            int status;
            APP_DBG("CMD_UPDATE len: %d", len);
            remote_addr = app_mesh_manage.transfer.addr[0]|(app_mesh_manage.transfer.addr[1]<<8);
            status = vendor_model_srv_send(remote_addr, app_mesh_manage.data.buf, len);
            if(status)
            {
                APP_DBG("update failed %d", status);
            }
            else
            {
                ota_update_node_address = remote_addr;
                tmos_start_task(App_TaskID, APP_OTA_UPDATE_TIMEOUT_EVT, APP_CMD_TIMEOUT);
            }
            break;
        }

        case CMD_VERIFY:
        {
            if(len < VERIFY_DATA_LEN)
            {
                APP_DBG("verify data err!");
                return;
            }
            uint16 remote_addr;
            int status;
            remote_addr = app_mesh_manage.verify.addr[0]|(app_mesh_manage.verify.addr[1]<<8);
            status = vendor_model_srv_send(remote_addr, app_mesh_manage.data.buf, len);
            if(status)
            {
                APP_DBG("verify failed %d", status);
            }
            else
            {
                ota_update_node_address = remote_addr;
                tmos_start_task(App_TaskID, APP_OTA_UPDATE_TIMEOUT_EVT, APP_CMD_TIMEOUT);
            }
            break;
        }

        case CMD_END:
        {
            if(len != END_DATA_LEN)
            {
                APP_DBG("end data err!");
                return;
            }
            uint16 remote_addr;
            int status;
            remote_addr = app_mesh_manage.end.addr[0]|(app_mesh_manage.end.addr[1]<<8);
            status = vendor_model_srv_send(remote_addr, app_mesh_manage.data.buf, END_DATA_LEN);
            if(status)
            {
                APP_DBG("end failed %d", status);
            }
            break;
        }

        case CMD_SET_SUB:
        {
            if(len != SET_SUB_DATA_LEN)
            {
                APP_DBG("set sub data err!");
                return;
            }
            uint16 remote_addr;
            int status;
            remote_addr = app_mesh_manage.set_sub.addr[0]|(app_mesh_manage.set_sub.addr[1]<<8);
            APP_DBG("CMD_SET_SUB %x ",remote_addr);
            status = vendor_model_srv_send(remote_addr, app_mesh_manage.data.buf, SET_SUB_DATA_LEN);
            if(status)
            {
                APP_DBG("set sub failed %d", status);
            }
            else
            {
                set_sub_node_address = remote_addr;
                tmos_start_task(App_TaskID, APP_SET_SUB_TIMEOUT_EVT, APP_CMD_TIMEOUT);
            }
            break;
        }

        // 本地复位命令，包含 1字节命令码
        case CMD_LOCAL_RESET:
        {
            if(len != LOCAL_RESET_DATA_LEN)
            {
                APP_DBG("local reset data err!");
                return;
            }
            APP_DBG("Local mesh reset");
            self_prov_addr = 0;
            bt_mesh_reset();
            break;
        }

        default:
        {
            APP_DBG("Invalid CMD ");
            break;
        }
    }
}

/******************************** endfile @ main ******************************/

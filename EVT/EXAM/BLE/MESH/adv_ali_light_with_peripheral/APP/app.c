/********************************** (C) COPYRIGHT *******************************
 * File Name          : app.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2021/03/24
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
#include "CONFIG.h"
#include "MESH_LIB.h"
#include "app_vendor_model.h"
#include "app_generic_onoff_model.h"
#include "app.h"
#include "HAL.h"
#include "peripheral.h"
#include "app_generic_lightness_model.h"
#include "app_generic_color_model.h"

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
#define ADV_TIMEOUT       K_MINUTES(10)

#define SELENCE_ADV_ON    0x01
#define SELENCE_ADV_OF    0x00

/*********************************************************************
 * GLOBAL TYPEDEFS
 */

static uint8_t MESH_MEM[1024 * 2] = {0};

extern const ble_mesh_cfg_t app_mesh_cfg;
extern const struct device  app_dev;

static uint8_t App_TaskID = 0; // Task ID for internal task/event processing

static uint16_t App_ProcessEvent(uint8_t task_id, uint16_t events);

#if(!CONFIG_BLE_MESH_PB_GATT)
NET_BUF_SIMPLE_DEFINE_STATIC(rx_buf, 65);
#endif /* !PB_GATT */

/*********************************************************************
 * LOCAL FUNCION
 */

static void link_open(bt_mesh_prov_bearer_t bearer);
static void link_close(bt_mesh_prov_bearer_t bearer, uint8_t reason);
static void prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index);
static void prov_reset(void);

static struct bt_mesh_cfg_srv cfg_srv = {
    .relay = BLE_MESH_RELAY_ENABLED,
    .beacon = BLE_MESH_BEACON_ENABLED,
#if(CONFIG_BLE_MESH_FRIEND)
    .frnd = BLE_MESH_FRIEND_ENABLED,
#endif
#if(CONFIG_BLE_MESH_PROXY)
    .gatt_proxy = BLE_MESH_GATT_PROXY_ENABLED,
#endif
    .default_ttl = 3,

    /* 8 transmissions with 10ms interval */
    .net_transmit = BLE_MESH_TRANSMIT(7, 10),
    .relay_retransmit = BLE_MESH_TRANSMIT(7, 10),
};

static struct bt_mesh_health_srv health_srv;

BLE_MESH_HEALTH_PUB_DEFINE(health_pub, 8);

uint16_t cfg_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = {BLE_MESH_KEY_UNUSED};
uint16_t cfg_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = {BLE_MESH_ADDR_UNASSIGNED};

uint16_t health_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = {BLE_MESH_KEY_UNUSED};
uint16_t health_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = {BLE_MESH_ADDR_UNASSIGNED};

uint16_t gen_onoff_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = {BLE_MESH_KEY_UNUSED};
uint16_t gen_onoff_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = {BLE_MESH_ADDR_UNASSIGNED};

uint16_t gen_lightness_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = {BLE_MESH_KEY_UNUSED};             //添加处  定义亮度属性
uint16_t gen_lightness_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = {BLE_MESH_ADDR_UNASSIGNED};

uint16_t gen_color_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = {BLE_MESH_KEY_UNUSED};             //定义色温属性
uint16_t gen_color_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = {BLE_MESH_ADDR_UNASSIGNED};

static struct bt_mesh_model root_models[] = {
    BLE_MESH_MODEL_CFG_SRV(cfg_srv_keys, cfg_srv_groups, &cfg_srv),
    BLE_MESH_MODEL_HEALTH_SRV(health_srv_keys, health_srv_groups, &health_srv, &health_pub),
    BLE_MESH_MODEL(BLE_MESH_MODEL_ID_GEN_ONOFF_SRV, gen_onoff_op, NULL, gen_onoff_srv_keys, gen_onoff_srv_groups, NULL),
    BLE_MESH_MODEL(BLE_MESH_MODEL_ID_LIGHT_LIGHTNESS_SRV, gen_lightness_op, NULL, gen_lightness_srv_keys, gen_lightness_srv_groups, NULL),      //添加root 亮度属性
    BLE_MESH_MODEL(BLE_MESH_MODEL_ID_LIGHT_CTL_TEMP_SRV , gen_color_op, NULL, gen_color_srv_keys, gen_color_srv_groups, NULL),      //添加root 色温属性
};

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
    .uuid = tm_uuid,
    .static_val_len = ARRAY_SIZE(static_key),
    .static_val = static_key,
    .link_open = link_open,
    .link_close = link_close,
    .complete = prov_complete,
    .reset = prov_reset,
};

/*********************************************************************
 * GLOBAL TYPEDEFS
 */

/*********************************************************************
 * @fn      silen_adv_set
 *
 * @brief   设置静默广播
 *
 * @param   flag   - 0（处于未配网广播状态），1（处于静默广播状态）.
 *
 * @return  none
 */
static void silen_adv_set(uint8_t flag)
{
    tm_uuid[13] &= ~BIT(0);
    tm_uuid[13] |= (BIT_MASK(1) & flag);
}

/*********************************************************************
 * @fn      prov_enable
 *
 * @brief   使能配网功能
 *
 * @return  none
 */
static void prov_enable(void)
{
    silen_adv_set(SELENCE_ADV_OF);

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

    tmos_start_task(App_TaskID, APP_SILENT_ADV_EVT, ADV_TIMEOUT);
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
    APP_DBG(" ");

    tmos_stop_task(App_TaskID, APP_SILENT_ADV_EVT);
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
    APP_DBG("");

    if(!bt_mesh_is_provisioned())
    {
        prov_enable();
    }
    else
    {
        /*天猫精灵不会下发Config_model_app_bind和Config_Model_Subscrption_Add消息。
         IOT设备需要自行给所有Element的所有model绑定下发的AppKey，并根据产品类型为各个
         model订阅相应的组播地址（具体品类组播地址请参阅各产品软件规范）。蓝牙Mesh设备
         完成配网后需要进行消息上报，上报消息包括该设备所有支持的可上报的属性。*/

        /* For Light Subscription group address */
        root_models[2].groups[0] = (uint16_t)0xC000;
        root_models[2].groups[1] = (uint16_t)0xCFFF;
        bt_mesh_store_mod_sub(&root_models[2]);

        root_models[2].keys[0] = (uint16_t)0x0000;
        bt_mesh_store_mod_bind(&root_models[2]);

        /* For Light Subscription group address */          //添加处  亮度属性
        root_models[3].groups[0] = (uint16_t)0xC000;
        root_models[3].groups[1] = (uint16_t)0xCFFF;
        bt_mesh_store_mod_sub(&root_models[3]);

        root_models[3].keys[0] = (uint16_t)0x0000;
        bt_mesh_store_mod_bind(&root_models[3]);

        /* For Light Subscription group address */          //添加处  色温属性
        root_models[4].groups[0] = (uint16_t)0xC000;
        root_models[4].groups[1] = (uint16_t)0xCFFF;
        bt_mesh_store_mod_sub(&root_models[4]);

        root_models[4].keys[0] = (uint16_t)0x0000;
        bt_mesh_store_mod_bind(&root_models[4]);

        /* For Light Subscription group address */
        vnd_models[0].groups[0] = (uint16_t)0xC000;
        vnd_models[0].groups[1] = (uint16_t)0xCFFF;
        bt_mesh_store_mod_sub(&vnd_models[0]);

        vnd_models[0].keys[0] = (uint16_t)0x0000;
        bt_mesh_store_mod_bind(&vnd_models[0]);
    }
}

/*********************************************************************
 * @fn      prov_complete
 *
 * @brief   配网完成回调，重新开始广播
 *
 * @param   net_idx     - 网络key的index
 * @param   addr        - 网络地址
 * @param   flags       - 是否处于key refresh状态
 * @param   iv_index    - 当前网络iv的index
 *
 * @return  none
 */
static void prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index)
{
    /* 设备上电后，如果已配网，也需要在1~10s随机间隔后上报所有支持的属性状态。 */
    tmosTimer rand_timer;
    APP_DBG(" ");

    rand_timer = K_SECONDS(5) + (tmos_rand() % K_SECONDS(6));
    tmos_start_task(App_TaskID, APP_SILENT_ADV_EVT, rand_timer);
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
    APP_DBG("");

    prov_enable();
}

/*********************************************************************
 * @fn      ind_end_cb
 *
 * @brief   发送复位事件完成回调
 *
 * @param   err     - 错误码
 * @param   cb_data - 回调参数
 *
 * @return  none
 */
static void ind_end_cb(int err, void *cb_data)
{
    APP_DBG(" bt_mesh_reset ");
    bt_mesh_reset();
}

static const struct bt_adv_ind_send_cb reset_cb = {
    .end = ind_end_cb,
};

/*********************************************************************
 * @fn      send_support_attr
 *
 * @brief   发送所有支持的可上报的属性给天猫精灵,此消息决定天猫精灵判断设备支持那些功能
 *
 * @param   none
 *
 * @return  none
 */
void send_support_attr(void)
{
    struct bt_mesh_indicate *ind;
    APP_DBG("");

    if(!bt_mesh_is_provisioned())
    {
        APP_DBG("Local Dev Unprovisioned");
        return;
    }

    ind = bt_mesh_ind_alloc(32);
    if(!ind)
    {
        APP_DBG("Unable allocate buffers");
        return;
    }
    ind->param.trans_cnt = 0x09;
    ind->param.period = K_MSEC(300);
    ind->param.send_ttl = BLE_MESH_TTL_DEFAULT;
    ind->param.tid = als_avail_tid_get();

    /* Init indication opcode */
    bt_mesh_model_msg_init(&(ind->buf->b), OP_VENDOR_MESSAGE_ATTR_INDICATION);

    /* Add tid field */
    net_buf_simple_add_u8(&(ind->buf->b), ind->param.tid);

    // 添加开关属性
    {
        /* Add generic onoff attrbute op */
        net_buf_simple_add_le16(&(ind->buf->b), ALI_GEN_ATTR_TYPE_POWER_STATE);

        /* Add current generic onoff status */
        net_buf_simple_add_u8(&(ind->buf->b), read_led_state(MSG_PIN));
    }
    //	// 可选根据阿里云设置的产品属性功能添加对应属性 ( BLE_MESH_MODEL_OP_LIGHT_LIGHTNESS_SET )
    //	// 这里添加亮度属性留作参考,还需要添加对应的opcode处理函数,添加方式参考开关属性的 gen_onoff_op 结构

    //添加亮度属性
    {
        /* Add brightness attrbute opcode */
        net_buf_simple_add_le16(&(ind->buf->b), ALI_GEN_ATTR_TYPE_BRIGHTNESS);

    /* Add brightness status (655~65535对应天猫控制亮度1~100) */
        net_buf_simple_add_le16(&(ind->buf->b), 65535);
    }

    //添加色温属性
    {
        /* Add brightness attrbute opcode */
        net_buf_simple_add_le16(&(ind->buf->b), ALI_GEN_ATTR_TYPE_COLOR);

    /* Add brightness status (992~20000对应天猫控制色温1~100) */
        net_buf_simple_add_le16(&(ind->buf->b), 20000);
    }

    bt_mesh_indicate_send(ind);
}

/*********************************************************************
 * @fn      send_led_state
 *
 * @brief   发送当前灯的状态给天猫精灵
 *
 * @param   none
 *
 * @return  none
 */
void send_led_state(void)
{
    APP_DBG("");
    struct indicate_param param = {
        .trans_cnt = 0x09,
        .period = K_MSEC(300),
        .send_ttl = BLE_MESH_TTL_DEFAULT,
        .tid = als_avail_tid_get(),
    };

    //toggle_led_state(MSG_PIN);      //翻转灯的状态
    //set_led_lightness(MSG_PIN, led_lightness);


    if(!bt_mesh_is_provisioned())
    {
        APP_DBG("Local Dev Unprovisioned");
        return;
    }

    send_led_indicate(&param);                  //发送灯的 开关 状态
    send_lightness_indicate(&param);            //发送灯的 亮度 状态
    send_color_indicate(&param);                //发送灯的 色温 状态
}

/*********************************************************************
 * @fn      send_reset_indicate
 *
 * @brief   发送复位事件给天猫精灵，发送完成后将清除配网状态，重置自身mesh网络
 *
 * @param   none
 *
 * @return  none
 */
void send_reset_indicate(void)
{
    struct bt_mesh_indicate *ind;
    APP_DBG("");

    if(!bt_mesh_is_provisioned())
    {
        APP_DBG("Local Dev Unprovisioned");
        return;
    }

    ind = bt_mesh_ind_alloc(16);
    if(!ind)
    {
        APP_DBG("Unable allocate buffers");
        return;
    }
    ind->param.trans_cnt = 0x09;
    ind->param.period = K_MSEC(300);
    ind->param.cb = &reset_cb;
    ind->param.send_ttl = BLE_MESH_TTL_DEFAULT;
    ind->param.tid = als_avail_tid_get();

    /* Init indication opcode */
    bt_mesh_model_msg_init(&(ind->buf->b), OP_VENDOR_MESSAGE_ATTR_INDICATION);

    /* Add tid field */
    net_buf_simple_add_u8(&(ind->buf->b), ind->param.tid);

    /* Add event report opcode */
    net_buf_simple_add_le16(&(ind->buf->b), ALI_GEN_ATTR_TYPE_EVENT_TRIGGER);

    /* Add reset event */
    net_buf_simple_add_u8(&(ind->buf->b), ALI_GEN_ATTR_TYPE_HARDWARE_RESET);

    bt_mesh_indicate_send(ind);
}

#define HAL_KEY_SEND_MSG    BIT(0)
#define HAL_KEY_RESET       BIT(1)

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
    APP_DBG("keys : %d ", keys);

    switch(keys)
    {
        case HAL_KEY_SEND_MSG:
            send_led_state();
            break;
        case HAL_KEY_RESET:
            send_reset_indicate();
            break;
    }
}

/*********************************************************************
 * @fn      app_silent_adv
 *
 * @brief   超时后如果还未配网成功，则进入静默广播模式,若已配网，发送支持的属性给天猫精灵
 *
 * @param   none
 *
 * @return  none
 */
static void app_silent_adv(void)
{
    APP_DBG("");
    if(bt_mesh_is_provisioned())
    {
        send_support_attr();
        return;
    }

    silen_adv_set(SELENCE_ADV_ON);

    /* Disable Scanner not response Provisioner message */
    bt_mesh_scan_disable();
}

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
        APP_DBG("head file error...\n");
        while(1)
            ;
    }

    info.base_addr = MESH_MEM;
    info.mem_len = ARRAY_SIZE(MESH_MEM);

#if(CONFIG_BLE_MESH_FRIEND)
    friend_init_register(bt_mesh_friend_init, friend_state);
#endif /* FRIEND */
#if(CONFIG_BLE_MESH_LOW_POWER)
    lpn_init_register(bt_mesh_lpn_init, lpn_state);
#endif /* LPN */

    err = bt_mesh_cfg_set(&app_mesh_cfg, &app_dev, MacAddr, &info);
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
        APP_DBG("Mesh network restored from flash");
    }
    else
    {
        prov_enable();
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
#if 0       //调试时，调用查看unknow回调状态
void my_test1(uint32_t opcode, struct bt_mesh_model *model,
        struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf )
{
    PRINT("指令有效:%x\n", opcode);

}
#endif

void App_Init(void)
{
    GAPRole_PeripheralInit();
    Peripheral_Init();
    App_TaskID = TMOS_ProcessEventRegister(App_ProcessEvent);

    als_vendor_init(vnd_models);
    blemesh_on_sync();
    HAL_KeyInit();
    HalKeyConfig(keyPress);
    set_led_state(MSG_PIN, 0);

    //bt_mesh_model_reg_elem_unkonw_op_cb((elem_unkonw_op_cb_t)my_test1);  //初始化调用，查看天猫指令发送时 unknow回调状态
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
    if(events & APP_SILENT_ADV_EVT)
    {
        app_silent_adv();
        return (events ^ APP_SILENT_ADV_EVT);
    }

    // Discard unknown events
    return 0;
}

/******************************** endfile @ main ******************************/

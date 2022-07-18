/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2020/08/06
 * Description        :
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/******************************************************************************/
/* 头文件包�? */
#include <time_units.h>
#include "CONFIG.h"
#include "HAL.h"
#include "ancs.h"
#include "ancs_client.h"
#include "time_units.h"


static void bt_ancs_notification_source_handler(struct bt_ancs_client *ancs_c,
        int err, const struct bt_ancs_evt_notif *notif);
static void bt_ancs_data_source_handler(struct bt_ancs_client *ancs_c,
        const struct bt_ancs_attr_response *response);
static void bt_ancs_write_response_handler(struct bt_ancs_client *ancs_c,
                       uint8_t err);

__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

#if(defined(BLE_MAC)) && (BLE_MAC == TRUE)
const uint8_t MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif

#define ATTR_DATA_SIZE BT_ANCS_ATTR_DATA_MAX

#define KEY_REQ_NOTI_ATTR   (1<<0)
#define KEY_REQ_APP_ATTR    (1<<1)
#define KEY_POS_ACTION      (1<<0)
#define KEY_NEG_ACTION      (1<<0)

static struct bt_ancs_subscribe_cb subscribe = {
    .data_source = bt_ancs_data_source_handler,
    .notification_source = bt_ancs_notification_source_handler,
    .write_response = bt_ancs_write_response_handler,
};


/* Local copy to keep track of the newest arriving notifications. */
static struct bt_ancs_evt_notif notification_latest;
/* Local copy of the newest notification attribute. */
static struct bt_ancs_attr notif_attr_latest;
/* Local copy of the newest app attribute. */
static struct bt_ancs_attr notif_attr_app_id_latest;

/* Buffers to store attribute data. */
static uint8_t attr_appid[ATTR_DATA_SIZE];
static uint8_t attr_title[ATTR_DATA_SIZE];
static uint8_t attr_subtitle[ATTR_DATA_SIZE];
static uint8_t attr_message[ATTR_DATA_SIZE];
static uint8_t attr_message_size[ATTR_DATA_SIZE];
static uint8_t attr_date[ATTR_DATA_SIZE];
static uint8_t attr_posaction[ATTR_DATA_SIZE];
static uint8_t attr_negaction[ATTR_DATA_SIZE];
static uint8_t attr_disp_name[ATTR_DATA_SIZE];

/* String literals for the iOS notification categories.
 * Used then printing to UART.
 */
static const char *lit_catid[BT_ANCS_CATEGORY_ID_COUNT] = {
    "Other",
    "Incoming Call",
    "Missed Call",
    "Voice Mail",
    "Social",
    "Schedule",
    "Email",
    "News",
    "Health And Fitness",
    "Business And Finance",
    "Location",
    "Entertainment"
};

/* String literals for the iOS notification event types.
 * Used then printing to UART.
 */
static const char *lit_eventid[BT_ANCS_EVT_ID_COUNT] = { "Added",
                             "Modified",
                             "Removed" };

/* String literals for the iOS notification attribute types.
 * Used when printing to UART.
 */
static const char *lit_attrid[BT_ANCS_NOTIF_ATTR_COUNT] = {
    "App Identifier",
    "Title",
    "Subtitle",
    "Message",
    "Message Size",
    "Date",
    "Positive Action Label",
    "Negative Action Label"
};

static const char *lit_appid[BT_ANCS_APP_ATTR_COUNT] = { "Display Name" };





static void notif_print(struct bt_ancs_evt_notif *notif)
{
    PRINT("\nNotification\n");
    PRINT("Event:       %s\n", lit_eventid[notif->evt_id]);
    PRINT("Category ID: %s\n", lit_catid[notif->category_id]);
    PRINT("Category Cnt:%u\n", (unsigned int)notif->category_count);
    PRINT("UID:         %u\n", (unsigned int)notif->notif_uid);

    PRINT("Flags:\n");
    if (notif->evt_flags.silent) {
        PRINT(" Silent\n");
    }
    if (notif->evt_flags.important) {
        PRINT(" Important\n");
    }
    if (notif->evt_flags.pre_existing) {
        PRINT(" Pre-existing\n");
    }
    if (notif->evt_flags.positive_action) {
        PRINT(" Positive Action\n");
    }
    if (notif->evt_flags.negative_action) {
        PRINT(" Negative Action\n");
    }
}

/**@brief Function for printing iOS notification attribute data.
 *
 * @param[in] attr Pointer to an iOS notification attribute.
 */
static void notif_attr_print(const struct bt_ancs_attr *attr)
{
    if (attr->attr_len != 0) {
        PRINT("%s: %s\n", lit_attrid[attr->attr_id],
               (char *)attr->attr_data);
    } else if (attr->attr_len == 0) {
        PRINT("%s: (N/A)\n", lit_attrid[attr->attr_id]);
    }
}

/**@brief Function for printing iOS notification attribute data.
 *
 * @param[in] attr Pointer to an iOS App attribute.
 */
static void app_attr_print(const struct bt_ancs_attr *attr)
{
    if (attr->attr_len != 0) {
        PRINT("%s: %s\n", lit_appid[attr->attr_id],
               (char *)attr->attr_data);
    } else if (attr->attr_len == 0) {
        PRINT("%s: (N/A)\n", lit_appid[attr->attr_id]);
    }
}

static int ancs_init(void)
{
    int err;

    err = bt_ancs_client_init(&ancs_c);
    if (err) {
        return err;
    }

    err = bt_ancs_register_attr(&ancs_c,
        BT_ANCS_NOTIF_ATTR_ID_APP_IDENTIFIER,
        attr_appid, ATTR_DATA_SIZE);
    if (err) {
        return err;
    }

    err = bt_ancs_register_app_attr(&ancs_c,
        BT_ANCS_APP_ATTR_ID_DISPLAY_NAME,
        attr_disp_name, sizeof(attr_disp_name));
    if (err) {
        return err;
    }

    err = bt_ancs_register_attr(&ancs_c,
        BT_ANCS_NOTIF_ATTR_ID_TITLE,
        attr_title, ATTR_DATA_SIZE);
    if (err) {
        return err;
    }

    err = bt_ancs_register_attr(&ancs_c,
        BT_ANCS_NOTIF_ATTR_ID_MESSAGE,
        attr_message, ATTR_DATA_SIZE);
    if (err) {
        return err;
    }

    err = bt_ancs_register_attr(&ancs_c,
        BT_ANCS_NOTIF_ATTR_ID_SUBTITLE,
        attr_subtitle, ATTR_DATA_SIZE);
    if (err) {
        return err;
    }

    err = bt_ancs_register_attr(&ancs_c,
        BT_ANCS_NOTIF_ATTR_ID_MESSAGE_SIZE,
        attr_message_size, ATTR_DATA_SIZE);
    if (err) {
        return err;
    }

    err = bt_ancs_register_attr(&ancs_c,
        BT_ANCS_NOTIF_ATTR_ID_DATE,
        attr_date, ATTR_DATA_SIZE);
    if (err) {
        return err;
    }

    err = bt_ancs_register_attr(&ancs_c,
        BT_ANCS_NOTIF_ATTR_ID_POSITIVE_ACTION_LABEL,
        attr_posaction, ATTR_DATA_SIZE);
    if (err) {
        return err;
    }

    err = bt_ancs_register_attr(&ancs_c,
        BT_ANCS_NOTIF_ATTR_ID_NEGATIVE_ACTION_LABEL,
        attr_negaction, ATTR_DATA_SIZE);

    return err;
}

static void bt_ancs_notification_source_handler(struct bt_ancs_client *ancs_c,
        int err, const struct bt_ancs_evt_notif *notif)
{
    if (!err) {
        notification_latest = *notif;
        notif_print(&notification_latest);
    }
}

static void bt_ancs_data_source_handler(struct bt_ancs_client *ancs_c,
        const struct bt_ancs_attr_response *response)
{
    switch (response->command_id) {
    case BT_ANCS_COMMAND_ID_GET_NOTIF_ATTRIBUTES:
        notif_attr_latest = response->attr;
        notif_attr_print(&notif_attr_latest);
        if (response->attr.attr_id ==
            BT_ANCS_NOTIF_ATTR_ID_APP_IDENTIFIER) {
            notif_attr_app_id_latest = response->attr;
        }
        break;

    case BT_ANCS_COMMAND_ID_GET_APP_ATTRIBUTES:
        app_attr_print(&response->attr);
        break;

    default:
        /* No implementation needed. */
        break;
    }
}

static void bt_ancs_write_response_handler(struct bt_ancs_client *ancs_c,
                       uint8_t err)
{
    switch (err) {
    case BT_ATT_ERR_ANCS_NP_UNKNOWN_COMMAND:
        PRINT("Error: Command ID was not recognized by the Notification Provider.\n");
        break;

    case BT_ATT_ERR_ANCS_NP_INVALID_COMMAND:
        PRINT("Error: Command failed to be parsed on the Notification Provider.\n");
        break;

    case BT_ATT_ERR_ANCS_NP_INVALID_PARAMETER:
        PRINT("Error: Parameter does not refer to an existing object on the Notification Provider.\n");
        break;

    case BT_ATT_ERR_ANCS_NP_ACTION_FAILED:
        PRINT("Error: Perform Notification Action Failed on the Notification Provider.\n");
        break;

    default:
        break;
    }
}

struct key_state {
    uint32_t timestamp_key1;
    uint32_t timestamp_key2;
};

/**
 * key1(PB22):
 *      single: request attributes for a notification.
 *      double: request attributes for a given app.
 *
 * key2(PB4):
 *      single: perform a positive action.
 *      double: perform a negative action.
 */
void button_changed(uint8_t keys)
{
    int err;
    static struct key_state k_s = {0};

    if(keys & HAL_KEY_SW_1){
        uint32_t current_time = get_current_time();
        if(current_time - k_s.timestamp_key1 < 600){
            if (notif_attr_app_id_latest.attr_id ==
                    BT_ANCS_NOTIF_ATTR_ID_APP_IDENTIFIER &&
                notif_attr_app_id_latest.attr_len != 0) {
                PRINT("Request for %s:\n",
                       notif_attr_app_id_latest.attr_data);
                err = bt_ancs_request_app_attr(
                    &ancs_c, notif_attr_app_id_latest.attr_data,
                    notif_attr_app_id_latest.attr_len,
                    bt_ancs_write_response_handler);
                if (err) {
                    PRINT("Failed requesting attributes for a given app (err: %d)\n",
                           err);
                }
            }
        } else {
            err = bt_ancs_request_attrs(&ancs_c, &notification_latest,
                            bt_ancs_write_response_handler);
            if(err){
                PRINT("Failed requesting attributes for a notification (err: %d)\n",
                          err);
            }
        }
        k_s.timestamp_key1 = current_time;
    }

    if(keys & HAL_KEY_SW_2){
        uint32_t current_time = get_current_time();
        if(current_time - k_s.timestamp_key2 < 600){
            PRINT("Performing Negative Action.\n");
            err = bt_ancs_notification_action(
                &ancs_c, notification_latest.notif_uid,
                BT_ANCS_ACTION_ID_NEGATIVE,
                bt_ancs_write_response_handler);
            if (err) {
                PRINT("Failed performing action (err: %d)\n",
                       err);
            }
        } else {
            PRINT("Performing Positive Action.\n");
            err = bt_ancs_notification_action(
                &ancs_c, notification_latest.notif_uid,
                BT_ANCS_ACTION_ID_POSITIVE,
                bt_ancs_write_response_handler);
            if (err) {
                PRINT("Failed performing action (err: %d)\n",
                       err);
            }
        }

        k_s.timestamp_key2 = current_time;
    }
}

/*********************************************************************
 * @fn      Main_Circulation
 *
 * @brief   主循�?
 *
 * @return  none
 */
__HIGH_CODE
__attribute__((noinline))
void Main_Circulation()
{
    while(1)
    {
        TMOS_SystemProcess();
    }
}

/*********************************************************************
 * @fn      main
 *
 * @brief   主函�?
 *
 * @return  none
 */
int main(void)
{
#if(defined(DCDC_ENABLE)) && (DCDC_ENABLE == TRUE)
    PWR_DCDCCfg(ENABLE);
#endif
    SetSysClock(CLK_SOURCE_PLL_60MHz);
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
#endif
#ifdef DEBUG
    GPIOA_SetBits(bTXD1);
    GPIOA_ModeCfg(bTXD1, GPIO_ModeOut_PP_5mA);
    UART1_DefInit();
#endif
    PRINT("%s\n", VER_LIB);
    CH58X_BLEInit();
    HAL_Init();

    HalKeyConfig(button_changed);
    ancs_init();
    ancs_subscribe_cb_register(&subscribe);
    GAPRole_PeripheralInit();
    peripheral_ancs_client_init();
    Main_Circulation();
}

/******************************** endfile @ main ******************************/

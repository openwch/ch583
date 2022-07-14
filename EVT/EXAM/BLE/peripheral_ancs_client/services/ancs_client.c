/*
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*********************************************************************
 * INCLUDES
 */
#include <stdlib.h>
#include <sys/byteorder.h>
#include "CH58x_common.h"
#include "string.h"
#include "CH58xBLE_LIB.h"
#include "ancs.h"
#include "ancs_client.h"
#include "ancs_client_internal.h"
#include "ancs_attr_parser.h"
#include "ancs_app_attr_get.h"


/*********************************************************************
 * MACROS
 */

/**< Index of the Event ID field when parsing notifications. */
#define BT_ANCS_NOTIF_EVT_ID_INDEX 0
/**< Index of the Flags field when parsing notifications. */
#define BT_ANCS_NOTIF_FLAGS_INDEX 1
/**< Index of the Category ID field when parsing notifications. */
#define BT_ANCS_NOTIF_CATEGORY_ID_INDEX 2
/**< Index of the Category Count field when parsing notifications. */
#define BT_ANCS_NOTIF_CATEGORY_CNT_INDEX 3
/**< Index of the Notification UID field when parsing notifications. */
#define BT_ANCS_NOTIF_NOTIF_UID 4

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */



/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */
/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */


/*********************************************************************
 * LOCAL FUNCTIONS
 */



/**@brief Function for checking whether the data in an iOS notification is out of bounds.
 *
 * @param[in] notif  An iOS notification.
 *
 * @retval 0       If the notification is within bounds.
 * @retval -1      If the notification is out of bounds.
 */
static int bt_ancs_verify_notification_format(const struct bt_ancs_evt_notif *notif)
{
    if ((notif->evt_id >= BT_ANCS_EVT_ID_COUNT) ||
        (notif->category_id >= BT_ANCS_CATEGORY_ID_COUNT)) {
        return -1;
    }
    return 0;
}

/**@brief Function for receiving and validating notifications received from
 *        the Notification Provider.
 *
 * @param[in] ancs_c   Pointer to an ANCS instance to which the event belongs.
 * @param[in] data_src Pointer to the data that was received from the Notification Provider.
 * @param[in] len      Length of the data that was received from the Notification Provider.
 */
static void parse_notif(struct bt_ancs_client *ancs_c,
            const uint8_t *data_src, const uint16_t len)
{
    int err;
    struct bt_ancs_evt_notif notif;

    if (len != BT_ANCS_NOTIF_DATA_LENGTH) {
        bt_ancs_do_ns_notif_cb(ancs_c, -EINVAL, &notif);
        return;
    }

    notif.evt_id = (enum bt_ancs_evt_id_values)
        data_src[BT_ANCS_NOTIF_EVT_ID_INDEX];

    notif.evt_flags.silent =
        (data_src[BT_ANCS_NOTIF_FLAGS_INDEX] >>
         BT_ANCS_EVENT_FLAG_SILENT) &
        0x01;

    notif.evt_flags.important =
        (data_src[BT_ANCS_NOTIF_FLAGS_INDEX] >>
         BT_ANCS_EVENT_FLAG_IMPORTANT) &
        0x01;

    notif.evt_flags.pre_existing =
        (data_src[BT_ANCS_NOTIF_FLAGS_INDEX] >>
         BT_ANCS_EVENT_FLAG_PREEXISTING) &
        0x01;

    notif.evt_flags.positive_action =
        (data_src[BT_ANCS_NOTIF_FLAGS_INDEX] >>
         BT_ANCS_EVENT_FLAG_POSITIVE_ACTION) &
        0x01;

    notif.evt_flags.negative_action =
        (data_src[BT_ANCS_NOTIF_FLAGS_INDEX] >>
         BT_ANCS_EVENT_FLAG_NEGATIVE_ACTION) &
        0x01;

    notif.category_id = (enum bt_ancs_category_id_val)
        data_src[BT_ANCS_NOTIF_CATEGORY_ID_INDEX];

    notif.category_count =
        data_src[BT_ANCS_NOTIF_CATEGORY_CNT_INDEX];
    notif.notif_uid =
        sys_get_le32(&data_src[BT_ANCS_NOTIF_NOTIF_UID]);

    err = bt_ancs_verify_notification_format(&notif);

    bt_ancs_do_ns_notif_cb(ancs_c, err, &notif);
}

int bt_ancs_client_init(struct bt_ancs_client *ancs_c)
{
    if (!ancs_c) {
        return -EINVAL;
    }

    memset(ancs_c, 0, sizeof(struct bt_ancs_client));

    return 0;
}

void ancs_reinit(struct bt_ancs_client *ancs_c)
{
    ancs_c->conn = 0;
    ancs_c->handle_cp = 0;
    ancs_c->handle_ns = 0;
    ancs_c->handle_ns_ccc = 0;
    ancs_c->handle_ds = 0;
    ancs_c->handle_ds_ccc = 0;
    ancs_c->state = 0;
}

int bt_ancs_subscribe_notification_source(struct bt_ancs_client *ancs_c,
                      bt_ancs_ns_notif_cb func)
{
    int err;

    if (!ancs_c || !func) {
        return -EINVAL;
    }

    if (atomic_test_and_set_bit(&ancs_c->state, ANCS_NS_NOTIF_ENABLED)) {
        return -EALREADY;
    }

    ancs_c->ns_notif_cb = func;

    ancs_c->ns_notif_params.pValue = GATT_bm_alloc(ancs_c->conn, ATT_WRITE_REQ, 2, NULL, 0);

    if(!ancs_c->ns_notif_params.pValue){
      return -ENOBUFS;
    }

    ancs_c->ns_notif_params.len = 2;
    ancs_c->ns_notif_params.pValue[0] = LO_UINT16(GATT_CLIENT_CFG_NOTIFY);
    ancs_c->ns_notif_params.pValue[1] = HI_UINT16(GATT_CLIENT_CFG_NOTIFY);
    ancs_c->ns_notif_params.handle = ancs_c->handle_ns_ccc;
    ancs_c->ns_notif_params.cmd = 0;
    ancs_c->ns_notif_params.sig = 0;

    err = GATT_WriteCharValue(ancs_c->conn, &ancs_c->ns_notif_params, ancs_taskid);

    if (err) {
        atomic_clear_bit(&ancs_c->state, ANCS_NS_NOTIF_ENABLED);
        GATT_bm_free((gattMsg_t *)&ancs_c->ns_notif_params, ATT_WRITE_REQ);
        PRINT("Subscribe Notification Source failed (err %d)\n", err);
    } else {
        PRINT("Notification Source subscribed\n");
    }

    return err;
}

int bt_ancs_subscribe_data_source(struct bt_ancs_client *ancs_c,
                  bt_ancs_ds_notif_cb func)
{
    int err;

    if (!ancs_c || !func) {
        return -EINVAL;
    }

    if (atomic_test_and_set_bit(&ancs_c->state, ANCS_DS_NOTIF_ENABLED)) {
        return -EALREADY;
    }

    ancs_c->ds_notif_cb = func;

    ancs_c->ds_notif_params.pValue = GATT_bm_alloc(ancs_c->conn, ATT_WRITE_REQ, 2, NULL, 0);
    if(!ancs_c->ds_notif_params.pValue){
        return -ENOBUFS;
    }

    ancs_c->ds_notif_params.len = 2;
    ancs_c->ds_notif_params.pValue[0] = LO_UINT16(GATT_CLIENT_CFG_NOTIFY);
    ancs_c->ds_notif_params.pValue[1] = HI_UINT16(GATT_CLIENT_CFG_NOTIFY);
    ancs_c->ds_notif_params.handle = ancs_c->handle_ds_ccc;
    ancs_c->ds_notif_params.cmd = 0;
    ancs_c->ds_notif_params.sig = 0;

    err = GATT_WriteCharValue(ancs_c->conn, &ancs_c->ds_notif_params, ancs_taskid);

    if (err) {
        atomic_clear_bit(&ancs_c->state, ANCS_DS_NOTIF_ENABLED);
        GATT_bm_free((gattMsg_t *)&ancs_c->ds_notif_params, ATT_WRITE_REQ);
        PRINT("Subscribe Notification Source failed (err %d)\n", err);
    } else {
        PRINT("Data Source subscribed\n");
    }


    return err;
}

int bt_ancs_unsubscribe_notification_source(struct bt_ancs_client *ancs_c)
{
    int err;

    if (!ancs_c) {
        return -EINVAL;
    }

    if (!atomic_test_bit(&ancs_c->state, ANCS_NS_NOTIF_ENABLED)) {
        return -EFAULT;
    }

    ancs_c->ns_notif_params.pValue = GATT_bm_alloc(ancs_c->conn, ATT_WRITE_REQ, 2, NULL, 0);
    if(!ancs_c->ns_notif_params.pValue){
        return -ENOBUFS;
    }

    ancs_c->ns_notif_params.len = 2;
    ancs_c->ns_notif_params.pValue[0] = 0;
    ancs_c->ns_notif_params.pValue[1] = 0;
    ancs_c->ns_notif_params.handle = ancs_c->handle_ds;
    ancs_c->ns_notif_params.cmd = 0;
    ancs_c->ns_notif_params.sig = 0;

    err = GATT_WriteCharValue(ancs_c->conn, &ancs_c->ns_notif_params, ancs_taskid);

    if (err) {
        PRINT("Unsubscribe Notification Source failed (err %d)\n", err);
        GATT_bm_free((gattMsg_t *)&ancs_c->ns_notif_params, ATT_WRITE_REQ);
    } else {
        atomic_clear_bit(&ancs_c->state, ANCS_NS_NOTIF_ENABLED);
        PRINT("Notification Source unsubscribed\n");
    }

    return err;
}

int bt_ancs_unsubscribe_data_source(struct bt_ancs_client *ancs_c)
{
    int err;

    if (!ancs_c) {
        return -EINVAL;
    }

    if (!atomic_test_bit(&ancs_c->state, ANCS_DS_NOTIF_ENABLED)) {
        return -EFAULT;
    }

    ancs_c->ds_notif_params.pValue = GATT_bm_alloc(ancs_c->conn, ATT_WRITE_REQ, 2, NULL, 0);
    if(!ancs_c->ds_notif_params.pValue){
        return -ENOBUFS;
    }

    ancs_c->ds_notif_params.len = 2;
    ancs_c->ds_notif_params.pValue[0] = 0;
    ancs_c->ds_notif_params.pValue[1] = 0;
    ancs_c->ds_notif_params.handle = ancs_c->handle_ns;
    ancs_c->ds_notif_params.cmd = 0;
    ancs_c->ds_notif_params.sig = 0;

    err = GATT_WriteCharValue(ancs_c->conn, &ancs_c->ds_notif_params, ancs_taskid);

    if (err) {
        PRINT("Unsubscribe Data Source failed (err %d)\n", err);
        GATT_bm_free((gattMsg_t *)&ancs_c->ds_notif_params, ATT_WRITE_REQ);
    } else {
        atomic_clear_bit(&ancs_c->state, ANCS_DS_NOTIF_ENABLED);
        PRINT("Data Source unsubscribed\n");
    }

    return err;
}

static uint16_t encode_notif_action(uint8_t *encoded_data, uint32_t uid,
                    enum bt_ancs_action_id_values action_id)
{
    uint8_t index = 0;

    encoded_data[index++] = BT_ANCS_COMMAND_ID_PERFORM_NOTIF_ACTION;
    sys_put_le32(uid, &encoded_data[index]);
    index += sizeof(uint32_t);
    encoded_data[index++] = (uint8_t)action_id;

    return index;
}


int bt_ancs_cp_write(struct bt_ancs_client *ancs_c, uint16_t len,
        bt_ancs_write_cb func)
{
    int err;

    attWriteReq_t *write_params = &ancs_c->cp_write_params;;

    write_params->pValue = GATT_bm_alloc(ancs_c->conn, ATT_WRITE_REQ, len, NULL, 0);
    if(!write_params->pValue){
        return -ENOBUFS;
    }

    ancs_c->cp_write_cb = func;

    write_params->handle = ancs_c->handle_cp;
    write_params->len = len;
    tmos_memcpy(write_params->pValue, ancs_c->cp_data, len);

    write_params->sig = 0;
    write_params->cmd = 0;


    err = GATT_WriteCharValue(ancs_c->conn, write_params, ancs_taskid);

    if(err) {
        atomic_clear_bit(&ancs_c->state, ANCS_CP_WRITE_PENDING);
        GATT_bm_free((gattMsg_t *)write_params, ATT_WRITE_REQ);
        PRINT("write error: %#x\n", err);
    }

    return err;
}


int bt_ancs_notification_action(struct bt_ancs_client *ancs_c, uint32_t uuid,
                enum bt_ancs_action_id_values action_id,
                bt_ancs_write_cb func)
{
    if (atomic_test_and_set_bit(&ancs_c->state, ANCS_CP_WRITE_PENDING)) {
        return -EBUSY;
    }

    uint8_t *data = ancs_c->cp_data;
    uint16_t len = encode_notif_action(data, uuid, action_id);

    return bt_ancs_cp_write(ancs_c, len, func);
}

static int bt_ancs_get_notif_attrs(struct bt_ancs_client *ancs_c,
                   const uint32_t uid, bt_ancs_write_cb func)
{
    if (atomic_test_and_set_bit(&ancs_c->state, ANCS_CP_WRITE_PENDING)) {
        return -EBUSY;
    }

    uint32_t index = 0;
    uint8_t *data = ancs_c->cp_data;

    ancs_c->number_of_requested_attr = 0;

    /* Encode Command ID. */
    *(data + index++) = BT_ANCS_COMMAND_ID_GET_NOTIF_ATTRIBUTES;

    /* Encode Notification UID. */
    sys_put_le32(uid, data + index);
    index += sizeof(uint32_t);

    /* Encode Attribute ID. */
    for(uint32_t attr = 0; attr < BT_ANCS_NOTIF_ATTR_COUNT; attr++) {
        if (ancs_c->ancs_notif_attr_list[attr].get) {
            *(data + index++) = (uint8_t)attr;

            if ((attr == BT_ANCS_NOTIF_ATTR_ID_TITLE) ||
                (attr == BT_ANCS_NOTIF_ATTR_ID_SUBTITLE) ||
                (attr == BT_ANCS_NOTIF_ATTR_ID_MESSAGE)) {
                /* Encode Length field. Only applicable for
                 * Title, Subtitle, and Message.
                 */
                sys_put_le16(ancs_c->ancs_notif_attr_list[attr]
                             .attr_len,
                         data + index);
                index += sizeof(uint16_t);
            }
            ancs_c->number_of_requested_attr++;
        }
    }

    ancs_c->parse_info.expected_number_of_attrs =
        ancs_c->number_of_requested_attr;

    return bt_ancs_cp_write(ancs_c, index, func);
}

int bt_ancs_request_attrs(struct bt_ancs_client *ancs_c,
              const struct bt_ancs_evt_notif *notif,
              bt_ancs_write_cb func)
{
    int err;

    err = bt_ancs_verify_notification_format(notif);
    if (err) {
        return err;
    }

    ancs_c->parse_info.parse_state = BT_ANCS_PARSE_STATE_COMMAND_ID;

    return bt_ancs_get_notif_attrs(ancs_c, notif->notif_uid, func);
}


int bt_ancs_register_attr(struct bt_ancs_client *ancs_c,
              const enum bt_ancs_notif_attr_id_val id,
              uint8_t *data, const uint16_t len)
{
    if (!ancs_c || !data) {
        return -EINVAL;
    }

    if (!len || len > BT_ANCS_ATTR_DATA_MAX) {
        return -EINVAL;
    }

    if ((size_t)id >= BT_ANCS_NOTIF_ATTR_COUNT) {
        return -EINVAL;
    }

    ancs_c->ancs_notif_attr_list[id].get = true;
    ancs_c->ancs_notif_attr_list[id].attr_len = len;
    ancs_c->ancs_notif_attr_list[id].attr_data = data;

    return 0;
}

int bt_ancs_register_app_attr(struct bt_ancs_client *ancs_c,
                  const enum bt_ancs_app_attr_id_val id,
                  uint8_t *data, const uint16_t len)
{
    if (!ancs_c || !data) {
        return -EINVAL;
    }

    if (!len || len > BT_ANCS_ATTR_DATA_MAX) {
        return -EINVAL;
    }

    if ((size_t)id >= BT_ANCS_APP_ATTR_COUNT) {
        return -EINVAL;
    }

    ancs_c->ancs_app_attr_list[id].get = true;
    ancs_c->ancs_app_attr_list[id].attr_len = len;
    ancs_c->ancs_app_attr_list[id].attr_data = data;

    return 0;
}

int bt_ancs_request_app_attr(struct bt_ancs_client *ancs_c,
                 const uint8_t *app_id, uint32_t len,
                 bt_ancs_write_cb func)
{
    return bt_ancs_app_attr_request(ancs_c, app_id, len, func);
}
/*********************************************************************
 * @fn      Ancs_handleNotification
 *
 * @brief   Handle notifications. 
 *
 * @param   pMsg - GATT message.
 *
 * @return  none
 */
void Ancs_handleNotification(gattMsgEvent_t *pMsg)
{
  static uint8_t importantAlertCnt = 0;
    
  uint8_t i;
  // Look up the handle in the handle cache
  for (i = 0; i < HDL_CACHE_LEN; i++)
  {
    if (pMsg->msg.handleValueNoti.handle == Ancs_handleCache[i])
    {
      break;
    }
  }

  // Perform processing for this handle 
  switch (i)
  {
    case HDL_ANCS_NTF_NOTIF_START:
      {
        parse_notif(&ancs_c, pMsg->msg.handleValueNoti.pValue,
                pMsg->msg.handleValueNoti.len);
      }
      break;

    case HDL_ANCS_DATA_SRC_START:
    {
      bt_ancs_parse_get_attrs_response(&ancs_c,
              pMsg->msg.handleValueNoti.pValue, pMsg->msg.handleValueNoti.len);
    }
    break;
 
      default:
      break;
  }
  
}

/*********************************************************************
*********************************************************************/

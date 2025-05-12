/********************************** (C) COPYRIGHT *******************************
 * File Name          : simpleGATTprofile.h
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2022/01/19
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef _BLE_IOCHUB_SERVICE_H
#define _BLE_IOCHUB_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

//#include "att.h"
#include "stdint.h"

#define BLE_IOCHUB_RX_BUFF_SIZE    1

#define BLE_IOCHUB_SERVICE_UUID 0xFFF3
#define BLE_IOCHUB_TXCHAR_UUID 0xFFF4
#define BLE_IOCHUB_RXCHAR_UUID 0xFFF5



typedef enum
{
    BLE_IOCHUB_EVT_TX_NOTI_DISABLED = 1,
    BLE_IOCHUB_EVT_TX_NOTI_ENABLED,
    BLE_IOCHUB_EVT_BLE_DATA_RECIEVED,
} ble_iochub_evt_type_t;

typedef struct
{
    uint8_t const *p_data; /**< A pointer to the buffer with received data. */
    uint16_t       length; /**< Length of received data. */
} ble_iochub_evt_rx_data_t;

typedef struct
{
    ble_iochub_evt_type_t    type;
    ble_iochub_evt_rx_data_t data;
} ble_iochub_evt_t;

typedef void (*ble_iochub_ProfileChangeCB_t)(uint16_t connection_handle, ble_iochub_evt_t *p_evt);

/*********************************************************************
 * API FUNCTIONS
 */

/*
 * ble_iochub_AddService- Initializes the raw pass GATT Profile service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 */

extern bStatus_t ble_iochub_add_service(ble_iochub_ProfileChangeCB_t cb);

extern uint8_t ble_iochub_notify_is_ready(uint16_t connHandle);

extern bStatus_t ble_iochub_notify(uint16_t connHandle, attHandleValueNoti_t *pNoti, uint8_t taskId);
/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* _BLE_IOCHUB_SERVICE_H */

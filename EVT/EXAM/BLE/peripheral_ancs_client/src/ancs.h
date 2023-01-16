/*
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 *
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 */

#ifndef ANCS_H
#define ANCS_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include "CH58xBLE_LIB.h"
#include "ancs_client.h"
#include <stdint.h>
/*********************************************************************
 * CONSTANTS
 */

// ANCS discovery states
enum
{
  DISC_IDLE = 0x00,                  // Idle state
  
  DISC_ANCS_START = 0x10,            // ANCS service
  DISC_ANCS_SVC,                     // Discover service
  DISC_ANCS_CHAR,                    // Discover all characteristics
  DISC_ANCS_CCCD,                    // Discover ANCS CCCD
  
  DISC_FAILED = 0xFF                 // Discovery failed
};

// ANCS handle cache indexes
enum
{
  HDL_ANCS_NTF_NOTIF_START,             // ANCS notification characteristic start handle
  HDL_ANCS_NTF_NOTIF_END,               // ANCS notification characteristic end handle
  HDL_ANCS_NTF_CCCD,                    // ANCS notification CCCD
  
  HDL_ANCS_CTRL_PT_START,
  HDL_ANCS_CTRL_PT_END,
  
  HDL_ANCS_DATA_SRC_START,             // ANCS data source characteristic start handle
  HDL_ANCS_DATA_SRC_END,               // ANCS data source characteristic end handle
  HDL_ANCS_DATA_SRC_CCCD,              // ANCS data source CCCD
  
  
  HDL_CACHE_LEN
};

  
/*********************************************************************
 * TYPEDEFS
 */

struct bt_ancs_subscribe_cb {
    void(*notification_source)(struct bt_ancs_client *ancs_c,
            int err, const struct bt_ancs_evt_notif *notif);
    void(*data_source)(struct bt_ancs_client *ancs_c,
            const struct bt_ancs_attr_response *response);
    void(*write_response)(struct bt_ancs_client *ancs_c,
            uint8_t err);
};

typedef struct
{
    uint16_t connHandle; // Connection handle of current connection
    uint16_t connInterval;
    uint16_t connSlaveLatency;
    uint16_t connTimeout;
} peripheralConnItem_t;

/*********************************************************************
 * MACROS
 */

#define START_DEVICE_EVT              0x0001
#define START_SUBSCRIPE_NOTISRC_EVT    0x0002
#define START_SUBSCRIPE_NOTIDATA_EVT   0x0004
#define START_DISCOVER_EVT              0x0008
#define START_PARAM_UPDATE_EVT        0x0010
#define START_PHY_UPDATE_EVT          0x0020

// Handle and 16-bit Bluetooth UUID pair indexes
#define ATT_BT_PAIR_HANDLE_IDX( i )        ( (i) * (2 + ATT_BT_UUID_SIZE) )
#define ATT_BT_PAIR_UUID_IDX( i )          ( ATT_BT_PAIR_HANDLE_IDX( (i) ) + 2 )

#define ATT_BT_PAIR_HANDLE( info, i )      ( BUILD_UINT16( (info)[ATT_BT_PAIR_HANDLE_IDX((i))], \
                                                           (info)[ATT_BT_PAIR_HANDLE_IDX((i))+1] ) )
#define ATT_BT_PAIR_UUID( info, i )        ( BUILD_UINT16( (info)[ATT_BT_PAIR_UUID_IDX((i))], \
                                                           (info)[ATT_BT_PAIR_UUID_IDX((i))+1] ) )


/*********************************************************************
 * GLOBAL
 */
enum
{
  CCCD_CONFIG_NOTIF = 0x00,
  CCCD_CONFIG_DATA,
  CCCD_CONFIG_DONE
};

extern struct bt_ancs_client ancs_c;

extern uint8_t ancs_taskid;

// Handle cache
extern uint16_t Ancs_handleCache[HDL_CACHE_LEN];

/*********************************************************************
 * FUNCTIONS
 */

/* 
 * ANCS service discovery functions.
 */
void peripheral_ancs_client_init(void);

void ancs_subscribe_cb_register(struct bt_ancs_subscribe_cb *cb);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ANCS_H */

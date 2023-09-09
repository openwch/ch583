/*
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 *
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 */

/*********************************************************************
 * INCLUDES
 */
#include "CONFIG.h"
#include "devinfoservice.h"
#include "ancs_client.h"
#include "ancs_client_internal.h"
#include "ancs.h"

/*********************************************************************
 * MACROS
 */

// Length of bd addr as a string
#define B_ADDR_STR_LEN                      15

/*********************************************************************
 * CONSTANTS
 */
// What is the advertising interval when device is discoverable (units of 625us, 80=50ms)
#define DEFAULT_ADVERTISING_INTERVAL         80

// Minimum connection interval (units of 1.25ms, 6=7.5ms)
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL    6

// Maximum connection interval (units of 1.25ms, 100=125ms)
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL    100

// Slave latency to use parameter update
#define DEFAULT_DESIRED_SLAVE_LATENCY        0


// Minimum connection interval (units of 1.25ms)
#define DEFAULT_UPDATE_MIN_CONN_INTERVAL    24

// Maximum connection interval (units of 1.25ms)
#define DEFAULT_UPDATE_MAX_CONN_INTERVAL    40

// Slave latency to use parameter update
#define DEFAULT_UPDATE_SLAVE_LATENCY        0

// Supervision timeout value (units of 10ms)
#define DEFAULT_UPDATE_CONN_TIMEOUT         42

// Default parameter update delay in 0.625ms
#define DEFAULT_PARAM_UPDATE_DELAY          3200

// Default phy update delay in 0.625ms
#define DEFAULT_PHY_UPDATE_DELAY            2400

// ANCS Service: 7905F431-B5CE-4E99-A40F-4B1E122D00D0
#define ANCS_SVC_UUID 0xD0, 0x00, 0x2D, 0x12, 0x1E, 0x4B, 0x0F, 0xA4, 0x99, 0x4E, 0xCE, 0xB5, 0x31, 0xF4, 0x05, 0x79

// Notification Source: UUID 9FBF120D-6301-42D9-8C58-25E699A21DBD (notifiable)
#define ANCS_NOTIF_SRC_CHAR_UUID        0x1DBD // Last 2 bytes of the 128bit-16bytes UUID

// Control point: UUID 69D1D8F3-45E1-49A8-9821-9BBDFDAAD9D9 (writeable with response)
#define ANCS_CTRL_PT_CHAR_UUID          0xD9D9

// Data Source: UUID 22EAC6E9-24D6-4BB5-BE44-B36ACE7C7BFB (notifiable)
#define ANCS_DATA_SRC_CHAR_UUID         0x7BFB

#define CHAR_DESC_HDL_UUID128_LEN        21  // 5 + 16bytes = 21



/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
uint16_t Ancs_handleCache[HDL_CACHE_LEN];

// Task ID for internal task/event processing
uint8_t ancs_taskid;
/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
struct bt_ancs_client ancs_c;

// GAP - SCAN RSP data (max size = 31 bytes)
static uint8_t scanRspData[] = {
    // complete name
    0x05, // length of this data
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'A',
    'N',
    'C',
    'S',

    // connection interval range
    0x05, // length of this data
    GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE,
    LO_UINT16(DEFAULT_DESIRED_MIN_CONN_INTERVAL), // 100ms
    HI_UINT16(DEFAULT_DESIRED_MIN_CONN_INTERVAL),
    LO_UINT16(DEFAULT_DESIRED_MAX_CONN_INTERVAL), // 1s
    HI_UINT16(DEFAULT_DESIRED_MAX_CONN_INTERVAL),

    // Tx power level
    0x02, // length of this data
    GAP_ADTYPE_POWER_LEVEL,
    0 // 0dBm
};

// GAP - Advertisement data (max size = 31 bytes, though this is
// best kept short to conserve power while advertising)
static uint8_t advertData[] = {
    // Flags; this sets the device to use limited discoverable
    // mode (advertises for 30 seconds at a time) instead of general
    // discoverable mode (advertises indefinitely)
    0x02, // length of this data
    GAP_ADTYPE_FLAGS,
    GAP_ADTYPE_FLAGS_LIMITED | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

    // service UUID, to notify central devices what services are included
    // in this peripheral
    0x11,                  // length of this data
    GAP_ADTYPE_SERVICES_LIST_128BIT, // some of the UUID's, but not all
    ANCS_SVC_UUID,

    0x05,
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'A', 'N', 'C', 'S',
};



// Connection handle of current connection
static peripheralConnItem_t ancs_conn_list = {
        .connHandle = GAP_CONNHANDLE_INIT
};


static uint8_t discoveryState = DISC_ANCS_START;

// GAP GATT Attributes
static uint8_t attDeviceName[GAP_DEVICE_NAME_LEN] = "ANCS";


/*********************************************************************
 * LOCAL FUNCTIONS
 */
uint16_t ANCS_ProcessEvent(uint8_t task_id, uint16_t events);
static void AncsProcessTMOSMsg(tmos_event_hdr_t *pMsg);
static void AncsApp_discoverService(gattMsgEvent_t *pMsg);
static void AncsStateNotificationCB(gapRole_States_t newState, gapRoleEvent_t *pEvent);
static void AncsParamUpdateCB(uint16_t connHandle, uint16_t connInterval,
                                uint16_t connSlaveLatency, uint16_t connTimeout);
static void AncsPasscodeCB(uint8_t *deviceAddr, uint16_t connectionHandle,
                              uint8_t uiInputs, uint8_t uiOutputs);
static void AncsPairStateCB(uint16_t connHandle, uint8_t state, uint8_t status);

/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static gapRolesCBs_t AncsPeripheralCBs = {
        AncsStateNotificationCB, // Profile State Change Callbacks
        NULL,              // When a valid RSSI is read from controller (not used by application)
        AncsParamUpdateCB
};

// Bond Manager Callbacks
static gapBondCBs_t AncsBondCB = {
        AncsPasscodeCB,
        AncsPairStateCB
};

static struct bt_ancs_subscribe_cb *bt_ancs_subscribe;

void ancs_subscribe_cb_register(struct bt_ancs_subscribe_cb *cb)
{
    bt_ancs_subscribe = cb;
}

/*********************************************************************
 * @fn      peripheral_ancs_client_init
 *
 * @brief   Initialization function for the Central App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notification).
 *
 * @param   task_id - the ID assigned by TMOS.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void peripheral_ancs_client_init(void)
{
    ancs_taskid = TMOS_ProcessEventRegister(ANCS_ProcessEvent);

    // Setup the GAP Peripheral Role Profile
    {
        uint8_t  initial_advertising_enable = TRUE;
        uint16_t desired_min_interval = DEFAULT_DESIRED_MIN_CONN_INTERVAL;
        uint16_t desired_max_interval = DEFAULT_DESIRED_MAX_CONN_INTERVAL;

        // Set the GAP Role Parameters
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &initial_advertising_enable);
        GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, sizeof(scanRspData), scanRspData);
        GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(advertData), advertData);
        GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL, sizeof(uint16_t), &desired_min_interval);
        GAPRole_SetParameter(GAPROLE_MAX_CONN_INTERVAL, sizeof(uint16_t), &desired_max_interval);
    }

    // Set the GAP Characteristics
    GGS_SetParameter(GGS_DEVICE_NAME_ATT, sizeof(attDeviceName), attDeviceName);

    {
        uint16_t advInt = DEFAULT_ADVERTISING_INTERVAL;

        // Set advertising interval
        GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, advInt);
        GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, advInt);

        // Enable scan req notify
        GAP_SetParamValue(TGAP_ADV_SCAN_REQ_NOTIFY, ENABLE);
    }

    // Setup the GAP Bond Manager
    {
        uint32_t passkey = 0; // passkey "000000"
        uint8_t  pairMode = GAPBOND_PAIRING_MODE_WAIT_FOR_REQ;
        uint8_t  mitm = TRUE;
        uint8_t  bonding = TRUE;
        uint8_t  ioCap = GAPBOND_IO_CAP_DISPLAY_ONLY;
        GAPBondMgr_SetParameter(GAPBOND_PERI_DEFAULT_PASSCODE, sizeof(uint32_t), &passkey);
        GAPBondMgr_SetParameter(GAPBOND_PERI_PAIRING_MODE, sizeof(uint8_t), &pairMode);
        GAPBondMgr_SetParameter(GAPBOND_PERI_MITM_PROTECTION, sizeof(uint8_t), &mitm);
        GAPBondMgr_SetParameter(GAPBOND_PERI_IO_CAPABILITIES, sizeof(uint8_t), &ioCap);
        GAPBondMgr_SetParameter(GAPBOND_PERI_BONDING_ENABLED, sizeof(uint8_t), &bonding);
    }

    // Initialize GATT Client
    GATT_InitClient();
    // Register to receive incoming ATT Indications/Notifications
    GATT_RegisterForInd(ancs_taskid);

    // Initialize GATT attributes
    GGS_AddService(GATT_ALL_SERVICES);           // GAP
    GATTServApp_AddService(GATT_ALL_SERVICES);   // GATT attributes
    DevInfo_AddService();

    // Setup a delayed profile startup
    tmos_set_event(ancs_taskid, START_DEVICE_EVT);
}

/*********************************************************************
 * @fn      Central_ProcessEvent
 *
 * @brief   Central Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
uint16_t ANCS_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if(events & SYS_EVENT_MSG)
    {
        uint8_t *pMsg;

        if((pMsg = tmos_msg_receive(ancs_taskid)) != NULL)
        {
            AncsProcessTMOSMsg((tmos_event_hdr_t *)pMsg);
            // Release the TMOS message
            tmos_msg_deallocate(pMsg);
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }

    if(events & START_DEVICE_EVT)
    {
        // Start the Device
        GAPRole_PeripheralStartDevice(ancs_taskid, &AncsBondCB, &AncsPeripheralCBs);
        return (events ^ START_DEVICE_EVT);
    }

    if(events & START_SUBSCRIPE_NOTIDATA_EVT)
    {
        int err ;
        if((err = bt_ancs_subscribe_data_source(&ancs_c,
                bt_ancs_subscribe->data_source)) != SUCCESS){
            if(err != -EALREADY)
                tmos_start_task(ancs_taskid, START_SUBSCRIPE_NOTIDATA_EVT, MS1_TO_SYSTEM_TIME(100));
        }
        return (events ^ START_SUBSCRIPE_NOTIDATA_EVT);
    }

    if(events & START_SUBSCRIPE_NOTISRC_EVT)
    {
        int err ;
        if((err = bt_ancs_subscribe_notification_source(&ancs_c,
                bt_ancs_subscribe->notification_source)) != SUCCESS){
            if(err != -EALREADY)
                tmos_start_task(ancs_taskid, START_SUBSCRIPE_NOTISRC_EVT, MS1_TO_SYSTEM_TIME(100));
        }
        return (events ^ START_SUBSCRIPE_NOTISRC_EVT);
    }


    if(events & START_DISCOVER_EVT)
    {
        AncsApp_discoverService(NULL);
        return (events ^ START_DISCOVER_EVT);
    }



    if(events & START_PARAM_UPDATE_EVT)
    {
        // start connect parameter update
        GAPRole_PeripheralConnParamUpdateReq(ancs_conn_list.connHandle,
                           DEFAULT_UPDATE_MIN_CONN_INTERVAL,
                           DEFAULT_UPDATE_MAX_CONN_INTERVAL,
                           DEFAULT_UPDATE_SLAVE_LATENCY,
                           DEFAULT_UPDATE_CONN_TIMEOUT,
                           ancs_taskid);
        return (events ^ START_PARAM_UPDATE_EVT);
    }

    if(events & START_PHY_UPDATE_EVT)
    {
        // start phy update
        PRINT("PHY Update %x...\n", GAPRole_UpdatePHY(ancs_conn_list.connHandle, 0, 
                    GAP_PHY_BIT_LE_2M, GAP_PHY_BIT_LE_2M, GAP_PHY_OPTIONS_NOPRE));

        return (events ^ START_PHY_UPDATE_EVT);
    }

    // Discard unknown events
    return 0;
}


/*********************************************************************
 * @fn      centralProcessGATTMsg
 *
 * @brief   Process GATT messages
 *
 * @return  none
 */
static void AncsProcessGATTMsg(gattMsgEvent_t *pMsg)
{
    if(!atomic_test_bit(&ancs_c.state, ANCS_BT_CONNECTED)){
        // In case a GATT message came after a connection has dropped,
        // ignore the message
        GATT_bm_free(&pMsg->msg, pMsg->method);
        return;
    }

    if(!atomic_test_bit(&ancs_c.state, ANCS_BT_DISCOVERY_COMPLETE)){
        AncsApp_discoverService(pMsg);
    } else if (pMsg->method == ATT_HANDLE_VALUE_NOTI ||
         pMsg->method == ATT_HANDLE_VALUE_IND) {
      // Now in ancs_client.c
      Ancs_handleNotification(pMsg);
    } else if ((pMsg->method == ATT_READ_RSP) ||
            ((pMsg->method == ATT_ERROR_RSP) &&
             (pMsg->msg.errorRsp.reqOpcode == ATT_READ_REQ))){

    } else if((pMsg->method == ATT_WRITE_RSP) ||
            ((pMsg->method == ATT_ERROR_RSP) &&
             (pMsg->msg.errorRsp.reqOpcode == ATT_WRITE_REQ))){
        bt_ancs_write_cb write_cb;

        write_cb = ancs_c.cp_write_cb;
        atomic_clear_bit(&ancs_c.state, ANCS_CP_WRITE_PENDING);
        if (write_cb) {
            write_cb(&ancs_c, pMsg->msg.errorRsp.errCode);
        }
    } else if(pMsg->method == ATT_ERROR_RSP){
        PRINT("response %#x error: %#x\n", pMsg->msg.errorRsp.reqOpcode,
                pMsg->msg.errorRsp.errCode);
    }

    GATT_bm_free(&pMsg->msg, pMsg->method);
}

/*********************************************************************
 * @fn      central_ProcessTMOSMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void AncsProcessTMOSMsg(tmos_event_hdr_t *pMsg)
{
    switch(pMsg->event)
    {
        case GATT_MSG_EVENT:
            AncsProcessGATTMsg((gattMsgEvent_t *)pMsg);
            break;
    }
}

/*********************************************************************
 * @fn      Peripheral_LinkEstablished
 *
 * @brief   Process link established.
 *
 * @param   pEvent - event to process
 *
 * @return  none
 */
static void Peripheral_LinkEstablished(gapRoleEvent_t *pEvent)
{
    gapEstLinkReqEvent_t *event = (gapEstLinkReqEvent_t *)pEvent;

    // See if already connected
    if(ancs_conn_list.connHandle != GAP_CONNHANDLE_INIT)
    {
        GAPRole_TerminateLink(pEvent->linkCmpl.connectionHandle);
        PRINT("Connection max...\n");
    }
    else
    {
        ancs_conn_list.connHandle = event->connectionHandle;
        ancs_conn_list.connInterval = event->connInterval;
        ancs_conn_list.connSlaveLatency = event->connLatency;
        ancs_conn_list.connTimeout = event->connTimeout;

        ancs_c.conn = ancs_conn_list.connHandle;
        atomic_set_bit(&ancs_c.state, ANCS_BT_CONNECTED);

        // Set timer for param update event
        tmos_start_task(ancs_taskid, START_PARAM_UPDATE_EVT, DEFAULT_PARAM_UPDATE_DELAY);


        discoveryState = DISC_ANCS_START;
        PRINT("Conn: %d - params: %d %d %d\n", event->connectionHandle,
                event->connInterval, event->connLatency, event->connTimeout);
    }
}

/*********************************************************************
 * @fn      Peripheral_LinkTerminated
 *
 * @brief   Process link terminated.
 *
 * @param   pEvent - event to process
 *
 * @return  none
 */
static void Peripheral_LinkTerminated(gapRoleEvent_t *pEvent)
{
    gapTerminateLinkEvent_t *event = (gapTerminateLinkEvent_t *)pEvent;

    if(event->connectionHandle == ancs_conn_list.connHandle)
    {
        ancs_conn_list.connHandle = GAP_CONNHANDLE_INIT;
        ancs_conn_list.connInterval = 0;
        ancs_conn_list.connSlaveLatency = 0;
        ancs_conn_list.connTimeout = 0;

        // Restart advertising
        {
            uint8_t advertising_enable = TRUE;
            GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &advertising_enable);
        }

        ancs_reinit(&ancs_c);
    }
    else
    {
        PRINT("ERR..\n");
    }
}

static void AncsParamUpdateCB(uint16_t connHandle, uint16_t connInterval,
        uint16_t connSlaveLatency, uint16_t connTimeout)
{
    if(connHandle == ancs_conn_list.connHandle)
    {
        ancs_conn_list.connInterval = connInterval;
        ancs_conn_list.connSlaveLatency = connSlaveLatency;
        ancs_conn_list.connTimeout = connTimeout;

        PRINT("update: %d - params: %d %d %d\n", connHandle,
                        connInterval, connSlaveLatency, connTimeout);
    }
    else
    {
        PRINT("ERR..\n");
    }
}

static void AncsStateNotificationCB(gapRole_States_t newState, gapRoleEvent_t *pEvent)
{
    switch(newState & GAPROLE_STATE_ADV_MASK)
    {
        case GAPROLE_STARTED:
            PRINT("Initialized..\n");
            break;

        case GAPROLE_ADVERTISING:
            if(pEvent->gap.opcode == GAP_LINK_TERMINATED_EVENT)
            {
                Peripheral_LinkTerminated(pEvent);
                PRINT("Disconnected.. Reason:%x\n", pEvent->linkTerminate.reason);
                PRINT("Advertising..\n");
            }
            else if(pEvent->gap.opcode == GAP_MAKE_DISCOVERABLE_DONE_EVENT)
            {
                PRINT("Advertising..\n");
            }
            break;

        case GAPROLE_CONNECTED:
            if(pEvent->gap.opcode == GAP_LINK_ESTABLISHED_EVENT)
            {
                Peripheral_LinkEstablished(pEvent);
                PRINT("Connected..\n");
            }
            break;

        case GAPROLE_CONNECTED_ADV:
            if(pEvent->gap.opcode == GAP_MAKE_DISCOVERABLE_DONE_EVENT)
            {
                PRINT("Connected Advertising..\n");
            }
            break;

        case GAPROLE_WAITING:
            if(pEvent->gap.opcode == GAP_END_DISCOVERABLE_DONE_EVENT)
            {
                PRINT("Waiting for advertising..\n");
            }
            else if(pEvent->gap.opcode == GAP_LINK_TERMINATED_EVENT)
            {
                Peripheral_LinkTerminated(pEvent);
                PRINT("Disconnected.. Reason:%x\n", pEvent->linkTerminate.reason);
            }
            else if(pEvent->gap.opcode == GAP_LINK_ESTABLISHED_EVENT)
            {
                if(pEvent->gap.hdr.status != SUCCESS)
                {
                    PRINT("Waiting for advertising..\n");
                }
                else
                {
                    PRINT("Error..\n");
                }
            }
            else
            {
                PRINT("Error..%x\n", pEvent->gap.opcode);
            }
            break;

        case GAPROLE_ERROR:
            PRINT("Error..\n");
            break;

        default:
            break;
    }
}

/*********************************************************************
 * @fn      pairStateCB
 *
 * @brief   Pairing state callback.
 *
 * @return  none
 */
static void AncsPairStateCB(uint16_t connHandle, uint8_t state, uint8_t status)
{
    if(state == GAPBOND_PAIRING_STATE_STARTED)
    {
        PRINT("Pairing started:%d\n", status);
    }
    else if(state == GAPBOND_PAIRING_STATE_COMPLETE)
    {
        if(status == SUCCESS)
        {
            PRINT("Pairing success\n");
        }
        else
        {
            PRINT("Pairing fail\n");
        }
    }
    else if(state == GAPBOND_PAIRING_STATE_BONDED)
    {
        if(status == SUCCESS)
        {
            PRINT("Bonding success\n");
            tmos_set_event(ancs_taskid, START_DISCOVER_EVT);
        }
    }
    else if(state == GAPBOND_PAIRING_STATE_BOND_SAVED)
    {
        if(status == SUCCESS)
        {
            PRINT("Bond save success\n");

            tmos_set_event(ancs_taskid, START_DISCOVER_EVT);
        }
        else
        {
            PRINT("Bond save failed: %d\n", status);
        }
    }
}

/*********************************************************************
 * @fn      centralPasscodeCB
 *
 * @brief   Passcode callback.
 *
 * @return  none
 */
static void AncsPasscodeCB(uint8_t *deviceAddr, uint16_t connectionHandle,
                              uint8_t uiInputs, uint8_t uiOutputs)
{
    uint32_t passcode;

    // Create random passcode
    passcode = tmos_rand();
    passcode %= 1000000;
    // Display passcode to user
    if(uiOutputs != 0)
    {
        PRINT("Passcode:%06d\n", (int)passcode);
    }
    // Send passcode response
    GAPBondMgr_PasscodeRsp(connectionHandle, SUCCESS, passcode);
}



/*********************************************************************
 * @fn      centralGATTDiscoveryEvent
 *
 * @brief   Process GATT discovery event
 *
 * @return  none
 */
static void AncsApp_discoverService(gattMsgEvent_t *pMsg)
{

    static uint16_t Ancs_svcStartHdl;
    static uint16_t Ancs_svcEndHdl;
    static uint8_t Ancs_endHdlIdx;
    static uint8_t isNotifCCCD = FALSE;

    switch (discoveryState)
    {
      case DISC_ANCS_START:
        {
          uint8_t uuid[ATT_UUID_SIZE] = {ANCS_SVC_UUID};

          // Initialize service discovery variables
          Ancs_svcStartHdl = Ancs_svcEndHdl = 0;
          Ancs_endHdlIdx = 0;

          // Discover ANCS service by UUID
          GATT_DiscPrimaryServiceByUUID(ancs_c.conn, uuid,
                                        ATT_UUID_SIZE, ancs_taskid);

          discoveryState = DISC_ANCS_SVC;
        }
        break;

      case DISC_ANCS_SVC:
        // Service found, store handles
        if (pMsg->method == ATT_FIND_BY_TYPE_VALUE_RSP &&
            pMsg->msg.findByTypeValueRsp.numInfo > 0)
        {
          Ancs_svcStartHdl =
            ATT_ATTR_HANDLE(pMsg->msg.findByTypeValueRsp.pHandlesInfo, 0);
          Ancs_svcEndHdl =
            ATT_GRP_END_HANDLE(pMsg->msg.findByTypeValueRsp.pHandlesInfo, 0);
        }

        // If procedure complete
        if ((pMsg->method == ATT_FIND_BY_TYPE_VALUE_RSP  &&
             pMsg->hdr.status == bleProcedureComplete) ||
            (pMsg->method == ATT_ERROR_RSP))
        {
          // If service found
          if (Ancs_svcStartHdl != 0)
          {
            // Discover all characteristics
            GATT_DiscAllChars(pMsg->connHandle, Ancs_svcStartHdl,
                              Ancs_svcEndHdl, ancs_taskid);

            discoveryState = DISC_ANCS_CHAR;
          }
          else
          {
            // Service not found
            discoveryState = DISC_FAILED;
            PRINT("ANCS SVC not present\n");
          }
        }
        break;

      case DISC_ANCS_CHAR:
        {
          // Characteristics found, chache them
          uint8_t   *pHandleValuePairList;
          uint16_t  handle;
          uint16_t  uuid;
          if (pMsg->method == ATT_READ_BY_TYPE_RSP &&
              pMsg->msg.readByTypeRsp.numPairs > 0 &&
              pMsg->msg.readByTypeRsp.len == CHAR_DESC_HDL_UUID128_LEN)
          {
            pHandleValuePairList = pMsg->msg.readByTypeRsp.pDataList;
            uint8_t i;
            // For each handle value pair in the list of chars in ANCS service
            for (i = pMsg->msg.readByTypeRsp.numPairs; i > 0; i--)
            {
              // Parse characteristic declaration
              handle = BUILD_UINT16(pHandleValuePairList[3],
                                    pHandleValuePairList[4]);
              uuid = BUILD_UINT16(pHandleValuePairList[5],
                                  pHandleValuePairList[6]);

              // If looking for end handle
              if (Ancs_endHdlIdx != 0)
              {
                // End handle is characteristic declaration handle - 1
                  Ancs_handleCache[Ancs_endHdlIdx] =
                          BUILD_UINT16(pHandleValuePairList[0],
                                      pHandleValuePairList[1]) - 1;

                  Ancs_endHdlIdx = 0;
              }

              // If UUID is of interest, cache handle
              switch (uuid)
              {
                case ANCS_NOTIF_SRC_CHAR_UUID:
                    ancs_c.handle_ns = handle;
                    Ancs_handleCache[HDL_ANCS_NTF_NOTIF_START] = handle;
                    Ancs_endHdlIdx = HDL_ANCS_NTF_NOTIF_END;
                    break;

                case ANCS_CTRL_PT_CHAR_UUID:
                    ancs_c.handle_cp = handle;
                    Ancs_handleCache[HDL_ANCS_CTRL_PT_START] = handle;
                    Ancs_endHdlIdx = HDL_ANCS_CTRL_PT_END;
                    break;

                case ANCS_DATA_SRC_CHAR_UUID:
                    ancs_c.handle_ds = handle;
                    Ancs_handleCache[HDL_ANCS_DATA_SRC_START] = handle;
                    Ancs_endHdlIdx = HDL_ANCS_DATA_SRC_END;
                    break;

                default:
                    break;
              }

              pHandleValuePairList += CHAR_DESC_HDL_UUID128_LEN;
            }
          }

          // If procedure complete
          if ((pMsg->method == ATT_READ_BY_TYPE_RSP  &&
               pMsg->hdr.status == bleProcedureComplete) ||
              (pMsg->method == ATT_ERROR_RSP))
          {
            // Special case of end handle at end of service
            if (Ancs_endHdlIdx != 0)
            {
              Ancs_handleCache[Ancs_endHdlIdx] = Ancs_svcEndHdl;
              Ancs_endHdlIdx = 0;
            }

            // If notification source char is missing, there is something wrong
            if (Ancs_handleCache[HDL_ANCS_NTF_NOTIF_START] == 0)
            {
              PRINT("Notif src not found\n");
              discoveryState = DISC_FAILED;
            }
            else if (Ancs_handleCache[HDL_ANCS_NTF_NOTIF_START] <
                     Ancs_handleCache[HDL_ANCS_NTF_NOTIF_END])
            {
              // Discover ANCS Notification Source CCCD
              GATT_DiscAllCharDescs(pMsg->connHandle,
                                    Ancs_handleCache[HDL_ANCS_NTF_NOTIF_START] + 1,
                                    Ancs_handleCache[HDL_ANCS_NTF_NOTIF_END],
                                    ancs_taskid);
              isNotifCCCD = TRUE;
              discoveryState = DISC_ANCS_CCCD;
            }
            else
            {
              PRINT("CCCD missing\n");
              // Missing required characteristic descriptor
              Ancs_handleCache[HDL_ANCS_NTF_NOTIF_START] = 0;
              discoveryState = DISC_FAILED;
            }
          }
          // Discover all characteristics of ANCS service until end handle
          else
          {
            GATT_DiscAllChars(pMsg->connHandle, handle+1,
                              Ancs_svcEndHdl, ancs_taskid);

          }

        }
        break;

      case DISC_ANCS_CCCD:
        {
          // Characteristic descriptors found
          if (pMsg->method == ATT_FIND_INFO_RSP &&
              pMsg->msg.findInfoRsp.numInfo > 0 &&
              pMsg->msg.findInfoRsp.format == 0x01)
          {
            uint8_t i;
            // For each handle/uuid pair
            for (i = 0; i < pMsg->msg.findInfoRsp.numInfo; i++)
            {
              // Look for CCCD
              if (ATT_BT_PAIR_UUID(pMsg->msg.findInfoRsp.pInfo, i) ==
                  GATT_CLIENT_CHAR_CFG_UUID)
              {
                // CCCD found
                // if it is Notification Source CCCD
                if (isNotifCCCD == TRUE)
                {
                    ancs_c.handle_ns_ccc =
                            ATT_BT_PAIR_HANDLE(pMsg->msg.findInfoRsp.pInfo, i);
                    Ancs_handleCache[HDL_ANCS_NTF_CCCD] =
                            ATT_BT_PAIR_HANDLE(pMsg->msg.findInfoRsp.pInfo, i);
                }
                // else it is Data Source CCCD
                else
                {
                    ancs_c.handle_ds_ccc =
                            ATT_BT_PAIR_HANDLE(pMsg->msg.findInfoRsp.pInfo, i);
                    Ancs_handleCache[HDL_ANCS_DATA_SRC_CCCD] =
                            ATT_BT_PAIR_HANDLE(pMsg->msg.findInfoRsp.pInfo, i);
                }
                break;
              }
            }
          }

          // If procedure complete
          if ((pMsg->method == ATT_FIND_INFO_RSP  &&
               pMsg->hdr.status == bleProcedureComplete) ||
              (pMsg->method == ATT_ERROR_RSP))
          {
            // Discover ANCS Data Source characteristic descriptors
            if (isNotifCCCD == TRUE &&
                Ancs_handleCache[HDL_ANCS_DATA_SRC_CCCD] == 0)
            {
              GATT_DiscAllCharDescs(pMsg->connHandle,
                                    Ancs_handleCache[HDL_ANCS_DATA_SRC_START] + 1,
                                    Ancs_handleCache[HDL_ANCS_DATA_SRC_END],
                                    ancs_taskid);

              isNotifCCCD = FALSE;

            }
            else
            {
              discoveryState = DISC_IDLE;
              atomic_set_bit(&ancs_c.state, ANCS_BT_DISCOVERY_COMPLETE);
              PRINT("discover complete\n");
              tmos_set_event(ancs_taskid, START_SUBSCRIPE_NOTIDATA_EVT);
              tmos_set_event(ancs_taskid, START_SUBSCRIPE_NOTISRC_EVT);

            }
          }
        }
        break;

      default:
        break;
    }
}


/************************ endfile @ central **************************/

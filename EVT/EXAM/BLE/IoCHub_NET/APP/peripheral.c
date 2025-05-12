/********************************** (C) COPYRIGHT *******************************
 * File Name          : peripheral.C
 * Author             : zhangxiyi @WCH
 * Version            : v0.1
 * Date               : 2020/11/26
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "CONFIG.h"
#include "devinfoservice.h"
#include "gattprofile.h"
#include "peripheral.h"
#include "ble_iochub_service.h"
#include "app_drv_fifo.h"

extern uint8_t DP_LIGHT_CTRL(uint8_t* light_sta);
extern volatile uint8_t switch_sta;
uint8_t Peripheral_TaskID = INVALID_TASK_ID; // Task ID for internal task/event processing

//
static uint8_t to_test_buffer[BLE_BUFF_MAX_LEN - 4 - 3];

//The buffer length should be a power of 2
#define APP_BLE_TX_BUFFER_LENGTH    512U
#define APP_BLE_RX_BUFFER_LENGTH    2048U

//The tx buffer and rx buffer for app_drv_fifo
//length should be a power of 2
static uint8_t app_ble_tx_buffer[APP_BLE_TX_BUFFER_LENGTH] = {0};
static uint8_t app_ble_rx_buffer[APP_BLE_RX_BUFFER_LENGTH] = {0};

static app_drv_fifo_t app_ble_tx_fifo;
static app_drv_fifo_t app_ble_rx_fifo;


//fifo length less that MTU-3, retry times
uint32_t iochub_to_ble_send_evt_cnt = 0;


void app_ble_tx_data(uint8_t *data, uint16_t length)
{
    uint16_t write_length = length;
    app_drv_fifo_write(&app_ble_tx_fifo, data, &write_length);
}


//ble iochub service callback handler
void on_bleiochubServiceEvt(uint16_t connection_handle, ble_iochub_evt_t *p_evt)
{
    switch(p_evt->type)
    {
        case BLE_IOCHUB_EVT_TX_NOTI_DISABLED:
            PRINT("%02x:bleiochub_EVT_TX_NOTI_DISABLED\r\n", connection_handle);
            tmos_stop_task(Peripheral_TaskID, IOCHUB_ALL_DP_UPLOAD_EVT);
            break;
        case BLE_IOCHUB_EVT_TX_NOTI_ENABLED:
            PRINT("%02x:bleiochub_EVT_TX_NOTI_ENABLED\r\n", connection_handle);
            tmos_start_task(Peripheral_TaskID, IOCHUB_ALL_DP_UPLOAD_EVT, MS1_TO_SYSTEM_TIME(1000));
            break;
        case BLE_IOCHUB_EVT_BLE_DATA_RECIEVED:
            PRINT("BLE RX DATA len:%d\r\n", p_evt->data.length);

            //for notify back test
            //to ble
            uint16_t to_write_length = p_evt->data.length;
//            //for notify back test
//            //to ble
//            app_drv_fifo_write(&app_ble_tx_fifo, (uint8_t *)p_evt->data.p_data, &to_write_length);
//            tmos_start_task(Peripheral_TaskID, IOCHUB_TO_BLE_SEND_EVT, 0);
//            //end of nofify back test

            app_drv_fifo_write(&app_ble_rx_fifo, (uint8_t *)p_evt->data.p_data, &to_write_length);
            tmos_start_task(Peripheral_TaskID, IOCHUB_DATA_PROCESS_EVT, 2);

            break;
        default:
            break;
    }
}

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// How often to perform periodic event
#define SBP_PERIODIC_EVT_PERIOD              1600

// How often to perform read rssi event
#define SBP_READ_RSSI_EVT_PERIOD             3200

// Parameter update delay
#define SBP_PARAM_UPDATE_DELAY               6400

// What is the advertising interval when device is discoverable (units of 625us, 80=50ms)
#define DEFAULT_ADVERTISING_INTERVAL         160

// Limited discoverable mode advertises for 30.72s, and then stops
// General discoverable mode advertises indefinitely
#define DEFAULT_DISCOVERABLE_MODE            GAP_ADTYPE_FLAGS_GENERAL

// Minimum connection interval (units of 1.25ms, 10=12.5ms)
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL    8

// Maximum connection interval (units of 1.25ms, 100=125ms)
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL    20

// Slave latency to use parameter update
#define DEFAULT_DESIRED_SLAVE_LATENCY        0

// Supervision timeout value (units of 10ms, 100=1s)
#define DEFAULT_DESIRED_CONN_TIMEOUT         100

// Company Identifier: WCH
#define WCH_COMPANY_ID                       0x07D7

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

//for send to ble
typedef enum
{
    SEND_TO_BLE_TO_SEND = 1,
    SEND_TO_BLE_ALLOC_FAILED,
    SEND_TO_BLE_SEND_FAILED,
} send_to_ble_state_t;
send_to_ble_state_t send_to_ble_state = SEND_TO_BLE_TO_SEND;

blePaControlConfig_t pa_lna_ctl;

//static uint8 Peripheral_TaskID = INVALID_TASK_ID;   // Task ID for internal task/event processing

// GAP - SCAN RSP data (max size = 31 bytes)
static uint8 scanRspData[] = {
    // complete name
    17, // length of this data
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'c', 'h', '5', '8', '3', '_', 'b', 'l', 'e', '_', 'i', 'o', 'c', 'h', 'u', 'b',
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
// best kept short to conserve power while advertisting)
static uint8 advertData[] = {
    // Flags; this sets the device to use limited discoverable
    // mode (advertises for 30 seconds at a time) instead of general
    // discoverable mode (advertises indefinitely)
    0x02, // length of this data
    GAP_ADTYPE_FLAGS,
    DEFAULT_DISCOVERABLE_MODE | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

    // service UUID, to notify central devices what services are included
    // in this peripheral
    0x03,                  // length of this data
    GAP_ADTYPE_16BIT_MORE, // some of the UUID's, but not all
    LO_UINT16(BLE_IOCHUB_SERVICE_UUID),
    HI_UINT16(BLE_IOCHUB_SERVICE_UUID)};

// GAP GATT Attributes
static uint8_t attDeviceName[GAP_DEVICE_NAME_LEN] = "CH583_ble_iochub";

// Connection item list
static peripheralConnItem_t peripheralConnList;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void Peripheral_ProcessTMOSMsg(tmos_event_hdr_t *pMsg);
static void peripheralStateNotificationCB(gapRole_States_t newState, gapRoleEvent_t *pEvent);

static void peripheralParamUpdateCB(uint16 connHandle, uint16 connInterval,
                                    uint16 connSlaveLatency, uint16 connTimeout);
static void peripheralInitConnItem(peripheralConnItem_t *peripheralConnList);
static void peripheralRssiCB(uint16 connHandle, int8 rssi);
static uint8_t ble_recv_frame(uint8_t* frame_buf, uint8_t frame_dlc);
/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static gapRolesCBs_t Peripheral_PeripheralCBs = {
    peripheralStateNotificationCB, // Profile State Change Callbacks
    peripheralRssiCB,              // When a valid RSSI is read from controller (not used by application)
    peripheralParamUpdateCB};

// Broadcast Callbacks
static gapRolesBroadcasterCBs_t Broadcaster_BroadcasterCBs = {
    NULL, // Not used in peripheral role
    NULL  // Receive scan request callback
};

// GAP Bond Manager Callbacks
static gapBondCBs_t Peripheral_BondMgrCBs = {
    NULL, // Passcode callback (not used by application)
    NULL  // Pairing / Bonding state Callback (not used by application)
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      Peripheral_Init
 *
 * @brief   Initialization function for the Peripheral App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by TMOS.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void Peripheral_Init()
{
    //tx fifo and tx fifo
    //The buffer length should be a power of 2
    app_drv_fifo_init(&app_ble_tx_fifo, app_ble_tx_buffer, APP_BLE_TX_BUFFER_LENGTH);
    app_drv_fifo_init(&app_ble_rx_fifo, app_ble_rx_buffer, APP_BLE_RX_BUFFER_LENGTH);

    Peripheral_TaskID = TMOS_ProcessEventRegister(Peripheral_ProcessEvent);

    // Setup the GAP Peripheral Role Profile
    {
        uint8  initial_advertising_enable = TRUE;
        uint16 desired_min_interval = 6;
        uint16 desired_max_interval = 1000;

        // Set the GAP Role Parameters
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &initial_advertising_enable);
        GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, sizeof(scanRspData), scanRspData);
        GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(advertData), advertData);
        GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL, sizeof(uint16), &desired_min_interval);
        GAPRole_SetParameter(GAPROLE_MAX_CONN_INTERVAL, sizeof(uint16), &desired_max_interval);
    }

    // Set advertising interval
    {
        uint16 advInt = DEFAULT_ADVERTISING_INTERVAL;

        GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, advInt);
        GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, advInt);
    }

    // Setup the GAP Bond Manager
    {
        uint32 passkey = 0; // passkey "000000"
        uint8  pairMode = GAPBOND_PAIRING_MODE_WAIT_FOR_REQ;
        uint8  mitm = TRUE;
        uint8  bonding = TRUE;
        uint8  ioCap = GAPBOND_IO_CAP_DISPLAY_ONLY;
        GAPBondMgr_SetParameter(GAPBOND_PERI_DEFAULT_PASSCODE, sizeof(uint32), &passkey);
        GAPBondMgr_SetParameter(GAPBOND_PERI_PAIRING_MODE, sizeof(uint8), &pairMode);
        GAPBondMgr_SetParameter(GAPBOND_PERI_MITM_PROTECTION, sizeof(uint8), &mitm);
        GAPBondMgr_SetParameter(GAPBOND_PERI_IO_CAPABILITIES, sizeof(uint8), &ioCap);
        GAPBondMgr_SetParameter(GAPBOND_PERI_BONDING_ENABLED, sizeof(uint8), &bonding);
    }

    // Initialize GATT attributes
    GGS_AddService(GATT_ALL_SERVICES);         // GAP
    GATTServApp_AddService(GATT_ALL_SERVICES); // GATT attributes
    DevInfo_AddService();                      // Device Information Service
    ble_iochub_add_service(on_bleiochubServiceEvt);

    // Set the GAP Characteristics
    GGS_SetParameter(GGS_DEVICE_NAME_ATT, sizeof(attDeviceName), attDeviceName);

    // Init Connection Item
    peripheralInitConnItem(&peripheralConnList);

    // Register receive scan request callback
    GAPRole_BroadcasterSetCB(&Broadcaster_BroadcasterCBs);

    // Setup a delayed profile startup
    tmos_set_event(Peripheral_TaskID, SBP_START_DEVICE_EVT);
}

/*********************************************************************
 * @fn      peripheralInitConnItem
 *
 * @brief   Init Connection Item
 *
 * @param   peripheralConnList -
 *
 * @return  NULL
 */
static void peripheralInitConnItem(peripheralConnItem_t *peripheralConnList)
{
    peripheralConnList->connHandle = GAP_CONNHANDLE_INIT;
    peripheralConnList->connInterval = 0;
    peripheralConnList->connSlaveLatency = 0;
    peripheralConnList->connTimeout = 0;
}

uint32_t get_fattime(void)
{
    return 0;
}


#define CMD_BUF_LEN     512

static uint8_t _cmdl_buff[CMD_BUF_LEN];
static buffer_t cmdl_buff = {
    .data = _cmdl_buff,
    .length = 0,
    .capacity = CMD_BUF_LEN,
};

/*********************************************************************
 * @fn      Peripheral_ProcessEvent
 *
 * @brief   Peripheral Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
uint16 Peripheral_ProcessEvent(uint8 task_id, uint16 events)
{
    static attHandleValueNoti_t noti;
    //  VOID task_id; // TMOS required parameter that isn't used in this function

    if(events & SYS_EVENT_MSG)
    {
        uint8 *pMsg;

        if((pMsg = tmos_msg_receive(Peripheral_TaskID)) != NULL)
        {
            Peripheral_ProcessTMOSMsg((tmos_event_hdr_t *)pMsg);
            // Release the TMOS message
            tmos_msg_deallocate(pMsg);
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }

    if(events & SBP_START_DEVICE_EVT)
    {
        // Start the Device
        GAPRole_PeripheralStartDevice(Peripheral_TaskID, &Peripheral_BondMgrCBs, &Peripheral_PeripheralCBs);
        return (events ^ SBP_START_DEVICE_EVT);
    }
    if(events & SBP_PARAM_UPDATE_EVT)
    {
        // Send connect param update request
        GAPRole_PeripheralConnParamUpdateReq(peripheralConnList.connHandle,
                                             DEFAULT_DESIRED_MIN_CONN_INTERVAL,
                                             DEFAULT_DESIRED_MAX_CONN_INTERVAL,
                                             DEFAULT_DESIRED_SLAVE_LATENCY,
                                             DEFAULT_DESIRED_CONN_TIMEOUT,
                                             Peripheral_TaskID);

        //        GAPRole_PeripheralConnParamUpdateReq( peripheralConnList.connHandle,
        //                                              10,
        //                                              20,
        //                                              0,
        //                                              400,
        //                                              Peripheral_TaskID);

        return (events ^ SBP_PARAM_UPDATE_EVT);
    }

    if(events & IOCHUB_TO_BLE_SEND_EVT)
    {
        static uint16_t read_length = 0;
        ;
        uint8_t result = 0xff;
        switch(send_to_ble_state)
        {
            case SEND_TO_BLE_TO_SEND:

                //notify is not enabled
                if(!ble_iochub_notify_is_ready(peripheralConnList.connHandle))
                {
                    if(peripheralConnList.connHandle == GAP_CONNHANDLE_INIT)
                    {
                        //connection lost, flush rx fifo here
                        app_drv_fifo_flush(&app_ble_tx_fifo);
                    }
                    break;
                }
                read_length = ATT_GetMTU(peripheralConnList.connHandle) - 3;

                if(app_drv_fifo_length(&app_ble_tx_fifo) >= read_length)
                {
                    PRINT("FIFO_LEN:%d\r\n", app_drv_fifo_length(&app_ble_tx_fifo));
                    result = app_drv_fifo_read(&app_ble_tx_fifo, to_test_buffer, &read_length);
                    iochub_to_ble_send_evt_cnt = 0;
                }
                else
                {
                    if(iochub_to_ble_send_evt_cnt > 10)
                    {
                        result = app_drv_fifo_read(&app_ble_tx_fifo, to_test_buffer, &read_length);
                        iochub_to_ble_send_evt_cnt = 0;
                    }
                    else
                    {
                        tmos_start_task(Peripheral_TaskID, IOCHUB_TO_BLE_SEND_EVT, 4);
                        iochub_to_ble_send_evt_cnt++;
//                        PRINT("NO TIME OUT\r\n");
                    }
                }

                if(APP_DRV_FIFO_RESULT_SUCCESS == result)
                {
                    noti.len = read_length;
                    noti.pValue = GATT_bm_alloc(peripheralConnList.connHandle, ATT_HANDLE_VALUE_NOTI, noti.len, NULL, 0);
                    if(noti.pValue != NULL)
                    {
                        tmos_memcpy(noti.pValue, to_test_buffer, noti.len);
                        result = ble_iochub_notify(peripheralConnList.connHandle, &noti, 0);
                        if(result != SUCCESS)
                        {
                            PRINT("R1:%02x\r\n", result);
                            send_to_ble_state = SEND_TO_BLE_SEND_FAILED;
                            GATT_bm_free((gattMsg_t *)&noti, ATT_HANDLE_VALUE_NOTI);
                            tmos_start_task(Peripheral_TaskID, IOCHUB_TO_BLE_SEND_EVT, 2);
                        }
                        else
                        {
                            send_to_ble_state = SEND_TO_BLE_TO_SEND;
                            //app_fifo_write(&app_ble_tx_fifo,to_test_buffer,&read_length);
                            //app_drv_fifo_write(&app_ble_tx_fifo,to_test_buffer,&read_length);
                            read_length = 0;
                            tmos_start_task(Peripheral_TaskID, IOCHUB_TO_BLE_SEND_EVT, 2);
                        }
                    }
                    else
                    {
                        send_to_ble_state = SEND_TO_BLE_ALLOC_FAILED;
                        tmos_start_task(Peripheral_TaskID, IOCHUB_TO_BLE_SEND_EVT, 2);
                    }
                }
                else
                {
                    //send_to_ble_state = SEND_TO_BLE_FIFO_EMPTY;
                }
                break;
            case SEND_TO_BLE_ALLOC_FAILED:
            case SEND_TO_BLE_SEND_FAILED:

                noti.len = read_length;
                noti.pValue = GATT_bm_alloc(peripheralConnList.connHandle, ATT_HANDLE_VALUE_NOTI, noti.len, NULL, 0);
                if(noti.pValue != NULL)
                {
                    tmos_memcpy(noti.pValue, to_test_buffer, noti.len);
                    result = ble_iochub_notify(peripheralConnList.connHandle, &noti, 0);
                    if(result != SUCCESS)
                    {
                        PRINT("R2:%02x\r\n", result);
                        send_to_ble_state = SEND_TO_BLE_SEND_FAILED;
                        GATT_bm_free((gattMsg_t *)&noti, ATT_HANDLE_VALUE_NOTI);
                        tmos_start_task(Peripheral_TaskID, IOCHUB_TO_BLE_SEND_EVT, 2);
                    }
                    else
                    {
                        send_to_ble_state = SEND_TO_BLE_TO_SEND;
                        //app_drv_fifo_write(&app_ble_tx_fifo,to_test_buffer,&read_length);
                        read_length = 0;
                        tmos_start_task(Peripheral_TaskID, IOCHUB_TO_BLE_SEND_EVT, 2);
                    }
                }
                else
                {
                    send_to_ble_state = SEND_TO_BLE_ALLOC_FAILED;
                    tmos_start_task(Peripheral_TaskID, IOCHUB_TO_BLE_SEND_EVT, 2);
                }
                break;
            default:
                break;
        }
        return (events ^ IOCHUB_TO_BLE_SEND_EVT);
    }

    if(events & IOCHUB_DATA_PROCESS_EVT)
    {
        while(app_drv_fifo_length(&app_ble_rx_fifo))
        {
            if(cmdl_buff.length < CMD_BUF_LEN)
            {
                cmdl_buff.data[cmdl_buff.length] = app_drv_fifo_pop(&app_ble_rx_fifo);
                cmdl_buff.length++;
                if(cmdl_buff.length >= 7)
                {
                    if(0x00 == ble_recv_frame(cmdl_buff.data, cmdl_buff.length))
                    {
                        cmdl_buff.length = 0;
                    }
                }
            }
            else
            {
                app_drv_fifo_flush(&app_ble_rx_fifo);
                cmdl_buff.length = 0;
            }
        }
        cmdl_buff.length = 0;
        return (events ^ IOCHUB_DATA_PROCESS_EVT);
    }

    if(events & IOCHUB_ALL_DP_UPLOAD_EVT)
    {
        uint8_t light_sta = DP_LIGHT_CTRL(NULL);
        ble_send_frame(DP_ID_LIGHT, DP_TYPE_BOOL, &light_sta, 1);
        ble_send_frame(DP_ID_SWITCH, DP_TYPE_BOOL, (uint8_t *)(&switch_sta), 1);

        tmos_start_task(Peripheral_TaskID, IOCHUB_ALL_DP_UPLOAD_EVT, MS1_TO_SYSTEM_TIME(3000));
        return (events ^ IOCHUB_ALL_DP_UPLOAD_EVT);
    }

    if(events & IOCHUB_SWITCH_CHANGE_EVT)
    {
            ble_send_frame(DP_ID_SWITCH, DP_TYPE_BOOL, (uint8_t *)(&switch_sta), 1);
        return (events ^ IOCHUB_SWITCH_CHANGE_EVT);
    }
    // Discard unknown events
    return 0;
}

/*********************************************************************
 * @fn      Peripheral_ProcessTMOSMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void Peripheral_ProcessTMOSMsg(tmos_event_hdr_t *pMsg)
{
    switch(pMsg->event)
    {
        default:
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
    if(peripheralConnList.connHandle != GAP_CONNHANDLE_INIT)
    {
        GAPRole_TerminateLink(pEvent->linkCmpl.connectionHandle);
        PRINT("Connection max...\n");
    }
    else
    {
        peripheralConnList.connHandle = event->connectionHandle;
        peripheralConnList.connInterval = event->connInterval;
        peripheralConnList.connSlaveLatency = event->connLatency;
        peripheralConnList.connTimeout = event->connTimeout;

        // Set timer for param update event
        tmos_start_task(Peripheral_TaskID, SBP_PARAM_UPDATE_EVT, SBP_PARAM_UPDATE_DELAY);

        PRINT("Conn %x - Int %x \n", event->connectionHandle, event->connInterval);
        GPIOB_ResetBits(LED_LINK_PIN);
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

    if(event->connectionHandle == peripheralConnList.connHandle)
    {
        peripheralConnList.connHandle = GAP_CONNHANDLE_INIT;
        peripheralConnList.connInterval = 0;
        peripheralConnList.connSlaveLatency = 0;
        peripheralConnList.connTimeout = 0;

        // Restart advertising
        {
            uint8 advertising_enable = TRUE;
            GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8), &advertising_enable);
            GPIOB_SetBits(LED_LINK_PIN);
        }
    }
    else
    {
        PRINT("ERR..\n");
    }
}

/*********************************************************************
 * @fn      peripheralRssiCB
 *
 * @brief   RSSI callback.
 *
 * @param   connHandle - connection handle
 * @param   rssi - RSSI
 *
 * @return  none
 */
static void peripheralRssiCB(uint16 connHandle, int8 rssi)
{
    PRINT("RSSI -%d dB Conn  %x \n", -rssi, connHandle);
}

/*********************************************************************
 * @fn      peripheralParamUpdateCB
 *
 * @brief   Parameter update complete callback
 *
 * @param   connHandle - connect handle
 *          connInterval - connect interval
 *          connSlaveLatency - connect slave latency
 *          connTimeout - connect timeout
 *
 * @return  none
 */
static void peripheralParamUpdateCB(uint16 connHandle, uint16 connInterval,
                                    uint16 connSlaveLatency, uint16 connTimeout)
{
    if(connHandle == peripheralConnList.connHandle)
    {
        peripheralConnList.connInterval = connInterval;
        peripheralConnList.connSlaveLatency = connSlaveLatency;
        peripheralConnList.connTimeout = connTimeout;

        PRINT("Update %x - Int %x \n", connHandle, connInterval);
    }
    else
    {
        PRINT("peripheralParamUpdateCB err..\n");
    }
}

/*********************************************************************
 * @fn      peripheralStateNotificationCB
 *
 * @brief   Notification from the profile of a state change.
 *
 * @param   newState - new state
 *
 * @return  none
 */
static void peripheralStateNotificationCB(gapRole_States_t newState, gapRoleEvent_t *pEvent)
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
            }
            PRINT("Advertising..\n");
            break;

        case GAPROLE_CONNECTED:
            if(pEvent->gap.opcode == GAP_LINK_ESTABLISHED_EVENT)
            {
                Peripheral_LinkEstablished(pEvent);
                PRINT("Connected..\n");
            }
            break;

        case GAPROLE_CONNECTED_ADV:
            PRINT("Connected Advertising..\n");
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

uint8_t check_sum_8( uint8_t *data, uint8_t len)
{
    uint8_t cs = 0;
    int i;
    for(i = 0; i < len; i++)
    {
        cs += data[i];
    }
    return cs;
}

static uint8_t ble_recv_frame(uint8_t* frame_buf, uint8_t frame_dlc)
{
    uint8_t *p = NULL;
    p = frame_buf;
    if(*p++  != BLE_REC_FST)
    {
        return 0xFF;
    }

    if(*p++ != BLE_REC_SEC)
    {
        return 0xFF;
    }

    uint8_t dp_id = *p++;


    uint8_t dp_type = *p++;


    uint16_t data_len = (*p<<8) | *(p+1) ;

    p+=2;

    uint8_t cs = check_sum_8(frame_buf, frame_dlc-1);


    if(cs != frame_buf[frame_dlc - 1])
    {
//        PRINT("ble cs err: %02x - %02x\n",cs, frame_buf[frame_dlc - 1]);
        return 0xFF;
    }

    PRINT("frame_cmd : %d\n",dp_id);
    PRINT("frame_type : %d\n",dp_type);
    PRINT("data_len : %d\n",data_len);

    switch(dp_id)
    {
        case DP_ID_LIGHT:
        {
            PRINT("DP_ID_LIGHT:%d\n",*p);
            extern uint8_t DP_LIGHT_CTRL(uint8_t* light_sta);
            uint8_t light_sta = DP_LIGHT_CTRL(p);
            ble_send_frame(DP_ID_LIGHT, DP_TYPE_BOOL, &light_sta, 1);
        }break;

        default:
            break;
    }
    return 0x00;
}

uint8_t ble_send_frame(uint8_t frame_cmd, uint8_t frame_type, uint8_t* frame_buf, uint8_t frame_dlc)
{
    uint8_t *send_value = NULL;
    uint8_t check_sum;

    if(frame_buf == NULL)    return 0x01;

    uint16_t send_len = frame_dlc+BLE_HEAD_LEN+BLE_FOMRAT_LEN+BLE_TAIL_LEN;
    send_value = (uint8_t *)tmos_msg_allocate(send_len);
    uint8_t *p = send_value;
    *p++ = BLE_SEND_FST;
    *p++ = BLE_SEND_SEC;
    *p++ = frame_cmd;
    *p++ = frame_type;
    *p++ = frame_dlc >>8;
    *p++ = frame_dlc & 0xFF;
    tmos_memcpy(p,frame_buf,frame_dlc);
    p+=frame_dlc;
    check_sum = check_sum_8(send_value, (uint8_t)(p-send_value));
    *p++ = check_sum;

    uint16_t to_write_length = (uint8_t)(p-send_value);
    app_drv_fifo_write(&app_ble_tx_fifo, (uint8_t *)send_value, &to_write_length);
    PRINT("BLE TX DATA len:%d\r\n", to_write_length);

    for(uint8_t i = 0; i < to_write_length;i++)
    {
        PRINT("%02x ",*(send_value+i));
    }
    PRINT("\n");
    tmos_start_task(Peripheral_TaskID, IOCHUB_TO_BLE_SEND_EVT, 2);

    tmos_msg_deallocate(send_value);
    return 0;
}

uint8_t DP_LIGHT_CTRL(uint8_t* light_sta)
{
    if(light_sta != NULL)
    {
        if(*light_sta)    GPIOB_ResetBits(LED_LIGHT_PIN);
        else               GPIOB_SetBits(LED_LIGHT_PIN);
    }


    return (0 == GPIOB_ReadPortPin(LED_LIGHT_PIN));
}
/*********************************************************************
*********************************************************************/

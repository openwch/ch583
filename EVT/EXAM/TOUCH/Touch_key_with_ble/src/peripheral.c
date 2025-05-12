/********************************** (C) COPYRIGHT *******************************
 * File Name          : peripheral.C
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/12/10
 * Description        : ����ӻ�������Ӧ�ó��򣬳�ʼ���㲥���Ӳ�����Ȼ��㲥������������
 *                      ����������Ӳ�����ͨ���Զ������������
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "CONFIG.h"
#include "devinfoservice.h"
#include "gattprofile.h"
#include "peripheral.h"
#include "Touch.h"
#include "app_tmos.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// How often to perform periodic event
#define SBP_PERIODIC_EVT_PERIOD MS1_TO_SYSTEM_TIME(1000)

// How often to perform read rssi event
#define SBP_READ_RSSI_EVT_PERIOD MS1_TO_SYSTEM_TIME(2000)

// Parameter update delay
#define SBP_PARAM_UPDATE_DELAY MS1_TO_SYSTEM_TIME(4000)

// PHY update delay
#define SBP_PHY_UPDATE_DELAY MS1_TO_SYSTEM_TIME(2000)

// What is the advertising interval when device is discoverable (units of 625us, 80=50ms)
#define DEFAULT_ADVERTISING_INTERVAL MS1_TO_SYSTEM_TIME(500)

// Limited discoverable mode advertises for 30.72s, and then stops
// General discoverable mode advertises indefinitely
#define DEFAULT_DISCOVERABLE_MODE GAP_ADTYPE_FLAGS_GENERAL

// Minimum connection interval (units of 1.25ms, 6=7.5ms)
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL 6

// Maximum connection interval (units of 1.25ms, 100=125ms)
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL 10//100

// Slave latency to use parameter update
#define DEFAULT_DESIRED_SLAVE_LATENCY 0

// Supervision timeout value (units of 10ms, 100=1s)
#define DEFAULT_DESIRED_CONN_TIMEOUT 100

// Company Identifier: WCH
#define WCH_COMPANY_ID 0x07D7

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

UINT8V bleConnectState = 0; ///< 0:û������; 1:������
UINT8V advState = 0;        ///< 0:ֹͣ�㲥; 1:���ڹ㲥

uint8_t initial_advertising_enable = FALSE;

static uint8_t Peripheral_TaskID = INVALID_TASK_ID;    // Task ID for internal task/event processing


// GAP - SCAN RSP data (max size = 31 bytes)
static uint8_t scanRspData[] = {
// complete name
    0x0A,// length of this data
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'B',
    'l',
    'e',
    '_',
    'T',
    'o',
    'u',
    'c',
    'h',
    // connection interval range
    0x05,// length of this data
    GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE,
    LO_UINT16( DEFAULT_DESIRED_MIN_CONN_INTERVAL ),    // 100ms
    HI_UINT16( DEFAULT_DESIRED_MIN_CONN_INTERVAL ),
    LO_UINT16( DEFAULT_DESIRED_MAX_CONN_INTERVAL ),    // 1s
    HI_UINT16( DEFAULT_DESIRED_MAX_CONN_INTERVAL ),

    // Tx power level
    0x02,// length of this data
    GAP_ADTYPE_POWER_LEVEL, 0 // 0dBm
};

// GAP - Advertisement data (max size = 31 bytes, though this is
// best kept short to conserve power while advertising)
static uint8_t advertData[] = {
// Flags; this sets the device to use limited discoverable
// mode (advertises for 30 seconds at a time) instead of general
// discoverable mode (advertises indefinitely)
    0x02,// length of this data
    GAP_ADTYPE_FLAGS,
    DEFAULT_DISCOVERABLE_MODE | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

    // service UUID, to notify central devices what services are included
    // in this peripheral
    0x03,// length of this data
    GAP_ADTYPE_16BIT_MORE,    // some of the UUID's, but not all
    LO_UINT16( SIMPLEPROFILE_SERV_UUID ), HI_UINT16( SIMPLEPROFILE_SERV_UUID )
};

// GAP GATT Attributes
static uint8_t attDeviceName[ GAP_DEVICE_NAME_LEN ] = "Ble_Touch";

// Connection item list
peripheralConnItem_t peripheralConnList;

static uint16_t peripheralMTU = ATT_MTU_SIZE;

uint16_t peripheral_Mtu = 23;
/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void Peripheral_ProcessTMOSMsg(tmos_event_hdr_t *pMsg);
static void peripheralStateNotificationCB(gapRole_States_t newState, gapRoleEvent_t *pEvent);
static void performPeriodicTask(void);
static void simpleProfileChangeCB(uint8_t paramID, uint8_t *pValue, uint16_t len);
static void peripheralParamUpdateCB(uint16_t connHandle, uint16_t connInterval, uint16_t connSlaveLatency, uint16_t connTimeout);
static void peripheralInitConnItem(peripheralConnItem_t *peripheralConnList);
static void peripheralRssiCB(uint16_t connHandle, int8_t rssi);
// static void peripheralChar4Notify(uint8_t *pValue, uint16_t len);

/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static gapRolesCBs_t Peripheral_PeripheralCBs = {
    peripheralStateNotificationCB,    // Profile State Change Callbacks
    peripheralRssiCB,    // peripheralRssiCB; When a valid RSSI is read from controller (not used by application)
    peripheralParamUpdateCB
};

// Broadcast Callbacks
static gapRolesBroadcasterCBs_t Broadcaster_BroadcasterCBs = {
    NULL,    // Not used in peripheral role
    NULL
// Receive scan request callback
                };

// GAP Bond Manager Callbacks
static gapBondCBs_t Peripheral_BondMgrCBs = {
    NULL,    // Passcode callback (not used by application)
    NULL
// Pairing / Bonding state Callback (not used by application)
                };

// Simple GATT Profile Callbacks
static simpleProfileCBs_t Peripheral_SimpleProfileCBs = {

    simpleProfileChangeCB
// Characteristic value change callback
                };
/*********************************************************************
 * PUBLIC FUNCTIONS
 */

void blk_period(uint32_t timeUs)
{
    if (!wakeupflag)
    {
        PRINT("1\n");
        PeriodicDealData();
        PRINT("2\n");
    }
}
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
    advState = 0;
    bleConnectState = 0;
    Peripheral_TaskID = TMOS_ProcessEventRegister( Peripheral_ProcessEvent );

    // Setup the GAP Peripheral Role Profile
    {
        initial_advertising_enable = TRUE;
        advState = 1;
        uint16_t desired_min_interval = DEFAULT_DESIRED_MIN_CONN_INTERVAL;
        uint16_t desired_max_interval = DEFAULT_DESIRED_MAX_CONN_INTERVAL;

        // Set the GAP Role Parameters
        GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &initial_advertising_enable );
        GAPRole_SetParameter( GAPROLE_SCAN_RSP_DATA, sizeof(scanRspData), scanRspData );
        GAPRole_SetParameter( GAPROLE_ADVERT_DATA, sizeof(advertData), advertData );
        GAPRole_SetParameter( GAPROLE_MIN_CONN_INTERVAL, sizeof(uint16_t), &desired_min_interval );
        GAPRole_SetParameter( GAPROLE_MAX_CONN_INTERVAL, sizeof(uint16_t), &desired_max_interval );
    }

    // Set the GAP Characteristics
    GGS_SetParameter( GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, attDeviceName );

    {
        uint16_t advInt = DEFAULT_ADVERTISING_INTERVAL;

        // Set advertising interval
        GAP_SetParamValue( TGAP_DISC_ADV_INT_MIN, advInt );
        GAP_SetParamValue( TGAP_DISC_ADV_INT_MAX, advInt );

        // Enable scan req notify
        GAP_SetParamValue( TGAP_ADV_SCAN_REQ_NOTIFY, ENABLE );
    }

    // Setup the GAP Bond Manager
    {
        uint32_t passkey = 0;    // passkey "000000"
        uint8_t pairMode = GAPBOND_PAIRING_MODE_WAIT_FOR_REQ;
        uint8_t mitm = TRUE;
        uint8_t bonding = TRUE;
        uint8_t ioCap = GAPBOND_IO_CAP_DISPLAY_ONLY;
        GAPBondMgr_SetParameter( GAPBOND_PERI_DEFAULT_PASSCODE, sizeof(uint32_t), &passkey );
        GAPBondMgr_SetParameter( GAPBOND_PERI_PAIRING_MODE, sizeof(uint8_t), &pairMode );
        GAPBondMgr_SetParameter( GAPBOND_PERI_MITM_PROTECTION, sizeof(uint8_t), &mitm );
        GAPBondMgr_SetParameter( GAPBOND_PERI_IO_CAPABILITIES, sizeof(uint8_t), &ioCap );
        GAPBondMgr_SetParameter( GAPBOND_PERI_BONDING_ENABLED, sizeof(uint8_t), &bonding );
    }

    // Initialize GATT attributes
    GGS_AddService( GATT_ALL_SERVICES );    // GAP
    GATTServApp_AddService( GATT_ALL_SERVICES );    // GATT attributes
    DevInfo_AddService( );    // Device Information Service
    SimpleProfile_AddService( GATT_ALL_SERVICES );    // Simple GATT Profile

    // Setup the SimpleProfile Characteristic Values
    {
        uint8_t charValue1[ SIMPLEPROFILE_CHAR1_LEN ] = {

            1
        };
        uint8_t charValue2[ SIMPLEPROFILE_CHAR2_LEN ] = {

            2
        };

        SimpleProfile_SetParameter( SIMPLEPROFILE_CHAR1, SIMPLEPROFILE_CHAR1_LEN, charValue1 );
        SimpleProfile_SetParameter( SIMPLEPROFILE_CHAR2, SIMPLEPROFILE_CHAR2_LEN, charValue2 );
    }

    // Init Connection Item
    peripheralInitConnItem( &peripheralConnList );

    // Register callback with SimpleGATTprofile
    SimpleProfile_RegisterAppCBs( &Peripheral_SimpleProfileCBs );

    // Register receive scan request callback
    GAPRole_BroadcasterSetCB( &Broadcaster_BroadcasterCBs );

    // Setup a delayed profile startup
    tmos_set_event( Peripheral_TaskID, SBP_START_DEVICE_EVT );

    PRINT( "Peripheral Init Finish!\n" );
    LL_AdvertiseEventRegister(blk_period);
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
uint16_t Peripheral_ProcessEvent(uint8_t task_id, uint16_t events)
{
    //  VOID task_id; // TMOS required parameter that isn't used in this function

    if (events & SYS_EVENT_MSG)
    {
        uint8_t *pMsg;

        if ((pMsg = tmos_msg_receive( Peripheral_TaskID )) != NULL)
        {
            Peripheral_ProcessTMOSMsg( (tmos_event_hdr_t *) pMsg );
            // Release the TMOS message
            tmos_msg_deallocate( pMsg );
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }

    if (events & SBP_START_DEVICE_EVT)
    {
        // Start the Device
        GAPRole_PeripheralStartDevice( Peripheral_TaskID, &Peripheral_BondMgrCBs, &Peripheral_PeripheralCBs );
        return (events ^ SBP_START_DEVICE_EVT);
    }

    if (events & SBP_PERIODIC_EVT)
    {
        // Restart timer
        if (SBP_PERIODIC_EVT_PERIOD)
        {
            tmos_start_task( Peripheral_TaskID, SBP_PERIODIC_EVT, SBP_PERIODIC_EVT_PERIOD );
        }
        // Perform periodic application task
        performPeriodicTask( );
        return (events ^ SBP_PERIODIC_EVT);
    }

    if (events & SBP_PARAM_UPDATE_EVT)
    {
        // Send connect param update request
        GAPRole_PeripheralConnParamUpdateReq( peripheralConnList.connHandle,
                                                DEFAULT_DESIRED_MIN_CONN_INTERVAL,
                                                DEFAULT_DESIRED_MAX_CONN_INTERVAL,
                                                DEFAULT_DESIRED_SLAVE_LATENCY,
                                                DEFAULT_DESIRED_CONN_TIMEOUT, Peripheral_TaskID );

        return (events ^ SBP_PARAM_UPDATE_EVT);
    }

    if (events & SBP_PHY_UPDATE_EVT)
    {
        // start phy update
        PRINT(  "PHY Update %x...\n",
                GAPRole_UpdatePHY(peripheralConnList.connHandle, 0, GAP_PHY_BIT_LE_2M, GAP_PHY_BIT_LE_2M, 0) );

        return (events ^ SBP_PHY_UPDATE_EVT);
    }

    if (events & SBP_READ_RSSI_EVT)
    {
        GAPRole_ReadRssiCmd( peripheralConnList.connHandle );
        tmos_start_task( Peripheral_TaskID, SBP_READ_RSSI_EVT, SBP_READ_RSSI_EVT_PERIOD );
        return (events ^ SBP_READ_RSSI_EVT);
    }

    // Discard unknown events
    return 0;
}

/*********************************************************************
 * @fn      Peripheral_ProcessGAPMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void Peripheral_ProcessGAPMsg(gapRoleEvent_t *pEvent)
{
    switch (pEvent->gap.opcode)
    {
        case GAP_SCAN_REQUEST_EVENT:
        {
//            PRINT("Receive scan req from %02x %02x %02x %02x %02x %02x  ..\n", pEvent->scanReqEvt.scannerAddr[0],
//                  pEvent->scanReqEvt.scannerAddr[1], pEvent->scanReqEvt.scannerAddr[2], pEvent->scanReqEvt.scannerAddr[3],
//                  pEvent->scanReqEvt.scannerAddr[4], pEvent->scanReqEvt.scannerAddr[5]);
            break;
        }

        case GAP_PHY_UPDATE_EVENT:
        {
            PRINT( "Phy update Rx:%02x Tx:%02x ..\n", pEvent->linkPhyUpdate.connRxPHYS, pEvent->linkPhyUpdate.connTxPHYS );
            break;
        }

        default:
        break;
    }
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
    switch (pMsg->event)
    {
        case GAP_MSG_EVENT:
        {
            Peripheral_ProcessGAPMsg( (gapRoleEvent_t *) pMsg );
            break;
        }

        case GATT_MSG_EVENT:
        {
            gattMsgEvent_t *pMsgEvent;

            pMsgEvent = (gattMsgEvent_t *) pMsg;
            if (pMsgEvent->method == ATT_MTU_UPDATED_EVENT)
            {
                peripheralMTU = pMsgEvent->msg.exchangeMTUReq.clientRxMTU;
                PRINT( "mtu exchange: %d\n", pMsgEvent->msg.exchangeMTUReq.clientRxMTU );
            }
            break;
        }

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
    gapEstLinkReqEvent_t *event = (gapEstLinkReqEvent_t *) pEvent;

    // See if already connected
    if (peripheralConnList.connHandle != GAP_CONNHANDLE_INIT)
    {
        GAPRole_TerminateLink( pEvent->linkCmpl.connectionHandle );
        PRINT( "Connection max...\n" );
    }
    else
    {
        peripheralConnList.connHandle = event->connectionHandle;
        peripheralConnList.connInterval = event->connInterval;
        peripheralConnList.connSlaveLatency = event->connLatency;
        peripheralConnList.connTimeout = event->connTimeout;

        // Set timer for periodic event
        tmos_start_task( Peripheral_TaskID, SBP_PERIODIC_EVT, SBP_PERIODIC_EVT_PERIOD );

        // Set timer for param update event
        tmos_start_task( Peripheral_TaskID, SBP_PARAM_UPDATE_EVT, SBP_PARAM_UPDATE_DELAY );

        // Start read rssi
        //        tmos_start_task(Peripheral_TaskID, SBP_READ_RSSI_EVT, SBP_READ_RSSI_EVT_PERIOD);

        //Start touchkey Deal Data event
        tky_DealData_start();

        PRINT( "Conn %x - Int %x \n", event->connectionHandle, event->connInterval );
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
    gapTerminateLinkEvent_t *event = (gapTerminateLinkEvent_t *) pEvent;

    if (event->connectionHandle == peripheralConnList.connHandle)
    {
        peripheralConnList.connHandle = GAP_CONNHANDLE_INIT;
        peripheralConnList.connInterval = 0;
        peripheralConnList.connSlaveLatency = 0;
        peripheralConnList.connTimeout = 0;
        tmos_stop_task( Peripheral_TaskID, SBP_PERIODIC_EVT );
        tmos_stop_task( Peripheral_TaskID, SBP_READ_RSSI_EVT );

        // Restart advertising
        if (wakeupflag)
        {
            uint8_t advertising_enable = TRUE;
            GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &advertising_enable );
        }
    }
    else
    {
        PRINT( "ERR..\n" );
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
static void peripheralRssiCB(uint16_t connHandle, int8_t rssi)
{
    //    PRINT("RSSI -%d dB Conn  %x \n", -rssi, connHandle);
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
static void peripheralParamUpdateCB(uint16_t connHandle, uint16_t connInterval, uint16_t connSlaveLatency, uint16_t connTimeout)
{
    if (connHandle == peripheralConnList.connHandle)
    {
        peripheralConnList.connInterval = connInterval;
        peripheralConnList.connSlaveLatency = connSlaveLatency;
        peripheralConnList.connTimeout = connTimeout;

        PRINT( "Update %x - Int %x \n", connHandle, connInterval );
    }
    else
    {
        PRINT( "ERR..\n" );
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
    switch (newState & GAPROLE_STATE_ADV_MASK)
    {
        case GAPROLE_STARTED:
            PRINT( "Initialized..\n" );
        break;

        case GAPROLE_ADVERTISING:
            if (pEvent->gap.opcode == GAP_LINK_TERMINATED_EVENT)
            {
                Peripheral_LinkTerminated( pEvent );
                PRINT( "Disconnected.. Reason:%x\n", pEvent->linkTerminate.reason );
                advState = 1;
                PRINT( "Advertising..\n" );
            }
            else if (pEvent->gap.opcode == GAP_MAKE_DISCOVERABLE_DONE_EVENT)
            {
                advState = 1;
                PRINT( "Advertising..\n" );
            }
        break;

        case GAPROLE_CONNECTED:
            if (pEvent->gap.opcode == GAP_LINK_ESTABLISHED_EVENT)
            {
                Peripheral_LinkEstablished( pEvent );
                bleConnectState = 1;
                advState = 0;
                PRINT( "Connected..\n" );
            }
        break;

        case GAPROLE_CONNECTED_ADV:
            if (pEvent->gap.opcode == GAP_MAKE_DISCOVERABLE_DONE_EVENT)
            {
                bleConnectState = 1;
                advState = 1;
                PRINT( "Connected Advertising..\n" );
            }
        break;

        case GAPROLE_WAITING:
            if (pEvent->gap.opcode == GAP_END_DISCOVERABLE_DONE_EVENT)
            {
                PRINT( "Waiting for advertising..\n" );
            }
            else if (pEvent->gap.opcode == GAP_LINK_TERMINATED_EVENT)
            {
                Peripheral_LinkTerminated( pEvent );
                PRINT( "Disconnected.. Reason:%x\n", pEvent->linkTerminate.reason );
                bleConnectState = 0;
                peripheralStartAdv();
            }
            else if (pEvent->gap.opcode == GAP_LINK_ESTABLISHED_EVENT)
            {
                if (pEvent->gap.hdr.status != SUCCESS)
                {
                    PRINT( "Waiting for advertising..\n" );
                }
                else
                {
                    PRINT( "Error..\n" );
                }
            }
            else
            {
                PRINT( "Error..%x\n", pEvent->gap.opcode );
            }
        break;

        case GAPROLE_ERROR:
            PRINT( "Error..\n" );
        break;

        default:
        break;
    }
}

/*********************************************************************
 * @fn      performPeriodicTask
 *
 * @brief   Perform a periodic application task. This function gets
 *          called every five seconds as a result of the SBP_PERIODIC_EVT
 *          TMOS event. In this example, the value of the third
 *          characteristic in the SimpleGATTProfile service is retrieved
 *          from the profile, and then copied into the value of the
 *          the fourth characteristic.
 *
 * @param   none
 *
 * @return  none
 */
static void performPeriodicTask(void)
{
    //  PRINT("bleConnectState:%u\n",bleConnectState);
    //    uint8_t notiData[SIMPLEPROFILE_CHAR2_LEN] = {0x88};
    //    peripheralChar2Notify(notiData, SIMPLEPROFILE_CHAR2_LEN);
}

/*********************************************************************
 * @fn      peripheralChar4Notify
 *
 * @brief   Prepare and send simpleProfileChar4 notification
 *
 * @param   pValue - data to notify
 *          len - length of data
 *
 * @return  none
 */
// static void peripheralChar4Notify(uint8_t *pValue, uint16_t len)
//{
//     attHandleValueNoti_t noti;
//     if(len > (peripheralMTU - 3))
//     {
//         PRINT("Too large noti\n");
//         return;
//     }
//     noti.len = len;
//     noti.pValue = GATT_bm_alloc(peripheralConnList.connHandle, ATT_HANDLE_VALUE_NOTI, noti.len, NULL, 0);
//     if(noti.pValue)
//     {
//         tmos_memcpy(noti.pValue, pValue, noti.len);
//         if(simpleProfile_Notify(peripheralConnList.connHandle, &noti) != SUCCESS)
//         {
//             GATT_bm_free((gattMsg_t *)&noti, ATT_HANDLE_VALUE_NOTI);
//         }
//     }
// }
void peripheralChar2Notify(uint8_t *pValue, uint16_t len)
{
    attHandleValueNoti_t noti;
    noti.len = min( (peripheral_Mtu - 3), len );
    noti.pValue = GATT_bm_alloc( peripheralConnList.connHandle, ATT_HANDLE_VALUE_NOTI, noti.len, NULL, 0 );
    tmos_memcpy( noti.pValue, pValue, noti.len );
    if (simpleProfile_Notify( peripheralConnList.connHandle, &noti ) != SUCCESS)
    {

        GATT_bm_free( (gattMsg_t *) &noti, ATT_HANDLE_VALUE_NOTI );
    }
}

/*********************************************************************
 * @fn      simpleProfileChangeCB
 *
 * @brief   Callback from SimpleBLEProfile indicating a value change
 *
 * @param   paramID - parameter ID of the value that was changed.
 *          pValue - pointer to data that was changed
 *          len - length of data
 *
 * @return  none
 */
static void simpleProfileChangeCB(uint8_t paramID, uint8_t *pValue, uint16_t len)
{
    switch (paramID)
    {
        case SIMPLEPROFILE_CHAR1:
        {
            uint8_t newValue[ SIMPLEPROFILE_CHAR1_LEN ];
            tmos_memcpy( newValue, pValue, len );
            PRINT( "profile ChangeCB CHAR1.. \n" );
            break;
        }

        default:
            // should not reach here!
        break;
    }
}

void peripheralStartAdv(void)
{
    if (!advState)
    {
        initial_advertising_enable = TRUE;
        GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &initial_advertising_enable );
        advState=1;
    }
}

void peripheralStoptAdv(void)
{
    if (advState)
    {
        initial_advertising_enable = FALSE;
        GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &initial_advertising_enable );
        advState=0;
    }
}
/*********************************************************************
 *********************************************************************/

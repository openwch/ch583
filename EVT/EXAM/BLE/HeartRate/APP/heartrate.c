/********************************** (C) COPYRIGHT *******************************
 * File Name          : heartrate.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2020/08/06
 * Description        : 心率计应用程序，初始化广播连接参数，然后广播，直至连接主机后，定时上传心率
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*********************************************************************
 * INCLUDES
 */

#include "CONFIG.h"
#include "battservice.h"
#include "devinfoservice.h"
#include "heartrate.h"
#include "heartrateservice.h"
/*********************************************************************
 * MACROS
 */

// Convert BPM to RR-Interval for data simulation purposes
#define HEARTRATE_BPM_TO_RR(bpm)             ((uint16_t)60 * 1024 / (uint16_t)(bpm))

/*********************************************************************
 * CONSTANTS
 */

// Fast advertising interval in 625us units
#define DEFAULT_FAST_ADV_INTERVAL            32

// Duration of fast advertising duration in (625us)
#define DEFAULT_FAST_ADV_DURATION            30000

// Slow advertising interval in 625us units
#define DEFAULT_SLOW_ADV_INTERVAL            1600

// Duration of slow advertising duration in (625us) (set to 0 for continuous advertising)
#define DEFAULT_SLOW_ADV_DURATION            0

// How often to perform heart rate periodic event
#define DEFAULT_HEARTRATE_PERIOD             2000

// Minimum connection interval (units of 1.25ms)
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL    20

// Maximum connection interval (units of 1.25ms)
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL    160

// Slave latency to use if parameter update request
#define DEFAULT_DESIRED_SLAVE_LATENCY        1

// Supervision timeout value (units of 10ms)
#define DEFAULT_DESIRED_CONN_TIMEOUT         1000

// Delay of start connect paramter update
#define DEFAULT_CONN_PARAM_UPDATE_DELAY      1600

// Battery level is critical when it is less than this %
#define DEFAULT_BATT_CRITICAL_LEVEL          6

// Battery measurement period in (625us)
#define DEFAULT_BATT_PERIOD                  24000

// Some values used to simulate measurements
#define BPM_DEFAULT                          73
#define BPM_MAX                              80
#define ENERGY_INCREMENT                     10
#define FLAGS_IDX_MAX                        7

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
static uint8_t heartRate_TaskID; // Task ID for internal task/event processing

static gapRole_States_t gapProfileState = GAPROLE_INIT;

// GAP Profile - Name attribute for SCAN RSP data
static uint8_t scanRspData[] = {
    0x12, // length of this data
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'H',
    'e',
    'a',
    'r',
    't',
    ' ',
    'R',
    'a',
    't',
    'e',
    ' ',
    'S',
    'e',
    'n',
    's',
    'o',
    'r'};

static uint8_t advertData[] = {
    // flags
    0x02,
    GAP_ADTYPE_FLAGS,
    GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,
    // service UUIDs
    0x05,
    GAP_ADTYPE_16BIT_MORE,
    LO_UINT16(HEARTRATE_SERV_UUID),
    HI_UINT16(HEARTRATE_SERV_UUID),
    LO_UINT16(BATT_SERV_UUID),
    HI_UINT16(BATT_SERV_UUID)};

// Device name attribute value
static uint8_t attDeviceName[GAP_DEVICE_NAME_LEN] = "Heart Rate Sensor";

// GAP connection handle
static uint16_t gapConnHandle;

// Heart rate measurement value stored in this structure
static attHandleValueNoti_t heartRateMeas;

// Components of heart rate measurement structure
static uint8_t  heartRateBpm = BPM_DEFAULT;
static uint16_t heartRateEnergy = 0;
static uint16_t heartRateRrInterval1 = HEARTRATE_BPM_TO_RR(BPM_DEFAULT);
static uint16_t heartRateRrInterval2 = HEARTRATE_BPM_TO_RR(BPM_DEFAULT);

// flags for simulated measurements
static const uint8_t heartRateFlags[FLAGS_IDX_MAX] = {
    HEARTRATE_FLAGS_CONTACT_NOT_SUP,
    HEARTRATE_FLAGS_CONTACT_NOT_DET,
    HEARTRATE_FLAGS_CONTACT_DET | HEARTRATE_FLAGS_ENERGY_EXP,
    HEARTRATE_FLAGS_CONTACT_DET | HEARTRATE_FLAGS_RR,
    HEARTRATE_FLAGS_CONTACT_DET | HEARTRATE_FLAGS_ENERGY_EXP | HEARTRATE_FLAGS_RR,
    HEARTRATE_FLAGS_FORMAT_UINT16 | HEARTRATE_FLAGS_CONTACT_DET | HEARTRATE_FLAGS_ENERGY_EXP | HEARTRATE_FLAGS_RR,
    0x00};

static uint8_t heartRateFlagsIdx = 0;

// Advertising user-cancelled state
static BOOL heartRateAdvCancelled = FALSE;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void heartRate_ProcessTMOSMsg(tmos_event_hdr_t *pMsg);
static void HeartRateGapStateCB(gapRole_States_t newState, gapRoleEvent_t *pEvent);
static void heartRatePeriodicTask(void);
static void heartRateBattPeriodicTask(void);
static void heartRateMeasNotify(void);
static void heartRateCB(uint8_t event);
static void heartRateBattCB(uint8_t event);

/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static gapRolesCBs_t heartRatePeripheralCB = {
    HeartRateGapStateCB, // Profile State Change Callbacks
    NULL,                // When a valid RSSI is read from controller
    NULL};

// Bond Manager Callbacks
static gapBondCBs_t heartRateBondCB = {
    NULL, // Passcode callback
    NULL  // Pairing state callback
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      HeartRate_Init
 *
 * @brief   Initialization function for the Heart Rate App Task.
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
void HeartRate_Init()
{
    heartRate_TaskID = TMOS_ProcessEventRegister(HeartRate_ProcessEvent);

    // Setup the GAP Peripheral Role Profile
    {
        uint8_t initial_advertising_enable = TRUE;

        // Set the GAP Role Parameters
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &initial_advertising_enable);
        GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, sizeof(scanRspData), scanRspData);
        GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(advertData), advertData);
    }
    // Set the GAP Characteristics
    GGS_SetParameter(GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, attDeviceName);

    // Setup the GAP Bond Manager
    {
        uint32_t passkey = 0; // passkey "000000"
        uint8_t  pairMode = GAPBOND_PAIRING_MODE_WAIT_FOR_REQ;
        uint8_t  mitm = FALSE;
        uint8_t  ioCap = GAPBOND_IO_CAP_DISPLAY_ONLY;
        uint8_t  bonding = TRUE;
        GAPBondMgr_SetParameter(GAPBOND_PERI_DEFAULT_PASSCODE, sizeof(uint32_t), &passkey);
        GAPBondMgr_SetParameter(GAPBOND_PERI_PAIRING_MODE, sizeof(uint8_t), &pairMode);
        GAPBondMgr_SetParameter(GAPBOND_PERI_MITM_PROTECTION, sizeof(uint8_t), &mitm);
        GAPBondMgr_SetParameter(GAPBOND_PERI_IO_CAPABILITIES, sizeof(uint8_t), &ioCap);
        GAPBondMgr_SetParameter(GAPBOND_PERI_BONDING_ENABLED, sizeof(uint8_t), &bonding);
    }

    // Setup the Heart Rate Characteristic Values
    {
        uint8_t sensLoc = HEARTRATE_SENS_LOC_WRIST;
        HeartRate_SetParameter(HEARTRATE_SENS_LOC, sizeof(uint8_t), &sensLoc);
    }

    // Setup Battery Characteristic Values
    {
        uint8_t critical = DEFAULT_BATT_CRITICAL_LEVEL;
        Batt_SetParameter(BATT_PARAM_CRITICAL_LEVEL, sizeof(uint8_t), &critical);
    }

    // Initialize GATT attributes
    GGS_AddService(GATT_ALL_SERVICES);         // GAP
    GATTServApp_AddService(GATT_ALL_SERVICES); // GATT attributes
    HeartRate_AddService(GATT_ALL_SERVICES);
    DevInfo_AddService();
    Batt_AddService();

    // Register for Heart Rate service callback
    HeartRate_Register(heartRateCB);

    // Register for Battery service callback;
    Batt_Register(heartRateBattCB);

    // Setup a delayed profile startup
    tmos_set_event(heartRate_TaskID, START_DEVICE_EVT);
}

/*********************************************************************
 * @fn      HeartRate_ProcessEvent
 *
 * @brief   Heart Rate Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
uint16_t HeartRate_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if(events & SYS_EVENT_MSG)
    {
        uint8_t *pMsg;

        if((pMsg = tmos_msg_receive(heartRate_TaskID)) != NULL)
        {
            heartRate_ProcessTMOSMsg((tmos_event_hdr_t *)pMsg);

            // Release the TMOS message
            tmos_msg_deallocate(pMsg);
        }

        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }

    if(events & START_DEVICE_EVT)
    {
        // Start the Device
        GAPRole_PeripheralStartDevice(heartRate_TaskID, &heartRateBondCB, &heartRatePeripheralCB);

        return (events ^ START_DEVICE_EVT);
    }

    if(events & HEART_PERIODIC_EVT)
    {
        // Perform periodic heart rate task
        heartRatePeriodicTask();

        return (events ^ HEART_PERIODIC_EVT);
    }

    if(events & BATT_PERIODIC_EVT)
    {
        // Perform periodic battery task
        heartRateBattPeriodicTask();

        return (events ^ BATT_PERIODIC_EVT);
    }

    if(events & HEART_CONN_PARAM_UPDATE_EVT)
    {
        // Send param update.
        GAPRole_PeripheralConnParamUpdateReq(gapConnHandle,
                                             DEFAULT_DESIRED_MIN_CONN_INTERVAL,
                                             DEFAULT_DESIRED_MAX_CONN_INTERVAL,
                                             DEFAULT_DESIRED_SLAVE_LATENCY,
                                             DEFAULT_DESIRED_CONN_TIMEOUT,
                                             heartRate_TaskID);

        return (events ^ HEART_CONN_PARAM_UPDATE_EVT);
    }

    // Discard unknown events
    return 0;
}

/*********************************************************************
 * @fn      heartRate_ProcessTMOSMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void heartRate_ProcessTMOSMsg(tmos_event_hdr_t *pMsg)
{
    switch(pMsg->event)
    {
        default:

            break;
    }
}

/*********************************************************************
 * @fn      heartRateMeasNotify
 *
 * @brief   Prepare and send a heart rate measurement notification
 *
 * @return  none
 */
static void heartRateMeasNotify(void)
{
    heartRateMeas.pValue = GATT_bm_alloc(gapConnHandle, ATT_HANDLE_VALUE_NOTI, HEARTRATE_MEAS_LEN, NULL, 0);

    if(heartRateMeas.pValue != NULL)
    {
        uint8_t *p = heartRateMeas.pValue;
        uint8_t  flags = heartRateFlags[heartRateFlagsIdx];

        // build heart rate measurement structure from simulated values
        *p++ = flags;
        *p++ = heartRateBpm;
        if(flags & HEARTRATE_FLAGS_FORMAT_UINT16)
        {
            // additional byte for 16 bit format
            *p++ = 0;
        }
        if(flags & HEARTRATE_FLAGS_ENERGY_EXP)
        {
            *p++ = LO_UINT16(heartRateEnergy);
            *p++ = HI_UINT16(heartRateEnergy);
        }
        if(flags & HEARTRATE_FLAGS_RR)
        {
            *p++ = LO_UINT16(heartRateRrInterval1);
            *p++ = HI_UINT16(heartRateRrInterval1);
            *p++ = LO_UINT16(heartRateRrInterval2);
            *p++ = HI_UINT16(heartRateRrInterval2);
        }
        heartRateMeas.len = (uint8_t)(p - heartRateMeas.pValue);
        if(HeartRate_MeasNotify(gapConnHandle, &heartRateMeas) != SUCCESS)
        {
            GATT_bm_free((gattMsg_t *)&heartRateMeas, ATT_HANDLE_VALUE_NOTI);
        }
        // update simulated values
        heartRateEnergy += ENERGY_INCREMENT;
        if(++heartRateBpm == BPM_MAX)
        {
            heartRateBpm = BPM_DEFAULT;
        }
        heartRateRrInterval1 = heartRateRrInterval2 = HEARTRATE_BPM_TO_RR(heartRateBpm);
    }
}

/*********************************************************************
 * @fn      HeartRateGapStateCB
 *
 * @brief   Notification from the profile of a state change.
 *
 * @param   newState - new state
 *
 * @return  none
 */
static void HeartRateGapStateCB(gapRole_States_t newState, gapRoleEvent_t *pEvent)
{
    // if connected
    if(newState == GAPROLE_CONNECTED)
    {
        if(pEvent->gap.opcode == GAP_LINK_ESTABLISHED_EVENT)
        {
            // Get connection handle
            gapConnHandle = pEvent->linkCmpl.connectionHandle;

            // Set timer to update connection parameters
            tmos_start_task(heartRate_TaskID, HEART_CONN_PARAM_UPDATE_EVT, DEFAULT_CONN_PARAM_UPDATE_DELAY);
        }
    }
    // if disconnected
    else if(gapProfileState == GAPROLE_CONNECTED &&
            newState != GAPROLE_CONNECTED)
    {
        uint8_t advState = TRUE;

        // stop periodic measurement
        tmos_stop_task(heartRate_TaskID, HEART_PERIODIC_EVT);

        // reset client characteristic configuration descriptors
        HeartRate_HandleConnStatusCB(gapConnHandle, LINKDB_STATUS_UPDATE_REMOVED);
        Batt_HandleConnStatusCB(gapConnHandle, LINKDB_STATUS_UPDATE_REMOVED);

        // link loss -- use fast advertising
        GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, DEFAULT_FAST_ADV_INTERVAL);
        GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, DEFAULT_FAST_ADV_INTERVAL);
        GAP_SetParamValue(TGAP_GEN_DISC_ADV_MIN, DEFAULT_FAST_ADV_DURATION);

        // Enable advertising
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &advState);
    }
    // if advertising stopped
    else if(gapProfileState == GAPROLE_ADVERTISING &&
            newState == GAPROLE_WAITING)
    {
        // if advertising stopped by user
        if(heartRateAdvCancelled)
        {
            heartRateAdvCancelled = FALSE;
        }
        // if fast advertising switch to slow
        else if(GAP_GetParamValue(TGAP_DISC_ADV_INT_MIN) == DEFAULT_FAST_ADV_INTERVAL)
        {
            uint8_t advState = TRUE;

            GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, DEFAULT_SLOW_ADV_INTERVAL);
            GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, DEFAULT_SLOW_ADV_INTERVAL);
            GAP_SetParamValue(TGAP_GEN_DISC_ADV_MIN, DEFAULT_SLOW_ADV_DURATION);
            GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &advState);
        }
    }
    // if started
    else if(newState == GAPROLE_STARTED)
    {
        // Set the system ID from the bd addr
        uint8_t systemId[DEVINFO_SYSTEM_ID_LEN];
        GAPRole_GetParameter(GAPROLE_BD_ADDR, systemId);

        // shift three bytes up
        systemId[7] = systemId[5];
        systemId[6] = systemId[4];
        systemId[5] = systemId[3];

        // set middle bytes to zero
        systemId[4] = 0;
        systemId[3] = 0;

        DevInfo_SetParameter(DEVINFO_SYSTEM_ID, DEVINFO_SYSTEM_ID_LEN, systemId);
    }

    gapProfileState = newState;
}

/*********************************************************************
 * @fn      heartRateCB
 *
 * @brief   Callback function for heart rate service.
 *
 * @param   event - service event
 *
 * @return  none
 */
static void heartRateCB(uint8_t event)
{
    if(event == HEARTRATE_MEAS_NOTI_ENABLED)
    {
        // if connected start periodic measurement
        if(gapProfileState == GAPROLE_CONNECTED)
        {
            tmos_start_task(heartRate_TaskID, HEART_PERIODIC_EVT, DEFAULT_HEARTRATE_PERIOD);
        }
    }
    else if(event == HEARTRATE_MEAS_NOTI_DISABLED)
    {
        // stop periodic measurement
        tmos_stop_task(heartRate_TaskID, HEART_PERIODIC_EVT);
    }
    else if(event == HEARTRATE_COMMAND_SET)
    {
        // reset energy expended
        heartRateEnergy = 0;
    }
}

/*********************************************************************
 * @fn      heartRateBattCB
 *
 * @brief   Callback function for battery service.
 *
 * @param   event - service event
 *
 * @return  none
 */
static void heartRateBattCB(uint8_t event)
{
    if(event == BATT_LEVEL_NOTI_ENABLED)
    {
        // if connected start periodic measurement
        if(gapProfileState == GAPROLE_CONNECTED)
        {
            tmos_start_task(heartRate_TaskID, BATT_PERIODIC_EVT, DEFAULT_BATT_PERIOD);
        }
    }
    else if(event == BATT_LEVEL_NOTI_DISABLED)
    {
        // stop periodic measurement
        tmos_stop_task(heartRate_TaskID, BATT_PERIODIC_EVT);
    }
}

/*********************************************************************
 * @fn      heartRatePeriodicTask
 *
 * @brief   Perform a periodic heart rate application task.
 *
 * @param   none
 *
 * @return  none
 */
static void heartRatePeriodicTask(void)
{
    if(gapProfileState == GAPROLE_CONNECTED)
    {
        // send heart rate measurement notification
        heartRateMeasNotify();

        // Restart timer
        tmos_start_task(heartRate_TaskID, HEART_PERIODIC_EVT, DEFAULT_HEARTRATE_PERIOD);
    }
}

/*********************************************************************
 * @fn      heartRateBattPeriodicTask
 *
 * @brief   Perform a periodic task for battery measurement.
 *
 * @param   none
 *
 * @return  none
 */
static void heartRateBattPeriodicTask(void)
{
    if(gapProfileState == GAPROLE_CONNECTED)
    {
        // perform battery level check
        Batt_MeasLevel();

        // Restart timer
        tmos_start_task(heartRate_TaskID, BATT_PERIODIC_EVT, DEFAULT_BATT_PERIOD);
    }
}

/*********************************************************************
*********************************************************************/

/********************************** (C) COPYRIGHT *******************************
 * File Name          : runningsensor.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/12/12
 * Description        : 跑步传感器应用程序，初始化广播连接参数，然后广播，直至连接主机后，定时上传速度
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
#include "runningservice.h"
#include "runningsensor.h"

/*********************************************************************
 * MACROS
 */

#define DISTANCE_TRAVELED(speed)             ((speed) * ((DEFAULT_RSC_PERIOD) / 1000))

/*********************************************************************
 * CONSTANTS
 */

// Fast advertising interval in 625us units
#define DEFAULT_FAST_ADV_INTERVAL            48

// Duration of fast advertising duration in (625us)
#define DEFAULT_FAST_ADV_DURATION            20000

// Duration of advertising to white list members only after link termination
#define DEFAULT_WHITE_LIST_ADV_DURATION      10000

// Slow advertising interval in 625us units
#define DEFAULT_SLOW_ADV_INTERVAL            1600

// Duration of slow advertising duration in (625us) (set to 0 for continuous advertising)
#define DEFAULT_SLOW_ADV_DURATION            20000

// How often to perform sensor's periodic event (625us)
#define DEFAULT_RSC_PERIOD                   2000

// Minimum connection interval (units of 1.25ms)
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL    20

// Maximum connection interval (units of 1.25ms)
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL    160

// Slave latency to use if parameter update request
#define DEFAULT_DESIRED_SLAVE_LATENCY        0

// Supervision timeout value (units of 10ms)
#define DEFAULT_DESIRED_CONN_TIMEOUT         1000

// Sensor sends a slave security request.
#define DEFAULT_PAIRING_PARAMETER            GAPBOND_PAIRING_MODE_INITIATE

// Bonded devices' addresses are stored in white list.
#define USING_WHITE_LIST                     FALSE

// Request bonding.
#define REQ_BONDING                          TRUE

// Time alloted for service discovery before requesting more energy efficient connection parameters
#define SVC_DISC_DELAY                       5000

// After 15 seconds of no user input with notifications off, terminate connection
#define NEGLECT_TIMEOUT_DELAY                24000

// Setting this to true lets this device disconnect after a period of no use.
#define USING_NEGLECT_TIMEOUT                TRUE

// For simulated measurements
#define FLAGS_IDX_MAX                        4

// Values for simulated measurements. units in revolutions in centimeters
#define STRIDE_LENGTH_RUNNING                500
#define STRIDE_LENGTH_WALKING                350

// Cadence = footfalls/minute
#define RUNNING_CADENCE                      180
#define WALKING_CADENCE                      85
// Speed = centimeters/second.
#define RUNNING_SPEED                        180
#define WALKING_SPEED                        50

// Motion that the runner is making
#define STANDING_STILL                       0
#define WALKING_MOTION                       1
#define RUNNING_MOTION                       2
#define MOTION_IDX_MAX                       3

// Length of running measurement value
#define RSC_MEAS_LEN                         10
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
static uint8_t sensor_TaskID; // Task ID for internal task/event processing

static gapRole_States_t gapProfileState = GAPROLE_INIT;

static uint8_t sensorUsingWhiteList = FALSE;

// GAP Profile - Name attribute for SCAN RSP data
static uint8_t scanRspData[] = {
    0x0B, // length of this data
    GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'R',
    'S',
    'C',
    ' ',
    'S',
    'e',
    'n',
    's',
    'o',
    'r',
};

static uint8_t advertData[] = {
    // flags
    0x02,
    GAP_ADTYPE_FLAGS,
    GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,
    // service UUIDs
    0x03,
    GAP_ADTYPE_16BIT_MORE,
    LO_UINT16(RSC_SERV_UUID),
    HI_UINT16(RSC_SERV_UUID),
};

// Device name attribute value
static uint8_t attDeviceName[GAP_DEVICE_NAME_LEN] = "RSC Sensor";

// GAP connection handle
static uint16_t gapConnHandle;

// Running measurement value stored in this structure
static attHandleValueNoti_t sensorMeas;

// Flags for simulated measurements
static const uint8_t sensorFlags[FLAGS_IDX_MAX] = {
    RSC_FLAGS_ALL,
    RSC_FLAGS_AT_REST,
    RSC_FLAGS_STRIDE,
    RSC_FLAGS_DIST
};

// Flag index
static uint8_t sensorFlagsIdx = 0;

// Advertising user-cancelled state
static BOOL sensorAdvCancelled = FALSE;

// RSC parameters
static uint32_t totalDistance = 0;
static uint16_t instSpeed = 0;
static uint8_t  instCadence = 0;
static uint16_t instStrideLength = 0;
static uint8_t  sensorLocation = RSC_NO_SENSOR_LOC;
static uint8_t  motion = STANDING_STILL;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void      sensor_ProcessTMOSMsg(tmos_event_hdr_t *pMsg);
static void      SensorGapStateCB(gapRole_States_t newState, gapRoleEvent_t *pEvent);
static void      sensorPeriodicTask(void);
static void      sensorMeasNotify(void);
static bStatus_t SensorCB(uint8_t event, uint32_t *pNewCummVal);

/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static gapRolesCBs_t runningPeripheralCB = {
    SensorGapStateCB, // Profile State Change Callbacks
    NULL,             // When a valid RSSI is read from controller
    NULL
};

// Bond Manager Callbacks
static gapBondCBs_t runningBondCB = {
    NULL, // Passcode callback
    NULL  // Pairing state callback
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      RunningSensor_Init
 *
 * @brief   Initialization function for the Running App Task.
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
void RunningSensor_Init()
{
    sensor_TaskID = TMOS_ProcessEventRegister(RunningSensor_ProcessEvent);

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
        uint8_t  pairMode = DEFAULT_PAIRING_PARAMETER;
        uint8_t  mitm = FALSE;
        uint8_t  ioCap = GAPBOND_IO_CAP_NO_INPUT_NO_OUTPUT;
        uint8_t  bonding = REQ_BONDING;
        uint8_t  autoSync = USING_WHITE_LIST;

        GAPBondMgr_SetParameter(GAPBOND_PERI_DEFAULT_PASSCODE, sizeof(uint32_t), &passkey);
        GAPBondMgr_SetParameter(GAPBOND_PERI_PAIRING_MODE, sizeof(uint8_t), &pairMode);
        GAPBondMgr_SetParameter(GAPBOND_PERI_MITM_PROTECTION, sizeof(uint8_t), &mitm);
        GAPBondMgr_SetParameter(GAPBOND_PERI_IO_CAPABILITIES, sizeof(uint8_t), &ioCap);
        GAPBondMgr_SetParameter(GAPBOND_PERI_BONDING_ENABLED, sizeof(uint8_t), &bonding);
        GAPBondMgr_SetParameter(GAPBOND_AUTO_SYNC_WL, sizeof(uint8_t), &autoSync);
    }

    // Initialize GATT attributes
    GGS_AddService(GATT_ALL_SERVICES);         // GAP
    GATTServApp_AddService(GATT_ALL_SERVICES); // GATT attributes
    Running_AddService(GATT_ALL_SERVICES);
    DevInfo_AddService();

    // Register for running service callback
    Running_Register(SensorCB);

    // Setup RSC profile data
    {
        uint16_t features = RSC_FULL_SUPPORT;
        uint8_t  sensorLocationCurrent = RSC_SENSOR_LOC_0;
        uint8_t  sensorLocation1 = RSC_SENSOR_LOC_0;
        uint8_t  sensorLocation2 = RSC_SENSOR_LOC_1;
        uint8_t  sensorLocation3 = RSC_SENSOR_LOC_2;
        uint8_t  sensorLocation4 = RSC_SENSOR_LOC_3;

        // Add available sensor locations
        Running_SetParameter(RSC_AVAIL_SENS_LOCS, sizeof(uint8_t), &sensorLocation1);
        Running_SetParameter(RSC_AVAIL_SENS_LOCS, sizeof(uint8_t), &sensorLocation2);
        Running_SetParameter(RSC_AVAIL_SENS_LOCS, sizeof(uint8_t), &sensorLocation3);
        Running_SetParameter(RSC_AVAIL_SENS_LOCS, sizeof(uint8_t), &sensorLocation4);

        // Set sensor location
        Running_SetParameter(RSC_SENS_LOC, sizeof(uint8_t), &sensorLocationCurrent);

        // Support all features
        Running_SetParameter(RSC_FEATURE, sizeof(uint16_t), &features);
    }

    // Setup a delayed profile startup
    tmos_set_event(sensor_TaskID, START_DEVICE_EVT);
}

/*********************************************************************
 * @fn      RunningSensor_ProcessEvent
 *
 * @brief   Running Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
uint16_t RunningSensor_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if(events & SYS_EVENT_MSG)
    {
        uint8_t *pMsg;

        if((pMsg = tmos_msg_receive(sensor_TaskID)) != NULL)
        {
            sensor_ProcessTMOSMsg((tmos_event_hdr_t *)pMsg);

            // Release the TMOS message
            tmos_msg_deallocate(pMsg);
        }

        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }

    if(events & START_DEVICE_EVT)
    {
        // Start the Device
        GAPRole_PeripheralStartDevice(sensor_TaskID, &runningBondCB, &runningPeripheralCB);

        return (events ^ START_DEVICE_EVT);
    }

    if(events & RSC_PERIODIC_EVT)
    {
        // Perform periodic sensor's periodic task
        sensorPeriodicTask();

        return (events ^ RSC_PERIODIC_EVT);
    }

    if(events & RSC_CONN_PARAM_UPDATE_EVT)
    {
        // Send param update.
        GAPRole_PeripheralConnParamUpdateReq(gapConnHandle,
                                             DEFAULT_DESIRED_MIN_CONN_INTERVAL,
                                             DEFAULT_DESIRED_MAX_CONN_INTERVAL,
                                             DEFAULT_DESIRED_SLAVE_LATENCY,
                                             DEFAULT_DESIRED_CONN_TIMEOUT,
                                             sensor_TaskID);

        // Assuming service discovery complete, start neglect timer
        if(USING_NEGLECT_TIMEOUT)
        {
            tmos_start_task(sensor_TaskID, RSC_NEGLECT_TIMEOUT_EVT, NEGLECT_TIMEOUT_DELAY);
        }

        return (events ^ RSC_CONN_PARAM_UPDATE_EVT);
    }

    if(events & RSC_NEGLECT_TIMEOUT_EVT)
    {
        // No user input, terminate connection.
        GAPRole_TerminateLink(gapConnHandle);

        return (events ^ RSC_NEGLECT_TIMEOUT_EVT);
    }

    // Discard unknown events
    return 0;
}

/*********************************************************************
 * @fn      sensor_ProcessTMOSMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void sensor_ProcessTMOSMsg(tmos_event_hdr_t *pMsg)
{
    switch(pMsg->event)
    {
        default:
            break;
    }
}

/*********************************************************************
 * @fn      sensorMeasNotify
 *
 * @brief   Prepare and send a RSC measurement notification
 *
 * @return  none
 */
static void sensorMeasNotify(void)
{
    static uint16_t centimeters = 0;

    sensorMeas.pValue = GATT_bm_alloc(gapConnHandle, ATT_HANDLE_VALUE_NOTI,
                                      RSC_MEAS_LEN, NULL, 0);
    if(sensorMeas.pValue != NULL)
    {
        uint8_t *p = sensorMeas.pValue;
        uint8_t  flags = sensorFlags[sensorFlagsIdx];

        switch(motion)
        {
            case STANDING_STILL:
                instSpeed = instCadence = instStrideLength = STANDING_STILL;
                // 0 for walking bit
                flags = flags & 0xFB; //0b1111 1011
                break;

            case WALKING_MOTION:
                instStrideLength = STRIDE_LENGTH_WALKING;
                instCadence = WALKING_CADENCE;
                instSpeed = WALKING_SPEED;
                // 0 for walking bit
                flags = flags & 0xFB;
                break;

            case RUNNING_MOTION:
                instStrideLength = STRIDE_LENGTH_RUNNING;
                instCadence = RUNNING_CADENCE;
                instSpeed = RUNNING_SPEED;
                // set in 1 for walking bit.
                flags = flags | 0x04;
                break;

            default: // Do nothing
                break;
        }

        // Add distance
        centimeters += (uint16_t)DISTANCE_TRAVELED(instSpeed);

        // If traveled at least a meter
        if(centimeters >= 100)
        {
            // add distance, truncated to meters
            totalDistance += (centimeters / 100);
            // and continue to store the remaining centimeters
            centimeters %= 100;
        }

        // Build RSC measurement structure from simulated values
        // Flags simulate the isPresent bits.
        *p++ = flags;

        //Included regardless of flags.
        *p++ = LO_UINT16(instSpeed);
        *p++ = HI_UINT16(instSpeed);
        *p++ = instCadence;

        if(flags & RSC_FLAGS_STRIDE)
        {
            *p++ = LO_UINT16(instStrideLength);
            *p++ = HI_UINT16(instStrideLength);
        }

        if(flags & RSC_FLAGS_DIST)
        {
            *p++ = BREAK_UINT32(totalDistance, 0);
            *p++ = BREAK_UINT32(totalDistance, 1);
            *p++ = BREAK_UINT32(totalDistance, 2);
            *p++ = BREAK_UINT32(totalDistance, 3);
        }

        // Get length
        sensorMeas.len = (uint8_t)(p - sensorMeas.pValue);

        // set simulated measurement flag index
        if(++sensorFlagsIdx == FLAGS_IDX_MAX)
        {
            sensorFlagsIdx = 0;
        }

        // Send to service to send the notification
        if(Running_MeasNotify(gapConnHandle, &sensorMeas) != SUCCESS)
        {
            GATT_bm_free((gattMsg_t *)&sensorMeas, ATT_HANDLE_VALUE_NOTI);
        }
    }
}

/*********************************************************************
 * @fn      SensorGapStateCB
 *
 * @brief   Notification from the profile of a state change.
 *
 * @param   newState - new state
 *
 * @return  none
 */
static void SensorGapStateCB(gapRole_States_t newState, gapRoleEvent_t *pEvent)
{
    // if connected
    if(newState == GAPROLE_CONNECTED)
    {
        if(pEvent->gap.opcode == GAP_LINK_ESTABLISHED_EVENT)
        {
            // Get connection handle
            gapConnHandle = pEvent->linkCmpl.connectionHandle;

            // Set timer to update connection parameters
            // 5 seconds should allow enough time for Service Discovery by the collector to finish
            tmos_start_task(sensor_TaskID, RSC_CONN_PARAM_UPDATE_EVT, SVC_DISC_DELAY);
        }
    }
    // if disconnected
    else if(gapProfileState == GAPROLE_CONNECTED &&
            newState != GAPROLE_CONNECTED)
    {
        uint8_t advState = TRUE;
        uint8_t bondCount = 0;

        // stop periodic measurement
        tmos_stop_task(sensor_TaskID, RSC_PERIODIC_EVT);

        // reset client characteristic configuration descriptors
        Running_HandleConnStatusCB(gapConnHandle, LINKDB_STATUS_UPDATE_REMOVED);
        //Batt_HandleConnStatusCB( gapConnHandle, LINKDB_STATUS_UPDATE_REMOVED );

        // If not already using white list, begin to do so.
        GAPBondMgr_GetParameter(GAPBOND_BOND_COUNT, &bondCount);

        if(USING_WHITE_LIST && sensorUsingWhiteList == FALSE && bondCount > 0)
        {
            uint8_t value = GAP_FILTER_POLICY_WHITE;

            GAPRole_SetParameter(GAPROLE_ADV_FILTER_POLICY, sizeof(uint8_t), &value);

            sensorUsingWhiteList = TRUE;
        }

        // link loss -- use fast advertising
        GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, DEFAULT_FAST_ADV_INTERVAL);
        GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, DEFAULT_FAST_ADV_INTERVAL);
        GAP_SetParamValue(TGAP_GEN_DISC_ADV_MIN, DEFAULT_WHITE_LIST_ADV_DURATION);

        // Enable advertising
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &advState);
    }
    // if advertising stopped
    else if(gapProfileState == GAPROLE_ADVERTISING &&
            newState == GAPROLE_WAITING)
    {
        uint8_t whiteListUsed = FALSE;

        // if white list is in use, disable to allow general access
        if(sensorUsingWhiteList == TRUE)
        {
            uint8_t value = GAP_FILTER_POLICY_ALL;

            GAPRole_SetParameter(GAPROLE_ADV_FILTER_POLICY, sizeof(uint8_t), &value);

            whiteListUsed = TRUE;

            sensorUsingWhiteList = FALSE;
        }

        // if advertising stopped by user
        if(sensorAdvCancelled)
        {
            sensorAdvCancelled = FALSE;
        }
        // if fast advertising interrupted to cancel white list
        else if(((!USING_WHITE_LIST) || whiteListUsed) &&
                (GAP_GetParamValue(TGAP_DISC_ADV_INT_MIN) == DEFAULT_FAST_ADV_INTERVAL))
        {
            uint8_t advState = TRUE;

            GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, DEFAULT_FAST_ADV_INTERVAL);
            GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, DEFAULT_FAST_ADV_INTERVAL);
            GAP_SetParamValue(TGAP_GEN_DISC_ADV_MIN, DEFAULT_FAST_ADV_DURATION);
            GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &advState);
        }
        // if fast advertising switch to slow or if was already slow but using white list.
        else if(((!USING_WHITE_LIST) || whiteListUsed) ||
                (GAP_GetParamValue(TGAP_DISC_ADV_INT_MIN) == DEFAULT_FAST_ADV_INTERVAL))
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
 * @fn      SensorCB
 *
 * @brief   Callback function for RSC service.
 *
 * @param   event - service event
 * @param   pNewCummVal - pointer to new total distanace data
 *                        if specified by event.  NULL otherwise.
 *
 * @return  SUCCESS if operation successful. FAILURE, otherwise.
 */
static bStatus_t SensorCB(uint8_t event, uint32_t *pNewCummVal)
{
    bStatus_t status = SUCCESS;

    switch(event)
    {
        case RSC_CMD_SET_CUMM_VAL:
            // Cancel neglect timer
            if(USING_NEGLECT_TIMEOUT)
            {
                tmos_stop_task(sensor_TaskID, RSC_NEGLECT_TIMEOUT_EVT);
            }

            // Update total distance
            totalDistance = *pNewCummVal;

            // Start neglect timer
            if(USING_NEGLECT_TIMEOUT)
            {
                tmos_start_task(sensor_TaskID, RSC_NEGLECT_TIMEOUT_EVT, NEGLECT_TIMEOUT_DELAY);
            }
            break;

        case RSC_CMD_START_SENS_CALIB:
            if(sensorFlags[sensorFlagsIdx] != RSC_FLAGS_AT_REST)
            {
                // Induce a calibration error for conformance testing
                status = FAILURE;
            }
            break;

        case RSC_CMD_UPDATE_SENS_LOC:
            // Cancel neglect timer
            if(USING_NEGLECT_TIMEOUT)
            {
                tmos_stop_task(sensor_TaskID, RSC_NEGLECT_TIMEOUT_EVT);
            }

            // Get updated sensor location
            Running_GetParameter(RSC_SENS_LOC, &sensorLocation);

            // Start neglect timer
            if(USING_NEGLECT_TIMEOUT)
            {
                tmos_start_task(sensor_TaskID, RSC_NEGLECT_TIMEOUT_EVT, NEGLECT_TIMEOUT_DELAY);
            }
            break;

        case RSC_MEAS_NOTI_ENABLED:
            // Cancel neglect timer
            if(USING_NEGLECT_TIMEOUT)
            {
                tmos_stop_task(sensor_TaskID, RSC_NEGLECT_TIMEOUT_EVT);
            }

            // if connected start periodic measurement
            if(gapProfileState == GAPROLE_CONNECTED)
            {
                tmos_start_task(sensor_TaskID, RSC_PERIODIC_EVT, DEFAULT_RSC_PERIOD);
            }
            break;

        case RSC_MEAS_NOTI_DISABLED:
            // stop periodic measurement
            tmos_stop_task(sensor_TaskID, RSC_PERIODIC_EVT);

            // Start neglect timer
            if(USING_NEGLECT_TIMEOUT)
            {
                tmos_start_task(sensor_TaskID, RSC_NEGLECT_TIMEOUT_EVT, NEGLECT_TIMEOUT_DELAY);
            }
            break;

        case RSC_READ_ATTR:
        case RSC_WRITE_ATTR:
            if(USING_NEGLECT_TIMEOUT)
            {
                // Cancel neglect timer
                tmos_stop_task(sensor_TaskID, RSC_NEGLECT_TIMEOUT_EVT);

                // Start neglect timer
                tmos_start_task(sensor_TaskID, RSC_NEGLECT_TIMEOUT_EVT, NEGLECT_TIMEOUT_DELAY);
            }
            break;

        default:
            break;
    }

    return (status);
}

/*********************************************************************
 * @fn      sensorPeriodicTask
 *
 * @brief   Perform a periodic running application task.
 *
 * @param   none
 *
 * @return  none
 */
static void sensorPeriodicTask(void)
{
    if(gapProfileState == GAPROLE_CONNECTED)
    {
        // Send speed and cadence measurement notification
        sensorMeasNotify();

        // Restart timer
        tmos_start_task(sensor_TaskID, RSC_PERIODIC_EVT, DEFAULT_RSC_PERIOD);
    }
}

/*********************************************************************
*********************************************************************/

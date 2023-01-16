/********************************** (C) COPYRIGHT *******************************
 * File Name          : runningservice.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/12/12
 * Description        : ÅÜ²½·þÎñ
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
#include "runningservice.h"
/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// Running Service Task Events
#define RSC_CMD_IND_SEND_EVT     0x0001

#define RSC_MEAS_VALUE_POS       2
#define RSC_MEAS_CFG_POS         3
#define RSC_COMMAND_VALUE_POS    9
#define RSC_COMMAND_CFG_POS      10
#define COMMAND_IND_LENGTH       2

#define RSC_CMD_LEN              (3 + RSC_MAX_SENSOR_LOCS)

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// RSC service
const uint8_t runningServUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(RSC_SERV_UUID), HI_UINT16(RSC_SERV_UUID)};

// RSC measurement characteristic
const uint8_t runningMeasUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(RSC_MEAS_UUID), HI_UINT16(RSC_MEAS_UUID)};

// RSC feature characteristic
const uint8_t runningFeatureUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(RSC_FEATURE_UUID), HI_UINT16(RSC_FEATURE_UUID)};

// RSC sensor location characteristic
const uint8_t runningSensLocUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(SENSOR_LOC_UUID), HI_UINT16(SENSOR_LOC_UUID)};

// RSC command characteristic
const uint8_t runningCommandUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(SC_CTRL_PT_UUID), HI_UINT16(SC_CTRL_PT_UUID)};

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

static runningServiceCB_t runningServiceCB = NULL;

static uint8_t supportedSensors = 0;
static BOOL    scOpInProgress = FALSE;

// Variables used in RSC command processing
static uint16_t            connectionHandle;
static attHandleValueInd_t rscCmdInd;

/*********************************************************************
 * Profile Attributes - variables
 */

// TaskID
uint8_t runningService_TaskID = 0;

// RSC Service attribute
static const gattAttrType_t runningService = {ATT_BT_UUID_SIZE, runningServUUID};

// Available sensor locations
static uint8_t supportedSensorLocations[RSC_MAX_SENSOR_LOCS] = {RSC_NO_SENSOR_LOC};

// Running Measurement Characteristic
// Note characteristic value is not stored here
static uint8_t       runningMeasProps = GATT_PROP_NOTIFY;
static uint8_t       runningMeas = 0;
static gattCharCfg_t runningMeasClientCharCfg[GATT_MAX_NUM_CONN];

// Feature Characteristic
static uint8_t  runningFeatureProps = GATT_PROP_READ;
static uint16_t runningFeatures = RSC_NO_SUPPORT;

// Sensor Location Characteristic
static uint8_t runningSensLocProps = GATT_PROP_READ;
static uint8_t runningSensLoc = RSC_NO_SENSOR_LOC;

// Command Characteristic
static uint8_t       runningCommandProps = GATT_PROP_WRITE | GATT_PROP_INDICATE;
static uint8_t       runningCommand = 0;
static gattCharCfg_t runningCommandClientCharCfg[GATT_MAX_NUM_CONN];

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t runningAttrTbl[] = {
    // RSC Service
    {
        {ATT_BT_UUID_SIZE, primaryServiceUUID}, /* type */
        GATT_PERMIT_READ,                       /* permissions */
        0,                                      /* handle */
        (uint8_t *)&runningService              /* pValue */
    },

    // RSC Measurement Declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &runningMeasProps},

    // Measurement Value
    {
        {ATT_BT_UUID_SIZE, runningMeasUUID},
        0,
        0,
        &runningMeas},

    // Measurement Client Characteristic Configuration
    {
        {ATT_BT_UUID_SIZE, clientCharCfgUUID},
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8_t *)&runningMeasClientCharCfg},

    // RSC Feature Declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &runningFeatureProps},

    // Feature Value
    {
        {ATT_BT_UUID_SIZE, runningFeatureUUID},
        GATT_PERMIT_READ,
        0,
        (uint8_t *)&runningFeatures},

    // RSC Sensor Location Declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &runningSensLocProps},

    // Sensor Location Value
    {
        {ATT_BT_UUID_SIZE, runningSensLocUUID},
        GATT_PERMIT_READ,
        0,
        &runningSensLoc},

    // RSC Command Declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &runningCommandProps},

    // Command Value
    {
        {ATT_BT_UUID_SIZE, runningCommandUUID},
        GATT_PERMIT_WRITE,
        0,
        &runningCommand},

    // Command Client Characteristic Configuration
    {
        {ATT_BT_UUID_SIZE, clientCharCfgUUID},
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8_t *)&runningCommandClientCharCfg}
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static uint8_t   running_ReadAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                    uint8_t *pValue, uint16_t *pLen, uint16_t offset,
                                    uint16_t maxLen, uint8_t method);
static bStatus_t running_WriteAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                     uint8_t *pValue, uint16_t len, uint16_t offset,
                                     uint8_t method);

static void running_ProcessTMOSMsg(tmos_event_hdr_t *pMsg);
static void running_ProcessGATTMsg(gattMsgEvent_t *pMsg);
static BOOL running_SensorLocSupported(uint8_t sensorLoc);
static void running_ProcessRSCCmd(uint16_t attrHandle, uint8_t *pValue, uint8_t len);

/*********************************************************************
 * PROFILE CALLBACKS
 */

// RSC Service Callbacks
gattServiceCBs_t runningCBs = {
    running_ReadAttrCB,  // Read callback function pointer
    running_WriteAttrCB, // Write callback function pointer
    NULL                 // Authorization callback function pointer
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      RunningService_Init
 *
 * @brief   collect the TMOS task ID.
 *
 * @param   task_id - TMOS task ID.
 *
 * @return  none
 */
void RunningService_Init(uint8_t task_id)
{
    // Only purpose is to obtain task ID
    runningService_TaskID = task_id;
}

/*********************************************************************
 * @fn      RunningService_ProcessEvent
 *
 * @brief   process incoming event.
 *
 * @param   task_id - TMOS task id.
 *
 * @param   events - event bit(s) set for the task(s)
 *
 * @return  none
 */
uint16_t RunningService_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if(events & SYS_EVENT_MSG)
    {
        uint8_t *pMsg;

        if((pMsg = tmos_msg_receive(runningService_TaskID)) != NULL)
        {
            running_ProcessTMOSMsg((tmos_event_hdr_t *)pMsg);

            // Release the TMOS message
            tmos_msg_deallocate(pMsg);
        }

        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }

    if(events & RSC_CMD_IND_SEND_EVT)
    {
        // Send the indication.
        if(GATT_Indication(connectionHandle, &rscCmdInd, FALSE, runningService_TaskID) != SUCCESS)
        {
            GATT_bm_free((gattMsg_t *)&rscCmdInd, ATT_HANDLE_VALUE_IND);
        }
        // Set Control Point Cfg done
        scOpInProgress = FALSE;

        return (events ^ RSC_CMD_IND_SEND_EVT);
    }

    return 0;
}

/*********************************************************************
 * @fn      running_ProcessTMOSMsg
 *
 * @brief   process incoming TMOS msg.
 *
 * @param   pMsg- pointer to messag to be read.
 *
 * @return  none
 */
void running_ProcessTMOSMsg(tmos_event_hdr_t *pMsg)
{
    switch(pMsg->event)
    {
        case GATT_MSG_EVENT:
            running_ProcessGATTMsg((gattMsgEvent_t *)pMsg);
            break;

        default: // do nothing
            break;
    }
}

/*********************************************************************
 * @fn      running_ProcessGATTMsg
 *
 * @brief   process incoming GATT msg.
 *
 * @param   pMsg- pointer to messag to be read.
 *
 * @return  none
 */
void running_ProcessGATTMsg(gattMsgEvent_t *pMsg)
{
    if(pMsg->method == ATT_HANDLE_VALUE_CFM)
    {
        // Indication receipt was confirmed by the client.
        // This is a placeholder for future.
    }
}

/*********************************************************************
 * @fn      running_SensorLocSupported
 *
 * @brief   check to see if sensor location is supported
 *
 * @param   sensorLoc - location to check for
 *
 * @return  TRUE if supported, FALSE otherwise
 */
static BOOL running_SensorLocSupported(uint8_t sensorLoc)
{
    uint8_t i;

    for(i = 0; i < supportedSensors; i++)
    {
        if(supportedSensorLocations[i] == sensorLoc)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/*********************************************************************
 * @fn      Running_AddService
 *
 * @brief   Initializes the RSC service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  Success or Failure
 */
bStatus_t Running_AddService(uint32_t services)
{
    uint8_t status = SUCCESS;

    // Initialize Client Characteristic Configuration attributes
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, runningMeasClientCharCfg);
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, runningCommandClientCharCfg);

    if(services & RUNNING_SERVICE)
    {
        // Register GATT attribute list and CBs with GATT Server App
        status = GATTServApp_RegisterService(runningAttrTbl,
                                             GATT_NUM_ATTRS(runningAttrTbl),
                                             GATT_MAX_ENCRYPT_KEY_SIZE,
                                             &runningCBs);
    }

    return (status);
}

/*********************************************************************
 * @fn      Running_Register
 *
 * @brief   Register a callback function with the RSC Service.
 *
 * @param   pfnServiceCB - Callback function.
 *
 * @return  None.
 */
void Running_Register(runningServiceCB_t pfnServiceCB)
{
    runningServiceCB = pfnServiceCB;
}

/*********************************************************************
 * @fn      Running_SetParameter
 *
 * @brief   Set a RSC parameter.
 *
 * @param   param - Profile parameter ID
 * @param   len - length of data to right
 * @param   value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16_t will be cast to
 *          uint16_t pointer).
 *
 * @return  bStatus_t
 */
bStatus_t Running_SetParameter(uint8_t param, uint8_t len, void *pValue)
{
    bStatus_t ret = SUCCESS;

    switch(param)
    {
        case RSC_SENS_LOC:
        {
            runningSensLoc = *((uint8_t *)pValue);
        }
        break;

        case RSC_FEATURE:
        {
            runningFeatures = *((uint16_t *)pValue);
        }
        break;

        case RSC_AVAIL_SENS_LOCS:
            if(supportedSensors < RSC_MAX_SENSOR_LOCS)
            {
                supportedSensorLocations[supportedSensors++] = *((uint8_t *)pValue);
            }
            break;

        default:
            ret = INVALIDPARAMETER;
            break;
    }

    return (ret);
}

/*********************************************************************
 * @fn      Running_GetParameter
 *
 * @brief   Get a RSC parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to get.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16_t will be cast to
 *          uint16_t pointer).
 *
 * @return  bStatus_t
 */
bStatus_t Running_GetParameter(uint8_t param, void *value)
{
    bStatus_t ret = SUCCESS;

    switch(param)
    {
        case RSC_FEATURE:
            *((uint16_t *)value) = runningFeatures;
            break;

        case RSC_SENS_LOC:
            *((uint8_t *)value) = runningSensLoc;
            break;

        case RSC_COMMAND:
            *((uint8_t *)value) = runningCommand;
            break;

        default:
            ret = INVALIDPARAMETER;
            break;
    }

    return (ret);
}

/*********************************************************************
 * @fn          Running_MeasNotify
 *
 * @brief       Send a notification containing a RSC
 *              measurement.
 *
 * @param       connHandle - connection handle
 * @param       pNoti - pointer to notification structure
 *
 * @return      Success or Failure
 */
bStatus_t Running_MeasNotify(uint16_t connHandle, attHandleValueNoti_t *pNoti)
{
    uint16_t value = GATTServApp_ReadCharCfg(connHandle, runningMeasClientCharCfg);

    // If notifications enabled
    if(value & GATT_CLIENT_CFG_NOTIFY)
    {
        // Set the handle
        pNoti->handle = runningAttrTbl[RSC_MEAS_VALUE_POS].handle;

        // Send the notification
        return GATT_Notification(connHandle, pNoti, FALSE);
    }

    return bleIncorrectMode;
}

/*********************************************************************
 * @fn      running_ProcessRSCCmd
 *
 * @brief   process an incoming RSC command.
 *
 * @param   attrHandle - attribute handle
 * @param   pValue - pointer to data to be written
 * @param   len - length of data
 *
 * @return  none
 */
static void running_ProcessRSCCmd(uint16_t attrHandle, uint8_t *pValue, uint8_t len)
{
    uint8_t rscStatus = RSC_SUCCESS;

    // See if need to alloc payload for new indication.
    if(rscCmdInd.pValue == NULL)
    {
        rscCmdInd.pValue = GATT_bm_alloc(connectionHandle, ATT_HANDLE_VALUE_IND,
                                         RSC_CMD_LEN, NULL, 0);
        if(rscCmdInd.pValue == NULL)
        {
            return; // failed to alloc space!
        }
    }

    // Set Control Point Cfg in progress
    scOpInProgress = TRUE;

    // Set indication info to be sent out
    rscCmdInd.handle = attrHandle;

    rscCmdInd.len = 3;
    rscCmdInd.pValue[0] = RSC_COMMAND_RSP;
    rscCmdInd.pValue[1] = pValue[0];

    switch(pValue[0])
    {
        case RSC_SET_CUMM_VAL:
            // If total distance is a feature
            if((len <= 5) && (runningFeatures & RSC_TOTAL_DIST_SUPP))
            {
                uint32_t totalDistance;

                // full 32 bits were specified.
                if((len - 1) == 4)
                {
                    totalDistance = BUILD_UINT32(pValue[1], pValue[2], pValue[3], pValue[4]);
                }
                else
                {
                    int i;
                    totalDistance = 0;

                    // In case only lower bits were specified and upper bits remain zero.
                    for(i = 0; i < (len - 1); ++i)
                    {
                        totalDistance += pValue[i + 1] << (i * 8);
                    }
                }

                // Notify app
                if(runningServiceCB != NULL)
                {
                    (*runningServiceCB)(RSC_CMD_SET_CUMM_VAL, &totalDistance);
                }
            }
            else // characteristic not supported.
            {
                rscStatus = RSC_INVALID_PARAMETER;
            }
            break;

        case RSC_START_SENS_CALIB:
            // If sensor calibration is supported
            if((len == 1) && (runningFeatures & RSC_SENSOR_CALIB_SUPP))
            {
                // Notify app
                if(runningServiceCB != NULL)
                {
                    if((*runningServiceCB)(RSC_CMD_START_SENS_CALIB, NULL) != SUCCESS)
                    {
                        // Calibration wasn't started
                        rscStatus = RSC_OPERATION_FAILED;
                    }
                }
            }
            else // characteristic not supported.
            {
                // Send an indication with the list.
                rscStatus = RSC_INVALID_PARAMETER;
            }
            break;

        case RSC_UPDATE_SENS_LOC:
            // If multiple sensor locations is supported and that this is a valid location.
            if((len == 2) &&
               (runningFeatures & RSC_MULTI_SENS_SUPP) &&
               (running_SensorLocSupported(pValue[1]) == TRUE))
            {
                // Update sensor location
                runningSensLoc = pValue[1];

                // Notify app
                if(runningServiceCB != NULL)
                {
                    (*runningServiceCB)(RSC_CMD_UPDATE_SENS_LOC, NULL);
                }
            }
            else // characteristic not supported.
            {
                rscStatus = RSC_INVALID_PARAMETER;
            }
            break;

        case RSC_REQ_SUPP_SENS_LOC:
            // If multiple sensor locations are supported and list requested
            if((len == 1) && (runningFeatures & RSC_MULTI_SENS_SUPP))
            {
                rscCmdInd.len += supportedSensors;
                tmos_memcpy(&(rscCmdInd.pValue[3]), supportedSensorLocations, supportedSensors);
            }
            else // characteristic not supported.
            {
                // Send an indication with the list.
                rscStatus = RSC_INVALID_PARAMETER;
            }
            break;

        default:
            // Send an indication with opcode not suported response
            rscStatus = RSC_OPCODE_NOT_SUPPORTED;
            break;
    }

    // Send indication of operation result
    rscCmdInd.pValue[2] = rscStatus;

    // Ask our task to send out indication
    tmos_set_event(runningService_TaskID, RSC_CMD_IND_SEND_EVT);
}

/*********************************************************************
 * @fn          Running_HandleConnStatusCB
 *
 * @brief       RSC Service link status change handler function.
 *
 * @param       connHandle - connection handle
 * @param       changeType - type of change
 *
 * @return      none
 */
void Running_HandleConnStatusCB(uint16_t connHandle, uint8_t changeType)
{
    // Make sure this is not loopback connection
    if(connHandle != LOOPBACK_CONNHANDLE)
    {
        // Reset Client Char Config if connection has dropped
        if((changeType == LINKDB_STATUS_UPDATE_REMOVED) ||
           ((changeType == LINKDB_STATUS_UPDATE_STATEFLAGS) &&
            (!linkDB_Up(connHandle))))
        {
            GATTServApp_InitCharCfg(connHandle, runningMeasClientCharCfg);
            GATTServApp_InitCharCfg(connHandle, runningCommandClientCharCfg);
        }
    }
}

/*********************************************************************
 * @fn          running_ReadAttrCB
 *
 * @brief       Read an attribute.
 *
 * @param       connHandle - connection message was received on
 * @param       pAttr - pointer to attribute
 * @param       pValue - pointer to data to be read
 * @param       pLen - length of data to be read
 * @param       offset - offset of the first octet to be read
 * @param       maxLen - maximum length of data to be read
 *
 * @return      Success or Failure
 */
static uint8_t running_ReadAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                  uint8_t *pValue, uint16_t *pLen, uint16_t offset,
                                  uint16_t maxLen, uint8_t method)
{
    bStatus_t status = SUCCESS;

    uint16_t uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);

    // Make sure it's not a blob operation (no attributes in the profile are long)
    if(offset > 0)
    {
        return (ATT_ERR_ATTR_NOT_LONG);
    }

    switch(uuid)
    {
        // Read Sensor Location
        case SENSOR_LOC_UUID:
        {
            *pLen = 1;
            pValue[0] = *pAttr->pValue;
        }
        break;

        // Read Running Feature List
        case RSC_FEATURE_UUID:
        {
            *pLen = 2;
            pValue[0] = LO_UINT16(runningFeatures);
            pValue[1] = HI_UINT16(runningFeatures);
        }
        break;

        default:
            status = ATT_ERR_ATTR_NOT_FOUND;
            break;
    }

    // Notify app
    if(runningServiceCB != NULL)
    {
        (*runningServiceCB)(RSC_READ_ATTR, NULL);
    }

    return (status);
}

/*********************************************************************
 * @fn      running_WriteAttrCB
 *
 * @brief   Validate attribute data prior to a write operation
 *
 * @param   connHandle - connection message was received on
 * @param   pAttr - pointer to attribute
 * @param   pValue - pointer to data to be written
 * @param   len - length of data
 * @param   offset - offset of the first octet to be written
 *
 * @return  Success or Failure
 */
static bStatus_t running_WriteAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                     uint8_t *pValue, uint16_t len, uint16_t offset,
                                     uint8_t method)
{
    bStatus_t status = SUCCESS;
    uint16_t  uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);

    if(offset > 0)
    {
        return (ATT_ERR_ATTR_NOT_LONG);
    }

    switch(uuid)
    {
        case SC_CTRL_PT_UUID:
            // Make sure Control Point Cfg is not already in progress
            if(scOpInProgress == TRUE)
            {
                status = RSC_ERR_PROC_IN_PROGRESS;
            }
            // Make sure Control Point Cfg is configured for Indications
            else if((runningCommandClientCharCfg[connHandle].value & GATT_CLIENT_CFG_INDICATE) == FALSE)
            {
                status = RSC_ERR_CCC_IMPROPER_CFG;
            }
            else
            {
                // Process RSC command
                running_ProcessRSCCmd(pAttr->handle, pValue, len);
                connectionHandle = connHandle;
            }
            break;

        // For Measure and Commands CCC
        case GATT_CLIENT_CHAR_CFG_UUID:
            if(pAttr->handle == runningAttrTbl[RSC_COMMAND_CFG_POS].handle)
            {
                status = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len,
                                                        offset, GATT_CLIENT_CFG_INDICATE);
                // Notify app
                if(runningServiceCB != NULL)
                {
                    (*runningServiceCB)(RSC_WRITE_ATTR, NULL);
                }
            }
            else if(pAttr->handle == runningAttrTbl[RSC_MEAS_CFG_POS].handle)
            {
                status = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len,
                                                        offset, GATT_CLIENT_CFG_NOTIFY);
                if(status == SUCCESS)
                {
                    // Notify app
                    if(runningServiceCB != NULL)
                    {
                        uint16_t charCfg = BUILD_UINT16(pValue[0], pValue[1]);

                        (*runningServiceCB)(((charCfg == GATT_CFG_NO_OPERATION) ? RSC_MEAS_NOTI_DISABLED : RSC_MEAS_NOTI_ENABLED), NULL);
                    }
                }
            }
            break;

        default:
            status = ATT_ERR_ATTR_NOT_FOUND;
            break;
    }

    return (status);
}

/*********************************************************************
*********************************************************************/

/********************************** (C) COPYRIGHT *******************************
 * File Name          : heartrateservice.C
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/12/11
 * Description        : 心率服务
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
#include "heartrateservice.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// Position of heart rate measurement value in attribute array
#define HEARTRATE_MEAS_VALUE_POS    2

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// Heart rate service
const uint8_t heartRateServUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(HEARTRATE_SERV_UUID), HI_UINT16(HEARTRATE_SERV_UUID)};

// Heart rate measurement characteristic
const uint8_t heartRateMeasUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(HEARTRATE_MEAS_UUID), HI_UINT16(HEARTRATE_MEAS_UUID)};

// Sensor location characteristic
const uint8_t heartRateSensLocUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(BODY_SENSOR_LOC_UUID), HI_UINT16(BODY_SENSOR_LOC_UUID)};

// Command characteristic
const uint8_t heartRateCommandUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(HEARTRATE_CTRL_PT_UUID), HI_UINT16(HEARTRATE_CTRL_PT_UUID)};

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

static heartRateServiceCB_t heartRateServiceCB;

/*********************************************************************
 * Profile Attributes - variables
 */

// Heart Rate Service attribute
static const gattAttrType_t heartRateService = {ATT_BT_UUID_SIZE, heartRateServUUID};

// Heart Rate Measurement Characteristic
// Note characteristic value is not stored here
static uint8_t       heartRateMeasProps = GATT_PROP_NOTIFY;
static uint8_t       heartRateMeas = 0;
static gattCharCfg_t heartRateMeasClientCharCfg[GATT_MAX_NUM_CONN];

// Sensor Location Characteristic
static uint8_t heartRateSensLocProps = GATT_PROP_READ;
static uint8_t heartRateSensLoc = 0;

// Command Characteristic
static uint8_t heartRateCommandProps = GATT_PROP_WRITE;
static uint8_t heartRateCommand = 0;

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t heartRateAttrTbl[] = {
    // Heart Rate Service
    {
        {ATT_BT_UUID_SIZE, primaryServiceUUID}, /* type */
        GATT_PERMIT_READ,                       /* permissions */
        0,                                      /* handle */
        (uint8_t *)&heartRateService            /* pValue */
    },

    // Heart Rate Measurement Declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &heartRateMeasProps},

    // Heart Rate Measurement Value
    {
        {ATT_BT_UUID_SIZE, heartRateMeasUUID},
        0,
        0,
        &heartRateMeas},

    // Heart Rate Measurement Client Characteristic Configuration
    {
        {ATT_BT_UUID_SIZE, clientCharCfgUUID},
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8_t *)&heartRateMeasClientCharCfg},

    // Sensor Location Declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &heartRateSensLocProps},

    // Sensor Location Value
    {
        {ATT_BT_UUID_SIZE, heartRateSensLocUUID},
        GATT_PERMIT_READ,
        0,
        &heartRateSensLoc},

    // Command Declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &heartRateCommandProps},

    // Command Value
    {
        {ATT_BT_UUID_SIZE, heartRateCommandUUID},
        GATT_PERMIT_WRITE,
        0,
        &heartRateCommand}
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static uint8_t   heartRate_ReadAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                      uint8_t *pValue, uint16_t *pLen, uint16_t offset,
                                      uint16_t maxLen, uint8_t method);
static bStatus_t heartRate_WriteAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                       uint8_t *pValue, uint16_t len, uint16_t offset,
                                       uint8_t method);

/*********************************************************************
 * PROFILE CALLBACKS
 */
// Heart Rate Service Callbacks
gattServiceCBs_t heartRateCBs = {
    heartRate_ReadAttrCB,  // Read callback function pointer
    heartRate_WriteAttrCB, // Write callback function pointer
    NULL                   // Authorization callback function pointer
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      HeartRate_AddService
 *
 * @brief   Initializes the Heart Rate service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  Success or Failure
 */
bStatus_t HeartRate_AddService(uint32_t services)
{
    uint8_t status = SUCCESS;

    // Initialize Client Characteristic Configuration attributes
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, heartRateMeasClientCharCfg);

    if(services & HEARTRATE_SERVICE)
    {
        // Register GATT attribute list and CBs with GATT Server App
        status = GATTServApp_RegisterService(heartRateAttrTbl,
                                             GATT_NUM_ATTRS(heartRateAttrTbl),
                                             GATT_MAX_ENCRYPT_KEY_SIZE,
                                             &heartRateCBs);
    }

    return (status);
}

/*********************************************************************
 * @fn      HeartRate_Register
 *
 * @brief   Register a callback function with the Heart Rate Service.
 *
 * @param   pfnServiceCB - Callback function.
 *
 * @return  None.
 */
extern void HeartRate_Register(heartRateServiceCB_t pfnServiceCB)
{
    heartRateServiceCB = pfnServiceCB;
}

/*********************************************************************
 * @fn      HeartRate_SetParameter
 *
 * @brief   Set a Heart Rate parameter.
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
bStatus_t HeartRate_SetParameter(uint8_t param, uint8_t len, void *value)
{
    bStatus_t ret = SUCCESS;
    switch(param)
    {
        case HEARTRATE_MEAS_CHAR_CFG:
            // Need connection handle
            //heartRateMeasClientCharCfg.value = *((uint16_t*)value);
            break;

        case HEARTRATE_SENS_LOC:
            heartRateSensLoc = *((uint8_t *)value);
            break;

        default:
            ret = INVALIDPARAMETER;
            break;
    }

    return (ret);
}

/*********************************************************************
 * @fn      HeartRate_GetParameter
 *
 * @brief   Get a Heart Rate parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to get.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16_t will be cast to
 *          uint16_t pointer).
 *
 * @return  bStatus_t
 */
bStatus_t HeartRate_GetParameter(uint8_t param, void *value)
{
    bStatus_t ret = SUCCESS;
    switch(param)
    {
        case HEARTRATE_MEAS_CHAR_CFG:
            // Need connection handle
            //*((uint16_t*)value) = heartRateMeasClientCharCfg.value;
            break;

        case HEARTRATE_SENS_LOC:
            *((uint8_t *)value) = heartRateSensLoc;
            break;

        case HEARTRATE_COMMAND:
            *((uint8_t *)value) = heartRateCommand;
            break;

        default:
            ret = INVALIDPARAMETER;
            break;
    }

    return (ret);
}

/*********************************************************************
 * @fn          HeartRate_MeasNotify
 *
 * @brief       Send a notification containing a heart rate
 *              measurement.
 *
 * @param       connHandle - connection handle
 * @param       pNoti - pointer to notification structure
 *
 * @return      Success or Failure
 */
bStatus_t HeartRate_MeasNotify(uint16_t connHandle, attHandleValueNoti_t *pNoti)
{
    uint16_t value = GATTServApp_ReadCharCfg(connHandle, heartRateMeasClientCharCfg);

    // If notifications enabled
    if(value & GATT_CLIENT_CFG_NOTIFY)
    {
        // Set the handle
        pNoti->handle = heartRateAttrTbl[HEARTRATE_MEAS_VALUE_POS].handle;

        // Send the notification
        return GATT_Notification(connHandle, pNoti, FALSE);
    }

    return bleIncorrectMode;
}

/*********************************************************************
 * @fn          heartRate_ReadAttrCB
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
static uint8_t heartRate_ReadAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
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

    if(uuid == BODY_SENSOR_LOC_UUID)
    {
        *pLen = 1;
        pValue[0] = *pAttr->pValue;
    }
    else
    {
        status = ATT_ERR_ATTR_NOT_FOUND;
    }

    return (status);
}

/*********************************************************************
 * @fn      heartRate_WriteAttrCB
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
static bStatus_t heartRate_WriteAttrCB(uint16_t connHandle, gattAttribute_t *pAttr,
                                       uint8_t *pValue, uint16_t len, uint16_t offset,
                                       uint8_t method)
{
    bStatus_t status = SUCCESS;

    uint16_t uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
    switch(uuid)
    {
        case HEARTRATE_CTRL_PT_UUID:
            if(offset > 0)
            {
                status = ATT_ERR_ATTR_NOT_LONG;
            }
            else if(len != 1)
            {
                status = ATT_ERR_INVALID_VALUE_SIZE;
            }
            else if(*pValue != HEARTRATE_COMMAND_ENERGY_EXP)
            {
                status = HEARTRATE_ERR_NOT_SUP;
            }
            else
            {
                *(pAttr->pValue) = pValue[0];

                (*heartRateServiceCB)(HEARTRATE_COMMAND_SET);
            }
            break;

        case GATT_CLIENT_CHAR_CFG_UUID:
            status = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len,
                                                    offset, GATT_CLIENT_CFG_NOTIFY);
            if(status == SUCCESS)
            {
                uint16_t charCfg = BUILD_UINT16(pValue[0], pValue[1]);

                (*heartRateServiceCB)((charCfg == GATT_CFG_NO_OPERATION) ? HEARTRATE_MEAS_NOTI_DISABLED : HEARTRATE_MEAS_NOTI_ENABLED);
            }
            break;

        default:
            status = ATT_ERR_ATTR_NOT_FOUND;
            break;
    }

    return (status);
}

/*********************************************************************
 * @fn          HeartRate_HandleConnStatusCB
 *
 * @brief       Heart Rate Service link status change handler function.
 *
 * @param       connHandle - connection handle
 * @param       changeType - type of change
 *
 * @return      none
 */
void HeartRate_HandleConnStatusCB(uint16_t connHandle, uint8_t changeType)
{
    // Make sure this is not loopback connection
    if(connHandle != LOOPBACK_CONNHANDLE)
    {
        // Reset Client Char Config if connection has dropped
        if((changeType == LINKDB_STATUS_UPDATE_REMOVED) ||
           ((changeType == LINKDB_STATUS_UPDATE_STATEFLAGS) &&
            (!linkDB_Up(connHandle))))
        {
            GATTServApp_InitCharCfg(connHandle, heartRateMeasClientCharCfg);
        }
    }
}

/*********************************************************************
*********************************************************************/

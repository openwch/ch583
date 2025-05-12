/********************************** (C) COPYRIGHT *******************************
 * File Name          : gattprofile.C
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/12/10
 * Description        : �Զ���������ֲ�ͬ���Եķ��񣬰����ɶ�����д��֪ͨ���ɶ���д����ȫ�ɶ�
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "CONFIG.h"
#include "gattprofile.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// Position of simpleProfilechar4 value in attribute array
#define SIMPLEPROFILE_CHAR4_VALUE_POS    11

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// Simple GATT Profile Service UUID: 0xFFF0
CONST uint8 simpleProfileServUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(SIMPLEPROFILE_SERV_UUID), HI_UINT16(SIMPLEPROFILE_SERV_UUID)};

// Characteristic 1 UUID: 0xFFF1
CONST uint8 simpleProfilechar1UUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(SIMPLEPROFILE_CHAR1_UUID), HI_UINT16(SIMPLEPROFILE_CHAR1_UUID)};

// Characteristic 2 UUID: 0xFFF2
CONST uint8 simpleProfilechar2UUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(SIMPLEPROFILE_CHAR2_UUID), HI_UINT16(SIMPLEPROFILE_CHAR2_UUID)};

// Characteristic 3 UUID: 0xFFF3
CONST uint8 simpleProfilechar3UUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(SIMPLEPROFILE_CHAR3_UUID), HI_UINT16(SIMPLEPROFILE_CHAR3_UUID)};

// Characteristic 4 UUID: 0xFFF4
CONST uint8 simpleProfilechar4UUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(SIMPLEPROFILE_CHAR4_UUID), HI_UINT16(SIMPLEPROFILE_CHAR4_UUID)};

// Characteristic 5 UUID: 0xFFF5
CONST uint8 simpleProfilechar5UUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(SIMPLEPROFILE_CHAR5_UUID), HI_UINT16(SIMPLEPROFILE_CHAR5_UUID)};

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

static simpleProfileCBs_t *simpleProfile_AppCBs = NULL;

/*********************************************************************
 * Profile Attributes - variables
 */

// Simple Profile Service attribute
static CONST gattAttrType_t simpleProfileService = {ATT_BT_UUID_SIZE, simpleProfileServUUID};

// Simple Profile Characteristic 1 Properties
static uint8 simpleProfileChar1Props = GATT_PROP_READ | GATT_PROP_WRITE;

// Characteristic 1 Value
static uint8 simpleProfileChar1[SIMPLEPROFILE_CHAR1_LEN] = {0};

// Simple Profile Characteristic 1 User Description
static uint8 simpleProfileChar1UserDesp[] = "Characteristic 1\0";

// Simple Profile Characteristic 2 Properties
static uint8 simpleProfileChar2Props = GATT_PROP_READ;

// Characteristic 2 Value
static uint8 simpleProfileChar2[SIMPLEPROFILE_CHAR2_LEN] = {0};

// Simple Profile Characteristic 2 User Description
static uint8 simpleProfileChar2UserDesp[] = "Characteristic 2\0";

// Simple Profile Characteristic 3 Properties
static uint8 simpleProfileChar3Props = GATT_PROP_WRITE;

// Characteristic 3 Value
static uint8 simpleProfileChar3[SIMPLEPROFILE_CHAR3_LEN] = {0};

// Simple Profile Characteristic 3 User Description
static uint8 simpleProfileChar3UserDesp[] = "Characteristic 3\0";

// Simple Profile Characteristic 4 Properties
static uint8 simpleProfileChar4Props = GATT_PROP_NOTIFY;

// Characteristic 4 Value
static uint8 simpleProfileChar4[SIMPLEPROFILE_CHAR4_LEN] = {0};

// Simple Profile Characteristic 4 Configuration Each client has its own
// instantiation of the Client Characteristic Configuration. Reads of the
// Client Characteristic Configuration only shows the configuration for
// that client and writes only affect the configuration of that client.
static gattCharCfg_t simpleProfileChar4Config[PERIPHERAL_MAX_CONNECTION];

// Simple Profile Characteristic 4 User Description
static uint8 simpleProfileChar4UserDesp[] = "Characteristic 4\0";

// Simple Profile Characteristic 5 Properties
static uint8 simpleProfileChar5Props = GATT_PROP_READ;

// Characteristic 5 Value
static uint8 simpleProfileChar5[SIMPLEPROFILE_CHAR5_LEN] = {0};

// Simple Profile Characteristic 5 User Description
static uint8 simpleProfileChar5UserDesp[] = "Characteristic 5\0";

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t simpleProfileAttrTbl[] = {
    // Simple Profile Service
    {
        {ATT_BT_UUID_SIZE, primaryServiceUUID}, /* type */
        GATT_PERMIT_READ,                       /* permissions */
        0,                                      /* handle */
        (uint8 *)&simpleProfileService          /* pValue */
    },

    // Characteristic 1 Declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &simpleProfileChar1Props},

    // Characteristic Value 1
    {
        {ATT_BT_UUID_SIZE, simpleProfilechar1UUID},
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        simpleProfileChar1},

    // Characteristic 1 User Description
    {
        {ATT_BT_UUID_SIZE, charUserDescUUID},
        GATT_PERMIT_READ,
        0,
        simpleProfileChar1UserDesp},

    // Characteristic 2 Declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &simpleProfileChar2Props},

    // Characteristic Value 2
    {
        {ATT_BT_UUID_SIZE, simpleProfilechar2UUID},
        GATT_PERMIT_READ,
        0,
        simpleProfileChar2},

    // Characteristic 2 User Description
    {
        {ATT_BT_UUID_SIZE, charUserDescUUID},
        GATT_PERMIT_READ,
        0,
        simpleProfileChar2UserDesp},

    // Characteristic 3 Declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &simpleProfileChar3Props},

    // Characteristic Value 3
    {
        {ATT_BT_UUID_SIZE, simpleProfilechar3UUID},
        GATT_PERMIT_WRITE,
        0,
        simpleProfileChar3},

    // Characteristic 3 User Description
    {
        {ATT_BT_UUID_SIZE, charUserDescUUID},
        GATT_PERMIT_READ,
        0,
        simpleProfileChar3UserDesp},

    // Characteristic 4 Declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &simpleProfileChar4Props},

    // Characteristic Value 4
    {
        {ATT_BT_UUID_SIZE, simpleProfilechar4UUID},
        0,
        0,
        simpleProfileChar4},

    // Characteristic 4 configuration
    {
        {ATT_BT_UUID_SIZE, clientCharCfgUUID},
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8 *)simpleProfileChar4Config},

    // Characteristic 4 User Description
    {
        {ATT_BT_UUID_SIZE, charUserDescUUID},
        GATT_PERMIT_READ,
        0,
        simpleProfileChar4UserDesp},

    // Characteristic 5 Declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &simpleProfileChar5Props},

    // Characteristic Value 5
    {
        {ATT_BT_UUID_SIZE, simpleProfilechar5UUID},
        GATT_PERMIT_AUTHEN_READ,
        0,
        simpleProfileChar5},

    // Characteristic 5 User Description
    {
        {ATT_BT_UUID_SIZE, charUserDescUUID},
        GATT_PERMIT_READ,
        0,
        simpleProfileChar5UserDesp},
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static bStatus_t simpleProfile_ReadAttrCB(uint16 connHandle, gattAttribute_t *pAttr,
                                          uint8 *pValue, uint16 *pLen, uint16 offset, uint16 maxLen, uint8 method);
static bStatus_t simpleProfile_WriteAttrCB(uint16 connHandle, gattAttribute_t *pAttr,
                                           uint8 *pValue, uint16 len, uint16 offset, uint8 method);

static void simpleProfile_HandleConnStatusCB(uint16 connHandle, uint8 changeType);

/*********************************************************************
 * PROFILE CALLBACKS
 */
// Simple Profile Service Callbacks
gattServiceCBs_t simpleProfileCBs = {
    simpleProfile_ReadAttrCB,  // Read callback function pointer
    simpleProfile_WriteAttrCB, // Write callback function pointer
    NULL                       // Authorization callback function pointer
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SimpleProfile_AddService
 *
 * @brief   Initializes the Simple Profile service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  Success or Failure
 */
bStatus_t SimpleProfile_AddService(uint32 services)
{
    uint8 status = SUCCESS;

    // Initialize Client Characteristic Configuration attributes
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, simpleProfileChar4Config);

    // Register with Link DB to receive link status change callback
    linkDB_Register(simpleProfile_HandleConnStatusCB);

    if(services & SIMPLEPROFILE_SERVICE)
    {
        // Register GATT attribute list and CBs with GATT Server App
        status = GATTServApp_RegisterService(simpleProfileAttrTbl,
                                             GATT_NUM_ATTRS(simpleProfileAttrTbl),
                                             GATT_MAX_ENCRYPT_KEY_SIZE,
                                             &simpleProfileCBs);
    }

    return (status);
}

/*********************************************************************
 * @fn      SimpleProfile_RegisterAppCBs
 *
 * @brief   Registers the application callback function. Only call
 *          this function once.
 *
 * @param   callbacks - pointer to application callbacks.
 *
 * @return  SUCCESS or bleAlreadyInRequestedMode
 */
bStatus_t SimpleProfile_RegisterAppCBs(simpleProfileCBs_t *appCallbacks)
{
    if(appCallbacks)
    {
        simpleProfile_AppCBs = appCallbacks;

        return (SUCCESS);
    }
    else
    {
        return (bleAlreadyInRequestedMode);
    }
}

/*********************************************************************
 * @fn      SimpleProfile_SetParameter
 *
 * @brief   Set a Simple Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   len - length of data to right
 * @param   value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t SimpleProfile_SetParameter(uint8 param, uint8 len, void *value)
{
    bStatus_t ret = SUCCESS;
    switch(param)
    {
        case SIMPLEPROFILE_CHAR1:
            if(len == SIMPLEPROFILE_CHAR1_LEN)
            {
                tmos_memcpy(simpleProfileChar1, value, SIMPLEPROFILE_CHAR1_LEN);
            }
            else
            {
                ret = bleInvalidRange;
            }
            break;

        case SIMPLEPROFILE_CHAR2:
            if(len == SIMPLEPROFILE_CHAR2_LEN)
            {
                tmos_memcpy(simpleProfileChar2, value, SIMPLEPROFILE_CHAR2_LEN);
            }
            else
            {
                ret = bleInvalidRange;
            }
            break;

        case SIMPLEPROFILE_CHAR3:
            if(len == SIMPLEPROFILE_CHAR3_LEN)
            {
                tmos_memcpy(simpleProfileChar3, value, SIMPLEPROFILE_CHAR3_LEN);
            }
            else
            {
                ret = bleInvalidRange;
            }
            break;

        case SIMPLEPROFILE_CHAR4:
            if(len == SIMPLEPROFILE_CHAR4_LEN)
            {
                tmos_memcpy(simpleProfileChar4, value, SIMPLEPROFILE_CHAR4_LEN);
            }
            else
            {
                ret = bleInvalidRange;
            }
            break;

        case SIMPLEPROFILE_CHAR5:
            if(len == SIMPLEPROFILE_CHAR5_LEN)
            {
                tmos_memcpy(simpleProfileChar5, value, SIMPLEPROFILE_CHAR5_LEN);
            }
            else
            {
                ret = bleInvalidRange;
            }
            break;

        default:
            ret = INVALIDPARAMETER;
            break;
    }

    return (ret);
}

/*********************************************************************
 * @fn      SimpleProfile_GetParameter
 *
 * @brief   Get a Simple Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to put.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t SimpleProfile_GetParameter(uint8 param, void *value)
{
    bStatus_t ret = SUCCESS;
    switch(param)
    {
        case SIMPLEPROFILE_CHAR1:
            tmos_memcpy(value, simpleProfileChar1, SIMPLEPROFILE_CHAR1_LEN);
            break;

        case SIMPLEPROFILE_CHAR2:
            tmos_memcpy(value, simpleProfileChar2, SIMPLEPROFILE_CHAR2_LEN);
            break;

        case SIMPLEPROFILE_CHAR3:
            tmos_memcpy(value, simpleProfileChar3, SIMPLEPROFILE_CHAR3_LEN);
            break;

        case SIMPLEPROFILE_CHAR4:
            tmos_memcpy(value, simpleProfileChar4, SIMPLEPROFILE_CHAR4_LEN);
            break;

        case SIMPLEPROFILE_CHAR5:
            tmos_memcpy(value, simpleProfileChar5, SIMPLEPROFILE_CHAR5_LEN);
            break;

        default:
            ret = INVALIDPARAMETER;
            break;
    }

    return (ret);
}

/*********************************************************************
 * @fn          simpleProfile_Notify
 *
 * @brief       Send a notification containing a heart rate
 *              measurement.
 *
 * @param       connHandle - connection handle
 * @param       pNoti - pointer to notification structure
 *
 * @return      Success or Failure
 */
bStatus_t simpleProfile_Notify(uint16 connHandle, attHandleValueNoti_t *pNoti)
{
    uint16 value = GATTServApp_ReadCharCfg(connHandle, simpleProfileChar4Config);

    // If notifications enabled
    if(value & GATT_CLIENT_CFG_NOTIFY)
    {
        // Set the handle
        pNoti->handle = simpleProfileAttrTbl[SIMPLEPROFILE_CHAR4_VALUE_POS].handle;

        // Send the notification
        return GATT_Notification(connHandle, pNoti, FALSE);
    }
    return bleIncorrectMode;
}

/*********************************************************************
 * @fn          simpleProfile_ReadAttrCB
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
static bStatus_t simpleProfile_ReadAttrCB(uint16 connHandle, gattAttribute_t *pAttr,
                                          uint8 *pValue, uint16 *pLen, uint16 offset, uint16 maxLen, uint8 method)
{
    bStatus_t status = SUCCESS;


    // Make sure it's not a blob operation (no attributes in the profile are long)
    if(offset > 0)
    {
        return (ATT_ERR_ATTR_NOT_LONG);
    }

    if(pAttr->type.len == ATT_BT_UUID_SIZE)
    {
        // 16-bit UUID
        uint16 uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
        switch(uuid)
        {
            // No need for "GATT_SERVICE_UUID" or "GATT_CLIENT_CHAR_CFG_UUID" cases;
            // gattserverapp handles those reads

            // characteristics 1 and 2 have read permissions
            // characteritisc 3 does not have read permissions; therefore it is not
            //   included here
            // characteristic 4 does not have read permissions, but because it
            //   can be sent as a notification, it is included here
            case SIMPLEPROFILE_CHAR1_UUID:
                *pLen = SIMPLEPROFILE_CHAR1_LEN;
                tmos_memcpy(pValue, pAttr->pValue, SIMPLEPROFILE_CHAR1_LEN);
                break;

            case SIMPLEPROFILE_CHAR2_UUID:
                *pLen = SIMPLEPROFILE_CHAR2_LEN;
                tmos_memcpy(pValue, pAttr->pValue, SIMPLEPROFILE_CHAR2_LEN);
                break;

            case SIMPLEPROFILE_CHAR4_UUID:
                *pLen = SIMPLEPROFILE_CHAR4_LEN;
                tmos_memcpy(pValue, pAttr->pValue, SIMPLEPROFILE_CHAR4_LEN);
                break;

            case SIMPLEPROFILE_CHAR5_UUID:
                *pLen = SIMPLEPROFILE_CHAR5_LEN;
                tmos_memcpy(pValue, pAttr->pValue, SIMPLEPROFILE_CHAR5_LEN);
                break;

            default:
                // Should never get here! (characteristics 3 and 4 do not have read permissions)
                *pLen = 0;
                status = ATT_ERR_ATTR_NOT_FOUND;
                break;
        }
    }
    else
    {
        // 128-bit UUID
        *pLen = 0;
        status = ATT_ERR_INVALID_HANDLE;
    }

    return (status);
}

/*********************************************************************
 * @fn      simpleProfile_WriteAttrCB
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
static bStatus_t simpleProfile_WriteAttrCB(uint16 connHandle, gattAttribute_t *pAttr,
                                           uint8 *pValue, uint16 len, uint16 offset, uint8 method)
{
    bStatus_t status = SUCCESS;
    uint8     notifyApp = 0xFF;

    // If attribute permissions require authorization to write, return error
    if(gattPermitAuthorWrite(pAttr->permissions))
    {
        // Insufficient authorization
        return (ATT_ERR_INSUFFICIENT_AUTHOR);
    }

    if(pAttr->type.len == ATT_BT_UUID_SIZE)
    {
        // 16-bit UUID
        uint16 uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
        switch(uuid)
        {
            case SIMPLEPROFILE_CHAR1_UUID:
                //Validate the value
                // Make sure it's not a blob oper
                if(offset == 0)
                {
                    if(len > SIMPLEPROFILE_CHAR1_LEN)
                    {
                        status = ATT_ERR_INVALID_VALUE_SIZE;
                    }
                }
                else
                {
                    status = ATT_ERR_ATTR_NOT_LONG;
                }

                //Write the value
                if(status == SUCCESS)
                {
                    tmos_memcpy(pAttr->pValue, pValue, SIMPLEPROFILE_CHAR1_LEN);
                    notifyApp = SIMPLEPROFILE_CHAR1;
                }
                break;

            case SIMPLEPROFILE_CHAR3_UUID:
                //Validate the value
                // Make sure it's not a blob oper
                if(offset == 0)
                {
                    if(len > SIMPLEPROFILE_CHAR3_LEN)
                    {
                        status = ATT_ERR_INVALID_VALUE_SIZE;
                    }
                }
                else
                {
                    status = ATT_ERR_ATTR_NOT_LONG;
                }

                //Write the value
                if(status == SUCCESS)
                {
                    tmos_memcpy(pAttr->pValue, pValue, SIMPLEPROFILE_CHAR3_LEN);
                    notifyApp = SIMPLEPROFILE_CHAR3;
                }
                break;

            case GATT_CLIENT_CHAR_CFG_UUID:
                status = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len,
                                                        offset, GATT_CLIENT_CFG_NOTIFY);
                break;

            default:
                // Should never get here! (characteristics 2 and 4 do not have write permissions)
                status = ATT_ERR_ATTR_NOT_FOUND;
                break;
        }
    }
    else
    {
        // 128-bit UUID
        status = ATT_ERR_INVALID_HANDLE;
    }

    // If a charactersitic value changed then callback function to notify application of change
    if((notifyApp != 0xFF) && simpleProfile_AppCBs && simpleProfile_AppCBs->pfnSimpleProfileChange)
    {
        simpleProfile_AppCBs->pfnSimpleProfileChange(notifyApp);
    }

    return (status);
}

/*********************************************************************
 * @fn          simpleProfile_HandleConnStatusCB
 *
 * @brief       Simple Profile link status change handler function.
 *
 * @param       connHandle - connection handle
 * @param       changeType - type of change
 *
 * @return      none
 */
static void simpleProfile_HandleConnStatusCB(uint16 connHandle, uint8 changeType)
{
    // Make sure this is not loopback connection
    if(connHandle != LOOPBACK_CONNHANDLE)
    {
        // Reset Client Char Config if connection has dropped
        if((changeType == LINKDB_STATUS_UPDATE_REMOVED) ||
           ((changeType == LINKDB_STATUS_UPDATE_STATEFLAGS) &&
            (!linkDB_Up(connHandle))))
        {
            GATTServApp_InitCharCfg(connHandle, simpleProfileChar4Config);
        }
    }
}

/*********************************************************************
*********************************************************************/

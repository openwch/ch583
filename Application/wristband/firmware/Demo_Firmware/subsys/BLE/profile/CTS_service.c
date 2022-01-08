/********************************** (C) COPYRIGHT *******************************
* File Name          : CTS_service.C
* Author             : WCH
* Version            : V1.0
* Date               : 2018/12/11
* Description        : current time service

*******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "CONFIG.h"
#include "CTS_service.h"
#include <stdbool.h>
#include <stddef.h>

#include "debug/DEBUG.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// Position of heart rate measurement value in attribute array
#define CURRENTTIME_MEAS_VALUE_POS            2

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// Current time service
const uint8 CurrentTimeServUUID[ATT_BT_UUID_SIZE] =
{
  LO_UINT16(CURRENT_TIME_SERV_UUID), HI_UINT16(CURRENT_TIME_SERV_UUID)
};

// Current time
const uint8 CurrentTimeUUID[ATT_BT_UUID_SIZE] =
{
  LO_UINT16(CURRENT_TIME_UUID), HI_UINT16(CURRENT_TIME_UUID)
};

// Local time information
const uint8 LocalTimeInfoUUID[ATT_BT_UUID_SIZE] =
{
  LO_UINT16(LOCAL_TIME_INFO_UUID), HI_UINT16(LOCAL_TIME_INFO_UUID)
};

// Reference time information
const uint8 RefTimeInfoUUID[ATT_BT_UUID_SIZE] =
{
  LO_UINT16(REF_TIME_INFO_UUID), HI_UINT16(REF_TIME_INFO_UUID)
};

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

static CurrentTimeServiceCB_t CurrentTimeServiceCB;

/*********************************************************************
 * Profile Attributes - variables
 */

static const gattAttrType_t CurrentTimeService = { ATT_BT_UUID_SIZE, CurrentTimeServUUID };

static uint8 CurrentTimeProps = GATT_PROP_READ | GATT_PROP_NOTIFY | GATT_PROP_WRITE;
static uint8 CurrentTime[LOCAL_TIMEI_INFO_LEN] = {0};
static gattCharCfg_t CurrentTimeClientCharCfg[GATT_MAX_NUM_CONN];

static uint8 LocalTimeInfoProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint8 LocalTimeInfo[LOCAL_TIMEI_INFO_LEN] = {0};


static uint8 RefTimeInfoProps = GATT_PROP_READ;
static uint8 RefTimeInfo[REF_TIME_INFO_LEN] = {0};

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t CurrentTimeAttrTbl[] =
{
  // Current Time Service
  {
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&CurrentTimeService              /* pValue */
  },

    // Current Time Declaration
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &CurrentTimeProps
    },

    // Current Time Value
    {
      { ATT_BT_UUID_SIZE, CurrentTimeUUID },
      GATT_PERMIT_READ | GATT_PERMIT_WRITE,
      0,
      CurrentTime
    },

    // Current Time Characteristic Configuration
    {
      { ATT_BT_UUID_SIZE, clientCharCfgUUID },
      GATT_PERMIT_READ | GATT_PERMIT_WRITE,
      0,
      (uint8 *) &CurrentTimeClientCharCfg
    },

    // Local Time Information Declaration
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &LocalTimeInfoProps
    },

    // Local Time Information Value
    {
      { ATT_BT_UUID_SIZE, LocalTimeInfoUUID },
      GATT_PERMIT_READ | GATT_PERMIT_WRITE,
      0,
      LocalTimeInfo
    },

    // Reference Time Information Declaration
    {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &RefTimeInfoProps
    },

    // Reference Time Information Value
    {
      { ATT_BT_UUID_SIZE, RefTimeInfoUUID },
      GATT_PERMIT_READ,
      0,
      RefTimeInfo
    },

};


/*********************************************************************
 * LOCAL FUNCTIONS
 */
static uint8 CurrentTime_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                          uint8 *pValue, uint16 *pLen, uint16 offset,
                                          uint16 maxLen, uint8 method );
static bStatus_t CurrentTime_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                           uint8 *pValue, uint16 len, uint16 offset,
                                           uint8 method );
void CurrentTime_HandleConnStatusCB( uint16 connHandle, uint8 changeType );

/*********************************************************************
 * PROFILE CALLBACKS
 */
// Heart Rate Service Callbacks
gattServiceCBs_t CurrentTimeCBs =
{
  CurrentTime_ReadAttrCB,  // Read callback function pointer
  CurrentTime_WriteAttrCB, // Write callback function pointer
  NULL                   // Authorization callback function pointer
};

struct RTC_DateTimeTypeDef
{
    uint16_t year;
    uint8_t month;
    uint8_t date;
    uint8_t week;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

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
bStatus_t CurrentTime_AddService( uint32 services )
{
  uint8 status = SUCCESS;

  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, CurrentTimeClientCharCfg );

  // Register with Link DB to receive link status change callback
  linkDB_Register( CurrentTime_HandleConnStatusCB );

  if ( services & CURRENTTIME_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( CurrentTimeAttrTbl,
                                          GATT_NUM_ATTRS( CurrentTimeAttrTbl ),
                                          GATT_MAX_ENCRYPT_KEY_SIZE,
                                          &CurrentTimeCBs );

  }

  return ( status );
}

/*********************************************************************
 * @fn      CurrentTime_Register
 *
 * @brief   Register a callback function with the Heart Rate Service.
 *
 * @param   pfnServiceCB - Callback function.
 *
 * @return  None.
 */
extern void CurrentTime_Register( CurrentTimeServiceCB_t pfnServiceCB )
{
  CurrentTimeServiceCB = pfnServiceCB;
}

/*********************************************************************
 * @fn      CurrentTime_SetParameter
 *
 * @brief   Set a Heart Rate parameter.
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
bStatus_t CurrentTime_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case CURRENT_TIME_VAL:
        if(len ==  CURRENT_TIME_VAL_LEN)
            tmos_memcpy(CurrentTime, value, len);
      break;

    case LOCAL_TIME_INFO:
        if(len <= LOCAL_TIMEI_INFO_LEN)
            tmos_memcpy(LocalTimeInfo, value, len);
      break;

//    case REF_TIME_INFO:
//        RefTimeInfo = *(uint8_t *)value;
//      break;

    default:
        ret = INVALIDPARAMETER;
      break;
  }
  return ( ret );
}

/*********************************************************************
 * @fn      CurrentTime_GetParameter
 *
 * @brief   Get a Heart Rate parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to get.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t CurrentTime_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case CURRENT_TIME_VAL:
        tmos_memcpy(value, CurrentTime, CURRENT_TIME_VAL_LEN);
    break;

    case LOCAL_TIME_INFO:
        tmos_memcpy(value, LocalTimeInfo, LOCAL_TIMEI_INFO_LEN);
    break;

    case REF_TIME_INFO:
        tmos_memcpy(value, RefTimeInfo, REF_TIME_INFO_LEN);
    break;

    default:
        ret = INVALIDPARAMETER;
      break;
  }

  return ( ret );
}

/*********************************************************************
 * @fn          CurrentTime_MeasNotify
 *
 * @brief       Send a notification containing a heart rate
 *              measurement.
 *
 * @param       connHandle - connection handle
 * @param       pNoti - pointer to notification structure
 *
 * @return      Success or Failure
 */
bStatus_t CurrentTime_MeasNotify( uint16 connHandle, attHandleValueNoti_t *pNoti )
{
  uint16 value = GATTServApp_ReadCharCfg( connHandle, CurrentTimeClientCharCfg );

  // If notifications enabled
  if ( value & GATT_CLIENT_CFG_NOTIFY )
  {
    // Set the handle
    pNoti->handle = CurrentTimeAttrTbl[CURRENTTIME_MEAS_VALUE_POS].handle;

    // Send the notification
    return GATT_Notification( connHandle, pNoti, false );
  }

  return bleIncorrectMode;
}

/*********************************************************************
 * @fn          CurrentTime_ReadAttrCB
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
static uint8 CurrentTime_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                          uint8 *pValue, uint16 *pLen, uint16 offset,
                                          uint16 maxLen, uint8 method )
{
  bStatus_t status = SUCCESS;

  uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);


  // Make sure it's not a blob operation (no attributes in the profile are long)
  if ( offset > 0 )
  {
    return ( ATT_ERR_ATTR_NOT_LONG );
  }

  if ( pAttr->type.len == ATT_BT_UUID_SIZE )
  {
     switch(uuid) {
     case CURRENT_TIME_UUID:
         *pLen = CURRENT_TIME_VAL_LEN;
         tmos_memcpy(pValue, pAttr->pValue, CURRENT_TIME_VAL_LEN);
         break;

     case LOCAL_TIME_INFO_UUID:
         *pLen = LOCAL_TIMEI_INFO_LEN;
          tmos_memcpy(pValue, pAttr->pValue, LOCAL_TIMEI_INFO_LEN);
         break;

     case REF_TIME_INFO_UUID:
         *pLen = CURRENT_TIME_VAL_LEN;
          tmos_memcpy(pValue, pAttr->pValue, REF_TIME_INFO_LEN);
         break;

     default:
         break;
     }
  } else {
      // 128-bit UUID
      *pLen = 0;
      status = ATT_ERR_INVALID_HANDLE;
  }


  return ( status );
}

/*********************************************************************
 * @fn      CurrentTime_WriteAttrCB
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
static bStatus_t CurrentTime_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                           uint8 *pValue, uint16 len, uint16 offset,
                                           uint8 method )
{

  bStatus_t status = SUCCESS;
  uint8 notifyApp = 0xFF;
  uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
  switch ( uuid )
  {
    case CURRENT_TIME_UUID:
        //Validate the value
        // Make sure it's not a blob oper
        if ( offset == 0 )
        {
          if ( len > CURRENT_TIME_VAL_LEN )
          {
            status = ATT_ERR_INVALID_VALUE_SIZE;
          }
        }
        else
        {
          status = ATT_ERR_ATTR_NOT_LONG;
        }

        //Write the value
        if ( status == SUCCESS )
        {
          tmos_memcpy( pAttr->pValue, pValue, CURRENT_TIME_VAL_LEN );
          //timestamp2time
          notifyApp = CURRENT_TIME_VAL;

        }
      break;

    case LOCAL_TIME_INFO_UUID:
        //Validate the value
        // Make sure it's not a blob oper
        if ( offset == 0 )
        {
          if ( len > LOCAL_TIMEI_INFO_LEN )
          {
            status = ATT_ERR_INVALID_VALUE_SIZE;
          }
        }
        else
        {
          status = ATT_ERR_ATTR_NOT_LONG;
        }

        //Write the value
        if ( status == SUCCESS )
        {
          tmos_memcpy( pAttr->pValue, pValue, LOCAL_TIMEI_INFO_LEN );
          notifyApp = LOCAL_TIME_INFO;
        }
      break;

    case GATT_CLIENT_CHAR_CFG_UUID:
      status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
                                               offset, GATT_CLIENT_CFG_NOTIFY );
      break;

    default:
      status = ATT_ERR_ATTR_NOT_FOUND;
      break;
  }

  // If a charactersitic value changed then callback function to notify application of change
  if ( (notifyApp != 0xFF ) && CurrentTimeServiceCB  )
  {
      (*CurrentTimeServiceCB)( notifyApp, pValue, len );
  }

  return ( status );
}

/*********************************************************************
 * @fn          CurrentTime_HandleConnStatusCB
 *
 * @brief       Current Time Service link status change handler function.
 *
 * @param       connHandle - connection handle
 * @param       changeType - type of change
 *
 * @return      none
 */
void CurrentTime_HandleConnStatusCB( uint16 connHandle, uint8 changeType )
{
  // Make sure this is not loopback connection
  if ( connHandle != LOOPBACK_CONNHANDLE )
  {
    // Reset Client Char Config if connection has dropped
    if ( ( changeType == LINKDB_STATUS_UPDATE_REMOVED )      ||
         ( ( changeType == LINKDB_STATUS_UPDATE_STATEFLAGS ) &&
           ( !linkDB_Up( connHandle ) ) ) )
    {
      GATTServApp_InitCharCfg( connHandle, CurrentTimeClientCharCfg );
    }
  }
}


/*********************************************************************
*********************************************************************/

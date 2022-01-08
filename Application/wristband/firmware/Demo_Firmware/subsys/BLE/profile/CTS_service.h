/********************************** (C) COPYRIGHT *******************************
* File Name          : CTS_service.h
* Author             : WCH
* Version            : V1.0
* Date               : 2021/09/30
* Description        :

*******************************************************************************/

#ifndef CTS_SERVICE_H
#define CTS_SERVICE_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */


// Current Time Service Parameters
#define CURRENT_TIME_VAL            0
#define LOCAL_TIME_INFO             1
#define REF_TIME_INFO               2

#define CURRENT_TIME_VAL_LEN        8
#define LOCAL_TIMEI_INFO_LEN        6   //tbc
#define REF_TIME_INFO_LEN           1   //tbc
// Heart Rate Service bit fields
#define CURRENTTIME_SERVICE                   0x00000001


/*********************************************************************
 * TYPEDEFS
 */

// Current Time Service callback function
typedef void (*CurrentTimeServiceCB_t)(uint8 paramID, uint8_t *pValue, uint16_t len);

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * Profile Callbacks
 */


/*********************************************************************
 * API FUNCTIONS
 */

/*
 * CurrentTime_AddService- Initializes the Heart Rate service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 */

extern bStatus_t CurrentTime_AddService( uint32 services );

/*
 * CurrentTime_Register - Register a callback function with the
 *          Current Time Service
 *
 * @param   pfnServiceCB - Callback function.
 */

extern void CurrentTime_Register( CurrentTimeServiceCB_t pfnServiceCB );

/*
 * CurrentTime_SetParameter - Set a Current Time parameter.
 *
 *    param - Profile parameter ID
 *    len - length of data to right
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 */
extern bStatus_t CurrentTime_SetParameter( uint8 param, uint8 len, void *value );

/*
 * CurrentTime_GetParameter - Get a Current Time parameter.
 *
 *    param - Profile parameter ID
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16 will be cast to
 *          uint16 pointer).
 */
extern bStatus_t CurrentTime_GetParameter( uint8 param, void *value );

/*********************************************************************
 * @fn          CurrentTime_MeasNotify
 *
 * @brief       Send a notification containing a current time
 *              measurement.
 *
 * @param       connHandle - connection handle
 * @param       pNoti - pointer to notification structure
 *
 * @return      Success or Failure
 */
extern bStatus_t CurrentTime_MeasNotify( uint16 connHandle, attHandleValueNoti_t *pNoti );

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
extern void CurrentTime_HandleConnStatusCB( uint16 connHandle, uint8 changeType );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* HEARTRATESERVICE_H */

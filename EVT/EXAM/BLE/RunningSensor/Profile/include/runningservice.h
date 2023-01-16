/********************************** (C) COPYRIGHT *******************************
 * File Name          : runningservice.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/12/12
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef RUNNINGSERVICE_H
#define RUNNINGSERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */

#define RUNNING_SERVICE             0x00000001

//ATT Error Codes
#define RSC_ERR_PROC_IN_PROGRESS    0x80
#define RSC_ERR_CCC_IMPROPER_CFG    0x81

#define RSC_SUCCESS                 1
#define RSC_OPCODE_NOT_SUPPORTED    2
#define RSC_INVALID_PARAMETER       3
#define RSC_OPERATION_FAILED        4

//RSC Service Parameters
#define RSC_MEAS                    1
#define RSC_MEAS_CHAR_CFG           2
#define RSC_FEATURE                 3
#define RSC_SENS_LOC                4
#define RSC_COMMAND                 5
#define RSC_COMMAND_CHAR_CFG        6
#define RSC_AVAIL_SENS_LOCS         7

//RSC Fields
#define RSC_INST_STRIDE_PRESENT     0x01
#define RSC_TOTAL_DIST_PRESENT      0x02
#define RSC_WALKING_OR_RUNNING      0x04

//RSC SUPPORTED FEATURES
#define RSC_NO_SUPPORT              0x00
#define RSC_STRIDE_SUPP             0x01
#define RSC_TOTAL_DIST_SUPP         0x02
#define RSC_WALKING_RUNNING_SUPP    0x04
#define RSC_SENSOR_CALIB_SUPP       0x08
#define RSC_MULTI_SENS_SUPP         0x10
#define RSC_FULL_SUPPORT            0x1F

//RSC Censor Locations
#define RSC_NO_SENSOR_LOC           0x00
#define RSC_SENSOR_LOC_0            0x01
#define RSC_SENSOR_LOC_1            0x02
#define RSC_SENSOR_LOC_2            0x04
#define RSC_SENSOR_LOC_3            0x08
#define RSC_ALL_SENSORS             0xFF

//Spec says there are 17 possible.
#define RSC_MAX_SENSOR_LOCS         8

//RSC Commands - arbitrarly assigned
#define RSC_SET_CUMM_VAL            1
#define RSC_START_SENS_CALIB        2
#define RSC_UPDATE_SENS_LOC         3
#define RSC_REQ_SUPP_SENS_LOC       4
#define RSC_COMMAND_RSP             16

// Values for flags
#define RSC_FLAGS_AT_REST           0x00
#define RSC_FLAGS_STRIDE            0x01
#define RSC_FLAGS_DIST              0x02
#define RSC_FLAGS_ALL               0x03

#define DEFAULT_NOTI_INTERVAL       1000        // in milliseconds

#define VALUE_ROLL_OVER             64000       // in milliseconds

// Callback events
#define RSC_CMD_SET_CUMM_VAL        1
#define RSC_CMD_START_SENS_CALIB    2
#define RSC_CMD_UPDATE_SENS_LOC     3
#define RSC_MEAS_NOTI_ENABLED       4
#define RSC_MEAS_NOTI_DISABLED      5
#define RSC_READ_ATTR               6
#define RSC_WRITE_ATTR              7

/*********************************************************************
 * TYPEDEFS
 */

// RSC service callback function
typedef bStatus_t (*runningServiceCB_t)(uint8_t event, uint32_t *pNewCummVal);

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * Profile Callbacks
 */

/*********************************************************************
 * API FUNCTIONS
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
extern void RunningService_Init(uint8_t task_id);

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
extern uint16_t RunningService_ProcessEvent(uint8_t task_id, uint16_t events);

/*
 * @fn      Runninging_AddService
 *
 * @brief   Initializes the RSC service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 * @return  status
 */
extern bStatus_t Running_AddService(uint32_t services);

/*
 * @fn      Running_Register
 *
 * @brief   Register a callback function with the
 *          RSC Service
 *
 * @param   pfnServiceCB - Callback function.
 *
 * @return  none
 */
extern void Running_Register(runningServiceCB_t pfnServiceCB);

/*
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
 * @return  status
 */
extern bStatus_t Running_SetParameter(uint8_t param, uint8_t len, void *value);

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
bStatus_t Running_GetParameter(uint8_t param, void *value);

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
extern bStatus_t Running_MeasNotify(uint16_t connHandle, attHandleValueNoti_t *pNoti);

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
extern void Running_HandleConnStatusCB(uint16_t connHandle, uint8_t changeType);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* RUNNINGSERVICE_H */

/********************************** (C) COPYRIGHT *******************************
 * File Name          : cyclingservice.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/12/10
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef CYCLINGSERVICE_H
#define CYCLINGSERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */

#define CYCLING_SERVICE                0x00000001

//ATT Error Codes
#define CSC_ERR_PROC_IN_PROGRESS       0x80
#define CSC_ERR_CCC_IMPROPER_CFG       0x81

#define CSC_SUCCESS                    1
#define CSC_OPCODE_NOT_SUPPORTED       2
#define CSC_INVALID_PARAMETER          3

//CSC Service Parameters
#define CSC_MEAS                       1
#define CSC_MEAS_CHAR_CFG              2
#define CSC_FEATURE                    3
#define CSC_SENS_LOC                   4
#define CSC_COMMAND                    5
#define CSC_COMMAND_CHAR_CFG           6
#define CSC_AVAIL_SENS_LOCS            7

//CSC Fields
#define CSC_WHEEL_REV_PRESENT          0x01
#define CSC_CRANK_REV_PRESENT          0x02

//CSC SUPPORTED FEATURES
#define CSC_NO_SUPPORT                 0x00
#define CSC_WHEEL_REV_SUPP             0x01
#define CSC_CRANK_REV_SUPP             0x02
#define CSC_MULTI_SENS_SUPP            0x04
#define CSC_FULL_SUPPORT               0x07

//CSC Censor Locations
#define CSC_SENSOR_LOC_OTHER           0
#define CSC_SENSOR_LOC_TOP_OF_SHOE     1
#define CSC_SENSOR_LOC_IN_SHOE         2
#define CSC_SENSOR_LOC_HIP             3
#define CSC_SENSOR_LOC_FRONT_WHEEL     4
#define CSC_SENSOR_LOC_LEFT_CRANK      5
#define CSC_SENSOR_LOC_RIGHT_CRANK     6
#define CSC_SENSOR_LOC_LEFT_PEDAL      7
#define CSC_SENSOR_LOC_RIGHT_PEDAL     8
#define CSC_SENSOR_LOC_FRONT_HUB       9
#define CSC_SENSOR_LOC_REAR_DROPOUT    10
#define CSC_SENSOR_LOC_CHAINSTAY       11
#define CSC_SENSOR_LOC_REAR_WHEEL      12
#define CSC_SENSOR_LOC_REAR_HUB        13

//Spec says there are 14 possible.
#define CSC_MAX_SENSOR_LOCS            14

//CSC Commands
#define CSC_SET_CUMM_VAL               1
#define CSC_START_SENS_CALIB           2
#define CSC_UPDATE_SENS_LOC            3
#define CSC_REQ_SUPP_SENS_LOC          4
#define CSC_COMMAND_RSP                16

// Values for flags
#define CSC_FLAGS_AT_REST              0x00
#define CSC_FLAGS_SPEED                0x01
#define CSC_FLAGS_CADENCE              0x02
#define CSC_FLAGS_SPEED_CADENCE        0x03

#define DEFAULT_NOTI_INTERVAL          1000        // in milliseconds

#define VALUE_ROLL_OVER                64000       // in milliseconds

// Callback events
#define CSC_CMD_SET_CUMM_VAL           1
#define CSC_CMD_START_SENS_CALIB       2
#define CSC_CMD_UPDATE_SENS_LOC        3
#define CSC_MEAS_NOTI_ENABLED          4
#define CSC_MEAS_NOTI_DISABLED         5
#define CSC_READ_ATTR                  6
#define CSC_WRITE_ATTR                 7

/*********************************************************************
 * TYPEDEFS
 */

// CSC service callback function
typedef void (*cyclingServiceCB_t)(uint8_t event, uint32_t *pNewCummVal);

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
 * @fn      CyclingService_Init
 *
 * @brief   collect the TMOS task ID.
 *
 * @return  none
 */
extern void CyclingService_Init(void);

/*********************************************************************
 * @fn      CyclingService_ProcessEvent
 *
 * @brief   process incoming event.
 *
 * @param   task_id - TMOS task id.
 *
 * @param   events - event bit(s) set for the task(s)
 *
 * @return  remaining event bits
 */
extern uint16_t CyclingService_ProcessEvent(uint8_t task_id, uint16_t events);

/*
 * @fn      Cycling_AddService
 *
 * @brief   Initializes the CSC service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 */
extern bStatus_t Cycling_AddService(uint32_t services);

/*
 * @fn      Cycling_Register
 *
 * @brief   Register a callback function with the
 *          CSC Service.
 *
 * @param   pfnServiceCB - Callback function.
 *
 * @return  none
 */

extern void Cycling_Register(cyclingServiceCB_t pfnServiceCB);

/*
 * @fn      Cycling_SetParameter
 *
 * @brief   Set a CSC parameter.
 *
 * @param   param - Profile parameter ID
 * @param   len - length of data to right
 * @param   value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16_t will be cast to
 *          uint16_t pointer).
 */
extern bStatus_t Cycling_SetParameter(uint8_t param, uint8_t len, void *value);

/*********************************************************************
 * @fn      Cycling_GetParameter
 *
 * @brief   Get a CSC parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to get.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate
 *          data type (example: data type of uint16_t will be cast to
 *          uint16_t pointer).
 *
 * @return  bStatus_t
 */
bStatus_t Cycling_GetParameter(uint8_t param, void *value);

/*********************************************************************
 * @fn          Cycling_MeasNotify
 *
 * @brief       Send a notification containing a CSC
 *              measurement.
 *
 * @param       connHandle - connection handle
 * @param       pNoti - pointer to notification structure
 *
 * @return      Success or Failure
 */
extern bStatus_t Cycling_MeasNotify(uint16_t connHandle, attHandleValueNoti_t *pNoti);

/*********************************************************************
 * @fn          Cycling_HandleConnStatusCB
 *
 * @brief       CSC Service link status change handler function.
 *
 * @param       connHandle - connection handle
 * @param       changeType - type of change
 *
 * @return      none
 */
extern void Cycling_HandleConnStatusCB(uint16_t connHandle, uint8_t changeType);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif

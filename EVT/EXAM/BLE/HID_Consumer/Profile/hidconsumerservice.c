/********************************** (C) COPYRIGHT *******************************
 * File Name          : hidconsumerservice.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2018/12/13
 * Description        : 用户控制服务
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "CONFIG.h"
#include "hidconsumerservice.h"
#include "hiddev.h"
#include "battservice.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
#define DEF_DESKTOP_X_MAX    800
#define DEF_DESKTOP_Y_MAX    800
/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// HID service
const uint8_t hidServUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(HID_SERV_UUID), HI_UINT16(HID_SERV_UUID)};

// HID Information characteristic
const uint8_t hidInfoUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(HID_INFORMATION_UUID), HI_UINT16(HID_INFORMATION_UUID)};

// HID Report Map characteristic
const uint8_t hidReportMapUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(REPORT_MAP_UUID), HI_UINT16(REPORT_MAP_UUID)};

// HID Control Point characteristic
const uint8_t hidControlPointUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(HID_CTRL_PT_UUID), HI_UINT16(HID_CTRL_PT_UUID)};

// HID Report characteristic
const uint8_t hidReportUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(REPORT_UUID), HI_UINT16(REPORT_UUID)};

// HID Protocol Mode characteristic
const uint8_t hidProtocolModeUUID[ATT_BT_UUID_SIZE] = {
    LO_UINT16(PROTOCOL_MODE_UUID), HI_UINT16(PROTOCOL_MODE_UUID)};

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

// HID Information characteristic value
static const uint8_t hidInfo[HID_INFORMATION_LEN] = {
    LO_UINT16(0x0111), HI_UINT16(0x0111), // bcdHID (USB HID version)
    0x00,                                 // bCountryCode
    HID_FEATURE_FLAGS                     // Flags
};

// HID Report Map characteristic value
static const uint8_t hidReportMap[] = {
    // Report map for PC
//    0x05,0x0c,
//    0x09,0x01,
//    0xA1,0x01,
//    0x85,0x01,
//    0x15,0x00,
//    0x26,0xff,0x1f,
//    0x19,0x00,
//    0x2A,0xff,0x1f,
//    0x75,0x10,
//    0x95,0x01,
//    0x81,0x00,
//    0xc0

    // Report map for Android
    0x05, 0x0c, // USAGE_PAGE (Consumer Devices)
    0x09, 0x01, // USAGE (Consumer Control)
    0xa1, 0x01, // COLLECTION (Application)
    0x85, 0x01, //   REPORT_ID (1)

    0x15, 0x00, //   LOGICAL_MINIMUM (0)
    0x25, 0x01, //   LOGICAL_MAXIMUM (1)
    0x15, 0x00, //   LOGICAL_MINIMUM (0)
    0x25, 0x01, //   LOGICAL_MAXIMUM (1)
    0x75, 0x08, //   REPORT_SIZE (8)
    0x95, 0x01, //   REPORT_COUNT (1)
    0x09, 0xe9, //   USAGE (Volume Up)
    0x81, 0x06, //   INPUT (Data,Var,Rel)
    0x09, 0xea, //   USAGE (Volume Down)
    0x81, 0x06, //   INPUT (Data,Var,Rel)

    0xc0 // END_COLLECTION
};

// HID report map length
uint16_t hidReportMapLen = sizeof(hidReportMap);

// HID report mapping table
static hidRptMap_t hidRptMap[HID_NUM_REPORTS];

/*********************************************************************
 * Profile Attributes - variables
 */

// HID Service attribute
static const gattAttrType_t hidService = {ATT_BT_UUID_SIZE, hidServUUID};

// Include attribute (Battery service)
static uint16_t include = GATT_INVALID_HANDLE;

// HID Information characteristic
static uint8_t hidInfoProps = GATT_PROP_READ;

// HID Report Map characteristic
static uint8_t hidReportMapProps = GATT_PROP_READ;

// HID External Report Reference Descriptor
static uint8_t hidExtReportRefDesc[ATT_BT_UUID_SIZE] =
    {LO_UINT16(BATT_LEVEL_UUID), HI_UINT16(BATT_LEVEL_UUID)};

// HID Control Point characteristic
static uint8_t hidControlPointProps = GATT_PROP_WRITE_NO_RSP;
static uint8_t hidControlPoint;

// HID Protocol Mode characteristic
static uint8_t hidProtocolModeProps = GATT_PROP_READ | GATT_PROP_WRITE_NO_RSP;
uint8_t        hidProtocolMode = HID_PROTOCOL_MODE_REPORT;

// HID Report characteristic, Consumer input
static uint8_t       hidReportConsumerInProps = GATT_PROP_READ | GATT_PROP_NOTIFY;
static uint8_t       hidReportConsumerIn;
static gattCharCfg_t hidReportConsumerInClientCharCfg[GATT_MAX_NUM_CONN];

// HID Report Reference characteristic descriptor, Consumer input
static uint8_t hidReportRefConsumerIn[HID_REPORT_REF_LEN] =
    {HID_RPT_ID_CONSUMER_IN, HID_REPORT_TYPE_INPUT};

// Feature Report
static uint8_t hidReportFeatureProps = GATT_PROP_READ | GATT_PROP_WRITE;
static uint8_t hidReportFeature;

// HID Report Reference characteristic descriptor, Feature
static uint8_t hidReportRefFeature[HID_REPORT_REF_LEN] =
    {HID_RPT_ID_FEATURE, HID_REPORT_TYPE_FEATURE};

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t hidAttrTbl[] = {
    // HID Service
    {
        {ATT_BT_UUID_SIZE, primaryServiceUUID}, /* type */
        GATT_PERMIT_READ,                       /* permissions */
        0,                                      /* handle */
        (uint8_t *)&hidService                  /* pValue */
    },

    // Included service (battery)
    {
        {ATT_BT_UUID_SIZE, includeUUID},
        GATT_PERMIT_READ,
        0,
        (uint8_t *)&include},

    // HID Information characteristic declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &hidInfoProps},

    // HID Information characteristic
    {
        {ATT_BT_UUID_SIZE, hidInfoUUID},
        GATT_PERMIT_ENCRYPT_READ,
        0,
        (uint8_t *)hidInfo},

    // HID Control Point characteristic declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &hidControlPointProps},

    // HID Control Point characteristic
    {
        {ATT_BT_UUID_SIZE, hidControlPointUUID},
        GATT_PERMIT_ENCRYPT_WRITE,
        0,
        &hidControlPoint},

    // HID Protocol Mode characteristic declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &hidProtocolModeProps},

    // HID Protocol Mode characteristic
    {
        {ATT_BT_UUID_SIZE, hidProtocolModeUUID},
        GATT_PERMIT_ENCRYPT_READ | GATT_PERMIT_ENCRYPT_WRITE,
        0,
        &hidProtocolMode},

    // HID Report Map characteristic declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &hidReportMapProps},

    // HID Report Map characteristic
    {
        {ATT_BT_UUID_SIZE, hidReportMapUUID},
        GATT_PERMIT_ENCRYPT_READ,
        0,
        (uint8_t *)hidReportMap},

    // HID External Report Reference Descriptor
    {
        {ATT_BT_UUID_SIZE, extReportRefUUID},
        GATT_PERMIT_READ,
        0,
        hidExtReportRefDesc

    },

    // HID Report characteristic, Consumer input declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &hidReportConsumerInProps},

    // HID Report characteristic, Consumer input
    {
        {ATT_BT_UUID_SIZE, hidReportUUID},
        GATT_PERMIT_ENCRYPT_READ,
        0,
        &hidReportConsumerIn},

    // HID Report characteristic client characteristic configuration
    {
        {ATT_BT_UUID_SIZE, clientCharCfgUUID},
        GATT_PERMIT_READ | GATT_PERMIT_ENCRYPT_WRITE,
        0,
        (uint8_t *)&hidReportConsumerInClientCharCfg},

    // HID Report Reference characteristic descriptor, Consumer input
    {
        {ATT_BT_UUID_SIZE, reportRefUUID},
        GATT_PERMIT_READ,
        0,
        hidReportRefConsumerIn},

    // Feature Report declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &hidReportFeatureProps},

    // Feature Report
    {
        {ATT_BT_UUID_SIZE, hidReportUUID},
        GATT_PERMIT_ENCRYPT_READ | GATT_PERMIT_ENCRYPT_WRITE,
        0,
        &hidReportFeature},

    // HID Report Reference characteristic descriptor, feature
    {
        {ATT_BT_UUID_SIZE, reportRefUUID},
        GATT_PERMIT_READ,
        0,
        hidReportRefFeature},
};

// Attribute index enumeration-- these indexes match array elements above
enum
{
    HID_SERVICE_IDX,                 // HID Service
    HID_INCLUDED_SERVICE_IDX,        // Included Service
    HID_INFO_DECL_IDX,               // HID Information characteristic declaration
    HID_INFO_IDX,                    // HID Information characteristic
    HID_CONTROL_POINT_DECL_IDX,      // HID Control Point characteristic declaration
    HID_CONTROL_POINT_IDX,           // HID Control Point characteristic
    HID_PROTOCOL_MODE_DECL_IDX,      // HID Protocol Mode characteristic declaration
    HID_PROTOCOL_MODE_IDX,           // HID Protocol Mode characteristic
    HID_REPORT_MAP_DECL_IDX,         // HID Report Map characteristic declaration
    HID_REPORT_MAP_IDX,              // HID Report Map characteristic
    HID_EXT_REPORT_REF_DESC_IDX,     // HID External Report Reference Descriptor
    HID_REPORT_CONSUMER_IN_DECL_IDX, // HID Report characteristic, Consumer input declaration
    HID_REPORT_CONSUMER_IN_IDX,      // HID Report characteristic, Consumer input
    HID_REPORT_CONSUMER_IN_CCCD_IDX, // HID Report characteristic client characteristic configuration
    HID_REPORT_REF_Consumer_IN_IDX,  // HID Report Reference characteristic descriptor, Consumer input
    HID_FEATURE_DECL_IDX,            // Feature Report declaration
    HID_FEATURE_IDX,                 // Feature Report
    HID_REPORT_REF_FEATURE_IDX       // HID Report Reference characteristic descriptor, feature
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * PROFILE CALLBACKS
 */

// Service Callbacks
gattServiceCBs_t hidCBs = {
    HidDev_ReadAttrCB,  // Read callback function pointer
    HidDev_WriteAttrCB, // Write callback function pointer
    NULL                // Authorization callback function pointer
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      Hid_AddService
 *
 * @brief   Initializes the HID Service by registering
 *          GATT attributes with the GATT server.
 *
 * @return  Success or Failure
 */
bStatus_t Hid_AddService(void)
{
    uint8_t status = SUCCESS;

    // Initialize Client Characteristic Configuration attributes
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, hidReportConsumerInClientCharCfg);

    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService(hidAttrTbl, GATT_NUM_ATTRS(hidAttrTbl), GATT_MAX_ENCRYPT_KEY_SIZE, &hidCBs);

    // Set up included service
    Batt_GetParameter(BATT_PARAM_SERVICE_HANDLE,
                      &GATT_INCLUDED_HANDLE(hidAttrTbl, HID_INCLUDED_SERVICE_IDX));

    // Construct map of reports to characteristic handles
    // Each report is uniquely identified via its ID and type

    // Consumer input report
    hidRptMap[0].id = hidReportRefConsumerIn[0];
    hidRptMap[0].type = hidReportRefConsumerIn[1];
    hidRptMap[0].handle = hidAttrTbl[HID_REPORT_CONSUMER_IN_IDX].handle;
    hidRptMap[0].cccdHandle = hidAttrTbl[HID_REPORT_CONSUMER_IN_CCCD_IDX].handle;
    hidRptMap[0].mode = HID_PROTOCOL_MODE_REPORT;

    // Feature report
    hidRptMap[1].id = hidReportRefFeature[0];
    hidRptMap[1].type = hidReportRefFeature[1];
    hidRptMap[1].handle = hidAttrTbl[HID_FEATURE_IDX].handle;
    hidRptMap[1].cccdHandle = 0;
    hidRptMap[1].mode = HID_PROTOCOL_MODE_REPORT;

    // Battery level input report
    Batt_GetParameter(BATT_PARAM_BATT_LEVEL_IN_REPORT, &(hidRptMap[2]));

    // Setup report ID map
    HidDev_RegisterReports(HID_NUM_REPORTS, hidRptMap);

    return (status);
}

/*********************************************************************
 * @fn      Hid_SetParameter
 *
 * @brief   Set a HID Kbd parameter.
 *
 * @param   id - HID report ID.
 * @param   type - HID report type.
 * @param   uuid - attribute uuid.
 * @param   len - length of data to right.
 * @param   pValue - pointer to data to write.  This is dependent on
 *          the input parameters and WILL be cast to the appropriate
 *          data type (example: data type of uint16_t will be cast to
 *          uint16_t pointer).
 *
 * @return  GATT status code.
 */
uint8_t Hid_SetParameter(uint8_t id, uint8_t type, uint16_t uuid, uint8_t len, void *pValue)
{
    bStatus_t ret = SUCCESS;

    switch(uuid)
    {
        case REPORT_UUID:
            if(type == HID_REPORT_TYPE_FEATURE)
            {
                if(len == 1)
                {
                    hidReportFeature = *((uint8_t *)pValue);
                }
                else
                {
                    ret = ATT_ERR_INVALID_VALUE_SIZE;
                }
            }
            else
            {
                ret = ATT_ERR_ATTR_NOT_FOUND;
            }
            break;

        default:
            // ignore the request
            break;
    }

    return (ret);
}

/*********************************************************************
 * @fn      Hid_GetParameter
 *
 * @brief   Get a HID Kbd parameter.
 *
 * @param   id - HID report ID.
 * @param   type - HID report type.
 * @param   uuid - attribute uuid.
 * @param   pLen - length of data to be read
 * @param   pValue - pointer to data to get.  This is dependent on
 *          the input parameters and WILL be cast to the appropriate
 *          data type (example: data type of uint16_t will be cast to
 *          uint16_t pointer).
 *
 * @return  GATT status code.
 */
uint8_t Hid_GetParameter(uint8_t id, uint8_t type, uint16_t uuid, uint16_t *pLen, void *pValue)
{
    switch(uuid)
    {
        case REPORT_UUID:
            if(type == HID_REPORT_TYPE_FEATURE)
            {
                *((uint8_t *)pValue) = hidReportFeature;
                *pLen = 1;
            }
            else
            {
                *pLen = 0;
            }
            break;

        default:
            *pLen = 0;
            break;
    }

    return (SUCCESS);
}

/*********************************************************************
*********************************************************************/

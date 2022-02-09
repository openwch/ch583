/********************************** (C) COPYRIGHT *******************************
* File Name          : app_vendor2_model.h
* Author             : WCH
* Version            : V1.0
* Date               : 2018/11/12
* Description        : 
*******************************************************************************/

#ifndef app_vendor2_model_H
#define app_vendor2_model_H

#ifdef __cplusplus
extern "C"
{
#endif
/******************************************************************************/

#include "MESH_LIB.h"

/******************************************************************************/

#define CID_ALI_GENIE                               0x01A8

#define OP_VENDOR_MESSAGE_ATTR_GET                  BLE_MESH_MODEL_OP_3(0xD0, CID_ALI_GENIE)
#define OP_VENDOR_MESSAGE_ATTR_SET                  BLE_MESH_MODEL_OP_3(0xD1, CID_ALI_GENIE)
#define OP_VENDOR_MESSAGE_ATTR_SET_UNACK            BLE_MESH_MODEL_OP_3(0xD2, CID_ALI_GENIE)
#define OP_VENDOR_MESSAGE_ATTR_STATUS               BLE_MESH_MODEL_OP_3(0xD3, CID_ALI_GENIE)
#define OP_VENDOR_MESSAGE_ATTR_INDICATION           BLE_MESH_MODEL_OP_3(0xD4, CID_ALI_GENIE)
#define OP_VENDOR_MESSAGE_ATTR_CONFIRMATION         BLE_MESH_MODEL_OP_3(0xD5, CID_ALI_GENIE)
#define OP_VENDOR_MESSAGE_ATTR_TRANSPARENT_MSG      BLE_MESH_MODEL_OP_3(0xCF, CID_ALI_GENIE)

#define ALI_SYS_ATTR_TYPE_ERROR                     0x0000
#define ALI_SYS_ATTR_TYPE_VERSION                   0xFF01
#define ALI_SYS_ATTR_TYPE_DEV_FEATURE               0xFF02
#define ALI_SYS_ATTR_TYPE_TOTAL_FLASH_SIZE          0xFF03
#define ALI_SYS_ATTR_TYPE_USED_FLASH_SIZE           0xFF04
#define ALI_SYS_ATTR_TYPE_FREE_FLASH_SIZE           0xFF05
#define ALI_SYS_ATTR_TYPE_ENGINEER_MODE             0xFF06

#define ALI_GEN_ATTR_TYPE_WORK_STATUS               0xF001
#define ALI_GEN_ATTR_TYPE_USER_ID                   0xF002
#define ALI_GEN_ATTR_TYPE_DEVICE_NAME               0xF003
#define ALI_GEN_ATTR_TYPE_MODE_NUMBER               0xF004
#define ALI_GEN_ATTR_TYPE_ONOFF_PLAN                0xF008
#define ALI_GEN_ATTR_TYPE_EVENT_TRIGGER             0xF009
#define ALI_GEN_ATTR_TYPE_EVENT_CLEAR               0xF019
#define ALI_GEN_ATTR_TYPE_SIGNAL_INTENSITY          0xF00A
#define ALI_GEN_ATTR_TYPE_DELTA_VALUE               0xF00B
#define ALI_GEN_ATTR_TYPE_ELEMENT_COUNT             0xF00C
#define ALI_GEN_ATTR_TYPE_ATTR_SWITCH               0xF00D
#define ALI_GEN_ATTR_TYPE_REMOTE_ADDRESS            0xF00E
#define ALI_GEN_ATTR_TYPE_NEARBY_SIGNAL_INTEN       0xF00F
#define ALI_GEN_ATTR_TYPE_SET_VALUE_TIMING          0xF010
#define ALI_GEN_ATTR_TYPE_SET_VALUE_PERIODIC        0xF011
#define ALI_GEN_ATTR_TYPE_DEL_TIMING                0xF012
#define ALI_GEN_ATTR_TYPE_REQ_UPDATE_TIMING         0xF013
#define ALI_GEN_ATTR_TYPE_SETTING_TIMING            0xF01D
#define ALI_GEN_ATTR_TYPE_TIME_ZONE                 0xF01E
#define ALI_GEN_ATTR_TYPE_UNIX_TIMER                0xF01F
#define ALI_GEN_ATTR_TYPE_POWERDOWN_MEM             0xF021

#define ALI_GEN_ATTR_TYPE_CALORIES                  0x06D5
#define ALI_GEN_ATTR_TYPE_SPORTCOUNT                0x0212
#define ALI_GEN_ATTR_TYPE_POWER_STATE               0x0100
#define ALI_GEN_ATTR_TYPE_BRIGHTNESS                0x0121
#define ALI_GEN_ATTR_TYPE_HARDWARE_RESET            0x0023
#define ALI_GEN_ATTR_TYPE_WINDSPEED                 0x010A
#define ALI_GEN_ATTR_TYPE_RGBCOLOR                  0x028E
#define ALI_GEN_ATTR_TYPE_ANGLEAUTO_LR_ONOFF        0x0500
#define ALI_GEN_ATTR_TYPE_ANGLEAUTO_UD_ONOFF        0x0501

#define ALI_TM_SUB_ADDRESS                          0xF000

/** Default number of Indication */
#define CONFIG_INDICATE_NUM (3)

/******************************************************************************/

extern struct bt_mesh_model vnd2_models[1];
extern const struct bt_mesh_model_cb bt_mesh_als_vendor2_model_cb;
/******************************************************************************/
struct bt_mesh_indicate *bt_mesh_ind2_alloc( uint16_t len );
void bt_mesh_indicate2_send( struct bt_mesh_indicate *ind );
void send_led2_indicate(struct indicate_param *param);
void send_angle_auto_LR_indicate(struct indicate_param *param);

void bt_mesh_indicate2_reset(void);



#ifdef __cplusplus
}
#endif

#endif

#include <stddef.h>
#include <sys/byteorder.h>
#include <string.h>
#include <stdlib.h>

#include <CTS.h>
#include "config.h"
#include "HAL/HAL.h"
#include "CTS_service.h"
#include "CH58x_common.h"
#include "HAL/RTC.h"

uint8_t CurrentTime_TaskID = INVALID_TASK_ID;

static void CurrentTime_ProcessTMOSMsg(tmos_event_hdr_t *pMsg) {
    switch (pMsg->event) {
    case GAP_MSG_EVENT: {

        break;
    }

    default:
        break;
    }
}

void cts_init(void){
    char str[] = "+8.00";
    CurrentTime_SetParameter(LOCAL_TIME_INFO, strlen(str) ,str);
}


uint16_t CurrentTime_ProcessEvent(uint8 task_id, uint16 events) {
    if (events & SYS_EVENT_MSG) {
        uint8 *pMsg;

        if ((pMsg = tmos_msg_receive(CurrentTime_TaskID)) != NULL) {
            CurrentTime_ProcessTMOSMsg((tmos_event_hdr_t *) pMsg);
            // Release the TMOS message
            tmos_msg_deallocate(pMsg);
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }

    return 0;
}

/*********************************************************************
 * @fn      CurrentTimeCB
 *
 * @brief   Callback function for heart rate service.
 *
 * @param   event  - notify app
 *          pValue - data
 *          len    - data length
 *
 * @return  none
 */
static void CurrentTimeCB(uint8 paramID, uint8_t *pValue, uint16_t len) {

    switch (paramID) {
    case CURRENT_TIME_VAL: {
        timestamp = pValue[0];
        timestamp |= pValue[1] << 8;
        timestamp |= pValue[2] << 16;
        timestamp |= pValue[3] << 24;
        /* 时间补偿 */
        timestamp += offset_time * 60 * 60;
        /* 调用系统函数 */
        mytime = localtime(&timestamp);
        char str[50];
        strftime(str, 100, "%F %T", mytime);
        PRINT("get time: %s\n", str);

    }
        break;

    case LOCAL_TIME_INFO:{
        uint8_t buf[5];
        float temp;
        tmos_memcpy(buf, pValue, len);
        if(buf[0] == '+'){
            temp = atof(buf+1);

        } else if(buf[0] == '-'){
            temp = -atof(buf+1);

        } else{
        }

        CurrentTime_SetParameter(LOCAL_TIME_INFO, len, pValue);

        /* 时间补偿 */
         timestamp += (temp - offset_time) * 60 * 60;
         offset_time = temp;
         /* 调用系统函数 */
         mytime = localtime(&timestamp);

    }break;

    default:
        break;
    }

}

void CurrentTime_Init(void) {

    CurrentTime_TaskID = TMOS_ProcessEventRegister(CurrentTime_ProcessEvent);

    CurrentTime_AddService(GATT_ALL_SERVICES);

    CurrentTime_Register(CurrentTimeCB);

    cts_init();
}

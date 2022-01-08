#include "battery.h"
#include "battservice.h"
#include "CH58xBLE_LIB.h"
#include <stddef.h>
#include "debug/DEBUG.h"
#include "peripheral.h"
#include "HAL/ADC.h"

// How often to perform BATTERY periodic event
#define DEFAULT_BATTERY_PERIOD              15000
// Battery level is critical when it is less than this %
#define DEFAULT_BATT_CRITICAL_LEVEL           6


static uint8_t Battery_TaskID = INVALID_TASK_ID;

static void BattPeriodicTask( void );


static void Battery_ProcessTMOSMsg(tmos_event_hdr_t *pMsg) {
    switch (pMsg->event) {
    case GAP_MSG_EVENT: {

        break;
    }

    default:
        break;
    }
}

uint16_t Battery_ProcessEvent(uint8 task_id, uint16 events) {
    if (events & SYS_EVENT_MSG) {
        uint8 *pMsg;

        if ((pMsg = tmos_msg_receive(Battery_TaskID)) != NULL) {
            Battery_ProcessTMOSMsg((tmos_event_hdr_t *) pMsg);
            // Release the TMOS message
            tmos_msg_deallocate(pMsg);
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }

    if ( events & BATTERY_PERIODIC_EVT )
    {
      // Perform periodic battery task
        BattPeriodicTask();

      return (events ^ BATTERY_PERIODIC_EVT);
    }


    return 0;
}


/*********************************************************************
 * @fn      BatteryCB
 *
 * @brief   Callback function for battery service.
 *
 * @param   event - service event
 *
 * @return  none
 */
static void BatteryCB(uint8 event)
{
  if (event == BATT_LEVEL_NOTI_ENABLED)
  {
    // if connected start periodic measurement
    if (peripheralConnList.connHandle == GAPROLE_CONNECTED)
    {
        LOG_DEBUG("BLE: start battery notification.");
      tmos_start_task( Battery_TaskID, BATTERY_PERIODIC_EVT, DEFAULT_BATTERY_PERIOD );
    }
  }
  else if (event == BATT_LEVEL_NOTI_DISABLED)
  {
    // stop periodic measurement
      LOG_DEBUG("BLE: stop battery notification.");
    tmos_stop_task( Battery_TaskID, BATTERY_PERIODIC_EVT );
  }
}

/*********************************************************************
 * @fn      BattPeriodicTask
 *
 * @brief   Perform a periodic task for battery measurement.
 *
 * @param   none
 *
 * @return  none
 */
static void BattPeriodicTask( void )
{
  if (peripheralConnList.connHandle == GAPROLE_CONNECTED)
  {
    // perform battery level check
    Batt_MeasLevel( );

    // Restart timer
    tmos_start_task( Battery_TaskID, BATTERY_PERIODIC_EVT, DEFAULT_BATTERY_PERIOD );
  } else {
      // stop periodic measurement
      tmos_stop_task( Battery_TaskID, BATTERY_PERIODIC_EVT );
  }
}

void Battery_Init(void) {

    Battery_TaskID = TMOS_ProcessEventRegister(Battery_ProcessEvent);

    // Setup Battery Characteristic Values
    {
      uint8 critical = DEFAULT_BATT_CRITICAL_LEVEL;
      Batt_SetParameter( BATT_PARAM_CRITICAL_LEVEL, sizeof (uint8 ), &critical );
    }

    Batt_Setup(HAL_ADC_CHANNEL_8, BATT_ADC_LEVEL_2V, BATT_ADC_LEVEL_3V3, NULL, NULL, NULL);

    Batt_AddService();

    Batt_Register(BatteryCB);

}

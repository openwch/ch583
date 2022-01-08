/********************************** (C) COPYRIGHT *******************************
* File Name          : heartrate.c
* Author             : WCH
* Version            : V1.0
* Date               : 2020/08/06
* Description        : 心率计应用程序，初始化广播连接参数，然后广播，直至连接主机后，定时上传心率
            
*******************************************************************************/

/*********************************************************************
 * INCLUDES
 */

#include "CONFIG.h"
#include "CH58x_common.h"
#include "devinfoservice.h"
#include "heartrate.h"
#include "heartrateservice.h"
#include "peripheral.h"
#include "HAL.h"
/*********************************************************************
 * MACROS
 */

// Convert BPM to RR-Interval for data simulation purposes
#define HEARTRATE_BPM_TO_RR(bpm)              ((uint16) 60 * 1024 / (uint16) (bpm))

/*********************************************************************
 * CONSTANTS
 */

// Fast advertising interval in 625us units
#define DEFAULT_FAST_ADV_INTERVAL             32

// Duration of fast advertising duration in (625us)
#define DEFAULT_FAST_ADV_DURATION             30000

// Slow advertising interval in 625us units
#define DEFAULT_SLOW_ADV_INTERVAL             1600

// Duration of slow advertising duration in (625us) (set to 0 for continuous advertising)
#define DEFAULT_SLOW_ADV_DURATION             0

// How often to perform heart rate periodic event
#define DEFAULT_HEARTRATE_PERIOD              1000

// Minimum connection interval (units of 1.25ms)
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL     20

// Maximum connection interval (units of 1.25ms)
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL     160

// Slave latency to use if parameter update request
#define DEFAULT_DESIRED_SLAVE_LATENCY         1

// Supervision timeout value (units of 10ms)
#define DEFAULT_DESIRED_CONN_TIMEOUT          1000

// Delay of start connect paramter update
#define DEFAULT_CONN_PARAM_UPDATE_DELAY       1600

// Battery level is critical when it is less than this %
#define DEFAULT_BATT_CRITICAL_LEVEL           6 

// Battery measurement period in (625us)
#define DEFAULT_BATT_PERIOD                   24000

// Some values used to simulate measurements
#define BPM_DEFAULT                           73
#define BPM_MAX                               80
#define ENERGY_INCREMENT                      10
#define FLAGS_IDX_MAX                         7

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
uint8 heartRate_TaskID;   // Task ID for internal task/event processing


// Heart rate measurement value stored in this structure
static attHandleValueNoti_t heartRateMeas;

// Components of heart rate measurement structure
static uint8 heartRateBpm = BPM_DEFAULT;
static uint16 heartRateEnergy = 0;
static uint16 heartRateRrInterval1 = HEARTRATE_BPM_TO_RR(BPM_DEFAULT);
static uint16 heartRateRrInterval2 = HEARTRATE_BPM_TO_RR(BPM_DEFAULT);

// flags for simulated measurements
//static const uint8 heartRateFlags[FLAGS_IDX_MAX] =
//{
//  HEARTRATE_FLAGS_CONTACT_NOT_SUP,
//  HEARTRATE_FLAGS_CONTACT_NOT_DET,
//  HEARTRATE_FLAGS_CONTACT_DET | HEARTRATE_FLAGS_ENERGY_EXP,
//  HEARTRATE_FLAGS_CONTACT_DET | HEARTRATE_FLAGS_RR,
//  HEARTRATE_FLAGS_CONTACT_DET | HEARTRATE_FLAGS_ENERGY_EXP | HEARTRATE_FLAGS_RR,
//  HEARTRATE_FLAGS_FORMAT_UINT16 | HEARTRATE_FLAGS_CONTACT_DET | HEARTRATE_FLAGS_ENERGY_EXP | HEARTRATE_FLAGS_RR,
//  0x00
//};

static uint8_t heartRateFlags = HEARTRATE_FLAGS_CONTACT_NOT_DET;

//static uint8 heartRateFlagsIdx = 5;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void heartRate_ProcessTMOSMsg( tmos_event_hdr_t *pMsg );
static void heartRatePeriodicTask( void );
static void heartRateMeasNotify(void);
static void heartRateCB(uint8 event);

/*********************************************************************
 * PROFILE CALLBACKS
 */



/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      HeartRate_Init
 *
 * @brief   Initialization function for the Heart Rate App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by TMOS.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void HeartRate_Init( )
{
  heartRate_TaskID = TMOS_ProcessEventRegister(HeartRate_ProcessEvent);


  // Setup the Heart Rate Characteristic Values
  {
    uint8 sensLoc = HEARTRATE_SENS_LOC_WRIST;
    HeartRate_SetParameter( HEARTRATE_SENS_LOC, sizeof ( uint8 ), &sensLoc );
  }
  
  HeartRate_AddService( GATT_ALL_SERVICES );
  
  // Register for Heart Rate service callback
  HeartRate_Register( heartRateCB );
    
  // Setup a delayed profile startup
  tmos_set_event( heartRate_TaskID, START_DEVICE_EVT );
}

/*********************************************************************
 * @fn      HeartRate_ProcessEvent
 *
 * @brief   Heart Rate Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
uint16 HeartRate_ProcessEvent( uint8 task_id, uint16 events )
{
  
  if ( events & SYS_EVENT_MSG )
  {
    uint8 *pMsg;

    if ( (pMsg = tmos_msg_receive( heartRate_TaskID )) != NULL )
    {
      heartRate_ProcessTMOSMsg( (tmos_event_hdr_t *)pMsg );

      // Release the TMOS message
       tmos_msg_deallocate( pMsg );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  if ( events & HEART_PERIODIC_EVT )
  {
    // Perform periodic heart rate task
    heartRatePeriodicTask();
    
    return (events ^ HEART_PERIODIC_EVT);
  }  

  // Discard unknown events
  return 0;
}

/*********************************************************************
 * @fn      heartRate_ProcessTMOSMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void heartRate_ProcessTMOSMsg( tmos_event_hdr_t *pMsg )
{
  switch ( pMsg->event )
  {

      case HEARTBT_MSG_EVT:
          if(pMsg->status) {
              SendMSG_t *msg = (SendMSG_t *)pMsg;
              heartRateBpm = *(uint8_t *)msg->pData;
              heartRateFlags = HEARTRATE_FLAGS_CONTACT_DET;
          } else
              heartRateFlags = HEARTRATE_FLAGS_CONTACT_NOT_DET;
          break;
		default:
			
      break;
  }
}

/*********************************************************************
 * @fn      heartRateMeasNotify
 *
 * @brief   Prepare and send a heart rate measurement notification
 *
 * @return  none
 */
static void heartRateMeasNotify(void)
{
  heartRateMeas.pValue = GATT_bm_alloc(peripheralConnList.connHandle , ATT_HANDLE_VALUE_NOTI, HEARTRATE_MEAS_LEN, NULL,0);
	
  if (heartRateMeas.pValue != NULL)
  {
		uint8 *p = heartRateMeas.pValue;
		uint8 flags = heartRateFlags;//heartRateFlags[heartRateFlagsIdx];
		
		// build heart rate measurement structure from simulated values
		*p++ = flags;
		*p++ = heartRateBpm;
		if (flags & HEARTRATE_FLAGS_FORMAT_UINT16)
		{
			// additional byte for 16 bit format
			*p++ = 0;
		}
		if (flags & HEARTRATE_FLAGS_ENERGY_EXP)
		{
			*p++ = LO_UINT16(heartRateEnergy);
			*p++ = HI_UINT16(heartRateEnergy);
		}
		if (flags & HEARTRATE_FLAGS_RR)
		{
			*p++ = LO_UINT16(heartRateRrInterval1);
			*p++ = HI_UINT16(heartRateRrInterval1);  
			*p++ = LO_UINT16(heartRateRrInterval2);
			*p++ = HI_UINT16(heartRateRrInterval2);  
		}
		heartRateMeas.len = (uint8) (p - heartRateMeas.pValue);
		if( HeartRate_MeasNotify( peripheralConnList.connHandle , &heartRateMeas ) != SUCCESS )
		{
			GATT_bm_free((gattMsg_t *)&heartRateMeas, ATT_HANDLE_VALUE_NOTI);
		}
		// update simulated values 
		heartRateEnergy += ENERGY_INCREMENT;

		heartRateRrInterval1 = heartRateRrInterval2 = HEARTRATE_BPM_TO_RR(heartRateBpm);
	}
}


/*********************************************************************
 * @fn      heartRateCB
 *
 * @brief   Callback function for heart rate service.
 *
 * @param   event - service event
 *
 * @return  none
 */
static void heartRateCB(uint8 event)
{

  if (event == HEARTRATE_MEAS_NOTI_ENABLED)
  {
    // if connected start periodic measurement
    if( peripheralConnList.connHandle != GAP_CONNHANDLE_INIT )
    {
      tmos_start_task( heartRate_TaskID, HEART_PERIODIC_EVT, MS1_TO_SYSTEM_TIME(DEFAULT_HEARTRATE_PERIOD) );
    } 
  }
  else if (event == HEARTRATE_MEAS_NOTI_DISABLED)
  {
    // stop periodic measurement
    tmos_stop_task( heartRate_TaskID, HEART_PERIODIC_EVT );
  }
  else if (event == HEARTRATE_COMMAND_SET)
  {
    // reset energy expended
    heartRateEnergy = 0;
  }
}


/*********************************************************************
 * @fn      heartRatePeriodicTask
 *
 * @brief   Perform a periodic heart rate application task.
 *
 * @param   none
 *
 * @return  none
 */
static void heartRatePeriodicTask( void )
{
  if( peripheralConnList.connHandle != GAP_CONNHANDLE_INIT )
  {
    // send heart rate measurement notification
    heartRateMeasNotify();

    // Restart timer
    tmos_start_task( heartRate_TaskID, HEART_PERIODIC_EVT, MS1_TO_SYSTEM_TIME(DEFAULT_HEARTRATE_PERIOD) );
  }
}



/*********************************************************************
*********************************************************************/

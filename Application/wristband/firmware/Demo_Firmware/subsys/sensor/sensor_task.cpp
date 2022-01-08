#include "MPU9250/MPU9250.h"
#include "MPU9250/sensor_MPU9250.h"
#include "max30102/sensor_max30102.h"
#include "sensor_task.h"

extern "C"{
#include "config.h"
#include <stdint.h>
#include "HAL/HAL.h"
#include "LCD_show/show_task.h"
#include "heartrate.h"
}

uint8_t Sensor_TaskID = INVALID_TASK_ID;

static uint32_t last_time = millis();

static void Sensor_ProcessTMOSMsg( tmos_event_hdr_t *pMsg )
{
  switch ( pMsg->event )
  {
    case GAP_MSG_EVENT:
    {

      break;
    }

    case HEARTBT_MSG_EVT:
        if (pMsg->status) {
            LOG_INFO("MAX30102 Enable");

            static bool isinit = false;
            if(!isinit) {
                if(setupMax30102()){
                    tmos_start_task(Sensor_TaskID, MAX30105_EVENT,MS1_TO_SYSTEM_TIME(2));
                    isinit = true;
                }
            } else {
                WakeMax30102();
                tmos_start_task(Sensor_TaskID, MAX30105_EVENT,MS1_TO_SYSTEM_TIME(2));

            }
        } else {
            LOG_INFO("MAX30102 Disable");
            tmos_stop_task(Sensor_TaskID, MAX30105_EVENT);
            shutMax30102();
        }

    break;

    case STEP_MSG_EVT:
        if (pMsg->status) {
            LOG_INFO("MPU9250 Enable");
            static bool isinit = false;

            if(!isinit){
                if(setupMPU9250()){
                    tmos_start_task(Sensor_TaskID, MPU9250_EVENT,MS1_TO_SYSTEM_TIME(10));
                    isinit = true;
                }
            } else {
                WakeMPU9250();
                tmos_start_task(Sensor_TaskID, MPU9250_EVENT,MS1_TO_SYSTEM_TIME(10));

            }
        } else {
            LOG_INFO("MPU9250 Disbale");
            tmos_stop_task(Sensor_TaskID, MPU9250_EVENT);
            sleepMPU9250();
        }

        break;


    default:
        break;
  }
}


uint16_t Sensor_ProcessEvent( uint8 task_id, uint16 events )
{
    if ( events & SYS_EVENT_MSG ){
      uint8 *pMsg;

      if ( (pMsg = tmos_msg_receive( Sensor_TaskID )) != NULL ){
        Sensor_ProcessTMOSMsg( (tmos_event_hdr_t *)pMsg );
        // Release the TMOS message
        tmos_msg_deallocate( pMsg );
      }
      // return unprocessed events
      return (events ^ SYS_EVENT_MSG);
    }

    if ( events & MPU9250_EVENT ){
        static bool is_light = true;

        readMPU9250();

        int32_t steps_last = -1;
        if(IMU.num_steps != steps_last) {
            OnBoard_SendMsg(show_TaskID, STEP_MSG_EVT, 1, &IMU.num_steps);
        }
        steps_last = IMU.num_steps;

        if(IMU.pitch > -45.0 && IMU.pitch < 45.0 && (IMU.roll > 145.0 || IMU.roll < -135.0)) {
             if(!is_light && (millis() - last_time > 1000) ) {
                 OnBoard_SendMsg(show_TaskID, WRIST_MSG_EVT, 1, NULL);
                 last_time = millis();
                 is_light = true;
             }
        } else {
            if(is_light && (millis() - last_time > 1000)){
                OnBoard_SendMsg(show_TaskID, WRIST_MSG_EVT, 0, NULL);
                last_time = millis();
                is_light = false;
            }
        }

      tmos_start_task(Sensor_TaskID, MPU9250_EVENT, MS1_TO_SYSTEM_TIME(50));
      return (events ^ MPU9250_EVENT);
    }

    if ( events & MAX30105_EVENT ){

        static float beatsPerMinute;
        static int beatAvg;
        static float beatsPerMinute_last;
        static bool isNOsend = false;

        if(read_HeartRate(&beatsPerMinute, &beatAvg)){
          if(beatsPerMinute != beatsPerMinute_last) {
              OnBoard_SendMsg(show_TaskID, HEARTBT_MSG_EVT, 1, &beatAvg);
              OnBoard_SendMsg(heartRate_TaskID, HEARTBT_MSG_EVT, 1, &beatAvg);
              isNOsend = false;
          }
          beatsPerMinute_last = beatsPerMinute;
        } else {
            (!isNOsend)?OnBoard_SendMsg(heartRate_TaskID, HEARTBT_MSG_EVT, 0, NULL):0;
            isNOsend = true;
        }

//        read_SPO2();
        tmos_start_task(Sensor_TaskID, MAX30105_EVENT, MS1_TO_SYSTEM_TIME(15));
        return (events ^ MAX30105_EVENT);
    }

    return 0;
}



void Sensor_Task_Init(void) {

    Sensor_TaskID = TMOS_ProcessEventRegister( Sensor_ProcessEvent );

    if(setupMPU9250())
        tmos_start_task(Sensor_TaskID, MPU9250_EVENT,MS1_TO_SYSTEM_TIME(10));

//    if(setupMax30102()){
//            tmos_start_task(Sensor_TaskID, MAX30105_EVENT,MS1_TO_SYSTEM_TIME(2));
//        }
}



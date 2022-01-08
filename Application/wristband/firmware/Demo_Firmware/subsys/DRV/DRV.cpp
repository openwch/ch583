extern "C"{
    #include "config.h"
}
#include "DRV.h"
#include "DRV2605/Adafruit_DRV2605.h"

Adafruit_DRV2605 drv;

uint8_t DRV_TaskID = INVALID_TASK_ID;

static void DRV_ProcessTMOSMsg( tmos_event_hdr_t *pMsg )
{
  switch ( pMsg->event )
  {
    case GAP_MSG_EVENT:
    {

      break;
    }

    default:
        break;
  }
}

uint16_t DRV_ProcessEvent( uint8 task_id, uint16 events )
{
    static bool isgo = false;

    if ( events & SYS_EVENT_MSG ){
      uint8 *pMsg;

      if ( (pMsg = tmos_msg_receive( DRV_TaskID )) != NULL ){
          DRV_ProcessTMOSMsg( (tmos_event_hdr_t *)pMsg );
        // Release the TMOS message
        tmos_msg_deallocate( pMsg );
      }
      // return unprocessed events
      return (events ^ SYS_EVENT_MSG);
    }

    if(events & DRV_GO_EVT){
        if(!isgo){
//            LOG_INFO("drv go!");
            DRV_ENBALE();
            drv.go();
            isgo = true;
        }
        return (events ^ DRV_GO_EVT);
    }

    if(events & DRV_STOP_EVT){
        if(isgo){
//            LOG_INFO("drv stop");
            drv.stop();
            DRV_DISABLE();
            isgo = false;
        }
        return (events ^ DRV_STOP_EVT);
    }

    if(events & DRV_SHOCK_EVT){
//        LOG_INFO("drv shock!");
        tmos_start_task(DRV_TaskID, DRV_GO_EVT, MS1_TO_SYSTEM_TIME(10));
        tmos_start_task(DRV_TaskID, DRV_STOP_EVT, MS1_TO_SYSTEM_TIME(10 + SHOCK_TIME_MS));
        return (events ^ DRV_SHOCK_EVT);
    }

    if(events & DRV_TEST_EVT){
        if(!isgo){
            tmos_set_event(DRV_TaskID, DRV_GO_EVT);
        }else {
            tmos_set_event(DRV_TaskID, DRV_STOP_EVT);
        }
        tmos_start_task(DRV_TaskID, DRV_TEST_EVT, MS1_TO_SYSTEM_TIME(1000));
        return (events ^ DRV_TEST_EVT);
    }

    return 0;
}

void DRV_Task_Init(void) {

    DRV_TaskID = TMOS_ProcessEventRegister( DRV_ProcessEvent );

    DRV_ENBALE();

    IN_TRIG_NOTUSED();

    drv.begin();

    // I2C trigger by sending 'go' command
    drv.setMode(DRV2605_MODE_INTTRIG); // default, internal trigger when sending GO command

    drv.selectLibrary(1);
    drv.setWaveform(0, 84);  // ramp up medium 1, see datasheet part 11.2
    drv.setWaveform(1, 1);  // strong click 100%, see datasheet part 11.2
    drv.setWaveform(2, 0);  // end of waveforms


//    tmos_start_task(DRV_TaskID, DRV_TEST_EVT, MS1_TO_SYSTEM_TIME(1000));
}

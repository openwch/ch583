#include "CONFIG.h"
#include "dfu_service.h"
#include "dfu/dfu_port.h"

uint8 dfu_taskID = INVALID_TASK_ID;

void OTA_IAPWriteDataCB( unsigned char paramID, unsigned char *p_data, unsigned char w_len );


static OTAProfileCBs_t Peripheral_OTA_IAPProfileCBs =
{
    NULL,
    OTA_IAPWriteDataCB
};

void dfu_Init(void) {
//    dfu_taskID = TMOS_ProcessEventRegister( Peripheral_ProcessEvent );

    OTAProfile_AddService( GATT_ALL_SERVICES );

    //  Register callback with OTAGATTprofile
    OTAProfile_RegisterAppCBs( &Peripheral_OTA_IAPProfileCBs );

}


void OTA_IAPWriteDataCB( unsigned char paramID, unsigned char *p_data, unsigned char w_len )
{
    uint32_t param = 0;
    DFUErrCode ret;
    uint8_t buf[5]={0};
    uint8_t index = 0;

    ARG_UNUSED(paramID);

    if(w_len != sizeof(DFU_DATA_t)) return;

    ret = dfu_data_deal((DFU_DATA_t *)p_data, &param );

    if( unlikely(ret != DFU_NO_ERR) )
    {
      LOG_INFO("dfu cmd error");
      buf[index++] = ret;
    }

    if( param )
    {
      LOG_INFO("dfu param set");
      buf[index++] = (uint8_t) (param >> 24 & 0xff);
      buf[index++] = (uint8_t) (param >> 16 & 0xff);
      buf[index++] = (uint8_t) (param >> 8 & 0xff);
      buf[index++] = (uint8_t) (param >> 0 & 0xff);
      param = 0;
    }
    if(index){
        OTAProfile_SendData( OTAPROFILE_CHAR, buf, (index - 1));
    }

}

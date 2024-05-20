/********************************** (C) COPYRIGHT *******************************
* File Name : TouchKey_CFG.h
* Author             : WCH
* Version            : V1.0
* Date               : 2023/10/17
* Description        : 触摸按键参数配置头文件
* ********************************************************************************
* Copyright(c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention : This software(modified or not) and binary are used for
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#ifndef __TOUCH_KEY_CFG_H__
#define __TOUCH_KEY_CFG_H__
#ifdef __cplusplus
extern "C" {
#endif

typedef enum _TKY_QUEUE_ID
{
TKY_QUEUE_0 = 0,
TKY_QUEUE_1,
TKY_QUEUE_2,
TKY_QUEUE_3,
TKY_QUEUE_4,
TKY_QUEUE_5,
TKY_QUEUE_6,
TKY_QUEUE_7,
TKY_QUEUE_8,
TKY_QUEUE_9,
TKY_QUEUE_10,
TKY_QUEUE_11,

/* Add new above this */
TKY_QUEUE_END
} TKY_QUEUE_ID;

#define TKY_FILTER_MODE     			            3
#define TKY_FILTER_GRADE     			            6
#define TKY_BASE_REFRESH_ON_PRESS     			    0
#define TKY_BASE_UP_REFRESH_DOUBLE     			    0
#define TKY_BASE_DOWN_REFRESH_SLOW     			    0
#define TKY_BASE_REFRESH_SAMPLE_NUM     			1000
#define TKY_SHIELD_EN     			                0
#define TKY_SINGLE_PRESS_MODE     			        1
#define TKY_MAX_QUEUE_NUM     			            8

#define GEN_TKY_CH_INIT(qNum,chNum,chTime,disChTime,slpmode,chBaseline,trs,trs2) \
    {\
     .queueNum=qNum,.channelNum=chNum,.chargeTime=chTime,.disChargeTime=disChTime,\
     .sleepStatus=slpmode,\
     .baseLine = chBaseline,\
     .threshold=trs,\
     .threshold2=trs2\
    }

#define TKY_CHS_INIT \
	GEN_TKY_CH_INIT(TKY_QUEUE_0,6,2,3,0,3337,61,44),\
    GEN_TKY_CH_INIT(TKY_QUEUE_1,4,2,3,0,3459,66,49),\
    GEN_TKY_CH_INIT(TKY_QUEUE_2,5,2,3,0,3661,69,41),\
    GEN_TKY_CH_INIT(TKY_QUEUE_3,7,2,3,0,3323,62,45),\
    GEN_TKY_CH_INIT(TKY_QUEUE_4,9,2,3,0,3477,63,46),\
    GEN_TKY_CH_INIT(TKY_QUEUE_5,1,2,3,0,3646,62,43),\
    GEN_TKY_CH_INIT(TKY_QUEUE_6,10,2,3,0,3345,63,46),\
    GEN_TKY_CH_INIT(TKY_QUEUE_7,8,2,3,0,3467,67,40)


        /* Add new above this */

//***********************************************************
#ifdef __cplusplus
}
#endif

#endif /* __TOUCH_KEY_CFG_H__ */

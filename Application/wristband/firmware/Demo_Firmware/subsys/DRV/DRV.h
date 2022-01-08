#ifndef _DRV_H
#define _DRV_H

#include "CH58x_common.h"


#define DRV_GO_EVT              0x0001
#define DRV_STOP_EVT            0x0002
#define DRV_SHOCK_EVT           0x0004
#define DRV_TEST_EVT            0x0008

#define SHOCK_TIME_MS              400

#define DRV_ENBALE()          GPIOB_SetBits(GPIO_Pin_1),    \
                                GPIOB_ModeCfg(GPIO_Pin_1, GPIO_ModeOut_PP_5mA)
#define DRV_DISABLE()         GPIOB_ResetBits(GPIO_Pin_1),   \
                                GPIOB_ModeCfg(GPIO_Pin_1, GPIO_ModeOut_PP_5mA)
#define IN_TRIG_NOTUSED()     GPIOB_ResetBits(GPIO_Pin_0),   \
                                GPIOB_ModeCfg(GPIO_Pin_0, GPIO_ModeOut_PP_5mA)

extern uint8_t DRV_TaskID;

void DRV_Task_Init(void);

#endif

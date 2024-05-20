#ifndef APP_TMOS_H_
#define APP_TMOS_H_

#include "CH58x_common.h"

#define DEALDATA_EVT         0x0001
#define WAKEUP_DATA_DEAL_EVT 0x0002
#define DEBUG_PRINT_EVENT    0x0004

void PeriodicDealData(void);
void touch_on_TMOS_init(void);
void tky_on_TMOS_dataProcess(void);
void tky_DealData_stop(void);
void tky_DealData_start(void);

#endif

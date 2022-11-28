#ifndef TIME_UNITS_H
#define TIME_UNITS_H

#include "CH58x_common.h"

#ifdef CLK_OSC32K
#if (CLK_OSC32K==1)
#define FREQ_RTC    32000
#else
#define FREQ_RTC    32768
#endif
#endif /* CLK_OSC32K */


#define CLK_PER_US                  (1.0 / ((1.0 / FREQ_RTC) * 1000 * 1000)) // 0.032 clk/us
#define CLK_PER_MS                  (CLK_PER_US * 1000) // 32clk/ms

#define US_PER_CLK                  (1.0 / CLK_PER_US)      //   31.25us/clk
#define MS_PER_CLK                  (US_PER_CLK / 1000.0)     //   0.03125ms/clk

#define SYS_TO_US(clk)              ((uint32_t)((clk) * US_PER_CLK))
#define SYS_TO_MS(clk)              ((uint32_t)((clk) * MS_PER_CLK))

#define US_TO_SYS(us)               ((uint32_t)((us) * CLK_PER_US))
#define MS_TO_SYS(ms)               ((uint32_t)((ms) * CLK_PER_MS ))

static inline uint32_t get_current_time(void)
{
    return SYS_TO_MS(RTC_GetCycle32k());
}

#endif /* TIME_UNITS_H */

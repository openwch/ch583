#include "LCD_show/show_task.h"
#include "HAL/HAL.h"
#include "SPI/mySPI.h"
#include "HAL/RTC.h"
#include <time.h>
#include "ST7735S/pic.h"
#include "ST7735S/lcd.h"
#include "ST7735S/lcd_init.h"

uint8_t show_TaskID = INVALID_TASK_ID;

static void Show_ProcessTMOSMsg(tmos_event_hdr_t *pMsg) {
    switch (pMsg->event) {
    case GAP_MSG_EVENT: {

        break;
    }

    case HEARTBT_MSG_EVT: {
        if (pMsg->status) {
            SendMSG_t *msg = (SendMSG_t *) pMsg;
            msg->hdr.status ? LCD_ShowIntNum(20, 110, *((int *) msg->pData), 3, WHITE,
                            BLACK, 16, 0) : 0;
        }
    }
        break;

    case WRIST_MSG_EVT: {
        if (pMsg->status) {
            LCD_BLK_Set(); //打开背光
            LOG_INFO("open backlight");
        } else {
            LCD_BLK_Clr();
            LOG_INFO("close backlight");
        }
    }
        break;

    case STEP_MSG_EVT: {
        if (pMsg->status) {
            SendMSG_t *msg = (SendMSG_t *) pMsg;
            uint16_t tmp = *((uint16_t *) msg->pData);
            uint8_t size = 0;
            while (tmp) {
                size++;
                tmp /= 10;
            }
            msg->hdr.status ? LCD_ShowIntNum(20, 145, *((uint16_t *) msg->pData), size,
                            WHITE, BLACK, 16, 0) : 0;
        }
    }
        break;

    case SHOW_PIC_EVT: {
        tmos_set_event(show_TaskID, SHOW_PIC_EVENT);
    }
        break;

    case SHOW_TIME_EVT:{
        tmos_set_event(show_TaskID, SHOW_TIME_EVENT);
    }break;

    default:
        break;
    }
}

uint16_t Show_ProcessEvent(uint8 task_id, uint16 events)
{
    if (events & SYS_EVENT_MSG) {
        uint8 *pMsg;

        if ((pMsg = tmos_msg_receive(show_TaskID)) != NULL) {
            Show_ProcessTMOSMsg((tmos_event_hdr_t *) pMsg);
            // Release the TMOS message
            tmos_msg_deallocate(pMsg);
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }

    if (events & SHOW_HEART_EVENT) {

        static bool flag = false;

        if (flag) {
            LCD_ShowPicture(10,10,0x3b,0x3c,gImage_love_big);
            flag = false;
            tmos_start_task(show_TaskID, SHOW_HEART_EVENT,
                    MS1_TO_SYSTEM_TIME(200));
        } else {
            LCD_ShowPicture(10,10,0x3b,0x3c,gImage_love_small);
            flag = true;
            tmos_start_task(show_TaskID, SHOW_HEART_EVENT,
                    MS1_TO_SYSTEM_TIME(600));
        }
        return (events ^ SHOW_HEART_EVENT);
    }

    if (events & SHOW_PIC_EVENT) {
        tmos_stop_task(show_TaskID, SHOW_HEART_EVENT);
        tmos_clear_event(show_TaskID, SHOW_HEART_EVENT);

        tmos_stop_task(show_TaskID, SHOW_TIME_EVENT);
        tmos_clear_event(show_TaskID, SHOW_TIME_EVENT);
        LCD_ShowPicture(0, 0, 0x50, 0xA0, gImage_wch2);
        return (events ^ SHOW_PIC_EVENT);
    }

    if (events & SHOW_TIME_EVENT) {
        char str[50];
        strftime(str, 100, "%F %T", mytime);
        PRINT("time: %s\n", str);

        uint16_t year = mytime->tm_year + 1900;
        uint16_t yday = mytime->tm_yday;
        uint8_t month = mytime->tm_mon + 1;
        uint8_t week = mytime->tm_wday + 1;
        uint8_t day = mytime->tm_mday;
        uint8_t hour = mytime->tm_hour;
        uint8_t minute = mytime->tm_min;
        uint8_t second = mytime->tm_sec;


        static uint16_t year_l = 0;
        static uint16_t yday_l = 0;
        static uint8_t month_l = 0;
        static uint8_t week_l = 0;
        static uint8_t day_l = 0;
        static uint8_t hour_l = 0;
        static uint8_t minute_l = 0;


        if((year^year_l) || (yday^yday_l) || (month^month_l) || (week^week_l) || (day^day_l) || (hour^hour_l) || (minute^minute_l)){
            uint32_t starttime = millis();

            LCD_ShowIntNum0(10, 0, year, 4, WHITE, BLACK, 12, 0);
            LCD_ShowString(33, 0, "/", WHITE, BLACK, 12, 0);
            LCD_ShowIntNum0(40, 0, month, 2, WHITE, BLACK, 12, 0);
            LCD_ShowString(52, 0, "/", WHITE, BLACK, 12, 0);
            LCD_ShowIntNum0(59, 0, day, 2, WHITE, BLACK, 12, 0);
            LCD_ShowIntNum0(0, 20, hour, 2, WHITE, BLACK, 32, 0);
            LCD_ShowString(30, 20, ":", WHITE, BLACK, 32, 0);
            LCD_ShowIntNum0(45, 20, minute, 2, WHITE, BLACK, 32, 0);

            LOG_INFO("time: %d ms!", millis() - starttime);
        }

        year_l = year;
        yday_l = yday;
        month_l = month;
        week_l = week;
        day_l = day;
        hour_l = hour;
        minute_l = minute;
        tmos_start_task(show_TaskID, SHOW_TIME_EVENT, MS1_TO_SYSTEM_TIME(1000));
        return (events ^ SHOW_TIME_EVENT);
    }

    if (events & SHOW_STR_SPORT_EVENT) {
        tmos_stop_task(show_TaskID, SHOW_TIME_EVENT);
        tmos_clear_event(show_TaskID, SHOW_TIME_EVENT);

        LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);

        LCD_ShowString(20, 90, "beats", WHITE, BLACK, 16, 0);
        LCD_ShowString(20, 125, "steps", WHITE, BLACK, 16, 0);

        tmos_set_event(show_TaskID, SHOW_HEART_EVENT);
        return (events ^ SHOW_STR_SPORT_EVENT);
    }

    if (events & SHOW_STR_TIME_EVENT) {

        tmos_stop_task(show_TaskID, SHOW_HEART_EVENT);
        tmos_clear_event(show_TaskID, SHOW_HEART_EVENT);

        LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);
        uint16_t year = mytime->tm_year + 1900;
        uint16_t yday = mytime->tm_yday;
        uint8_t month = mytime->tm_mon + 1;
        uint8_t week = mytime->tm_wday + 1;
        uint8_t day = mytime->tm_mday;
        uint8_t hour = mytime->tm_hour;
        uint8_t minute = mytime->tm_min;
        uint8_t second = mytime->tm_sec;
        LCD_ShowIntNum0(10, 0, year, 4, WHITE, BLACK, 12, 0);
        LCD_ShowString(33, 0, "/", WHITE, BLACK, 12, 0);
        LCD_ShowIntNum0(40, 0, month, 2, WHITE, BLACK, 12, 0);
        LCD_ShowString(52, 0, "/", WHITE, BLACK, 12, 0);
        LCD_ShowIntNum0(59, 0, day, 2, WHITE, BLACK, 12, 0);
        LCD_ShowIntNum0(0, 20, hour, 2, WHITE, BLACK, 32, 0);
        LCD_ShowString(30, 20, ":", WHITE, BLACK, 32, 0);
        LCD_ShowIntNum0(45, 20, minute, 2, WHITE, BLACK, 32, 0);

        tmos_set_event(show_TaskID, SHOW_TIME_EVENT);
        return (events ^ SHOW_STR_TIME_EVENT);
    }


    if (events & SHOW_TEST_EVENT) {
//        LCD_Showtest(0, 0, LCD_W, LCD_H);

//        LCD_ShowPicture(0, 0, 0x50, 0x94, gImage_spring);
//        LCD_ShowPicture(10, 10, 0x50, 0x94, gImage_321);

        static uint8_t pic = 1;
        uint32_t start = millis();

        LCD_ShowPicture(0, 0, 0x50, 0xA0, gImage_wch2);
//        LCD_ShowString(33, 40, "wch", WHITE, BLACK, 16, 1);
//        SHOW_FUN(1);
        PRINT("show time %d ms\n", millis()-start);
        return (events ^ SHOW_TEST_EVENT);
    }

    return 0;
}

void Show_Task_Init(void)
{
    show_TaskID = TMOS_ProcessEventRegister(Show_ProcessEvent);

    MySPIinit();

    LCD_Init(); //LCD初始化

    LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);
    LCD_ShowPicture(0, 0, 0x50, 0xA0, gImage_wch1);


    tmos_set_event(show_TaskID, SHOW_STR_TIME_EVENT);
//    tmos_start_reload_task(show_TaskID, SHOW_TEST_EVENT, MS1_TO_SYSTEM_TIME(500));
}

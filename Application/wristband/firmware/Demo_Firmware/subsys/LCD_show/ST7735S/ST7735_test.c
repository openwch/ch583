#include "lcd.h"
#include "lcd_init.h"
//#include "pic.h"

void st7725_test(void) {
    float t=0;

    LCD_Init();//LCD初始化
    LCD_Fill(0,0,LCD_W,LCD_H,WHITE);

    while(1)
    {
        LCD_ShowChinese(40,0,"沁恒微电子",RED,WHITE,16,0);
        LCD_ShowString(10,20,"LCD_W:",RED,WHITE,16,0);
        LCD_ShowIntNum(58,20,LCD_W,3,RED,WHITE,16, 0);
        LCD_ShowString(10,40,"LCD_H:",RED,WHITE,16,0);
        LCD_ShowIntNum(58,40,LCD_H,3,RED,WHITE,16,0);
        LCD_ShowFloatNum1(10,60,t,4,RED,WHITE,16);
        t+=0.11;
//        LCD_ShowPicture(100,20,40,40,gImage_1);
    }
}

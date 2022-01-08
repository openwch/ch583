#include "lcd_init.h"
#include "SPI/mySPI.h"

void LCD_GPIO_Init(void) {
    GPIOA_SetBits( GPIO_Pin_0 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_4 |
            GPIO_Pin_5 | GPIO_Pin_6);
    GPIOA_ModeCfg( GPIO_Pin_0 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_4 |
            GPIO_Pin_5 | GPIO_Pin_6, GPIO_ModeOut_PP_5mA);

    GPIOA_ResetBits(GPIO_Pin_5);
}

/******************************************************************************
 函数说明：LCD串行数据写入函数
 入口数据：dat  要写入的串行数据
 返回值：  无
 ******************************************************************************/
__HIGH_CODE
void LCD_Writ_Bus(uint8_t dat)
{
    uint8_t i;
    LCD_CS_Clr();

    MySPIsenddata(dat);
    LCD_CS_Set();
}

__HIGH_CODE
void LCD_WR_buf(uint8_t *dat, uint16_t len)
{
//  LCD_Writ_Bus(dat);
    LCD_CS_Clr();
    MySPIsendbuf(dat, len);
    LCD_CS_Set();
}

/******************************************************************************
 函数说明：LCD写入数据
 入口数据：dat 写入的数据
 返回值：  无
 ******************************************************************************/
__HIGH_CODE
void LCD_WR_DATA8(uint8_t dat)
{
//	LCD_Writ_Bus(dat);

    LCD_CS_Clr();
    MySPIsenddata(dat);
    LCD_CS_Set();

}

/******************************************************************************
 函数说明：LCD写入数据
 入口数据：dat 写入的数据
 返回值：  无
 ******************************************************************************/
__HIGH_CODE
void LCD_WR_DATA(uint16_t dat)
{
    LCD_Writ_Bus(dat >> 8);
    LCD_Writ_Bus(dat);

//    LCD_Writ_Bus(dat);
//    LCD_Writ_Bus(dat<<8);

}

/******************************************************************************
 函数说明：LCD写入命令
 入口数据：dat 写入的命令
 返回值：  无
 ******************************************************************************/
__HIGH_CODE
void LCD_WR_REG(uint8_t dat)
{
    LCD_DC_Clr(); //写命令
    LCD_Writ_Bus(dat);
//	MySPIsenddata(dat);
    LCD_DC_Set(); //写数据
}

/******************************************************************************
 函数说明：设置起始和结束地址
 入口数据：x1,x2 设置列的起始和结束地址
 y1,y2 设置行的起始和结束地址
 返回值：  无
 ******************************************************************************/
__HIGH_CODE
void LCD_Address_Set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{

    if (USE_HORIZONTAL == 0) {
        LCD_WR_REG(0x2a); //列地址设置
        LCD_WR_DATA(x1 + 24);
        LCD_WR_DATA(x2 + 24);
        LCD_WR_REG(0x2b); //行地址设置
        LCD_WR_DATA(y1);
        LCD_WR_DATA(y2);
        LCD_WR_REG(0x2c); //储存器写
    } else if (USE_HORIZONTAL == 1) {
        LCD_WR_REG(0x2a); //列地址设置
        LCD_WR_DATA(x1 + 24);
        LCD_WR_DATA(x2 + 24);
        LCD_WR_REG(0x2b); //行地址设置
        LCD_WR_DATA(y1);
        LCD_WR_DATA(y2);
        LCD_WR_REG(0x2c); //储存器写
    } else if (USE_HORIZONTAL == 2) {
        LCD_WR_REG(0x2a); //列地址设置
        LCD_WR_DATA(x1);
        LCD_WR_DATA(x2);
        LCD_WR_REG(0x2b); //行地址设置
        LCD_WR_DATA(y1 + 24);
        LCD_WR_DATA(y2 + 24);
        LCD_WR_REG(0x2c); //储存器写
    } else {
        LCD_WR_REG(0x2a); //列地址设置
        LCD_WR_DATA(x1);
        LCD_WR_DATA(x2);
        LCD_WR_REG(0x2b); //行地址设置
        LCD_WR_DATA(y1 + 24);
        LCD_WR_DATA(y2 + 24);
        LCD_WR_REG(0x2c); //储存器写
    }
}

void LCD_Init(void) {
    LCD_GPIO_Init(); //初始化GPIO

    LCD_RES_Clr(); //复位
    DelayMs(100);
    LCD_RES_Set();
    DelayMs(100);

    LCD_BLK_Set(); //打开背光
    DelayMs(100);

    LCD_WR_REG(0x11); //Sleep exit
    DelayMs(120);                //Delay 120ms
    LCD_WR_REG(0xB1);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x3C);

    LCD_WR_REG(0xB2);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x3C);

    LCD_WR_REG(0xB3);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x3C);
    LCD_WR_DATA8(0x3C);

    LCD_WR_REG(0xB4);     //Dot inversion
    LCD_WR_DATA8(0x03);

    LCD_WR_REG(0xC0);
    LCD_WR_DATA8(0x0E);
    LCD_WR_DATA8(0x0E);
    LCD_WR_DATA8(0x04);

    LCD_WR_REG(0xC1);
    LCD_WR_DATA8(0xC5);

    LCD_WR_REG(0xC2);
    LCD_WR_DATA8(0x0d);
    LCD_WR_DATA8(0x00);

    LCD_WR_REG(0xC3);
    LCD_WR_DATA8(0x8D);
    LCD_WR_DATA8(0x2A);

    LCD_WR_REG(0xC4);
    LCD_WR_DATA8(0x8D);
    LCD_WR_DATA8(0xEE);

    LCD_WR_REG(0xC5);     //VCOM
    LCD_WR_DATA8(0x06); //1D  .06

    LCD_WR_REG(0x36);     //MX, MY, RGB mode
    if (USE_HORIZONTAL == 0)
        LCD_WR_DATA8(0x08);
    else if (USE_HORIZONTAL == 1)
        LCD_WR_DATA8(0xC8);
    else if (USE_HORIZONTAL == 2)
        LCD_WR_DATA8(0x78);
    else
        LCD_WR_DATA8(0xA8);

    LCD_WR_REG(0x3A);
    LCD_WR_DATA8(0x55);

    LCD_WR_REG(0xE0);
    LCD_WR_DATA8(0x0b);
    LCD_WR_DATA8(0x17);
    LCD_WR_DATA8(0x0a);
    LCD_WR_DATA8(0x0d);
    LCD_WR_DATA8(0x1a);
    LCD_WR_DATA8(0x19);
    LCD_WR_DATA8(0x16);
    LCD_WR_DATA8(0x1d);
    LCD_WR_DATA8(0x21);
    LCD_WR_DATA8(0x26);
    LCD_WR_DATA8(0x37);
    LCD_WR_DATA8(0x3c);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x09);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x10);

    LCD_WR_REG(0xE1);
    LCD_WR_DATA8(0x0c);
    LCD_WR_DATA8(0x19);
    LCD_WR_DATA8(0x09);
    LCD_WR_DATA8(0x0d);
    LCD_WR_DATA8(0x1b);
    LCD_WR_DATA8(0x19);
    LCD_WR_DATA8(0x15);
    LCD_WR_DATA8(0x1d);
    LCD_WR_DATA8(0x21);
    LCD_WR_DATA8(0x26);
    LCD_WR_DATA8(0x39);
    LCD_WR_DATA8(0x3E);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x09);
    LCD_WR_DATA8(0x05);
    LCD_WR_DATA8(0x10);
    LCD_WR_REG(0x29);     //Display on
}


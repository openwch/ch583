#include "lcd.h"
#include "lcd_init.h"
#include "lcdfont.h"
#include "SPI/mySPI.h"
#include "sys/slist.h"
#include "CH58xBLE_LIB.h"

static sys_slist_t pic_list;

struct show_node {
    sys_snode_t node;
    char *show_obj;
    const unsigned char *data;
};

/******************************************************************************
 函数说明：在指定区域填充颜色
 入口数据：xsta,ysta   起始坐标
 xend,yend   终止坐标
 color       要填充的颜色
 返回值：  无
 ******************************************************************************/
void LCD_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend,
        uint16_t color) {
    uint16_t i, j;
    LCD_Address_Set(xsta, ysta, xend - 1, yend - 1); //设置显示范围
    for (i = ysta; i < yend; i++) {
        for (j = xsta; j < xend; j++) {
            LCD_WR_DATA(color);
        }
    }
}

/******************************************************************************
 函数说明：在指定位置画点
 入口数据：x,y 画点坐标
 color 点的颜色
 返回值：  无
 ******************************************************************************/
void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color) {
    LCD_Address_Set(x, y, x, y); //设置光标位置 
    LCD_WR_DATA(color);
}

/******************************************************************************
 函数说明：画线
 入口数据：x1,y1   起始坐标
 x2,y2   终止坐标
 color   线的颜色
 返回值：  无
 ******************************************************************************/
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
        uint16_t color) {
    uint16_t t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, uRow, uCol;
    delta_x = x2 - x1; //计算坐标增量 
    delta_y = y2 - y1;
    uRow = x1; //画线起点坐标
    uCol = y1;
    if (delta_x > 0)
        incx = 1; //设置单步方向 
    else if (delta_x == 0)
        incx = 0; //垂直线 
    else {
        incx = -1;
        delta_x = -delta_x;
    }
    if (delta_y > 0)
        incy = 1;
    else if (delta_y == 0)
        incy = 0; //水平线 
    else {
        incy = -1;
        delta_y = -delta_y;
    }
    if (delta_x > delta_y)
        distance = delta_x; //选取基本增量坐标轴 
    else
        distance = delta_y;
    for (t = 0; t < distance + 1; t++) {
        LCD_DrawPoint(uRow, uCol, color); //画点
        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance) {
            xerr -= distance;
            uRow += incx;
        }
        if (yerr > distance) {
            yerr -= distance;
            uCol += incy;
        }
    }
}

/******************************************************************************
 函数说明：画矩形
 入口数据：x1,y1   起始坐标
 x2,y2   终止坐标
 color   矩形的颜色
 返回值：  无
 ******************************************************************************/
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
        uint16_t color) {
    LCD_DrawLine(x1, y1, x2, y1, color);
    LCD_DrawLine(x1, y1, x1, y2, color);
    LCD_DrawLine(x1, y2, x2, y2, color);
    LCD_DrawLine(x2, y1, x2, y2, color);
}

/******************************************************************************
 函数说明：画圆
 入口数据：x0,y0   圆心坐标
 r       半径
 color   圆的颜色
 返回值：  无
 ******************************************************************************/
void Draw_Circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color) {
    int a, b;
    a = 0;
    b = r;
    while (a <= b) {
        LCD_DrawPoint(x0 - b, y0 - a, color);             //3           
        LCD_DrawPoint(x0 + b, y0 - a, color);             //0           
        LCD_DrawPoint(x0 - a, y0 + b, color);             //1                
        LCD_DrawPoint(x0 - a, y0 - b, color);             //2             
        LCD_DrawPoint(x0 + b, y0 + a, color);             //4               
        LCD_DrawPoint(x0 + a, y0 - b, color);             //5
        LCD_DrawPoint(x0 + a, y0 + b, color);             //6 
        LCD_DrawPoint(x0 - b, y0 + a, color);             //7
        a++;
        if ((a * a + b * b) > (r * r))             //判断要画的点是否过远
                {
            b--;
        }
    }
}

/******************************************************************************
 函数说明：显示汉字串
 入口数据：x,y显示坐标
 *s 要显示的汉字串
 fc 字的颜色
 bc 字的背景色
 sizey 字号 可选 16 24 32
 mode:  0非叠加模式  1叠加模式
 返回值：  无
 ******************************************************************************/
void LCD_ShowChinese(uint16_t x, uint16_t y, uint8_t *s, uint16_t fc,
        uint16_t bc, uint8_t sizey, uint8_t mode) {
    while (*s != 0) {
        if (sizey == 12)
            LCD_ShowChinese12x12(x, y, s, fc, bc, sizey, mode);
        else if (sizey == 16)
            LCD_ShowChinese16x16(x, y, s, fc, bc, sizey, mode);
        else if (sizey == 24)
            LCD_ShowChinese24x24(x, y, s, fc, bc, sizey, mode);
        else if (sizey == 32)
            LCD_ShowChinese32x32(x, y, s, fc, bc, sizey, mode);
        else
            return;
        s += 2;
        x += sizey;
    }
}

/******************************************************************************
 函数说明：显示单个12x12汉字
 入口数据：x,y显示坐标
 *s 要显示的汉字
 fc 字的颜色
 bc 字的背景色
 sizey 字号
 mode:  0非叠加模式  1叠加模式
 返回值：  无
 ******************************************************************************/
void LCD_ShowChinese12x12(uint16_t x, uint16_t y, uint8_t *s, uint16_t fc,
        uint16_t bc, uint8_t sizey, uint8_t mode) {
    uint8_t i, j, m = 0;
    uint16_t k;
    uint16_t HZnum;             //汉字数目
    uint16_t TypefaceNum;             //一个字符所占字节大小
    uint16_t x0 = x;
    TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey;

    HZnum = sizeof(tfont12) / sizeof(typFNT_GB12);	//统计汉字数目
    for (k = 0; k < HZnum; k++) {
        if ((tfont12[k].Index[0] == *(s))
                && (tfont12[k].Index[1] == *(s + 1))) {
            LCD_Address_Set(x, y, x + sizey - 1, y + sizey - 1);
            for (i = 0; i < TypefaceNum; i++) {
                for (j = 0; j < 8; j++) {
                    if (!mode)	//非叠加方式
                    {
                        if (tfont12[k].Msk[i] & (0x01 << j))
                            LCD_WR_DATA(fc);
                        else
                            LCD_WR_DATA(bc);
                        m++;
                        if (m % sizey == 0) {
                            m = 0;
                            break;
                        }
                    } else	//叠加方式
                    {
                        if (tfont12[k].Msk[i] & (0x01 << j))
                            LCD_DrawPoint(x, y, fc);	//画一个点
                        x++;
                        if ((x - x0) == sizey) {
                            x = x0;
                            y++;
                            break;
                        }
                    }
                }
            }
        }
        continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
    }
}

/******************************************************************************
 函数说明：显示单个16x16汉字
 入口数据：x,y显示坐标
 *s 要显示的汉字
 fc 字的颜色
 bc 字的背景色
 sizey 字号
 mode:  0非叠加模式  1叠加模式
 返回值：  无
 ******************************************************************************/
void LCD_ShowChinese16x16(uint16_t x, uint16_t y, uint8_t *s, uint16_t fc,
        uint16_t bc, uint8_t sizey, uint8_t mode) {
    uint8_t i, j, m = 0;
    uint16_t k;
    uint16_t HZnum;  //汉字数目
    uint16_t TypefaceNum;  //一个字符所占字节大小
    uint16_t x0 = x;
    TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey;
    HZnum = sizeof(tfont16) / sizeof(typFNT_GB16);	//统计汉字数目
    for (k = 0; k < HZnum; k++) {
        if ((tfont16[k].Index[0] == *(s))
                && (tfont16[k].Index[1] == *(s + 1))) {
            LCD_Address_Set(x, y, x + sizey - 1, y + sizey - 1);
            for (i = 0; i < TypefaceNum; i++) {
                for (j = 0; j < 8; j++) {
                    if (!mode)	//非叠加方式
                    {
                        if (tfont16[k].Msk[i] & (0x01 << j))
                            LCD_WR_DATA(fc);
                        else
                            LCD_WR_DATA(bc);
                        m++;
                        if (m % sizey == 0) {
                            m = 0;
                            break;
                        }
                    } else	//叠加方式
                    {
                        if (tfont16[k].Msk[i] & (0x01 << j))
                            LCD_DrawPoint(x, y, fc);	//画一个点
                        x++;
                        if ((x - x0) == sizey) {
                            x = x0;
                            y++;
                            break;
                        }
                    }
                }
            }
        }
        continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
    }
}

/******************************************************************************
 函数说明：显示单个24x24汉字
 入口数据：x,y显示坐标
 *s 要显示的汉字
 fc 字的颜色
 bc 字的背景色
 sizey 字号
 mode:  0非叠加模式  1叠加模式
 返回值：  无
 ******************************************************************************/
void LCD_ShowChinese24x24(uint16_t x, uint16_t y, uint8_t *s, uint16_t fc,
        uint16_t bc, uint8_t sizey, uint8_t mode) {
    uint8_t i, j, m = 0;
    uint16_t k;
    uint16_t HZnum;  //汉字数目
    uint16_t TypefaceNum;  //一个字符所占字节大小
    uint16_t x0 = x;
    TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey;
    HZnum = sizeof(tfont24) / sizeof(typFNT_GB24);	//统计汉字数目
    for (k = 0; k < HZnum; k++) {
        if ((tfont24[k].Index[0] == *(s))
                && (tfont24[k].Index[1] == *(s + 1))) {
            LCD_Address_Set(x, y, x + sizey - 1, y + sizey - 1);
            for (i = 0; i < TypefaceNum; i++) {
                for (j = 0; j < 8; j++) {
                    if (!mode)	//非叠加方式
                    {
                        if (tfont24[k].Msk[i] & (0x01 << j))
                            LCD_WR_DATA(fc);
                        else
                            LCD_WR_DATA(bc);
                        m++;
                        if (m % sizey == 0) {
                            m = 0;
                            break;
                        }
                    } else	//叠加方式
                    {
                        if (tfont24[k].Msk[i] & (0x01 << j))
                            LCD_DrawPoint(x, y, fc);	//画一个点
                        x++;
                        if ((x - x0) == sizey) {
                            x = x0;
                            y++;
                            break;
                        }
                    }
                }
            }
        }
        continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
    }
}

/******************************************************************************
 函数说明：显示单个32x32汉字
 入口数据：x,y显示坐标
 *s 要显示的汉字
 fc 字的颜色
 bc 字的背景色
 sizey 字号
 mode:  0非叠加模式  1叠加模式
 返回值：  无
 ******************************************************************************/
void LCD_ShowChinese32x32(uint16_t x, uint16_t y, uint8_t *s, uint16_t fc,
        uint16_t bc, uint8_t sizey, uint8_t mode) {
    uint8_t i, j, m = 0;
    uint16_t k;
    uint16_t HZnum;  //汉字数目
    uint16_t TypefaceNum;  //一个字符所占字节大小
    uint16_t x0 = x;
    TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey;
    HZnum = sizeof(tfont32) / sizeof(typFNT_GB32);	//统计汉字数目
    for (k = 0; k < HZnum; k++) {
        if ((tfont32[k].Index[0] == *(s))
                && (tfont32[k].Index[1] == *(s + 1))) {
            LCD_Address_Set(x, y, x + sizey - 1, y + sizey - 1);
            for (i = 0; i < TypefaceNum; i++) {
                for (j = 0; j < 8; j++) {
                    if (!mode)	//非叠加方式
                    {
                        if (tfont32[k].Msk[i] & (0x01 << j))
                            LCD_WR_DATA(fc);
                        else
                            LCD_WR_DATA(bc);
                        m++;
                        if (m % sizey == 0) {
                            m = 0;
                            break;
                        }
                    } else	//叠加方式
                    {
                        if (tfont32[k].Msk[i] & (0x01 << j))
                            LCD_DrawPoint(x, y, fc);	//画一个点
                        x++;
                        if ((x - x0) == sizey) {
                            x = x0;
                            y++;
                            break;
                        }
                    }
                }
            }
        }
        continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
    }
}

/******************************************************************************
 函数说明：显示单个字符
 入口数据：x,y显示坐标
 num 要显示的字符
 fc 字的颜色
 bc 字的背景色
 sizey 字号
 mode:  0非叠加模式  1叠加模式
 返回值：  无
 ******************************************************************************/
__HIGH_CODE
void LCD_ShowChar(uint16_t x, uint16_t y, uint8_t num, uint16_t fc, uint16_t bc,
        uint8_t sizey, uint8_t mode) {
    uint8_t temp, sizex, t, m = 0;
    uint16_t i, TypefaceNum;  //一个字符所占字节大小
    uint16_t x0 = x;
    sizex = sizey / 2;
    TypefaceNum = (sizex / 8 + ((sizex % 8) ? 1 : 0)) * sizey;
    num = num - ' ';    //得到偏移后的值
    LCD_Address_Set(x, y, x + sizex - 1, y + sizey - 1);  //设置光标位置 
    for (i = 0; i < TypefaceNum; i++) {
        if (sizey == 12)
            temp = ascii_1206[num][i];		       //调用6x12字体
        else if (sizey == 16)
            temp = ascii_1608[num][i];		 //调用8x16字体
        else if (sizey == 24)
            temp = ascii_2412[num][i];		 //调用12x24字体
        else if (sizey == 32)
            temp = ascii_3216[num][i];		 //调用16x32字体
        else
            return;
        for (t = 0; t < 8; t++) {
            if (!mode)		 //非叠加模式
            {
                if (temp & (0x01 << t))
                    LCD_WR_DATA(fc);
                else
                    LCD_WR_DATA(bc);
                m++;
                if (m % sizex == 0) {
                    m = 0;
                    break;
                }
            } else		 //叠加模式
            {
                if (temp & (0x01 << t))
                    LCD_DrawPoint(x, y, fc);		 //画一个点
                x++;
                if ((x - x0) == sizex) {
                    x = x0;
                    y++;
                    break;
                }
            }
        }
    }
}

/******************************************************************************
 函数说明：显示字符串
 入口数据：x,y显示坐标
 *p 要显示的字符串
 fc 字的颜色
 bc 字的背景色
 sizey 字号
 mode:  0非叠加模式  1叠加模式
 返回值：  无
 ******************************************************************************/
__HIGH_CODE
void LCD_ShowString(uint16_t x, uint16_t y, const uint8_t *p, uint16_t fc,
        uint16_t bc, uint8_t sizey, uint8_t mode) {
    while (*p != '\0') {
        LCD_ShowChar(x, y, *p, fc, bc, sizey, mode);
        x += sizey / 2;
        p++;
    }
}

/******************************************************************************
 函数说明：显示数字
 入口数据：m底数，n指数
 返回值：  无
 ******************************************************************************/
__HIGH_CODE
uint32_t mypow(uint8_t m, uint8_t n) {
    uint32_t result = 1;
    while (n--)
        result *= m;
    return result;
}

/******************************************************************************
 函数说明：显示整数变量
 入口数据：x,y显示坐标
 num 要显示整数变量
 len 要显示的位数
 fc 字的颜色
 bc 字的背景色
 sizey 字号
 返回值：  无
 ******************************************************************************/
__HIGH_CODE
void LCD_ShowIntNum0(uint16_t x, uint16_t y, uint16_t num, uint8_t len,
        uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode) {
    uint8_t t, temp;
    uint8_t enshow = 0;
    uint8_t sizex = sizey / 2;
    for (t = 0; t < len; t++) {
        temp = (num / mypow(10, len - t - 1)) % 10;
        if (enshow == 0 && t < (len - 1)) {
            if (temp == 0) {
                LCD_ShowChar(x + t * sizex, y, '0', fc, bc, sizey, mode);
                continue;
            } else
                enshow = 1;

        }
        LCD_ShowChar(x + t * sizex, y, temp + 48, fc, bc, sizey, mode);
    }
}
__HIGH_CODE
void LCD_ShowIntNum(uint16_t x, uint16_t y, uint16_t num, uint8_t len,
        uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode) {
    uint8_t t, temp;
    uint8_t enshow = 0;
    uint8_t sizex = sizey / 2;
    for (t = 0; t < len; t++) {
        temp = (num / mypow(10, len - t - 1)) % 10;
        if (enshow == 0 && t < (len - 1)) {
            if (temp == 0) {
                LCD_ShowChar(x + t * sizex, y, ' ', fc, bc, sizey, mode);
                continue;
            } else
                enshow = 1;

        }
        LCD_ShowChar(x + t * sizex, y, temp + 48, fc, bc, sizey, mode);
    }
}

/******************************************************************************
 函数说明：显示两位小数变量
 入口数据：x,y显示坐标
 num 要显示小数变量
 len 要显示的位数
 fc 字的颜色
 bc 字的背景色
 sizey 字号
 返回值：  无
 ******************************************************************************/
void LCD_ShowFloatNum1(uint16_t x, uint16_t y, float num, uint8_t len,
        uint16_t fc, uint16_t bc, uint8_t sizey) {
    uint8_t t, temp, sizex;
    uint16_t num1;
    sizex = sizey / 2;
    num1 = num * 100;

    uint8_t tmp = num;
    uint8_t size = 0;
    while (tmp > 0) {
        tmp /= 10;
        size++;
    }
    size += 3;
    LOG_INFO("size: %d", size);

    for (t = 0; t < len; t++) {
        temp = (num1 / mypow(10, len - t - 1)) % 10;
        if (t == (len - 2)) {
            LCD_ShowChar(x + (len - 2) * sizex, y, '.', fc, bc, sizey, 0);
            t++;
            len += 1;
        }
//		LCD_ShowChar(x+t*sizex,y,temp+48,fc,bc,sizey,0);

        LOG_INFO(" %d", (temp));
        if (t <= (len - size)) {
            LCD_ShowChar(x + t * sizex, y, 32, fc, bc, sizey, 0);
        } else {
            LCD_ShowChar(x + t * sizex, y, temp + 48, fc, bc, sizey, 0);
        }
    }
}

/******************************************************************************
 函数说明：显示图片
 入口数据：x,y起点坐标
 length 图片长度
 width  图片宽度
 pic[]  图片数组    
 返回值：  无
 ******************************************************************************/
__HIGH_CODE
void LCD_ShowPicture(uint16_t x, uint16_t y, uint16_t length, uint16_t width,
        const uint8_t pic[])
{
    uint32_t k = 0;
    __attribute__((aligned(4)))  uint8_t tmpbuf[LCD_W*2];
    LCD_Address_Set(x, y, x + length - 1, y + width - 1);

    LCD_CS_Clr();
    for(int i = 0; i < width; i++) {
//        for(int p = 0; p < length * 2; p+=2){
//            tmpbuf[p+1] = pic[k++];
//            tmpbuf[p] = pic[k++];
//        }
        tmos_memcpy(tmpbuf, &pic[k+=length * 2], length * 2);

        uint8_t * addr = (uint8_t *)tmpbuf;
        MySPIsendbuf(addr, length * 2);
    }
    LCD_CS_Set();
}

static uint32_t color = 0;

uint16_t colorbuf[] = {
    0X0, 0X6B4D, 0X8410, 0XAD55, 0XC618, 0XD69A,
    0XDEFB, 0XF7BE, 0XFFFF, 0XFFDF, 0X62CA, 0XE618, 0XBC71, 0XF410, 0XCAEB,
    0XA145, 0XB104, 0X8000, 0X8800, 0XE000, 0XF800, 0XFA68, 0XFF3C, 0XFC0E,
    0XF920, 0XFB08, 0XECAF, 0XFBEA, 0XFA20, 0XFD0F, 0XFA60, 0XA285, 0XFC06,
    0XA348, 0XE4CC, 0X48E0, 0XFFBD, 0X8A22, 0XD343, 0XCAA0, 0XFB80, 0XFED7,
    0XF52C, 0XBB86, 0XFF9C, 0XFD8C, 0XCC27, 0X7202, 0XCBA4, 0XFF38, 0XF420,
    0XFC60, 0XFF5A, 0XD5B1, 0XDDD0, 0XFF59, 0XFEF5, 0XFCC0, 0XFF7A, 0XCD91,
    0X9B43, 0XFF36, 0XFFBC, 0XF6F6, 0XFF36, 0XFD20, 0XFFDE, 0XDD24, 0XBC21,
    0X49C0, 0XE60B, 0XFDE0, 0XFFDB, 0XE5C0, 0XFEA0, 0XFFD9, 0XF731, 0XEF55,
    0XBDAD, 0XE6C6, 0XFFFA, 0XFFFE, 0XF7BB, 0XFFFC, 0XFFDA, 0XFFF3, 0XCE69,
    0XFFE9, 0X8400, 0XFFE0, 0XFFE0, 0X6BA4, 0XCFE0, 0X6C64, 0X9E66, 0X5345,
    0X8F20, 0XAFE5, 0X9F29, 0X7FE0, 0X7FE0, 0X75C7, 0X9FE9, 0X67E0, 0X67EB,
    0XF7FE, 0X8DF1, 0X9772, 0X9FD3, 0X35E6, 0X2444, 0X3666, 0X320, 0X400,
    0X7E0, 0X2605, 0X14C5, 0X7731, 0X564F, 0X4F30, 0X13A6, 0XA7F9, 0X2C4A,
    0X3D8E, 0XF7FF, 0X7F0, 0X50B, 0X7D3, 0X6675, 0X7FFA, 0XDF1, 0X67FC,
    0X3739, 0X36B9, 0X2595, 0X4E99, 0XE7FF, 0XE7FF, 0XAF7D, 0X2A69, 0X410,
    0X451, 0X7FF, 0XAEFC, 0X67A, 0X5CF4, 0X411, 0XB71C, 0X30E, 0XAEDC,
    0X7DD9, 0X44D6, 0X5FF, 0X867D, 0X867F, 0X22F, 0X18A, 0X4416, 0XF7DF,
    0X7412, 0X7453, 0X1C9F, 0X273, 0X3FF, 0X5437, 0XB63B, 0X235, 0X5C38,
    0X64BD, 0X4C1C, 0X193, 0X92C, 0X174, 0X2A97, 0X435C, 0X21AF, 0X19F,
    0X99F, 0XFFDF, 0XE73F, 0XCE7F, 0X18CE, 0X10, 0X11, 0X19, 0X1F, 0X5A9C,
    0X49F1, 0X6AD9, 0X7B5D, 0X621F, 0XB4DF, 0X939B, 0X6199, 0X83B4, 0X5017,
    0XBD19, 0X895C, 0X881F, 0X4810, 0X9999, 0X901A, 0X7014, 0XDA7F, 0XE41F,
    0XBABA, 0XE67C, 0XDDFB, 0XCD19, 0XDD1B, 0XEC1D, 0X8010, 0X8811, 0XF81F,
    0XF014, 0XDB9A, 0XFD9C, 0XBAB3, 0XFB39, 0XC0B0, 0XF874, 0XF80F, 0XC810,
    0XE1D2, 0XF8B2, 0XE457, 0XFC17, 0XFB56, 0X4004, 0XFB96, 0XE00B, 0XFEDC,
    0X9806, 0XFF9E, 0XDB92, 0XD98C, 0XFC13, 0XD8A7, 0XFE19, 0XFDB8, 0XFD97,
    0XE126
};

static uint32_t temp = 0;

__HIGH_CODE
void LCD_Showtest(uint16_t x, uint16_t y, uint16_t length, uint16_t width)
{
    LCD_Address_Set(x, y, x + length - 1, y + width - 1);

    color = temp;
    for (uint32_t i = 0; i < width; i++) {
        for (uint32_t j = 0; j < length; j++) {
            LCD_WR_DATA((uint16_t)colorbuf[color]);
        }
        color++;
    }

    temp++;
    if(temp>= (sizeof(colorbuf)/sizeof(colorbuf[0])-1 - 160))
        temp = 0;
}

#define GEN_SHOW_NODE( )


void LCD_show(struct show_node *node, uint16_t x, uint16_t y, uint16_t length, uint16_t width)
{
    sys_slist_prepend(&pic_list, &node->node);
}


void LCD_pic_flow( )
{
    /*get current nodes*/

    /*draw last pic*/

    /*draw next pic*/
}

